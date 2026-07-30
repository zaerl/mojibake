# AGENTS.md

This file is the repository-specific working guide for coding agents.

## Project overview

Mojibake is a small, portable Unicode library. The core is strict C11 and is also compiled as
C++17 to catch C/C++ compatibility regressions. The repository contains:

- the C library and public headers in `src/`
- a header-only C++ wrapper in `src/cpp/mojibake.hpp`
- a native command-line client in `src/shell/`
- an Emscripten/WASM module and TypeScript API in `src/api/`
- a generated single-file distribution in `build-amalgamation/`
- conformance, unit, integration, C++, Node, and browser tests

CMake is the underlying build system; the root `Makefile` is the normal POSIX entry point.
`Makefile.nmake` provides the corresponding Windows/MSVC entry points. The public C API requires a
C11 compiler, and builds with `MJB_BUILD_CPP=ON` require C++17.

The checked-in data currently targets Unicode 18. Take the authoritative library and Unicode
versions from `CMakeLists.txt`, `src/mojibake.h`, and the generator scripts rather than duplicating
version values in new code.

## Working rules

- Run commands from the repository root unless a command below explicitly changes directory.
- Do not hand-edit a block or file marked as automatically generated. Change its generator and
  regenerate it.
- Keep the library buildable as both C11 and C++17. A successful C build alone is not sufficient
  for changes to shared C sources or public headers.
- Preserve embedded-NUL behavior: public string lengths are byte counts, not codepoint or code-unit
  counts. `MJB_NUL_TERMINATED` requests an encoding-aware terminator scan where supported.
- Keep platform portability in mind. CI covers GCC, Clang, Apple Clang, MSVC, MinGW, Emscripten,
  Linux ARM64, BSD systems, and Haiku.
- Add or update tests with behavior changes. Tests normally mirror the source module being changed.
- Do not modify Attractor sources in `tests/attractor/` unless the task is specifically about the
  test framework.

## Dependencies and first build

Native library builds need CMake 3.14 or newer, Make, and a supported C/C++ toolchain. Generator
work also needs Node.js and npm. Install generator dependencies once with:

```sh
cd utils/generate
npm ci
cd ../..
```

For TypeScript API work, also run `(cd src/api && npm ci)`. The WASM and browser workflows
additionally require Emscripten and a Chromium-based browser. Docker is required only for the
Docker and fuzz targets.

## Build commands

- `make` or `make all` - configure and build the default static Release library and CLI
- `make configure` - configure the default native build in `build/`
- `make build` - configure if necessary, then build
- `BUILD_TYPE=Debug make build` - build a Debug configuration
- `make build-cpp` - compile the C sources as C++17 and build the C++ wrapper configuration
- `make build-shared` - build a shared library
- `make build-asan` - build with AddressSanitizer
- `make build-ubsan` - build with UndefinedBehaviorSanitizer
- `FEATURE_CHARACTER_NAMES=OFF make build` - omit the large character-name tables
- `make amalgamation` - generate `mojibake.c`, `mojibake.h`, and `shell.c` in
  `build-amalgamation/`
- `make clean` - remove the Makefile-managed native, WASM, and amalgamation build directories

The common CMake feature switches are `MJB_BUILD_CPP`, `MJB_BUILD_WASM`, `MJB_BUILD_CLI`,
`MJB_BUILD_TESTS`, `MJB_INSTALL`, `MJB_USE_ASAN`, `MJB_USE_UBSAN`,
`MJB_FEATURE_CHARACTER_NAMES`, `MJB_FEATURE_COLLATION`, `MJB_FEATURE_IDNA`,
`MJB_FEATURE_SECURITY`, and the standard `BUILD_SHARED_LIBS`.

## Testing

Use the smallest focused test while iterating, then run the full relevant configurations before
finishing:

- `make test ARGS="-f normalization"` - run C tests whose registered name contains
  `normalization`
- `make test` - run the full C test executable
- `make test-cpp` - compile all C sources and C tests as C++17 and run the C++ wrapper tests
- `make test-asan` - run the C tests with AddressSanitizer
- `make test-ubsan` - run the C tests with UndefinedBehaviorSanitizer
- `make test-no-names` - test with character-name tables disabled
- `make test-minimal` / `make test-cpp-minimal` - test C11 and C++17 with all optional features
  disabled
- `make test-native` - run the default C11 and C++17 configurations
- `make test-features` - run all optional-feature configurations
- `make test-sanitizers` - run the AddressSanitizer and UndefinedBehaviorSanitizer configurations
- `make test-all` - run all local native configurations
- `make ctest` / `make ctest-cpp` - run the registered CMake tests, including embedded-project
  integration tests
