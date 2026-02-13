BUILD_DIR ?= build
WASM_BUILD_DIR ?= build-wasm
AMALGAMATION_BUILD_DIR ?= build-amalgamation
EMBEDDED_AMALGAMATION_BUILD_DIR ?= build-embedded-amalgamation
BUILD_TYPE ?= Release

# Source files that trigger regeneration.
GENERATE_SOURCES = utils/generate/generate.sh utils/generate/*.json utils/generate/*.ts

# SQLite source files
SQLITE_SOURCES = src/sqlite3/sqlite3.c src/sqlite3/sqlite3.h

.PHONY: all configure configure-embedded configure-cpp configure-shared configure-asan configure-null configure-wasm

all: configure build mojibake.db

# C targets
configure: $(SQLITE_SOURCES)
	@cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

# NULL-safe testing targets
configure-null: $(SQLITE_SOURCES)
	@cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DALLOW_EMBEDDED_NULLS=ON

# Embedded database targets
configure-embedded: $(SQLITE_SOURCES) generate-embedded-db
	@cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DUSE_EMBEDDED_DB=ON

# C++ targets
configure-cpp: $(SQLITE_SOURCES)
	@cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DBUILD_CPP=ON

# Shared library targets
configure-shared: $(SQLITE_SOURCES)
	@cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DBUILD_SHARED=ON

# AddressSanitizer targets
configure-asan: $(SQLITE_SOURCES)
	@cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DUSE_ASAN=ON

# WASM targets
configure-wasm: $(SQLITE_SOURCES)
	@emcmake cmake -S . -B $(WASM_BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DBUILD_WASM=ON

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

.PHONY: generate generate-locales generate-sqlite generate-embedded-db generate-site watch-site wasm coverage \
		amalgamation

# Generate source files and database
generate: $(GENERATE_SOURCES)
	@cd ./utils/generate && ./generate.sh $(ARGS)

# Generate locale files
generate-locales:
	@cd ./utils/generate && ./generate-locales.sh

# Manual target to force regeneration
generate-sqlite:
	@cd ./utils/sqlite3 && ./generate-sqlite.sh

# Generate embedded database header
generate-embedded-db:
	@cd ./utils/generate && npm run generate -- embedded-db

generate-site: src/site/index.html
	@cd ./utils/generate && npm run generate-site

watch-site: src/site/index.html
	@cd ./utils/generate && npm run watch-site

# Generate WASM library
wasm: build-wasm generate-site

# Generate TESTS.md file
coverage:
	@cd ./utils/generate && npm run coverage

# Generate amalgamation
amalgamation:
	@cd ./utils/generate && ./generate-amalgamation.sh

# Rule for mojibake.db
mojibake.db: $(GENERATE_SOURCES)
	@cd ./utils/generate && ./generate.sh $(ARGS)

# Rule to generate SQLite source files (only if they don't exist)
$(SQLITE_SOURCES):
	@cd ./utils/sqlite3 && ./generate-sqlite.sh

# Tools
.PHONY: update-version serve

# Update version in source files
update-version:
	@cd ./utils/generate && npm run generate -- update-version

# Watch API files
watch-api:
	cd ./src/api && node --watch index.js

# Serve WASM site with live reload
serve: wasm generate-site
	cd $(WASM_BUILD_DIR)/src && python3 -m http.server

.PHONY: test test-embedded test-cpp test-asan test-null ctest test-docker

# Run tests
test: BUILD_TYPE = Test
test: configure build mojibake.db
	build/tests/mojibake-test $(ARGS)

# Run tests with embedded NULL support
test-null: BUILD_TYPE = Test
test-null: configure-null build mojibake.db
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
	@rm mojibake.db

# Clean generated SQLite source files
clean-sqlite:
	@rm -f $(SQLITE_SOURCES)

clean: clean-native clean-wasm clean-amalgamation clean-embedded-amalgamation clean-embedded-amalgamation-header clean-database clean-sqlite

.PHONY: help

help:
	@echo "Available targets:"
	@echo "  all          - Build the project (default)"
	@echo "  build-embedded - Build with embedded database (no .db file needed)"
	@echo "  build-cpp    - Build the project with C++ compiler"
	@echo "  build-shared - Build the project as a shared library"
	@echo "  build-asan   - Build the project with AddressSanitizer"
	@echo "  build-wasm   - Build the project for WebAssembly"
	@echo "  generate     - Regenerate source files"
	@echo "  generate-locales - Generate locale files"
	@echo "  generate-sqlite - Generate SQLite source files"
	@echo "  generate-embedded-db - Generate embedded database header"
	@echo "  generate-site - Generate site"
	@echo "  wasm         - Build the project for WebAssembly"
	@echo "  coverage     - Run coverage analysis"
	@echo "  amalgamation - Generate single-file amalgamation"
	@echo "  update-version - Update version in source files"
	@echo "  watch-site   - Watch site files and regenerate on changes"
	@echo "  serve        - Serve site"
	@echo "  test         - Build and run tests"
	@echo "  test-embedded - Build and run tests with embedded database"
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

