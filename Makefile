BUILD_DIR ?= build
# Build directories
CPP_BUILD_DIR ?= $(BUILD_DIR)-cpp
SHARED_BUILD_DIR ?= $(BUILD_DIR)-shared
ASAN_BUILD_DIR ?= $(BUILD_DIR)-asan
UBSAN_BUILD_DIR ?= $(BUILD_DIR)-ubsan
# Build test directories
TEST_BUILD_DIR ?= $(BUILD_DIR)-test
TEST_CPP_BUILD_DIR ?= $(BUILD_DIR)-test-cpp
TEST_ASAN_BUILD_DIR ?= $(BUILD_DIR)-test-asan
TEST_UBSAN_BUILD_DIR ?= $(BUILD_DIR)-test-ubsan
TEST_NULL_BUILD_DIR ?= $(BUILD_DIR)-test-null

# WASM and amalgamation build directories
WASM_BUILD_DIR ?= build-wasm
AMALGAMATION_BUILD_DIR ?= build-amalgamation

# CMake build flags
BUILD_TYPE ?= Release
TEST_BUILD_TYPE ?= Test
CMAKE_NATIVE_BASE_FLAGS = -DBUILD_WASM=OFF
NATIVE_CMAKE_FLAGS = -DBUILD_CPP=OFF -DBUILD_SHARED=OFF $(CMAKE_NATIVE_BASE_FLAGS) \
	-DUSE_ASAN=OFF -DUSE_UBSAN=OFF -DALLOW_EMBEDDED_NULLS=OFF
CPP_CMAKE_FLAGS = -DBUILD_CPP=ON -DBUILD_SHARED=OFF $(CMAKE_NATIVE_BASE_FLAGS) \
	-DUSE_ASAN=OFF -DUSE_UBSAN=OFF -DALLOW_EMBEDDED_NULLS=OFF
SHARED_CMAKE_FLAGS = -DBUILD_CPP=OFF -DBUILD_SHARED=ON $(CMAKE_NATIVE_BASE_FLAGS) \
	-DUSE_ASAN=OFF -DUSE_UBSAN=OFF -DALLOW_EMBEDDED_NULLS=OFF
ASAN_CMAKE_FLAGS = -DBUILD_CPP=OFF -DBUILD_SHARED=OFF $(CMAKE_NATIVE_BASE_FLAGS) \
	-DUSE_ASAN=ON -DUSE_UBSAN=OFF -DALLOW_EMBEDDED_NULLS=OFF
UBSAN_CMAKE_FLAGS = -DBUILD_CPP=OFF -DBUILD_SHARED=OFF $(CMAKE_NATIVE_BASE_FLAGS) \
	-DUSE_ASAN=OFF -DUSE_UBSAN=ON -DALLOW_EMBEDDED_NULLS=OFF
NULL_CMAKE_FLAGS = -DBUILD_CPP=OFF -DBUILD_SHARED=OFF $(CMAKE_NATIVE_BASE_FLAGS) \
	-DUSE_ASAN=OFF -DUSE_UBSAN=OFF -DALLOW_EMBEDDED_NULLS=ON
WASM_CMAKE_FLAGS = -DBUILD_CPP=OFF -DBUILD_SHARED=OFF -DBUILD_WASM=ON -DUSE_ASAN=OFF \
	-DUSE_UBSAN=OFF -DALLOW_EMBEDDED_NULLS=OFF