- `make test-docker` - build and test in the Alpine Linux container
- `FUZZ_TIME=60 make fuzz` - fuzz the public API with libFuzzer in Docker

For WASM/API work:

- `make sync-api-wasm` - build with Emscripten and copy `mojibake.js` and `mojibake.wasm` into
  `src/api/`
- `make build-api` - sync WASM and build the TypeScript package
- `make test-wasm` - test the currently synced API artifacts in Node and a local browser
- `make wasm` - sync the WASM artifacts and generate the demo site
- `make watch-site` - build, serve, and watch the demo at `http://localhost:6251`
- `make watch-api` - watch the TypeScript API tests

Run `make sync-api-wasm` before `make test-wasm` when C code, exports, or the WASM ABI changed.
`MJB_BROWSER` can select a non-default Chromium-based browser for browser tests.

Tests use Attractor. Use `ATT_ASSERT_STATUS` from `tests/test.h` for `mjb_status` values and
`ATT_ASSERT` for ordinary values and predicates. Test names are registered in `tests/test.c` and
`tests/CMakeLists.txt`; C++ wrapper tests live in `tests/ext/cpp/`, and TypeScript tests live in
`src/api/tests/`. `make coverage` executes the native test binary with coverage accounting and
rewrites `TESTS.md`.

## Generated sources and Unicode data

The generator lives in `utils/generate/` and reads ignored, downloaded inputs from
`utils/generate/unicode-data/`. `make generate` invokes `utils/generate/scripts/generate.sh`; when
the input directories are absent, that script downloads the UCD, Unihan, emoji, collation, and
UTS #39 data before generating the checked-in outputs.

Important boundaries:

- `src/unicode-data.h` is the generated, compressed Unicode payload.
- `src/unicode-tables.c` and `src/unicode-tables.h` are hand-maintained lookup and decoding code.
  Edit them when changing lookup behavior or the payload format.
- `utils/generate/file-generators/unicode-data/` contains the table-specific emitters.
- `utils/generate/unicode-data-store.ts` is the in-memory representation shared by those emitters.
- `utils/generate/functions.ts` is the source of truth for generated public function declarations
  and API documentation metadata.
- `src/locales.h` carries a generated marker, but the current `make generate-locale` path refreshes
  CLDR tooling data rather than rebuilding that enum. Its intended emitter is
  `utils/generate/file-generators/locales-h.ts`; do not hand-edit the enum.

Useful generation targets:

- `make generate` - regenerate all normal checked-in outputs
- `make generate-unicode-tables` - regenerate only `src/unicode-data.h` from already downloaded
  Unicode inputs
- `make generate-site` - regenerate the WASM demo site
- `make generate-locale` - refresh the Italian CLDR data used by the locale generator tooling
- `make update-version` - update version-bearing project, package, test, and documentation files

Full generation updates generated sections or files including `src/unicode-data.h`,
`src/unicode.h`, `src/mojibake.h`, `src/properties.c`, `src/bidi.c`, `src/CMakeLists.txt`,
`src/api/unicode.ts`, `src/api/mojibake.d.ts`, `API.md`, and `tests/example.c`. Follow the generated
markers in those files; surrounding hand-written code remains editable.

When changing the generated table layout, make the change measurable and reversible:

```sh
wc -c \
  src/unicode-data.h \
  build/src/CMakeFiles/mojibake_lib.dir/unicode-tables.c.o \
  build/src/libmojibake.a
```

Capture that snapshot before and after the change. Then run:

```sh
(cd utils/generate && npm exec -- tsc --noEmit)
make generate-unicode-tables
make build
make test
```

Also run focused tests for every affected feature. Keep a compression change only when generated
source and compiled artifacts improve, behavior is unchanged, and performance remains in the same
range. Never judge table work only by the size of `src/unicode-data.h`; compiler output and lookup
cost matter too.

## Source layout

Core modules in `src/`:

