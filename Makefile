BUILD_DIR ?= build
WASM_BUILD_DIR ?= build-wasm
BUILD_TYPE ?= Release

# Source files that trigger regeneration.
GENERATE_SOURCES = utils/generate/generate.sh utils/generate/*.json utils/generate/*.ts

all: configure build

# C targets
configure:
	@cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

# C targets
build: configure
	@cmake --build $(BUILD_DIR)

# C++ targets
configure-cpp:
	@cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DBUILD_CPP=ON

# C++ targets
build-cpp: configure-cpp
	@cmake --build $(BUILD_DIR)

# AddressSanitizer targets
configure-asan:
	@cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DUSE_ASAN=ON

build-asan: configure-asan
	@cmake --build $(BUILD_DIR)

# WASM targets
configure-wasm:
	@emcmake cmake -S . -B $(WASM_BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DBUILD_WASM=ON

# WASM targets
build-wasm: configure-wasm
	@cd $(WASM_BUILD_DIR) && emmake make

# WASM targets
wasm: build-wasm
	cd ./utils/generate && npm run generate -- site

# Generate TESTS.md file
coverage:
	cd ./utils/generate && npm run coverage

# Generate source files and database
generate: $(GENERATE_SOURCES)
	cd ./utils/generate && ./generate.sh $(ARGS)

# Generate locale files
generate-locales:
	cd ./utils/generate && ./generate-locales.sh

# Generate amalgamation
generate-amalgamation:
	cd ./utils/generate && npm run generate -- amalgamation

generate-sqlite:
	cd ./utils/sqlite3 && ./generate-sqlite.sh

# Run tests
test: BUILD_TYPE = Test
test: configure build mojibake.db
	build/tests/mojibake-test $(ARGS)

# Run tests with C++ compiler
test-cpp: BUILD_TYPE = Test
test-cpp: configure-cpp build-cpp mojibake.db
	build/tests/mojibake-test $(ARGS)

# Run tests with AddressSanitizer
test-asan: BUILD_TYPE = Test
test-asan: configure-asan build-asan mojibake.db
	build/tests/mojibake-test $(ARGS)

# Run tests using CTest
ctest: BUILD_TYPE = Test
ctest: configure build mojibake.db
	cd $(BUILD_DIR) && ctest $(ARGS)

# Run tests in Docker container
test-docker:
	docker build -t mojibake .
	docker run mojibake

clean-build:
	@cmake --build $(BUILD_DIR) --target clean

clean-native:
	rm -rf $(BUILD_DIR)

clean-wasm:
	rm -rf $(WASM_BUILD_DIR)

clean: clean-native clean-wasm

help:
	@echo "Available targets:"
	@echo "  all          - Build the project (default)"
	@echo "  build-cpp    - Build the project with C++ compiler"
	@echo "  build-asan   - Build the project with AddressSanitizer"
	@echo "  test-cpp     - Build and run tests with C++ compiler"
	@echo "  test-asan    - Build and run tests with AddressSanitizer"
	@echo "  wasm         - Build the project for WebAssembly"
	@echo "  test         - Build and run tests"
	@echo "  ctest        - Build and run tests using CTest"
	@echo "  test-docker  - Build and run tests in Docker container"
	@echo "  clean        - Remove build artifacts"
	@echo "  clean-wasm   - Remove WASM build artifacts"
	@echo "  clean-native - Remove all build artifacts"
	@echo "  generate     - Regenerate source files"
	@echo "  coverage     - Run coverage analysis"

.PHONY: all clean clean-native clean-wasm clean-build configure configure-wasm configure-cpp \
		configure-asan build build-wasm build-cpp build-asan wasm test test-cpp test-asan ctest \
		test-docker generate coverage help
