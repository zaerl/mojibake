BUILD_DIR ?= build
# Build directories
CPP_BUILD_DIR ?= $(BUILD_DIR)-cpp
SHARED_BUILD_DIR ?= $(BUILD_DIR)-shared
ASAN_BUILD_DIR ?= $(BUILD_DIR)-asan
# Build test directories
TEST_BUILD_DIR ?= $(BUILD_DIR)-test
TEST_CPP_BUILD_DIR ?= $(BUILD_DIR)-test-cpp
TEST_ASAN_BUILD_DIR ?= $(BUILD_DIR)-test-asan
TEST_NULL_BUILD_DIR ?= $(BUILD_DIR)-test-null

# WASM and amalgamation build directories
WASM_BUILD_DIR ?= build-wasm
AMALGAMATION_BUILD_DIR ?= build-amalgamation

# CMake build flags
BUILD_TYPE ?= Release
CMAKE_NATIVE_BASE_FLAGS = -DBUILD_WASM=OFF
NATIVE_CMAKE_FLAGS = -DBUILD_CPP=OFF -DBUILD_SHARED=OFF $(CMAKE_NATIVE_BASE_FLAGS) \
	-DUSE_ASAN=OFF -DALLOW_EMBEDDED_NULLS=OFF
CPP_CMAKE_FLAGS = -DBUILD_CPP=ON -DBUILD_SHARED=OFF $(CMAKE_NATIVE_BASE_FLAGS) \
	-DUSE_ASAN=OFF -DALLOW_EMBEDDED_NULLS=OFF
SHARED_CMAKE_FLAGS = -DBUILD_CPP=OFF -DBUILD_SHARED=ON $(CMAKE_NATIVE_BASE_FLAGS) \
	-DUSE_ASAN=OFF -DALLOW_EMBEDDED_NULLS=OFF
ASAN_CMAKE_FLAGS = -DBUILD_CPP=OFF -DBUILD_SHARED=OFF $(CMAKE_NATIVE_BASE_FLAGS) \
	-DUSE_ASAN=ON -DALLOW_EMBEDDED_NULLS=OFF
NULL_CMAKE_FLAGS = -DBUILD_CPP=OFF -DBUILD_SHARED=OFF $(CMAKE_NATIVE_BASE_FLAGS) \
	-DUSE_ASAN=OFF -DALLOW_EMBEDDED_NULLS=ON
WASM_CMAKE_FLAGS = -DBUILD_CPP=OFF -DBUILD_SHARED=OFF -DBUILD_WASM=ON -DUSE_ASAN=OFF \
	-DALLOW_EMBEDDED_NULLS=OFF

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

cmake_configure = cmake -S . -B $(1) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) $(2)
cmake_build = cmake --build $(1) --config $(BUILD_TYPE)

.PHONY: all configure configure-cpp configure-shared configure-asan configure-null configure-wasm

all: configure build

# C targets
configure: $(UNICODE_DATA)
	@$(call cmake_configure,$(BUILD_DIR),$(NATIVE_CMAKE_FLAGS))

# NULL-safe testing targets
configure-null: $(UNICODE_DATA)
	@$(call cmake_configure,$(TEST_NULL_BUILD_DIR),$(NULL_CMAKE_FLAGS))

# C++ targets
configure-cpp: $(UNICODE_DATA)
	@$(call cmake_configure,$(CPP_BUILD_DIR),$(CPP_CMAKE_FLAGS))

# Shared library targets
configure-shared: $(UNICODE_DATA)
	@$(call cmake_configure,$(SHARED_BUILD_DIR),$(SHARED_CMAKE_FLAGS))

# AddressSanitizer targets
configure-asan: $(UNICODE_DATA)
	@$(call cmake_configure,$(ASAN_BUILD_DIR),$(ASAN_CMAKE_FLAGS))

# WASM targets
configure-wasm: $(UNICODE_DATA)
	@emcmake cmake -S . -B $(WASM_BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) $(WASM_CMAKE_FLAGS)

.PHONY: build build-cpp build-shared build-asan build-wasm

# C targets
build: configure
	@$(call cmake_build,$(BUILD_DIR))

# C++ targets
build-cpp: configure-cpp
	@$(call cmake_build,$(CPP_BUILD_DIR))

# Shared library targets
build-shared: configure-shared
	@$(call cmake_build,$(SHARED_BUILD_DIR))

