# Mojibake

[![Test](https://github.com/zaerl/mojibake/actions/workflows/test.yml/badge.svg)](https://github.com/zaerl/mojibake/actions/workflows/test.yml)
[![OpenSSF Best Practices](https://www.bestpractices.dev/projects/13557/badge)](https://www.bestpractices.dev/projects/13557)

**Mojibake** is a small, fast, self-contained Unicode library written in C11. It gives C and C++
programs normalization, case mapping, segmentation, bidirectional text, collation, IDNA, emoji,
confusable detection, and the full Unicode Character Database, all in one source file and one
header, with zero dependencies. It is released under the MIT License.

[Live demo and API reference](https://mojibake.zaerl.com) ·
[API.md](https://github.com/zaerl/mojibake/blob/main/API.md) ·
[C++ wrapper](https://github.com/zaerl/mojibake/blob/main/src/cpp/README.md) ·
[Changelog](https://github.com/zaerl/mojibake/blob/main/CHANGELOG.md) ·
[Roadmap](https://github.com/zaerl/mojibake/blob/main/ROADMAP.md)

## Why Mojibake

- **Drop-in.** Add `mojibake.c` and `mojibake.h` to your project and compile. No build system,
  no generated files, no runtime data to ship.
- **Complete.** Implements the Unicode algorithms most programs need, targeting Unicode 18.0.0.
- **Correct.** Passes the official Unicode conformance test suites for every supported algorithm,
  with millions of assertions run on every change. See the
  [conformance notes](https://github.com/zaerl/mojibake/blob/main/CONFORMANCE_REQUIREMENTS.md)
- **Encoding-agnostic.** Reads and writes UTF-8, UTF-16LE/BE, and UTF-32LE/BE, with explicit
  policies for malformed input and full support for embedded `U+0000`.
- **Embeddable.** Custom allocators, caller-owned output buffers, and build-time switches to drop
  tables you do not need.
- **Portable.** Builds as C11 and C++17 on Linux, macOS, Windows, the BSDs, Haiku, Android, and
  WebAssembly.

## Quick start

You don't need to install anything. There are two files (`mojibake.c`, `mojibake.h`) to add to your
C/C++ project. Download it here [mojibake-amalgamation-051.zip](https://github.com/zaerl/mojibake/releases/download/v0.5.1/mojibake-amalgamation-051.zip).

```c
#include <stdio.h>

#include "mojibake.h"

int main(void) {
    mjb_result result;

    // "Cafe" + U+0301 COMBINING ACUTE ACCENT (6 bytes) -> "Café" with U+00E9 (5 bytes)
    mjb_text cafe = mjb_text_utf8("Cafe\xCC\x81", 6);

    if(mjb_normalize(&cafe, MJB_MALFORMED_STOP, MJB_NORMALIZATION_NFC, MJB_ENC_UTF_8, &result,
           NULL) == MJB_STATUS_OK) {
        printf("NFC: %.*s (%zu bytes)\n", (int)result.output_size, result.output,
            result.output_size);
        mjb_result_free(&result);
    }

    // One flag, two codepoints, eight bytes
    mjb_text flag = mjb_text_utf8("🇮🇹", MJB_NUL_TERMINATED);
    size_t codepoints = 0;
    size_t graphemes = 0;

    if(mjb_codepoint_count(&flag, MJB_MALFORMED_STOP, &codepoints, NULL) == MJB_STATUS_OK &&
        mjb_grapheme_count(&flag, MJB_MALFORMED_STOP, &graphemes, NULL) == MJB_STATUS_OK) {
        printf("🇮🇹: %zu codepoints, %zu grapheme\n", codepoints, graphemes);
    }

    // Caseless identifier matching: "Straße" -> "strasse"
    mjb_text street = mjb_text_utf8("Straße", MJB_NUL_TERMINATED);

    if(mjb_nfkc_casefold(&street, MJB_MALFORMED_STOP, MJB_ENC_UTF_8, &result, NULL) ==
        MJB_STATUS_OK) {
        printf("Straße -> %.*s\n", (int)result.output_size, result.output);
        mjb_result_free(&result);
    }

    return 0;
}
```

```bash
cc -std=c11 example.c mojibake.c -o example
./example
```

```
NFC: Café (5 bytes)
🇮🇹: 2 codepoints, 1 grapheme
Straße -> strasse
```

A few conventions apply across the whole API:

- Functions return `mjb_status`, and **`MJB_STATUS_OK` is zero**: always compare against it.
  Predicates such as `mjb_is_utf8` return `bool`.
- Lengths are **byte counts**. Pass an exact length when the input may contain `U+0000`, or
  `MJB_NUL_TERMINATED` to scan for a terminator sized for the declared encoding.
- Allocating functions fill an `mjb_result` that you release with `mjb_result_free`. Their `_into`
  variants write into a buffer you own instead, and can measure the required size first.

[API.md](https://github.com/zaerl/mojibake/blob/main/API.md) explains these rules in depth.

<!-- site-skip -->
Every function can also be tried in the browser on the [demo site](https://mojibake.zaerl.com),
which runs the library compiled to WebAssembly.
<!-- /site-skip -->

## Feature highlights

**Text transformation**

- **Normalization**: NFC/NFD/NFKC/NFKD (`mjb_normalize`, `mjb_normalize_into`),
  identifier-oriented NFKC case folding (`mjb_nfkc_casefold`, `mjb_nfkc_casefold_into`), plus a
  fast quick-check
  (`mjb_normalization_quick_check`) ([UAX #15, Unicode 18.0.0](https://www.unicode.org/reports/tr15/tr15-58.html))
- **Case conversion**: uppercase, lowercase, titlecase, and case folding with full special-casing
  and conditional mappings (`mjb_map_case`, `mjb_map_case_into`)
- **Internationalized domain names**: strict nontransitional ToASCII and ToUnicode processing,
  including Punycode and validation (`mjb_idna_to_ascii`, `mjb_idna_to_unicode`)
  ([UTS #46](https://www.unicode.org/reports/tr46/))
- **Filtering**: strip controls, spaces, or numeric characters while normalizing
  (`mjb_filter`, `mjb_filter_into`)

**Text analysis**

- **Character database**: every Unicode Character Database property: category, script and
  Script_Extensions, block, plane, numeric value, name (`mjb_codepoint_info`,
  `mjb_codepoint_script_extensions`)
- **Segmentation**: grapheme clusters, words, sentences, and line-break opportunities
  ([UAX #29, Unicode 18.0.0](https://www.unicode.org/reports/tr29/tr29-49.html),
  [UAX #14, Unicode 18.0.0](https://www.unicode.org/reports/tr14/tr14-57.html))
- **Bidirectional text**: full Unicode Bidirectional Algorithm: paragraph resolution, line
  reordering, runs ([UAX #9, Unicode 18.0.0](https://www.unicode.org/reports/tr9/tr9-52.html))
- **Emoji**: codepoint properties, sequence analysis, RGI emoji detection
- **Terminal width**: grapheme-aware terminal-cell estimates with explicit narrow or East Asian
  profiles, emoji-sequence handling, and width-aware truncation
  (`mjb_terminal_width`, `mjb_grapheme_prefix_length_for_width`)

**Sorting and comparison**

- **Caseless matching**: Unicode default, canonical, compatibility, and identifier
  case-insensitive equality (`mjb_caseless_match`,
  [Unicode 18.0.0 Section 3.13.5](https://www.unicode.org/versions/Unicode18.0.0/core-spec/chapter-3/#G34145))
- **Collation**: Unicode Collation Algorithm string comparison and sort keys, with primary through
  quaternary strengths and shifted or non-ignorable variable weighting (`mjb_collation_compare`,
  `mjb_collation_key`, `mjb_collation_key_into`,
  [UTS #10, Unicode 18.0.0](https://www.unicode.org/reports/tr10/tr10-55.html))

**Security**

- **Mixed-script detection**: resolve a string to its augmented Unicode script set for
  identifier-policy checks (`mjb_resolved_script_set`,
  [UTS #39, Unicode 18.0.0](https://www.unicode.org/reports/tr39/tr39-34.html))
- **Confusable detection**: generate reusable skeletons and check if strings are visually
  confusable (`mjb_confusable_skeleton`, `mjb_confusable_skeleton_into`, `mjb_confusable_match`,
  [UTS #39, Unicode 18.0.0](https://www.unicode.org/reports/tr39/tr39-34.html))
- **Identifier validation**: XID/ID checks for parser and compiler authors
  (`mjb_is_identifier`, [UAX #31, Unicode 18.0.0](https://www.unicode.org/reports/tr31/tr31-45.html))

**Encodings and parsing**

- **Encodings**: the API accepts and outputs UTF-8, UTF-16LE, UTF-16BE, UTF-32LE, UTF-32BE
  strings, with detailed validation and explicit stop, replacement, or skip policies during
  conversion (`mjb_validate_encoding`, `mjb_convert_encoding`, `mjb_convert_encoding_into`)
- **Parsing and string functions**: forward and reverse decoding with malformed-input diagnostics
  (`mjb_decode_next`, `mjb_decode_previous`), codepoint callbacks (`mjb_for_each_codepoint`), and
  policy-aware codepoint counting (`mjb_codepoint_count`)
- **Locales**: strict BCP 47 language tag parsing (`mjb_locale_parse`)

**Quality**

- **Tested**: Mojibake uses [Attractor](https://github.com/zaerl/attractor/) as test suite and runs
  [millions of assertions](https://github.com/zaerl/mojibake/blob/main/TESTS.md), including the
  official Unicode conformance suites for supported algorithms
- **Fuzzed**: the public API is fuzzed with [libFuzzer](https://llvm.org/docs/LibFuzzer.html) over
  untrusted byte input
- **Sanitized**: the test suite runs clean under AddressSanitizer and UndefinedBehaviorSanitizer

## Unicode conformance

Unicode references in Mojibake are to the Unicode Standard, Version 18.0.0, and the Unicode
Character Database 18.0.0. Each advertised algorithm is checked against the official Unicode test
data where it exists; the full inventory is in
[CONFORMANCE_REQUIREMENTS.md](https://github.com/zaerl/mojibake/blob/main/CONFORMANCE_REQUIREMENTS.md).

Tailoring: `mjb_map_case` and `mjb_map_case_into` take a per-call `locale`. `MJB_LOCALE_NOT_SET`
selects the default Unicode mappings; Turkish and Azerbaijani tailor dotted-I casing and Turkic
case folding, and Lithuanian tailors dot-above casing. No other algorithm is locale-tailored:
collation uses the untailored DUCET.

## Using Mojibake

### Amalgamation

The [release](https://github.com/zaerl/mojibake/releases) zip contains `mojibake.c` and
`mojibake.h`, which hold every C source file and the Unicode data tables. Compile `mojibake.c`
with the rest of your project; any C11 or C++17 compiler works. The zip also contains `shell.c`,
an amalgamation of the CLI.

### CMake dependency

Mojibake can be embedded with either `add_subdirectory` or `FetchContent` and exposes the
`Mojibake::mojibake` target:

```cmake
include(FetchContent)

FetchContent_Declare(
    Mojibake
    GIT_REPOSITORY https://github.com/zaerl/mojibake.git
    GIT_TAG main
)
FetchContent_MakeAvailable(Mojibake)

target_link_libraries(your-target PRIVATE Mojibake::mojibake)
```

Tests, the CLI, and installation rules default to enabled for a standalone build and disabled when
Mojibake is embedded. They can be controlled with `MJB_BUILD_TESTS`, `MJB_BUILD_CLI`, and
`MJB_INSTALL`. Use CMake's standard `BUILD_SHARED_LIBS` option to select a shared or static
library. To install Mojibake as a package for `find_package(Mojibake CONFIG REQUIRED)`, see
[INSTALL.md](https://github.com/zaerl/mojibake/blob/main/INSTALL.md).

### C++20 wrapper

The header-only wrapper `src/cpp/mojibake.hpp` adds string and view types, RAII ownership, standard
ranges for segments and codepoints, and `std::system_error` exceptions:

```cpp
std::string text = mjb::nfc("Cafe\xCC\x81"); // "Caf\xC3\xA9"
std::u8string folded = mjb::casefold(u8"Straße"); // u8"strasse"
size_t clusters = mjb::grapheme_count(u"A\U0001F1EE\U0001F1F9"); // 2
```

See [src/cpp/README.md](https://github.com/zaerl/mojibake/blob/main/src/cpp/README.md) for details.

### WebAssembly and TypeScript

`src/api` packages the library compiled with Emscripten behind a typed TypeScript API for Node and
browsers. It powers the [demo site](https://mojibake.zaerl.com).

### Other languages

Mojibake has a plain C ABI, so any language with a C FFI can use it. The
[examples](https://github.com/zaerl/mojibake/tree/main/examples) directory has small programs in C,
C++, Python, Rust, and Zig.

### CLI

The `src/shell` directory builds the `mojibake` command-line tool, handy for inspecting text:

```bash
# Normalize to NFC: e + ◌́ -> é
mojibake nfc $'Café'

# Describe an emoji sequence: U+263A U+FE0F, basic and fully qualified
mojibake emoji "☺️"

# Show word boundaries
mojibake break word "Hello World"
```

Run `mojibake --help` for the full list of commands.

## Build-time features

Mojibake can compile out optional implementations and tables to reduce binary size. Features
default to enabled. CMake and the root Makefiles use the boolean values `ON` and `OFF`; when
invoking a C or C++ compiler directly, define the corresponding preprocessor macros as numeric
`1` or `0`.

- `MJB_FEATURE_CHARACTER_NAMES` controls the Unicode character-name tables used by
  `mjb_codepoint_info(...)` to fill `mjb_character.name`. When disabled, the tables are not
  compiled and `mjb_character.name` is reported as `Codepoint U+XXXX`. This reduces the compiled
  size by about 30%.
- `MJB_FEATURE_COLLATION` controls the Unicode Collation Algorithm implementation and DUCET
  tables. The `mjb_collation_*` functions return `MJB_STATUS_FEATURE_NOT_ENABLED` when support is
  disabled.
- `MJB_FEATURE_IDNA` controls the UTS #46 implementation, Punycode implementation, and IDNA
  mapping tables. The `mjb_idna_*` functions return `MJB_STATUS_FEATURE_NOT_ENABLED` when support is
  disabled.
- `MJB_FEATURE_SECURITY` controls the UTS #39 resolved-script and confusable implementations and
  confusable mapping tables. The `mjb_resolved_script_set`, `mjb_confusable_skeleton`,
  `mjb_confusable_skeleton_into`, and `mjb_confusable_match` functions return
  `MJB_STATUS_FEATURE_NOT_ENABLED` when support is disabled.

When compiling the amalgamation directly:

```bash
cc -DMJB_FEATURE_CHARACTER_NAMES=0 -DMJB_FEATURE_COLLATION=0 -DMJB_FEATURE_IDNA=0 \
  -DMJB_FEATURE_SECURITY=0 example.c mojibake.c -o example
```

With CMake:

```bash
cmake -S . -B build-minimal \
  -DMJB_FEATURE_CHARACTER_NAMES=OFF \
  -DMJB_FEATURE_COLLATION=OFF \
  -DMJB_FEATURE_IDNA=OFF \
  -DMJB_FEATURE_SECURITY=OFF
cmake --build build-minimal
```

With the provided Makefile:

```bash
make build BUILD_DIR=build-minimal FEATURE_CHARACTER_NAMES=OFF FEATURE_COLLATION=OFF \
  FEATURE_IDNA=OFF FEATURE_SECURITY=OFF
```

## Platform support

Mojibake requires a C11 compiler. Building the sources as C++ requires C++17, and the header-only
C++ wrapper requires C++20. The minimum supported compiler versions are:

| Toolchain   | Minimum version            |
| ----------- | -------------------------- |
| GCC         | 9                          |
| LLVM Clang  | 10                         |
| Apple Clang | 10                         |
| MSVC        | 19.30 (Visual Studio 2022) |

MinGW-w64 and Emscripten (WASM) are also tested as rolling compatibility targets.

Continuous integration covers:

1. Ubuntu Linux (gcc, g++, clang, clang++)
2. Ubuntu Linux ARM64 (gcc, g++)
3. Ubuntu Linux (Android NDK / ARM64)
4. Alpine Linux (musl x86_64, musl i386, musl s390x)
5. macOS (clang, clang++)
6. FreeBSD 15.1 (clang, clang++)
7. OpenBSD 7.9 (clang, clang++)
8. NetBSD 10.1 (clang, clang++)
9. Haiku R1/beta6 (clang, clang++)
10. Windows 11 (MSVC, MinGW-w64)

## Documentation

- [API.md](https://github.com/zaerl/mojibake/blob/main/API.md): concepts, conventions, and every
  public function
- [mojibake.zaerl.com](https://mojibake.zaerl.com): the same reference with live WASM examples
- [src/cpp/README.md](https://github.com/zaerl/mojibake/blob/main/src/cpp/README.md): the C++20
  wrapper
- [INSTALL.md](https://github.com/zaerl/mojibake/blob/main/INSTALL.md): installing as a CMake package
- [TESTS.md](https://github.com/zaerl/mojibake/blob/main/TESTS.md): test coverage and the Unicode
  test files used
- [CONFORMANCE_REQUIREMENTS.md](https://github.com/zaerl/mojibake/blob/main/CONFORMANCE_REQUIREMENTS.md):
  how Mojibake meets the Unicode conformance requirements

## Contributing

Bug reports and pull requests are welcome. See
[CONTRIBUTING.md](https://github.com/zaerl/mojibake/blob/main/CONTRIBUTING.md) for build and test
instructions, and [SECURITY.md](https://github.com/zaerl/mojibake/blob/main/SECURITY.md) to report
a vulnerability.

## License

Mojibake is released under the MIT License (see
[LICENSE](https://github.com/zaerl/mojibake/blob/main/LICENSE)).

## Thanks

Mojibake is built using the work of extraordinary individuals and teams.

1. Unicode Character Database - Copyright © 1991-2026 Unicode, Inc.
(see [license.txt](https://www.unicode.org/license.txt))
2. Unicode CLDR Project - Copyright © 2004-2026 Unicode, Inc.
(see [LICENSE](https://raw.githubusercontent.com/unicode-org/cldr/refs/heads/main/LICENSE))
