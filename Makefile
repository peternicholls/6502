CXX ?= g++
CXXFLAGS ?= -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror
INCLUDES = -ISources/BeebCore/include
CORE_SOURCES = $(wildcard Sources/BeebCore/src/*.cpp)
BUILD_DIR = .build

.PHONY: all test demo-rom clean

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

test: $(BUILD_DIR)/beeb-tests
	$(BUILD_DIR)/beeb-tests

clean:
	rm -rf $(BUILD_DIR)
