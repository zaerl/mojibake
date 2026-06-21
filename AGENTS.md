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
- `make watch-site` - Serve the WASM site locally at localhost:8080, regenerate on changes
- `make watch-api` - Start the JavaScript API server at localhost:3000, restart on changes

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

- `bidi.c` - Unicode Bidirectional Algorithm (TR9)
- `break-line.c` - Unicode Line Breaking algorithm (TR14)
- `break-sentence.c` - Unicode Sentence Breaking algorithm (TR29)
- `break-word.c` - Unicode Word Breaking algorithm (TR29)
- `buffer.c/.h` - Internal buffer used during normalization
- `case.c` - Unicode casing methods (upper, lower, title, casefold)
- `cjk.c` - CJK ideograph detection
- `codepoint.c` - Unicode codepoint operations
- `display.c` - Display width calculation (`mjb_display_width`)
- `east-asian-width.c` - East Asian width property (`mjb_codepoint_east_asian_width`)
- `emoji.c` - Emoji property detection (`mjb_codepoint_emoji`)
- `encoding.c` - String encoding detection
- `ext/cpp/mojibake.hpp` - Header-only C++ wrapper for the C library
- `filter.c` - String filtering (`mjb_string_filter`)
- `hangul.c` - Hangul syllable handling
- `mojibake.c/.h` - Main API and library initialization
- `next.c` - Character-by-character iteration (`mjb_next_character`)
- `normalization.c` - Unicode normalization (NFC, NFD, NFKC, NFKD)
- `plane.c` - Unicode plane operations
- `properties.c` - Codepoints properties
- `quick-check.c` - Normalization quick-check (`mjb_string_is_normalized`)
- `segmentation.c` - Grapheme Cluster Breaking algorithm (TR29)
- `shell/` provides CLI access to library functions
- `site/` files for generating the WASM version site
- `sqlite3/` - Embedded SQLite for Unicode data lookup
- `string.c` - Internal string output utilities
- `version.c` - Version query functions

Key headers:

- `unicode.h` - Unicode constants and enums
- `utf*.h` UTF encode/decode functions

Tests in `tests/` mirror the source structure with comprehensive coverage tracking. The
`tests/attractor/` directory contains the Attractor unit test framework used across all tests.
C++ wrapper tests are in `tests/ext/cpp/`.

## CLI access

The library can be accessed by the `build/src/shell/mojibake` executable, once compiled. The
`mojibake.sh` bash script can be used to simplify access on POSIX systems, `mojibake.bat` on Windows.

### Global options

- `-c / --codepoint` — interpret input as a list of codepoints (e.g. `U+0041`)
- `-j / --json-indent <0-10>` — pretty-print JSON output with the given indent level
- `-o / --output <plain|json>` — output format (default: `plain`)
- `-s / --show-allowed-symbols` — show allowed symbols
- `-v / --verbose` — verbose output
- `-V / --version` — print library version
- `-w / --width <n>` — output width

### Commands

- `bidi` - print the results of the bidirectional algorithm
- `break` — break the input into grapheme clusters and line breaks
- `char` — print character information for a string
- `codepoint` — print character information for a codepoint
- `filter` — filter input (normalize to NFC, strip spaces/controls/numeric characters)
- `nfd` / `nfkd` / `nfc` / `nfkc` — normalize input to the given Unicode normalization form
- `upper` / `lower` / `title` / `casefold` — case conversion

### Examples

```
./mojibake.sh char "A"                 # plain text character info
./mojibake.sh -o json char "A"         # JSON character info
./mojibake.sh nfd "ABC"                # NFD normalization
./mojibake.sh -c nfd "U+0041" "U+0042" # normalize from codepoint list
./mojibake.sh upper "Hello"            # uppercase conversion
./mojibake.sh break "Hello World"      # grapheme/line break analysis
```

## JavaScript API

The `src/api/` directory contains a Node.js HTTP server that exposes library functions over HTTP
using the WASM build of the library (requires `mojibake.wasm` and `mojibake.db` in that directory).

- `make watch-api` — start the server with `node --watch` (auto-restarts on file changes)
- Server listens on `http://0.0.0.0:3000`

### Request format

All requests are `GET` with query parameters:

- `?function=<name>` — the `mjb_*` function to call (e.g. `mjb_codepoint_character`)
- Additional parameters match the function's argument names

Without a `function` parameter the server returns the full list of available functions from
`functions.js`.

### Examples

```
# Get character info for codepoint 0x41 ('A')
GET http://localhost:3000/?function=mjb_codepoint_character&codepoint=U+0041

# Normalize a string to NFD
GET http://localhost:3000/?function=mjb_normalize&buffer=café&encoding=UTF-8&form=NFD
```

### Source files

- `index.js` — HTTP server, request routing, WASM interop
- `functions.js` — function registry (name, return type, argument types)
- `cstruct.js` — reads C structs from WASM linear memory
- `mojibake.js` / `mojibake.wasm` — compiled WASM library (generated by `make wasm`)

## Code Standards

- C11 standard with C17 compiler setting
- 4 spaces indentation, 100 character line limit
- clang-format for style consistency
- Comprehensive test coverage (see TESTS.md)
