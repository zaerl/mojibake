# AGENTS.md

This file provides guidance to coding agents when working with code in this repository.

## Build System

This is a C11 Unicode library using CMake and Make. The library is written to be small, fast and
portable. Key commands:

- `make all` - Full build including configure and build
- `make configure` - Configure CMake build system
- `make build` - Build the project (calls cmake --build)
- `make build-cpp` - Build the project with a C++ compiler
- `make build-shared` - Build the project as a shared library
- `make build-asan` - Build the project with AddressSanitizer
- `make test` - Build and run tests
- `make test-null` - Build and run tests with embedded NULL support
- `make test-cpp` - Build and run tests with a C++ compiler
- `make test-asan` - Build and run tests with AddressSanitizer
- `make clean` - Remove build artifacts
- `make generate` - Regenerate Unicode data from utils/generate/
- `make generate-unicode-tables` - Regenerate embedded Unicode lookup tables
- `make sync-api-wasm` - Copy the current WASM build output into `src/api`
- `make coverage` - Generate test coverage reports
- `make wasm` - Generate a WASM version of the library
- `make watch-site` - Serve the WASM site locally at localhost:6251, regenerate on changes
- `make watch-api` - Start the JavaScript API server at localhost:3000, restart on changes

Build configurations: Debug, Release, Test (via BUILD_TYPE environment variable)

## Unicode Data Generation

Runtime Unicode data lives in generated C tables:

- `src/unicode-data.h`
- `src/unicode-tables.h`
- `src/unicode-tables.c`

The generator reads downloaded Unicode data from `utils/generate/unicode-data/`.
It builds the generated C tables directly in memory. Prefer changing
`utils/generate/generate-unicode-tables.ts` and regenerating the tables instead of hand-editing
generated C.

The generated tables are intentionally compressed. Current techniques include:

- sparse page indexes for codepoint-keyed tables instead of direct page arrays
- packed 32-bit and 64-bit records for blocks, prefixes, emoji, decompositions, compositions,
  numeric values, simple case mappings, special casing, case folding, and collation contractions
- shared string, codepoint, and byte payload tables for names, decompositions, confusables, and
  collation weights
- interned property blobs and bitsets for compact property and mirrored-character data
- substring/suffix sharing for confusable skeletons and collation weight sequences

When changing generated table layout, keep each step measurable and reversible:

- capture this size snapshot before and after the change:
  ```sh
  wc -c \
    src/unicode-tables.c \
    build/src/CMakeFiles/mojibake_lib.dir/unicode-tables.c.o \
    build/src/libmojibake_lib.a
  ```
- run `node_modules/.bin/tsc --noEmit` from `utils/generate/`
- run `make generate-unicode-tables`, `make build`, focused tests for affected features, and
  `make test`
- keep the change only when behavior is unchanged, performance stays in the same range, and the
  generated source plus compiled artifacts are smaller

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
- `cpp/mojibake.hpp` - Header-only C++ wrapper for the C library
- `display.c` - Display width calculation (`mjb_display_width`)
- `east-asian-width.c` - East Asian width property (`mjb_codepoint_east_asian_width`)
- `emoji.c` - Emoji property detection (`mjb_codepoint_emoji`)
- `encoding.c` - String encoding detection, codepoint encoding, and encoding conversion
- `filter.c` - String filtering (`mjb_string_filter`)
- `hangul.c` - Hangul syllable handling
- `locales.c` - Locale APIs, including strict BCP 47 parsing (`mjb_locale_parse`)
- `mojibake.c/.h` - Main API and library initialization
- `next.c` - Character-by-character iteration (`mjb_next_character`)
- `normalization.c` - Unicode normalization (NFC, NFD, NFKC, NFKD)
- `plane.c` - Unicode plane operations
- `properties.c` - Codepoints properties
- `quick-check.c` - Normalization quick-check (`mjb_string_is_normalized`)
- `segmentation.c` - Grapheme Cluster Breaking algorithm (TR29)
- `shell/` provides CLI access to library functions
- `site/` files for generating the WASM version site
- `string.c` - Internal string output utilities
- `unicode-tables.c/.h` - Generated Unicode lookup tables
- `version.c` - Version query functions

Key headers:

- `locales.h` - Generated ISO-639-2 locale enum
- `mojibake.h` - Public API, including `mjb_locale_id`, `mjb_error`, and `mjb_locale_parse`
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
- `break` — break the input into grapheme, word, line, and sentence boundaries
- `char` — print character information for a string
- `codepoint` — print character information for a codepoint
- `filter` — filter input (normalize to NFC, strip spaces/controls/numeric characters)
- `locale` — parse a BCP 47 language tag
- `nfd` / `nfkd` / `nfc` / `nfkc` — normalize input to the given Unicode normalization form
- `upper` / `lower` / `title` / `casefold` — case conversion

### Examples

```
./mojibake.sh char "A"                 # plain text character info
./mojibake.sh -o json char "A"         # JSON character info
./mojibake.sh nfd "ABC"                # NFD normalization
./mojibake.sh -c nfd "U+0041" "U+0042" # normalize from codepoint list
./mojibake.sh upper "Hello"            # uppercase conversion
./mojibake.sh break "Hello World"      # all break analyses
./mojibake.sh break word "Hello World" # word break analysis
./mojibake.sh locale "sr-Latn-RS"      # BCP 47 language tag parsing
```

## JavaScript API

The `src/api/` directory contains a Node.js HTTP server that exposes library functions over HTTP
using the WASM build of the library (requires generated `mojibake.js` and `mojibake.wasm` in that
directory).

- `make wasm` — build WASM, generate the site, and refresh `src/api/mojibake.js`,
  `src/api/mojibake.wasm`, and `src/api/functions.js`
- `make sync-api-wasm` — refresh only the generated API artifacts from `build-wasm/src`
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