build-asan: configure-asan
	@$(call cmake_build,$(ASAN_BUILD_DIR))

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
	@cp utils/generate/functions.js src/api/functions.js

# Watch site files
watch-site: src/site/index.html
	@cd ./utils/generate && npm run watch-site

# Watch API files
watch-api: sync-api-wasm
	cd ./src/api && node --watch index.js

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

.PHONY: test test-cpp test-asan test-null ctest test-docker

# Run tests
test: BUILD_TYPE = Test
test: $(UNICODE_DATA)
	@$(call cmake_configure,$(TEST_BUILD_DIR),$(NATIVE_CMAKE_FLAGS))
	@$(call cmake_build,$(TEST_BUILD_DIR))
	$(TEST_BUILD_DIR)/tests/mojibake-test $(ARGS)

# Run tests with embedded NULL support
test-null: BUILD_TYPE = Test
test-null: $(UNICODE_DATA)
	@$(call cmake_configure,$(TEST_NULL_BUILD_DIR),$(NULL_CMAKE_FLAGS))
	@$(call cmake_build,$(TEST_NULL_BUILD_DIR))
	$(TEST_NULL_BUILD_DIR)/tests/mojibake-test $(ARGS)

# Run tests with C++ compiler
test-cpp: BUILD_TYPE = Test
test-cpp: $(UNICODE_DATA)
	@$(call cmake_configure,$(TEST_CPP_BUILD_DIR),$(CPP_CMAKE_FLAGS))
	@$(call cmake_build,$(TEST_CPP_BUILD_DIR))
	$(TEST_CPP_BUILD_DIR)/tests/mojibake-test $(ARGS)

# Run tests with AddressSanitizer
test-asan: BUILD_TYPE = Test
test-asan: $(UNICODE_DATA)
	@$(call cmake_configure,$(TEST_ASAN_BUILD_DIR),$(ASAN_CMAKE_FLAGS))
	@$(call cmake_build,$(TEST_ASAN_BUILD_DIR))
	$(TEST_ASAN_BUILD_DIR)/tests/mojibake-test $(ARGS)

# Run tests using CTest
ctest: BUILD_TYPE = Test
ctest: $(UNICODE_DATA)
	@$(call cmake_configure,$(TEST_BUILD_DIR),$(NATIVE_CMAKE_FLAGS))
	@$(call cmake_build,$(TEST_BUILD_DIR))
	cd $(TEST_BUILD_DIR) && ctest -C $(BUILD_TYPE) $(ARGS)

# Run tests in Docker container
test-docker:
	docker build -t mojibake .
	docker run mojibake

.PHONY: clean-build clean-native clean-wasm clean-amalgamation clean

# Clean targets
clean-build:
	@$(call cmake_build,$(BUILD_DIR)) --target clean

# Clean native build
clean-native:
	@rm -rf $(BUILD_DIR) $(CPP_BUILD_DIR) $(SHARED_BUILD_DIR) $(ASAN_BUILD_DIR) \
		$(TEST_BUILD_DIR) $(TEST_CPP_BUILD_DIR) $(TEST_ASAN_BUILD_DIR) $(TEST_NULL_BUILD_DIR)

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
	@echo "  build-wasm              - Build the project for WebAssembly"
	@echo "  clean                   - Remove build artifacts"
	@echo "  clean-amalgamation      - Remove amalgamation build artifacts"
	@echo "  clean-build             - Remove build artifacts"
	@echo "  clean-native            - Remove all build artifacts"
	@echo "  clean-wasm              - Remove WASM build artifacts"
	@echo "  coverage                - Run coverage analysis"
	@echo "  ctest                   - Build and run tests using CTest"
	@echo "  generate                - Regenerate source files"
	@echo "  generate-locale         - Generate locale file"
	@echo "  generate-site           - Generate site"
	@echo "  generate-unicode-tables - Generate embedded Unicode lookup tables"
	@echo "  sync-api-wasm           - Copy current WASM build artifacts into src/api"
	@echo "  test                    - Build and run tests"
	@echo "  test-asan               - Build and run tests with AddressSanitizer"
	@echo "  test-cpp                - Build and run tests with C++ compiler"
	@echo "  test-docker             - Build and run tests in Docker container"
	@echo "  test-null               - Build and run tests with embedded NULL support"
	@echo "  update-version          - Update version in source files"
	@echo "  wasm                    - Build the project for WebAssembly"
	@echo "  watch-site              - Watch site files, regenerate on changes and serve at localhost:6251"
