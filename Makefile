BUILD_DIR ?= build
BUILD_TYPE ?= Release

all: configure build

configure:
	@cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

build: configure
	@cmake --build $(BUILD_DIR)

rebuild: clean_build all

generate:
	cd ./utils && ./generate.sh $(ARGS)

generate_tests:
	cd ./utils && npm run generate-utf8-tests

test: BUILD_TYPE = Test
test: configure build
	WRD_DB_PATH=./mojibake.db build/tests/mojibake-test $(ARGS)

clean_build:
	@cmake --build $(BUILD_DIR) --target clean

clean:
	rm -rf $(BUILD_DIR) && rm -f mojibake.db

.PHONY: all clean clean_build configure build test rebuild generate generate_tests
