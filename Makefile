BUILD_DIR ?= build
WASM_BUILD_DIR ?= build-wasm
AMALGAMATION_BUILD_DIR ?= build-amalgamation
EMBEDDED_AMALGAMATION_BUILD_DIR ?= build-embedded-amalgamation
BUILD_TYPE ?= Release
NATIVE_CMAKE_FLAGS = -DBUILD_CPP=OFF -DBUILD_SHARED=OFF -DBUILD_WASM=OFF -DUSE_ASAN=OFF \
	-DALLOW_EMBEDDED_NULLS=OFF -DUSE_EMBEDDED_DB=OFF

# Source files that trigger regeneration.
GENERATE_SOURCES = utils/generate/generate.sh utils/generate/*.json utils/generate/*.ts

SQLITE_SOURCES = src/sqlite3/sqlite3.c src/sqlite3/sqlite3.h
UNICODE_TABLES = src/unicode-tables.c

.PHONY: all configure configure-embedded configure-cpp configure-shared configure-asan configure-null configure-wasm

all: configure build

# C targets
configure: $(UNICODE_TABLES)
	@cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) $(NATIVE_CMAKE_FLAGS)

# NULL-safe testing targets
configure-null: $(UNICODE_TABLES)
	@cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) $(NATIVE_CMAKE_FLAGS) -DALLOW_EMBEDDED_NULLS=ON

# Compatibility target: data tables are always embedded now.
configure-embedded: $(UNICODE_TABLES)
	@cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) $(NATIVE_CMAKE_FLAGS) -DUSE_EMBEDDED_DB=ON

# C++ targets
configure-cpp: $(UNICODE_TABLES)
	@cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) $(NATIVE_CMAKE_FLAGS) -DBUILD_CPP=ON

# Shared library targets
configure-shared: $(UNICODE_TABLES)
	@cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) $(NATIVE_CMAKE_FLAGS) -DBUILD_SHARED=ON

# AddressSanitizer targets
configure-asan: $(UNICODE_TABLES)
	@cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) $(NATIVE_CMAKE_FLAGS) -DUSE_ASAN=ON

# WASM targets
configure-wasm: $(UNICODE_TABLES)
	@emcmake cmake -S . -B $(WASM_BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) $(NATIVE_CMAKE_FLAGS) -DBUILD_WASM=ON

.PHONY: build build-embedded build-cpp build-shared build-asan build-wasm

# C targets
build: configure
	@cmake --build $(BUILD_DIR)

# Embedded database targets
build-embedded: configure-embedded
	@cmake --build $(BUILD_DIR)

# C++ targets
build-cpp: configure-cpp
	@cmake --build $(BUILD_DIR)

# Shared library targets
build-shared: configure-shared
	@cmake --build $(BUILD_DIR)

build-asan: configure-asan
	@cmake --build $(BUILD_DIR)

# WASM targets
build-wasm: configure-wasm
	@cd $(WASM_BUILD_DIR) && emmake make

.PHONY: generate generate-locale generate-sqlite generate-embedded-db generate-unicode-tables \
		generate-site sync-api-wasm watch-site \
		watch-api wasm coverage amalgamation update-version

# Generate source files and database
generate: $(GENERATE_SOURCES)
	@cd ./utils/generate && ./generate.sh $(ARGS)

# Generate locale files
generate-locale:
	@cd ./utils/generate && npm run generate -- generate-locale

# Manual target to force regeneration
generate-sqlite:
	@cd ./utils/sqlite3 && ./generate-sqlite.sh

# Generate embedded database header
generate-embedded-db:
	@cd ./utils/generate && npm run generate -- embedded-db

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
mojibake.db $(UNICODE_TABLES): $(GENERATE_SOURCES)
	@cd ./utils/generate && ./generate.sh $(ARGS)

# Rule to generate SQLite source files (only if they don't exist)
$(SQLITE_SOURCES):
	@cd ./utils/sqlite3 && ./generate-sqlite.sh

# Update version in source files
update-version:
	@cd ./utils/generate && npm run generate -- update-version

.PHONY: test test-embedded test-cpp test-asan test-null ctest test-docker

# Run tests
test: BUILD_TYPE = Test
test: configure build
	build/tests/mojibake-test $(ARGS)

# Run tests with embedded NULL support
test-null: BUILD_TYPE = Test
test-null: configure-null build
	build/tests/mojibake-test $(ARGS)

# Run tests with the embedded-table compatibility configuration
test-embedded: BUILD_TYPE = Test
test-embedded: configure-embedded build-embedded
	build/tests/mojibake-test $(ARGS)

# Run tests with C++ compiler
test-cpp: BUILD_TYPE = Test
test-cpp: configure-cpp build-cpp
	build/tests/mojibake-test $(ARGS)

# Run tests with AddressSanitizer
test-asan: BUILD_TYPE = Test
test-asan: configure-asan build-asan
	build/tests/mojibake-test $(ARGS)

# Run tests using CTest
ctest: BUILD_TYPE = Test
ctest: configure build
	cd $(BUILD_DIR) && ctest $(ARGS)

# Run tests in Docker container
test-docker:
	docker build -t mojibake .
	docker run mojibake

.PHONY: clean-build clean-native clean-wasm clean-amalgamation clean-embedded-amalgamation \
		clean-database clean-sqlite clean

# Clean targets
clean-build:
	@cmake --build $(BUILD_DIR) --target clean

# Clean native build
clean-native:
	@rm -rf $(BUILD_DIR)

# Clean WASM build
clean-wasm:
	@rm -rf $(WASM_BUILD_DIR)

# Clean amalgamation build
clean-amalgamation:
	@rm -rf $(AMALGAMATION_BUILD_DIR)

# Clean embedded amalgamation build
clean-embedded-amalgamation:
	@rm -rf $(EMBEDDED_AMALGAMATION_BUILD_DIR)

clean-embedded-amalgamation-header:
	@rm -f src/embedded-db.h

# Clean main database file
clean-database:
	@rm -f mojibake.db

# Clean generated SQLite source files
clean-sqlite:
	@rm -f $(SQLITE_SOURCES)

clean: clean-native clean-wasm clean-amalgamation clean-embedded-amalgamation clean-embedded-amalgamation-header clean-database clean-sqlite

.PHONY: help

help:
	@echo "Available targets:"
	@echo "  all          - Build the project (default)"
	@echo "  build-embedded - Compatibility alias for embedded-table build"
	@echo "  build-cpp    - Build the project with C++ compiler"
	@echo "  build-shared - Build the project as a shared library"
	@echo "  build-asan   - Build the project with AddressSanitizer"
	@echo "  build-wasm   - Build the project for WebAssembly"
	@echo "  generate     - Regenerate source files"
	@echo "  generate-locale - Generate locale file"
	@echo "  generate-sqlite - Generate SQLite source files"
	@echo "  generate-embedded-db - Generate legacy embedded database header"
	@echo "  generate-unicode-tables - Generate embedded Unicode lookup tables"
	@echo "  generate-site - Generate site"
	@echo "  sync-api-wasm - Copy current WASM build artifacts into src/api"
	@echo "  wasm         - Build the project for WebAssembly"
	@echo "  coverage     - Run coverage analysis"
	@echo "  amalgamation - Generate single-file amalgamation"
	@echo "  update-version - Update version in source files"
	@echo "  watch-site   - Watch site files, regenerate on changes and serve at localhost:6251"
	@echo "  test         - Build and run tests"
	@echo "  test-embedded - Build and run tests with embedded-table build"
	@echo "  test-cpp     - Build and run tests with C++ compiler"
	@echo "  test-asan    - Build and run tests with AddressSanitizer"
	@echo "  test-null    - Build and run tests with embedded NULL support"
	@echo "  ctest        - Build and run tests using CTest"
	@echo "  test-docker  - Build and run tests in Docker container"
	@echo "  clean-build  - Remove build artifacts"
	@echo "  clean-native - Remove all build artifacts"
	@echo "  clean-wasm   - Remove WASM build artifacts"
	@echo "  clean-amalgamation - Remove amalgamation build artifacts"
	@echo "  clean-embedded-amalgamation - Remove embedded amalgamation build artifacts"
	@echo "  clean-database - Remove main database file"
	@echo "  clean-sqlite - Remove generated SQLite source files"
	@echo "  clean        - Remove build artifacts"