- `bidi.c` - Unicode Bidirectional Algorithm (UAX #9)
- `break-line.c` - line breaking (UAX #14)
- `break-sentence.c`, `break-word.c`, `segmentation.c` - sentence, word, and grapheme breaking
  (UAX #29)
- `case.c`, `caseless.c` - full/simple casing, case folding, NFKC casefold, and caseless matching
- `collation.c` - locale-independent DUCET comparison and sort keys
- `codepoint.c`, `properties.c`, `plane.c`, `cjk.c` - character and property queries
- `encoding.c`, `utf.h`, `utf8.h`, `utf16.h`, `utf32.h` - validation, iteration, and conversion
- `normalization.c`, `quick-check.c`, `hangul.c`, `buffer.c/.h` - normalization and Hangul support
- `emoji.c`, `terminal-width.c`, `east-asian-width.c` - emoji classification and terminal width
- `identifier.c`, `security.c` - UAX #31 identifiers and UTS #39 confusables
- `filter.c`, `format.c`, `next.c` - filtering, UTF-8-safe formatting, and character iteration
- `locales.c` - strict BCP 47 parsing and the process-global casing locale
- `mojibake.c`, `mojibake-internal.h`, `string.c` - global state, allocation, output, and shared
  internals
- `unicode-tables.c/.h` - lookup layer over the generated payload
- `version.c` - library and Unicode version queries

Key public headers are `src/mojibake.h`, `src/unicode.h`, and `src/locales.h`. Supporting areas are
`src/shell/` for the CLI, `src/api/` for the TypeScript/WASM wrapper, `src/site/` for the demo,
`fuzz/` for the public-API harness and seed corpus, `examples/` for consumers, and `cmake/` plus
`ports/` for package integration.

## Public API conventions

Public declarations are generated from `utils/generate/functions.ts`; implementations remain in
the corresponding C module. When adding or changing a public function:

1. Update its function metadata and documentation in `utils/generate/functions.ts`.
2. Update the C implementation and focused C tests.
3. Update the C++ wrapper and `tests/ext/cpp/` when the wrapper exposes the API.
4. Update `src/api/index.ts` and `src/api/tests/` when the API is exposed through WASM.
5. Run `make generate` so `src/mojibake.h`, `API.md`, the WASM export list, and declarations stay in
   sync.
6. Run generator and API typechecks plus the relevant native, C++, and WASM tests.

Function metadata supports `details`, `returns`, `example`, `related`, `specs`, and per-argument
`ownership`. `related` entries are validated against the function list.

Result-producing APIs normally return `mjb_status`, are `MJB_NODISCARD`, and use
`MJB_STATUS_OK == 0` for success. Use `bool` only when truth is the semantic result, such as
encoding, codepoint, identifier, category, and emoji predicates.

Allocation and ownership are part of the API contract:

- allocating transforms return an `mjb_result`; release it with `mjb_result_free`
- matching `_into` variants use caller-owned storage, permit `output == NULL` to measure, report
  required byte size, and do not write partial output when capacity is insufficient
- core library allocations must use `mjb_alloc`, `mjb_realloc`, and `mjb_free` so custom memory
  functions remain effective; the standalone CLI may use the C allocator for its own storage
- free specialized owned structures with their matching API, such as
  `mjb_bidi_paragraph_free`

Validate arguments and overflow before dereferencing or allocating. On failure, leave outputs in
the state documented by the API and clean up every owned intermediate allocation.

## CLI

After `make build`, use `build/src/shell/mojibake` directly or the root wrappers
`./mojibake.sh` and `mojibake.bat`.

Global options are `-h/--help`, `-c/--codepoint`, `-j/--json-indent`,
`-o/--output plain|json`, `-s/--show-allowed-symbols`, `-v/--verbose`, and `-V/--version`.
Commands are `bidi`, `break`, `char`, `codepoint`, `emoji`, `filter`, `locale`, `nfd`, `nfkd`,
`nfc`, `nfkc`, `upper`, `lower`, `title`, `casefold`, and `casefold-simple`.

```sh
./mojibake.sh char "A"
./mojibake.sh -o json emoji "☺️"
./mojibake.sh -c nfd "U+0041" "U+0042"
./mojibake.sh break word "Hello World"
./mojibake.sh locale "sr-Latn-RS"
```

## Style and validation

- Use 4 spaces, never tabs, and keep code at 100 columns; `.clang-format` is authoritative.
- Preserve the project license header in new source files.
- Keep warnings clean under the flags in `CMakeLists.txt`.
- `make lint` runs Apple's `xcrun clang-format` and is therefore a macOS convenience target. On
  other platforms, run the equivalent installed `clang-format --dry-run --Werror` command.
- Typecheck generator changes with `(cd utils/generate && npm exec -- tsc --noEmit)`.
- Typecheck API changes with `(cd src/api && npm run typecheck)`.
- Do not regenerate unrelated outputs or include build products in a change.
- Before finishing, inspect `git diff` and report exactly which validation commands were run.
