# AGENTS.md

This file is the repository-specific working guide for coding agents. `CONTRIBUTING.md` is the
human-facing companion; when the two disagree, the `Makefile`, `CMakeLists.txt`, and CI workflows
are authoritative.

## Project overview

Mojibake is a small, portable Unicode library. The core is strict C11 and is also compiled as
C++17 to catch C/C++ compatibility regressions. It implements:

- encoding validation, iteration, and conversion between UTF-8, UTF-16 (LE/BE), and UTF-32
  (LE/BE), with malformed-input policies and diagnostics
- normalization (NFC, NFD, NFKC, NFKD, NFKC_Casefold) and quick checks (UAX #15)
- full and simple casing, case folding, locale-sensitive casing, and caseless matching
- grapheme, word, sentence (UAX #29), and line (UAX #14) segmentation
- the Unicode Bidirectional Algorithm (UAX #9)
- DUCET collation and sort keys (UTS #10)
- IDNA processing and Punycode (UTS #46, RFC 3492)
- identifiers (UAX #31) and confusable/security checks (UTS #39)
- character properties and names, blocks, scripts, numeric values, emoji properties and sequence
  classification (UTS #51), East Asian width (UAX #11), and terminal width
- strict BCP 47 language-tag parsing, filtering, and UTF-8-safe formatting

The repository contains:

- the C library and public headers in `src/`
- a header-only C++20 wrapper in `src/cpp/mojibake.hpp` (namespace `mjb`, see `src/cpp/README.md`)
- a native command-line client in `src/shell/`
- an Emscripten/WASM module and TypeScript API in `src/api/`, plus a demo site in `src/site/`
- a SwiftUI macOS/iOS demo app in `src/ui/Mojibake/` (Xcode project; not built by CMake or CI)
- a generated single-file distribution in `build-amalgamation/`
- consumer examples for C, C++, Python, Rust, and Zig in `examples/`
- conformance, unit, CLI, embedded-project, C++, Node, and browser tests, and a libFuzzer harness

CMake is the underlying build system; the root `Makefile` is the normal POSIX entry point and
`Makefile.nmake` provides matching Windows/MSVC targets. The public C API requires a C11 compiler,
and builds with `MJB_BUILD_CPP=ON` require C++17. The C++ wrapper and its tests require C++20;
`tests/CMakeLists.txt` skips the wrapper tests when the compiler lacks it.

The checked-in data currently targets Unicode 18. Take the authoritative library and Unicode
versions from `VERSION`, `CMakeLists.txt`, `src/mojibake.h`, and the generator scripts rather than
duplicating version values in new code.

## Working rules

- Run commands from the repository root unless a command below explicitly changes directory.
- Do not hand-edit a block or file marked as automatically generated. Change its generator and
  regenerate it. CI regenerates everything and fails if the checked-in outputs differ.
- Keep the library buildable as both C11 and C++17. A successful C build alone is not sufficient
  for changes to shared C sources or public headers.
- Keep optional features optional: code behind `MJB_FEATURE_CHARACTER_NAMES`,
  `MJB_FEATURE_COLLATION`, `MJB_FEATURE_IDNA`, or `MJB_FEATURE_SECURITY` must build and test with
  the feature off. Disabled APIs report `MJB_STATUS_FEATURE_NOT_ENABLED`.
- Preserve embedded-NUL behavior: public string lengths are byte counts, not codepoint or code-unit
  counts. `MJB_NUL_TERMINATED` requests an encoding-aware terminator scan where supported.
- Keep platform portability in mind. Do not assume little-endian hosts, a particular `char`
  signedness, POSIX headers in library code, or compiler extensions without a portable fallback.
- Add or update tests with behavior changes. Tests normally mirror the source module being changed.
- Do not modify the vendored Attractor framework in `tests/attractor.h` unless the task is
  specifically about the test framework. `src/shell/getopt/` is likewise vendored for Windows.
- Do not regenerate unrelated outputs or include build products in a change.
- Commits require a DCO sign-off (`git commit --signoff`) that matches the commit author.

## Dependencies and first build

Native library builds need CMake 3.14 or newer, Make, and a supported C/C++ toolchain. Generator
work also needs Node.js, npm, `curl`, and `unzip`. Install generator dependencies once with:

```sh
(cd utils/generate && npm ci)
```

For TypeScript API work, also run `(cd src/api && npm ci)`. The WASM and browser workflows
additionally require Emscripten and a Chromium-based browser. Docker is required only for the
Docker and fuzz targets. clang-tidy (for example Homebrew `llvm`) is needed only for `make tidy`.

The test executable reads the Unicode conformance files from `unicode-data/` and exits early if
`unicode-data/UCD/CaseFolding.txt` is missing. On a fresh checkout, fetch them without
regenerating anything:

```sh
./utils/generate/scripts/download-test-data.sh
```

`make generate` also downloads any missing inputs.

## Build commands

- `make` or `make all` - configure and build the default static Release library and CLI in
  `build/`
- `make configure` / `make build` - configure only, or configure if necessary and then build
- `BUILD_TYPE=Debug make build` - build a Debug configuration
- `WARNINGS_AS_ERRORS=ON make build` - match CI's warning strictness
- `make build-cpp` - compile the C sources as C++17 in `build-cpp/`
- `make build-shared` - build a shared library in `build-shared/`
- `make build-asan` / `make build-ubsan` - build with AddressSanitizer or
  UndefinedBehaviorSanitizer
- `FEATURE_CHARACTER_NAMES=OFF make build` - omit the large character-name tables; the
  `FEATURE_COLLATION`, `FEATURE_IDNA`, and `FEATURE_SECURITY` variables work the same way
- `make amalgamation` - generate `mojibake.c`, `mojibake.h`, and `shell.c` in
  `build-amalgamation/`
- `make -C examples` - build every consumer example (`example`, `example-cpp`, `example-python`,
  `example-rust`, `example-zig`, and the `example-no-*` feature-disabled variants)

The CMake options are `MJB_BUILD_CPP`, `MJB_BUILD_WASM`, `MJB_BUILD_CLI`, `MJB_BUILD_TESTS`,
`MJB_INSTALL`, `MJB_USE_ASAN`, `MJB_USE_UBSAN`, `MJB_WARNINGS_AS_ERRORS`,
`MJB_FEATURE_CHARACTER_NAMES`, `MJB_FEATURE_COLLATION`, `MJB_FEATURE_IDNA`,
`MJB_FEATURE_SECURITY`, and the standard `BUILD_SHARED_LIBS`. `MJB_BUILD_CLI`, `MJB_BUILD_TESTS`,
and `MJB_INSTALL` default to ON only when Mojibake is the top-level project. Installation exports a
`Mojibake` CMake package (`cmake/MojibakeConfig.cmake.in`).

### Build pitfalls

- Every configure target depends on `src/unicode-data.h`, which depends on the generator sources.
  If a file under `utils/generate/` is newer than `src/unicode-data.h`, `make build` and
  `make test` regenerate the payload first, which needs the downloaded `unicode-data/` inputs.
  When you did not intend to change generated data, `touch src/unicode-data.h` instead of
  regenerating.
- `make clean` removes every Makefile-managed build directory **and** the downloaded
  `unicode-data/` directory. Prefer `make clean-native`, `make clean-wasm`,
  `make clean-amalgamation`, or `make clean-generator`, or delete a single `build-*` directory.

## Testing

Use the smallest focused test while iterating, then run the full relevant configurations before
finishing:

- `make test ARGS="-f normalization"` - run C tests whose name contains `normalization`
  (`-f` takes a substring; add `-v` or `-vv` for more output and `-e` to stop at the first failure)
- `make test` - run the full C test executable (built in `build-test/` with the `Test` build type)
- `make test-cpp` - compile the library as C++17 and the tests as C++20, then run them with the
  C++ wrapper tests
- `make test-release` / `make test-cpp-release` - run optimized C or C++ builds
- `make test-asan` / `make test-ubsan` - run the C tests with a sanitizer
- `make test-no-names`, `make test-no-collation`, `make test-no-idna`, `make test-no-security` -
  test with one optional feature disabled
- `make test-minimal` / `make test-cpp-minimal` - test C11 and C++17 with all optional features
  disabled
- `make test-native` - `test` + `test-cpp`
- `make test-optimized` - `test-release` + `test-cpp-release`
- `make test-features` - every feature-disabled configuration
- `make test-sanitizers` - `test-asan` + `test-ubsan`
- `make test-all` - all of the above local native configurations
- `make ctest` / `make ctest-cpp` - run the registered CMake tests, including CLI output checks
  (`tests/shell/`) and the embedded-project integration test (`tests/cmake/`)
- `make test-docker` - build and test in the Alpine Linux container
- `FUZZ_TIME=60 make fuzz` - fuzz the public API with libFuzzer in Docker (`fuzz/fuzz.c`)

`build-test/tests/mojibake-test` can be run from any directory: it changes into the data root
given by the `MJB_TEST_SOURCE_DIR` environment variable, else the repository root recorded at
build time, else the current directory. See "Testing" in `CONTRIBUTING.md`.

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

### Writing tests

Tests use Attractor. Define a test case with `ATT_TEST(name) { ... }`; it registers itself before
`main()` runs, so no manual registration in `tests/test.c` is needed. Use `ATT_ASSERT(actual,
expected, "description")` for ordinary values and predicates and `ATT_ASSERT_STATUS` from
`tests/test.h` for `mjb_status` values.

- A new test file must be added to `TEST_SOURCES` in `tests/CMakeLists.txt`, and a new top-level
  test name should get a matching `mjb_add_test(...)` CTest entry there.
- Shared helpers live in `tests/utils/` and `tests/test.h`.
- The test allocator in `tests/test.c` checks every size the library passes to realloc and free,
  counts live blocks, and can inject failures with `mjb_test_allocator_fail_after()`. Use it to
  cover out-of-memory paths of allocating APIs.
- `tests/example.c` and `tests/ext/cpp/example.cpp` are generated from the examples in
  `utils/generate/functions.ts`; edit the examples there.
- C++ wrapper tests live in `tests/ext/cpp/`, TypeScript tests in `src/api/tests/`, and CLI output
  tests in `tests/shell/`.

`make coverage` runs the native test binary with coverage accounting and rewrites `TESTS.md`.

## Generated sources and Unicode data

The reference generator is the TypeScript project in `utils/generate/`. It reads ignored,
downloaded inputs from `unicode-data/`. `make generate` invokes
`utils/generate/scripts/generate.sh`, which downloads any missing UCD, Unihan, emoji, collation,
IDNA, and UTS #39 data, checks the IDNA and security data versions, and then generates the
checked-in outputs.

`utils/generator/` is a C++ port of the generator, built in `build-generator/` and driven by
`make generate-cpp` (`ARGS=unicode-tables`, `amalgamation`, `generate-locale`). `make
update-version` uses it. When you change an emitter that both generators implement, keep them
producing identical output.

Important boundaries:

- `src/unicode-data.h` is the generated, compressed Unicode payload.
- `src/unicode-tables.c` and `src/unicode-tables.h` are hand-maintained lookup and decoding code.
  Edit them when changing lookup behavior or the payload format.
- `utils/generate/file-generators/unicode-data/` contains the table-specific emitters.
- `utils/generate/unicode-data-store.ts` is the in-memory representation shared by those emitters.
- `utils/generate/sequence-pool.ts` packs the shared 16-bit codepoint sequence pools used by the
  decomposition, IDNA, and confusable emitters.
- `utils/generate/functions.ts` is the source of truth for generated public function declarations,
  API documentation, WASM exports, and the C and C++ examples.
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
  from `VERSION`

Full generation updates generated sections or files including `src/unicode-data.h`,
`src/unicode.h`, `src/mojibake.h`, `src/properties.c`, `src/bidi.c`, `src/locales.c`,
`src/CMakeLists.txt`, `src/api/unicode.ts`, `src/api/locales.ts`, `src/api/mojibake.d.ts`,
`API.md`, `tests/example.c`, and `tests/ext/cpp/example.cpp`. Follow the generated markers in those
files; surrounding hand-written code remains editable.

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
- `collation.c` - locale-independent DUCET comparison and sort keys (UTS #10)
- `codepoint.c`, `properties.c`, `plane.c`, `cjk.c` - character and property queries
- `encoding.c`, `utf.h` - validation, iteration, and conversion
- `normalization.c`, `quick-check.c`, `hangul.c`, `buffer.c` - normalization and Hangul support
- `emoji.c`, `terminal-width.c`, `east-asian-width.c` - emoji classification and terminal width
- `idna.c`, `punycode.c/.h` - UTS #46 IDNA processing and Punycode
- `identifier.c`, `security.c` - UAX #31 identifiers and UTS #39 confusables
- `filter.c`, `format.c`, `next.c` - filtering, UTF-8-safe formatting, and character iteration
- `locales.c` - strict BCP 47 parsing and the process-global casing locale
- `mojibake.c`, `mojibake-internal.h`, `string.c` - global state, allocation, output, and shared
  internals
- `unicode-tables.c/.h` - lookup layer over the generated payload
- `version.c` - library and Unicode version queries

Key public headers are `src/mojibake.h`, `src/unicode.h`, and `src/locales.h`. Other areas:

- `src/shell/` - the CLI (`main.c`, `shell.c`, one file per command group in `commands/`)
- `src/api/` - TypeScript wrapper (`index.ts`), synced WASM artifacts, and API tests
- `src/site/` - static demo site generated from the WASM build
- `src/ui/Mojibake/` - SwiftUI demo app
- `fuzz/` - libFuzzer harness, seed corpus, and Dockerfile
- `examples/` - C, C++, Python (ctypes), Rust, and Zig consumers
- `cmake/` - installed CMake package config
- `.github/workflows/` - `test.yml` (native matrix and feature configurations), `platforms.yml`
  (BSDs, Haiku, minimum compilers, strict C11 and amalgamation, MinGW, musl, big-endian s390x,
  Android and iOS cross builds), `quality.yml` (clang-format, clang-tidy, TypeScript typechecks,
  generated-file freshness), and `dependency-review.yml`

## Public API conventions

Public declarations are generated from `utils/generate/functions.ts`; implementations remain in
the corresponding C module. When adding or changing a public function:

1. Update its metadata, documentation, and examples in `utils/generate/functions.ts`.
2. Update the C implementation and focused C tests.
3. Update the C++ wrapper and `tests/ext/cpp/` when the wrapper exposes the API.
4. Set `wasm: true` and update `src/api/index.ts` and `src/api/tests/` when the API is exposed
   through WASM.
5. Run `make generate` so `src/mojibake.h`, `API.md`, the WASM export list, declarations, and
   generated examples stay in sync.
6. Run generator and API typechecks plus the relevant native, C++, feature-disabled, and WASM
   tests.

Function metadata supports `details`, `returns`, `examples` (`c` and `cpp`), `exampleFeature`,
`exampleCompileOnly`, `related`, `specs`, `section`, and per-argument `ownership`. `related`
entries are validated against the function list.

Result-producing APIs normally return `mjb_status`, are `MJB_NODISCARD`, and use
`MJB_STATUS_OK == 0` for success. Use `bool` only when truth is the semantic result, such as
encoding, codepoint, identifier, category, and emoji predicates. Decoding APIs take an
`mjb_malformed_policy` (stop, replace with U+FFFD, or skip) and can report an `mjb_diagnostic`.

Allocation and ownership are part of the API contract:

- allocating transforms return an `mjb_result`; release it with `mjb_result_free`. When
  `owned` is false, `output` borrows the caller's input (for example when the input is already
  normalized) and is not NUL-terminated; always use `output_size`
- matching `_into` variants use caller-owned storage, permit `output == NULL` to measure, report
  required byte size, and write in a single pass: when capacity is insufficient or another error
  occurs, the output may hold a partial prefix, but nothing is written past its capacity
- core library allocations must use the private `mjb_alloc`, `mjb_realloc`, and `mjb_free` helpers
  so the configured `mjb_allocator` remains effective; the standalone CLI may use the C allocator
  for its own storage
- `mjb_realloc` and `mjb_free` take the block's exact size, which is the size most recently
  requested for it; requested sizes are never 0. Track the capacity of every growable buffer,
  and hand allocated output to callers with `mjb_result_own` so `mjb_result.capacity` is set.
  The test allocator and the fuzzer check every size the library passes
- free specialized owned structures with their matching API, such as
  `mjb_bidi_paragraph_free`

The allocator and the casing locale are process-global state. The allocator is installed once,
before any other library call.

Validate arguments and overflow before dereferencing or allocating. On failure, leave outputs in
the state documented by the API and clean up every owned intermediate allocation.

## CLI

After `make build`, use `build/src/shell/mojibake` directly or the root wrappers
`./mojibake.sh` and `mojibake.bat`.

Global options are `-c/--codepoint`, `-d/--direction ltr|rtl|auto`, `-h/--help`,
`-j/--json-indent 0-10`, `-l/--locale <BCP 47 tag>`, `-o/--output plain|json`,
`-s/--show-allowed-symbols`, `-v/--verbose`, and `-V/--version`. Commands are `bidi`, `break`,
`char`, `codepoint`, `emoji`, `filter`, `locale`, `nfd`, `nfkd`, `nfc`, `nfkc`, `nfkc-casefold`,
`upper`, `lower`, `title`, `casefold`, and `casefold-simple`.

```sh
./mojibake.sh char "A"
./mojibake.sh -o json emoji "☺️"
./mojibake.sh -c nfd "U+0041" "U+0042"
./mojibake.sh break word "Hello World"
./mojibake.sh -l tr upper "istanbul"
./mojibake.sh locale "sr-Latn-RS"
```

When changing CLI output, update the expectations in `tests/shell/` and run `make ctest`.

## Style and validation

- Use 4 spaces, never tabs, and keep code at 100 columns; `.clang-format` is authoritative.
- Preserve the project license header in new source files.
- Keep warnings clean under the flags in `CMakeLists.txt`; CI builds with
  `WARNINGS_AS_ERRORS=ON`.
- `make lint` runs Apple's `xcrun clang-format` and is therefore a macOS convenience target. On
  other platforms, run the equivalent installed `clang-format --dry-run --Werror` command.
- `make tidy` builds the library, CLI, and tests in `build-tidy/` with clang-tidy enabled, using
  `.clang-tidy`. Set `CLANG_TIDY=/path/to/clang-tidy` to select a specific binary.
- Unused parameters are compiler errors under `MJB_WARNINGS_AS_ERRORS`. Mark intentionally unused
  parameters with `(void)name;` at the start of the function body.
- Typecheck generator changes with `(cd utils/generate && npm exec -- tsc --noEmit)`.
- Typecheck API changes with `(cd src/api && npm run typecheck)`.
- Record user-visible changes in `CHANGELOG.md`.

## Before finishing

Pick the checks that match the change, then inspect `git diff` and report exactly which
validation commands were run:

| Change | Minimum validation |
| --- | --- |
| Shared C source or public header | `make test-native WARNINGS_AS_ERRORS=ON` |
| Code behind a feature flag | the above plus `make test-features` |
| Allocation, buffers, or pointer arithmetic | the above plus `make test-sanitizers` |
| Public API | the above plus `make generate`, `make test-wasm` if WASM-exposed |
| Generator | `tsc --noEmit` in `utils/generate`, `make generate`, then the native tests |
| TypeScript API | `(cd src/api && npm run typecheck)` and `make test-wasm` |
| CLI | `make build` and `make ctest` |
