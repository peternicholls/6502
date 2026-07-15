CXX ?= g++
CXXFLAGS ?= -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror
INCLUDES = -ISources/BeebCore/include
CORE_SOURCES = $(wildcard Sources/BeebCore/src/*.cpp)
BUILD_DIR = .build
VERSION := $(strip $(shell sed -n '1p' VERSION))

.PHONY: all test check-version demo-rom clean

all: $(BUILD_DIR)/beeb-headless

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/beeb-tests: $(CORE_SOURCES) Tests/test_main.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

$(BUILD_DIR)/beeb-headless: $(CORE_SOURCES) Tools/beeb-headless/main.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

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

clean:
	rm -rf $(BUILD_DIR)