# Source files that trigger regeneration.
GENERATE_SOURCES = \
	utils/generate/generate.sh \
	utils/generate/*.json \
	utils/generate/*.ts \
	utils/generate/locales/*.ts \
	utils/generate/parse-ucd/*.ts \
	utils/generate/site/*.ts \
	utils/generate/tables/*.ts

UNICODE_DATA = src/unicode-data.h

all: configure build

.PHONY: all configure configure-cpp configure-shared configure-asan configure-ubsan configure-null \
	configure-wasm

# C targets
configure: $(UNICODE_DATA)
	@cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) $(NATIVE_CMAKE_FLAGS)

# NULL-safe testing targets
configure-null: $(UNICODE_DATA)
	@cmake -S . -B $(TEST_NULL_BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) $(NULL_CMAKE_FLAGS)

# C++ targets
configure-cpp: $(UNICODE_DATA)
	@cmake -S . -B $(CPP_BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) $(CPP_CMAKE_FLAGS)

# Shared library targets
configure-shared: $(UNICODE_DATA)
	@cmake -S . -B $(SHARED_BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) $(SHARED_CMAKE_FLAGS)

# AddressSanitizer targets
configure-asan: $(UNICODE_DATA)
	@cmake -S . -B $(ASAN_BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) $(ASAN_CMAKE_FLAGS)

# UndefinedBehaviorSanitizer targets
configure-ubsan: $(UNICODE_DATA)
	@cmake -S . -B $(UBSAN_BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) $(UBSAN_CMAKE_FLAGS)

# WASM targets
configure-wasm: $(UNICODE_DATA)
	@emcmake cmake -S . -B $(WASM_BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) $(WASM_CMAKE_FLAGS)

.PHONY: build build-cpp build-shared build-asan build-ubsan build-wasm

# C targets
build: configure
	@cmake --build $(BUILD_DIR) --config $(BUILD_TYPE)

# C++ targets
build-cpp: configure-cpp
	@cmake --build $(CPP_BUILD_DIR) --config $(BUILD_TYPE)

# Shared library targets
build-shared: configure-shared
	@cmake --build $(SHARED_BUILD_DIR) --config $(BUILD_TYPE)

build-asan: configure-asan
	@cmake --build $(ASAN_BUILD_DIR) --config $(BUILD_TYPE)

build-ubsan: configure-ubsan
	@cmake --build $(UBSAN_BUILD_DIR) --config $(BUILD_TYPE)

# WASM targets
build-wasm: configure-wasm
	@cd $(WASM_BUILD_DIR) && emmake make

.PHONY: generate generate-locale generate-unicode-tables \
		generate-site sync-api-wasm watch-site \
		watch-api wasm coverage amalgamation update-version

# Generate source files
generate: $(GENERATE_SOURCES)
	@cd ./utils/generate && ./generate.sh $(ARGS)

# Generate locale files
generate-locale:
	@cd ./utils/generate && npm run generate -- generate-locale

# Generate embedded Unicode lookup tables
generate-unicode-tables:
	@cd ./utils/generate && npm run generate -- unicode-tables

generate-site: src/site/index.html
	@cd ./utils/generate && npm run generate-site

sync-api-wasm: build-wasm
	@cp $(WASM_BUILD_DIR)/src/mojibake.js src/api/mojibake.js
	@cp $(WASM_BUILD_DIR)/src/mojibake.wasm src/api/mojibake.wasm

# Watch site files
watch-site: src/site/index.html
	@cd ./utils/generate && npm run watch-site

# Watch API files
watch-api: sync-api-wasm
	cd ./src/api && npm run dev

# Generate WASM library
wasm: sync-api-wasm generate-site

# Generate TESTS.md file
coverage:
	@cd ./utils/generate && npm run coverage

# Generate amalgamation
amalgamation:
	@cd ./utils/generate && ./generate-amalgamation.sh

# Rule for generated Unicode data
$(UNICODE_DATA): $(GENERATE_SOURCES)
	@cd ./utils/generate && ./generate.sh $(ARGS)

# Update version in source files
update-version:
	@cd ./utils/generate && npm run generate -- update-version

.PHONY: test test-all test-cpp test-asan test-ubsan test-null ctest ctest-cpp test-docker

# Run tests
test: $(UNICODE_DATA)
	@cmake -S . -B $(TEST_BUILD_DIR) -DCMAKE_BUILD_TYPE=$(TEST_BUILD_TYPE) $(NATIVE_CMAKE_FLAGS)
	@cmake --build $(TEST_BUILD_DIR) --config $(TEST_BUILD_TYPE)
	$(TEST_BUILD_DIR)/tests/mojibake-test $(ARGS)

# Run tests with embedded NULL support
test-null: $(UNICODE_DATA)
	@cmake -S . -B $(TEST_NULL_BUILD_DIR) -DCMAKE_BUILD_TYPE=$(TEST_BUILD_TYPE) $(NULL_CMAKE_FLAGS)
	@cmake --build $(TEST_NULL_BUILD_DIR) --config $(TEST_BUILD_TYPE)
	$(TEST_NULL_BUILD_DIR)/tests/mojibake-test $(ARGS)

# Run tests with C++ compiler
test-cpp: $(UNICODE_DATA)
	@cmake -S . -B $(TEST_CPP_BUILD_DIR) -DCMAKE_BUILD_TYPE=$(TEST_BUILD_TYPE) $(CPP_CMAKE_FLAGS)
	@cmake --build $(TEST_CPP_BUILD_DIR) --config $(TEST_BUILD_TYPE)
	$(TEST_CPP_BUILD_DIR)/tests/mojibake-test $(ARGS)

# Run tests with AddressSanitizer
test-asan: $(UNICODE_DATA)
	@cmake -S . -B $(TEST_ASAN_BUILD_DIR) -DCMAKE_BUILD_TYPE=$(TEST_BUILD_TYPE) $(ASAN_CMAKE_FLAGS)
	@cmake --build $(TEST_ASAN_BUILD_DIR) --config $(TEST_BUILD_TYPE)
	$(TEST_ASAN_BUILD_DIR)/tests/mojibake-test $(ARGS)

# Run tests with UndefinedBehaviorSanitizer
test-ubsan: $(UNICODE_DATA)
	@cmake -S . -B $(TEST_UBSAN_BUILD_DIR) -DCMAKE_BUILD_TYPE=$(TEST_BUILD_TYPE) $(UBSAN_CMAKE_FLAGS)
	@cmake --build $(TEST_UBSAN_BUILD_DIR) --config $(TEST_BUILD_TYPE)
	$(TEST_UBSAN_BUILD_DIR)/tests/mojibake-test $(ARGS)

# Run all local test configurations
test-all:
	$(MAKE) test
	$(MAKE) test-null
	$(MAKE) test-cpp
	$(MAKE) test-asan
	$(MAKE) test-ubsan

# Run tests using CTest
ctest: $(UNICODE_DATA)
	@cmake -S . -B $(TEST_BUILD_DIR) -DCMAKE_BUILD_TYPE=$(TEST_BUILD_TYPE) $(NATIVE_CMAKE_FLAGS)
	@cmake --build $(TEST_BUILD_DIR) --config $(TEST_BUILD_TYPE)
	cd $(TEST_BUILD_DIR) && ctest -C $(TEST_BUILD_TYPE) $(ARGS)

# Run C++ tests using CTest
ctest-cpp: $(UNICODE_DATA)
	@cmake -S . -B $(TEST_CPP_BUILD_DIR) -DCMAKE_BUILD_TYPE=$(TEST_BUILD_TYPE) $(CPP_CMAKE_FLAGS)
	@cmake --build $(TEST_CPP_BUILD_DIR) --config $(TEST_BUILD_TYPE)
	cd $(TEST_CPP_BUILD_DIR) && ctest -C $(TEST_BUILD_TYPE) $(ARGS)

# Run tests in Docker container
test-docker:
	docker build -t mojibake .
	docker run mojibake

.PHONY: clean-build clean-native clean-wasm clean-amalgamation clean

# Clean targets
clean-build:
	@cmake --build $(BUILD_DIR) --config $(BUILD_TYPE) --target clean

# Clean native build
clean-native:
	@rm -rf $(BUILD_DIR) $(CPP_BUILD_DIR) $(SHARED_BUILD_DIR) $(ASAN_BUILD_DIR) \
		$(UBSAN_BUILD_DIR) $(TEST_BUILD_DIR) $(TEST_CPP_BUILD_DIR) $(TEST_ASAN_BUILD_DIR) \
		$(TEST_UBSAN_BUILD_DIR) $(TEST_NULL_BUILD_DIR)

# Clean WASM build
clean-wasm:
	@rm -rf $(WASM_BUILD_DIR)

# Clean amalgamation build
clean-amalgamation:
	@rm -rf $(AMALGAMATION_BUILD_DIR)

clean: clean-native clean-wasm clean-amalgamation

.PHONY: help

help:
	@echo "Available targets:"
	@echo "  all                     - Build the project (default)"
	@echo "  amalgamation            - Generate single-file amalgamation"
	@echo "  build-asan              - Build the project with AddressSanitizer"
	@echo "  build-cpp               - Build the project with C++ compiler"
	@echo "  build-shared            - Build the project as a shared library"
	@echo "  build-ubsan             - Build the project with UndefinedBehaviorSanitizer"
	@echo "  build-wasm              - Build the project for WebAssembly"
	@echo "  clean                   - Remove build artifacts"
	@echo "  clean-amalgamation      - Remove amalgamation build artifacts"
	@echo "  clean-build             - Remove build artifacts"
	@echo "  clean-native            - Remove all build artifacts"
	@echo "  clean-wasm              - Remove WASM build artifacts"
	@echo "  coverage                - Run coverage analysis"
	@echo "  ctest                   - Build and run tests using CTest"
	@echo "  ctest-cpp               - Build and run C++ tests using CTest"
	@echo "  generate                - Regenerate source files"
	@echo "  generate-locale         - Generate locale file"
	@echo "  generate-site           - Generate site"
	@echo "  generate-unicode-tables - Generate embedded Unicode lookup tables"
	@echo "  sync-api-wasm           - Copy current WASM build artifacts into src/api"
	@echo "  test                    - Build and run tests"
	@echo "  test-all                - Build and run all local test configurations"
	@echo "  test-asan               - Build and run tests with AddressSanitizer"
	@echo "  test-cpp                - Build and run tests with C++ compiler"
	@echo "  test-docker             - Build and run tests in Docker container"
	@echo "  test-null               - Build and run tests with embedded NULL support"
	@echo "  test-ubsan              - Build and run tests with UndefinedBehaviorSanitizer"
	@echo "  update-version          - Update version in source files"
	@echo "  wasm                    - Build the project for WebAssembly"
	@echo "  watch-site              - Watch site files, regenerate on changes and serve at localhost:6251"
