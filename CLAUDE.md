# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build System

This is a C11 Unicode library using CMake and Make. The library is written to be small, fast and
portable. Key commands:

- `make all` - Full build including configure, build, and database generation
- `make configure` - Configure CMake build system
- `make build` - Build the project (calls cmake --build)
- `make build-embedded` - Build with embedded database (no .db file needed)
- `make test` - Run tests (requires mojibake.db)
- `make test-embedded` - Run tests with embedded database
- `make clean` - Remove build directory and database
- `make generate` - Regenerate Unicode data from utils/generate/
- `make generate-embedded-db` - Generate embedded database header
- `make coverage` - Generate test coverage reports
- `make wasm` - Generate a WASM version of the library

Build configurations: Debug, Release, Test (via BUILD_TYPE environment variable)

### Embedded Database

The library can be built with the database embedded directly in the binary (no separate .db file needed):
- Use `make build-embedded` for a single-file distribution
- CMake option: `-DUSE_EMBEDDED_DB=ON`

## Database Generation

The library requires `mojibake.db` (SQLite database with Unicode data from UCD.zip, Unihan.zip and
emoji archives):

- Generated from Unicode Character Database files in `utils/generate/UCD/`
- Table `unicode_data` has data from the UnicodeData.txt Unicode file
- Table `decompositions` has data to be used in the NFC and NFD normalizations
- Table `compatibility_decompositions` has data using in the NFKC and NFKD normalizations
- Table `property_ranges` has data from the codepoints properties grouped and saved in ranges to save space

## Architecture

Core modules in `src/`:

- `breaking.c` - Unicode Line Breaking algorithm
- `case.c` - Unicode casing methods
- `cjk.c` - CJK ideograph detection
- `mojibake.c/.h` - Main API and library initialization
- `encoding.c` - String encoding detection
- `codepoint.c` - Unicode codepoint operations
- `normalization.c` - Unicode normalization (NFC, NFD, NFKC, NFKD)
- `hangul.c` - Hangul syllable handling
- `plane.c` - Unicode plane operations
- `properties.c` - Codepoints properties
- `segmentation.c` - Word and Grapheme Cluster Breaking algorithm
- `sqlite3/` - Embedded SQLite for Unicode data lookup
- `shell/` provides CLI access to library functions
- `site/` files for generating the WASM version site
- `ext/cpp/mojibake.cpp` a C++ wrapper for the C library

Key headers:

- `unicode.h` - Unicode constants and enums
- `utf*.h` UTF encode/decode functions

Tests in `tests/` mirror the source structure with comprehensive coverage tracking.

## CLI access

The library can be accessed by the build/src/shell/mojibake executable, once compiled. The mojibake.sh
bash script can be used to simplify the access on POSIX systems, the mojibake.bat on Windows.

The CLI accept three commands:

## The `char` command

Used to get information from the `unicode_data` table.

`./mojibake.sh char "A"` return the information in plain text
`./mojibake.sh -o json char "A"` return the information in JSON

## The nfd and nfkd commands

Calculate the NFD and NFKD normalization of the string passed

`./mojibake.sh nfd "ABC"` return the NFD normalized form of the string passed
`./mojibake.sh -c nfd "U+0041" "U+0042" "U+0043"` by specifying `-c` you can pass a list of codepoints

The `NFC` and `NFKC` are yet to be implemented.

## Code Standards

- C11 standard with C17 compiler setting
- 4 spaces indentation, 100 character line limit
- clang-format for style consistency
- Comprehensive test coverage (see TESTS.md)
