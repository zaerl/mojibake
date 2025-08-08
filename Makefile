BUILD_DIR ?= build
BUILD_TYPE ?= Release

# Source files that trigger regeneration.
GENERATE_SOURCES = utils/generate/generate.sh utils/generate/*.json utils/generate/*.ts

all: configure build

configure:
	@cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

build: configure
	@cmake --build $(BUILD_DIR)

rebuild: clean_build all

coverage:
	cd ./utils/generate && npm run coverage

generate: $(GENERATE_SOURCES)
	cd ./utils/generate && ./generate.sh $(ARGS)

test: BUILD_TYPE = Test
test: configure build mojibake.db
	WRD_DB_PATH=./mojibake.db build/tests/mojibake-test $(ARGS)

clean_build:
	@cmake --build $(BUILD_DIR) --target clean

clean:
	rm -rf $(BUILD_DIR) && rm -f mojibake.db

help:
	@echo "Available targets:"
	@echo "  all        - Build the project (default)"
	@echo "  test       - Build and run tests"
	@echo "  clean      - Remove build artifacts"
	@echo "  generate   - Regenerate source files"
	@echo "  coverage   - Run coverage analysis"

.PHONY: all clean clean_build configure build test rebuild generate generate_tests coverage help
