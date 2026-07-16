CXX ?= g++
CXXFLAGS ?= -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror
THREAD_FLAGS ?= -pthread
INCLUDES = -ISources/BeebCore/include
CORE_SOURCES = $(wildcard Sources/BeebCore/src/*.cpp)
CORE_HEADERS = $(wildcard Sources/BeebCore/include/*.h Sources/BeebCore/include/beeb/*.hpp)
C0_TEST_SCRIPTS ?= $(wildcard Tests/C0/test-*.sh)
C1_TEST_SCRIPTS ?= $(filter-out Tests/C1/testlib.sh,$(wildcard Tests/C1/test-*.sh))
BUILD_DIR = .build/cpp
C0_PROFILE ?=
DOCS_PROFILE ?= auto
VERSION := $(strip $(shell sed -n '1p' VERSION))
ifeq ($(shell uname -s),Darwin)
SANITIZERS ?= undefined
else
SANITIZERS ?= address,undefined
endif

.PHONY: all test sanitize thread-sanitize test-c1 format-check check-version demo-rom test-c0 verify-c0 \
	verify-c0-references update-c0-reference measure-c0 \
	validate-c0-measurement docs docs-check clean

all: $(BUILD_DIR)/beeb-headless

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/beeb-tests: $(CORE_SOURCES) $(CORE_HEADERS) Tests/test_main.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(THREAD_FLAGS) $(INCLUDES) $(CORE_SOURCES) Tests/test_main.cpp -o $@

$(BUILD_DIR)/beeb-headless: $(CORE_SOURCES) $(CORE_HEADERS) Tools/beeb-headless/main.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(THREAD_FLAGS) $(INCLUDES) $(CORE_SOURCES) Tools/beeb-headless/main.cpp -o $@

$(BUILD_DIR)/beeb-evidence: $(CORE_SOURCES) $(CORE_HEADERS) Tools/beeb-evidence/main.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(THREAD_FLAGS) $(INCLUDES) $(CORE_SOURCES) Tools/beeb-evidence/main.cpp -o $@

$(BUILD_DIR)/make-demo-rom: Tools/make-demo-rom/main.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

demo-rom: $(BUILD_DIR)/make-demo-rom
	$(BUILD_DIR)/make-demo-rom $(BUILD_DIR)/cleanroom-demo.rom

check-version: $(BUILD_DIR)/beeb-headless
	@actual="$$($(BUILD_DIR)/beeb-headless --version)"; \
	test "$$actual" = "Beeb6502 $(VERSION)" || { \
		echo "version mismatch: VERSION=$(VERSION), binary=$$actual" >&2; exit 1; \
	}
	@grep -Fq "## [$(VERSION)] -" CHANGELOG.md || { \
		echo "CHANGELOG.md has no release entry for $(VERSION)" >&2; exit 1; \
	}

test: check-version $(BUILD_DIR)/beeb-tests
	$(BUILD_DIR)/beeb-tests

sanitize: | $(BUILD_DIR)
	$(CXX) -std=c++20 -O1 -g -Wall -Wextra -Wpedantic -Werror \
		$(THREAD_FLAGS) -fsanitize=$(SANITIZERS) -fno-omit-frame-pointer $(INCLUDES) \
		$(CORE_SOURCES) Tests/test_main.cpp -o $(BUILD_DIR)/beeb-tests-sanitize
	$(BUILD_DIR)/beeb-tests-sanitize --quick

thread-sanitize:
	C1_ONLY_TSAN=1 Tests/C1/test-runtime-races.sh

format-check:
	@files="$$(find Sources Tests Tools -type f \( -name '*.cpp' -o -name '*.h' -o -name '*.hpp' \) -print)"; \
	clang-format --dry-run --Werror $$files

test-c1:
	@status=0; \
	for test_script in $(C1_TEST_SCRIPTS); do \
		echo "C1 group: $$test_script"; \
		if "$$test_script"; then \
			echo "C1 group PASS: $$test_script"; \
		else \
			code=$$?; status=1; \
			echo "C1 group FAIL ($$code): $$test_script" >&2; \
		fi; \
	done; \
	exit $$status

test-c0:
	@status=0; \
	for test_script in $(C0_TEST_SCRIPTS); do \
		echo "$$test_script"; \
		"$$test_script" || status=$$?; \
	done; \
	exit $$status

verify-c0:
	scripts/verify-c0.sh $(if $(C0_PROFILE),--profile "$(C0_PROFILE)")

verify-c0-references:
	scripts/verify-c0-references.sh

update-c0-reference:
	@test -n "$(REFERENCE)" || { echo "REFERENCE is required" >&2; exit 2; }
	@test -n "$(REASON)" || { echo "REASON is required" >&2; exit 2; }
	scripts/update-c0-reference.sh --reference "$(REFERENCE)" --reason "$(REASON)"

measure-c0:
	scripts/measure-c0.sh --samples "$(or $(SAMPLES),5)" \
		--output "$(or $(OUTPUT),.build/c0/measurements/latest.txt)"

validate-c0-measurement:
	Tests/C0/test-measurement-record.sh

docs:
	scripts/build-docs.sh --profile "$(DOCS_PROFILE)"

docs-check:
	scripts/build-docs.sh --profile "$(DOCS_PROFILE)" --check

clean:
	rm -rf $(BUILD_DIR)
