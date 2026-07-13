# Mojibake

[![Test](https://github.com/zaerl/mojibake/actions/workflows/test.yml/badge.svg)](https://github.com/zaerl/mojibake/actions/workflows/test.yml)
[![OpenSSF Best Practices](https://www.bestpractices.dev/projects/13557/badge)](https://www.bestpractices.dev/projects/13557)

**Mojibake** is a low-level Unicode 17 text-processing library written in C11 and compatible
with C++17. It is released under the MIT License.

It aims to be:

1. Small
2. Easy to use
3. Fast
4. Self-contained

Mojibake do:

1. Run in all modern OSes (Linux, macOS, FreeBSD, OpenBSD, NetBSD, Windows 10/11)
2. Pass the official Unicode test suites for supported algorithms
3. Implement all Unicode standard algorithms
4. Satisfy all [Unicode Conformance Requirements](https://github.com/zaerl/mojibake/blob/main/CONFORMANCE_REQUIREMENTS.md)

You can find a demo site where you can find the API documentation and test the functions by using
WASM here: [https://mojibake.zaerl.com](https://mojibake.zaerl.com)

## Feature highlights

All the C files, together with the Unicode data tables, are concatenated into a single large file
and header: `mojibake.c` and `mojibake.h`. Zero dependencies.

**Text transformation**

- **Normalization**: NFC/NFD/NFKC/NFKD (`mjb_normalize`), identifier-oriented NFKC case folding
  (`mjb_nfkc_casefold`), plus a fast quick-check
  (`mjb_string_is_normalized`) ([UAX #15, Unicode 17.0.0](https://www.unicode.org/reports/tr15/tr15-57.html))
- **Case conversion**: uppercase, lowercase, titlecase, and case folding with full special-casing
  and conditional mappings (`mjb_case`)
- **Filtering**: strip controls, spaces, or numeric characters while normalizing
  (`mjb_string_filter`)

**Text analysis**

- **Character database**: every Unicode Character Database property: category, script and
  Script_Extensions, block, plane, numeric value, name (`mjb_codepoint_character`,
  `mjb_codepoint_script_extensions`)
- **Segmentation**: grapheme clusters, words, sentences, and line-break opportunities
  ([UAX #29, Unicode 17.0.0](https://www.unicode.org/reports/tr29/tr29-47.html),
  [UAX #14, Unicode 17.0.0](https://www.unicode.org/reports/tr14/tr14-55.html))
- **Bidirectional text**: full Unicode Bidirectional Algorithm: paragraph resolution, line
  reordering, runs ([UAX #9, Unicode 17.0.0](https://www.unicode.org/reports/tr9/tr9-51.html))
- **Emoji**: codepoint properties, sequence analysis, RGI emoji detection
- **Display width**: East Asian width and terminal display width, with width-aware truncation
  (`mjb_display_width`, `mjb_truncate_width`)

**Sorting and comparison**

- **Collation**: Unicode Collation Algorithm string comparison and sort keys, in shifted and
  non-ignorable modes (`mjb_string_compare`, `mjb_collation_key`,
  [UTS #10, Unicode 17.0.0](https://www.unicode.org/reports/tr10/tr10-53.html))

**Security**

- **Confusable detection**: generate reusable skeletons and check if strings are visually
  confusable (`mjb_confusable_skeleton`, `mjb_string_is_confusable`,
  [UTS #39, Unicode 17.0.0](https://www.unicode.org/reports/tr39/tr39-32.html))
- **Identifier validation**: XID/ID checks for parser and compiler authors
  (`mjb_string_is_identifier`, [UAX #31, Unicode 17.0.0](https://www.unicode.org/reports/tr31/tr31-43.html))

**Integration**

- **Encodings**: the API accepts and outputs UTF-8, UTF-16LE, UTF-16BE, UTF-32LE, UTF-32BE
  strings, with encoding detection and conversion (`mjb_string_encoding`,
  `mjb_string_convert_encoding`)
- **Parsing and string functions**: character-by-character iteration (`mjb_next_character`) and
  standard C `string.h`-style helpers (`mjb_string_length`, and others)
- **Locales**: strict BCP 47 language tag parsing (`mjb_locale_parse`)
- **Embeddable**: custom allocators (`mjb_set_memory_functions`), build-time feature flags to trim
  table size, a C++17 wrapper (`src/cpp/mojibake.hpp`), a CLI tool (`src/shell`), and a
  WASM + TypeScript API (`src/api`)
- **Tested**: Mojibake uses [Attractor](https://github.com/zaerl/attractor/) as test suite and run
  [1.5M+ assertions](https://github.com/zaerl/mojibake/blob/main/TESTS.md) including the
  official Unicode conformance suites for supported algorithms
- **Fuzz** It's fuzzed with `libFuzzer`
- `AddressSanitizer` and `UBSan` clean

## Usage

You don't need to install anything. Add the C source and header to your build.

1. Download it here
[mojibake-amalgamation-025.zip](https://github.com/zaerl/mojibake/releases/download/v0.2.5/mojibake-amalgamation-025.zip)
2. Unzip it
3. Add `mojibake.c` and `mojibake.h` to your project

Example:

```c
#include <stdio.h>
#include <string.h>
#include "mojibake.h"

// This is a simple example of how to use the Mojibake library.
int main(int argc, char * const argv[]) {
    printf("This is an example of Mojibake v%s\n", mjb_version());
    printf("Unicode version: %s\n", mjb_unicode_version());

    const char *input = "Cafe\xCC\x81";
    mjb_result result;

    if(mjb_normalize(input, strlen(input), MJB_ENC_UTF_8, MJB_NORMALIZATION_NFC, MJB_ENC_UTF_8,
        &result) != MJB_STATUS_OK) {
        return 1;
    }

    // This outputs "NFC: Café", e + ◌́ -> é
    printf("NFC: %.*s\n", (int)result.output_size, result.output);

    if(result.transformed) {
        mjb_free(result.output);
    }

    return 0;
}
```

### Build-time features

Mojibake can compile out optional feature tables to reduce binary size. Feature macros default to
enabled.

- `MJB_FEATURE_CHARACTER_NAMES` controls the Unicode character-name tables used by
`mjb_codepoint_character(...)` to fill `mjb_character.name`. When disabled, the tables are not
compiled and `mjb_character.name` is reported as `Codepoint U+XXXX`.

With CMake:

```bash
cmake -S . -B build-no-name -DMJB_FEATURE_CHARACTER_NAMES=OFF
cmake --build build-no-name
```

With the provided Makefile:

```bash
make build BUILD_DIR=build-no-name FEATURE_CHARACTER_NAMES=OFF
make test-no-names
```

### API documentation

See [API.md](https://github.com/zaerl/mojibake/blob/main/API.md) for the detailed documentation.

### CLI

The `src/shell` directory builds the `mojibake` CLI used to test the library. Example usage:

```bash
mojibake nfc $'Cafe\u0301'
```

Plain text output:

```bash
Café
```

Emoji sequence analysis:

```bash
mojibake emoji "☺️"
mojibake -c emoji 263A FE0F
```

### Coverage

Mojibake run a total of **1,610,136** tests, including all the official tests included in the
standard:

1. [auxiliary/GraphemeBreakTest.txt](https://www.unicode.org/Public/17.0.0/ucd/auxiliary/GraphemeBreakTest.txt)
2. [auxiliary/LineBreakTest.txt](https://www.unicode.org/Public/17.0.0/ucd/auxiliary/LineBreakTest.txt)
3. [auxiliary/SentenceBreakTest.txt](https://www.unicode.org/Public/17.0.0/ucd/auxiliary/SentenceBreakTest.txt)
4. [auxiliary/WordBreakTest.txt](https://www.unicode.org/Public/17.0.0/ucd/auxiliary/WordBreakTest.txt)
5. [BidiCharacterTest.txt](https://www.unicode.org/Public/17.0.0/ucd/BidiCharacterTest.txt)
6. [BidiTest.txt](https://www.unicode.org/Public/17.0.0/ucd/BidiTest.txt)
7. [CaseFolding.txt](https://www.unicode.org/Public/17.0.0/ucd/CaseFolding.txt)
8. [CollationTest/CollationTest_NON_IGNORABLE.txt](https://www.unicode.org/Public/17.0.0/uca/CollationTest.zip)
9. [CollationTest/CollationTest_SHIFTED.txt](https://www.unicode.org/Public/17.0.0/uca/CollationTest.zip)
10. [emoji-test.txt](https://www.unicode.org/Public/17.0.0/emoji/emoji-test.txt)
11. [intentional.txt](https://www.unicode.org/Public/security/latest/intentional.txt)
12. [NormalizationTest.txt](https://www.unicode.org/Public/17.0.0/ucd/NormalizationTest.txt)
13. [SpecialCasing.txt](https://www.unicode.org/Public/17.0.0/ucd/SpecialCasing.txt)

### Fuzzing

The public API is fuzzed with [libFuzzer](https://llvm.org/docs/LibFuzzer.html) over untrusted
byte input.

```bash
make fuzz # 60 seconds by default
make fuzz FUZZ_TIME=300
```

## Building from source

See [CONTRIBUTING.md](https://github.com/zaerl/mojibake/blob/main/CONTRIBUTING.md) for instructions.

## Licenses

Mojibake is released under the MIT License (see [LICENSE](https://github.com/zaerl/mojibake/blob/main/LICENSE)).

## Legalese

Here you can find the very detailed and boring informations needed to have this library conformant
to the Unicode standard, or at least what I got, at
[CONFORMANCE_REQUIREMENTS.md](https://github.com/zaerl/mojibake/blob/main/CONFORMANCE_REQUIREMENTS.md)

## Thanks

Mojibake is built using the work of extraordinary individuals and teams.

1. Unicode Character Database - Copyright © 1991-2026 Unicode, Inc.
(see [license.txt](https://www.unicode.org/license.txt))
2. Unicode CLDR Project - Copyright © 2004-2026 Unicode, Inc.
(see [LICENSE](https://raw.githubusercontent.com/unicode-org/cldr/refs/heads/main/LICENSE))
