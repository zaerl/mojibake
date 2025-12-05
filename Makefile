BUILD_DIR ?= build
WASM_BUILD_DIR ?= build-wasm
AMALGAMATION_BUILD_DIR ?= build-amalgamation
EMBEDDED_AMALGAMATION_BUILD_DIR ?= build-embedded-amalgamation
BUILD_TYPE ?= Release

# Source files that trigger regeneration.
GENERATE_SOURCES = utils/generate/generate.sh utils/generate/*.json utils/generate/*.ts

all: configure build

# C targets
configure:
	@cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

# Embedded database build
configure-embedded: generate-embedded-db
	@cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DUSE_EMBEDDED_DB=ON

# C targets
build: configure
	@cmake --build $(BUILD_DIR)

build-embedded: configure-embedded
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
wasm: build-wasm generate-site

# Generate TESTS.md file
coverage:
	cd ./utils/generate && bun run coverage

# Generate source files and database
generate: $(GENERATE_SOURCES)
	cd ./utils/generate && ./generate.sh $(ARGS)

# Rule for mojibake.db
mojibake.db: $(GENERATE_SOURCES)
	cd ./utils/generate && ./generate.sh $(ARGS)

# Generate locale files
generate-locales:
	cd ./utils/generate && ./generate-locales.sh

# Generate amalgamation
generate-amalgamation:
	cd ./utils/generate && ./generate-amalgamation.sh

generate-sqlite:
	cd ./utils/sqlite3 && ./generate-sqlite.sh

# Generate embedded database header
generate-embedded-db:
	@cd ./utils/generate && bun run generate -- embedded-db

generate-site: src/site/index.html
	cd ./utils/generate && bun run generate -- site

update-version:
	cd ./utils/generate && bun run generate -- update-version

watch-site:
	cd ./utils/generate && bunx --bun chokidar-cli "../../src/site/**/*" -c "bun run generate -- site && echo '[Regenerated]'" --initial

# Serve WASM site with live reload
serve: wasm generate-site
	cd $(WASM_BUILD_DIR)/src && python3 -m http.server

# Run tests
test: BUILD_TYPE = Test
test: configure build mojibake.db
	build/tests/mojibake-test $(ARGS)

# Run tests with embedded database
test-embedded: BUILD_TYPE = Test
test-embedded: configure-embedded build-embedded
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

clean-amalgamation:
	rm -rf $(AMALGAMATION_BUILD_DIR)

clean-embedded-amalgamation:
	rm -rf $(EMBEDDED_AMALGAMATION_BUILD_DIR)

clean-database:
	rm mojibake.db

clean: clean-native clean-wasm clean-amalgamation clean-embedded-amalgamation clean-database

help:
	@echo "Available targets:"
	@echo "  all          - Build the project (default)"
	@echo "  build-cpp    - Build the project with C++ compiler"
	@echo "  build-asan   - Build the project with AddressSanitizer"
	@echo "  build-embedded - Build with embedded database (no .db file needed)"
	@echo "  test         - Build and run tests"
	@echo "  test-embedded - Build and run tests with embedded database"
	@echo "  test-cpp     - Build and run tests with C++ compiler"
	@echo "  test-asan    - Build and run tests with AddressSanitizer"
	@echo "  ctest        - Build and run tests using CTest"
	@echo "  test-docker  - Build and run tests in Docker container"
	@echo "  wasm         - Build the project for WebAssembly"
	@echo "  serve        - Serve site"
	@echo "  clean        - Remove build artifacts"
	@echo "  clean-wasm   - Remove WASM build artifacts"
	@echo "  clean-native - Remove all build artifacts"
	@echo "  clean-amalgamation - Remove amalgamation build artifacts"
	@echo "  clean-embedded-amalgamation - Remove embedded amalgamation build artifacts"
	@echo "  generate     - Regenerate source files"
	@echo "  generate-embedded-db - Generate embedded database header"
	@echo "  generate-amalgamation - Generate single-file amalgamation"
	@echo "  update-version - Update version in source files"
	@echo "  coverage     - Run coverage analysis"

.PHONY: all clean clean-native clean-wasm clean-build clean-amalgamation clean-embedded-amalgamation \
		configure configure-wasm configure-cpp configure-asan configure-embedded build build-wasm \
		build-cpp build-asan build-embedded wasm test test-embedded test-cpp test-asan ctest test-docker \
		generate coverage serve help generate-site update-version generate-embedded-db generate-amalgamation
