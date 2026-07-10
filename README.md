# Mojibake

[![Test](https://github.com/zaerl/mojibake/actions/workflows/test.yml/badge.svg)](https://github.com/zaerl/mojibake/actions/workflows/test.yml)
[![OpenSSF Best Practices](https://www.bestpractices.dev/projects/13557/badge)](https://www.bestpractices.dev/projects/13557)

**Mojibake**[^1] is a low-level Unicode 17 library written in C11. It can be compiled
as C++17 as well. It is released under the MIT License.

It aims to be:

1. Small
2. Easy to use
3. Fast
4. Self-contained

Mojibake do:

1. Run in all modern OSes (Linux, macOS, FreeBSD, OpenBSD, NetBSD, Windows 10/11)
2. Pass the official Unicode test suites for supported algorithms
3. Implement all Unicode standard algorithms
4. Satisfy all [Unicode Conformance Requirements](https://github.com/zaerl/mojibake/blob/main/CONFORMANCE-REQUIREMENTS.md)

You can find a demo site where you can find the API documentation and test the functions by using WASM here: [https://mojibake.zaerl.com](https://mojibake.zaerl.com)

## Feature highlights

All the C files, together with the Unicode data tables, are concatenated into a single large file
and header: `mojibake.c` and `mojibake.h`. Zero dependencies.

**Text transformation**

- **Normalization**: NFC/NFD/NFKC/NFKD (`mjb_normalize`), plus a fast quick-check
  (`mjb_string_is_normalized`) ([UAX #15, Unicode 17.0.0](https://www.unicode.org/reports/tr15/tr15-57.html))
- **Case conversion**: uppercase, lowercase, titlecase, and case folding with full special-casing
  and conditional mappings (`mjb_case`)
- **Filtering**: strip controls, spaces, or numeric characters while normalizing
  (`mjb_string_filter`)

**Text analysis**

- **Character database**: every Unicode Character Database property: category, script, block,
  plane, numeric value, name (`mjb_codepoint_character`)
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

- **Confusable detection**: check if a string is visually confusable with another
  (`mjb_string_is_confusable`, [UTS #39, Unicode 17.0.0](https://www.unicode.org/reports/tr39/tr39-32.html))
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
[mojibake-amalgamation-024.zip](https://github.com/zaerl/mojibake/releases/download/v0.2.4/mojibake-amalgamation-024.zip)
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

    if(mjb_normalize(input, strlen(input), MJB_NORMALIZATION_NFC, MJB_ENC_UTF_8, MJB_ENC_UTF_8,
        &result) != MJB_STATUS_OK) {
        return 1;
    }

    // Output NFC: Café
    // e + ◌́ into é
    printf("NFC: %.*s\n", (int)result.output_size, result.output);

    if(result.transformed) {
        mjb_free(result.output);
    }

    return 0;
}
```

This library works only in little-endian systems to avoid adding too much overhead. This means that
it works on all modern general-purpose CPUs today (x86, x86-64, ARMv8, RISC-V, etc.) It has been
tested on:

1. Linux
2. macOS
3. FreeBSD
4. OpenBSD
5. NetBSD
6. Windows 10/11

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

Mojibake run a total of **1,560,857** tests, including all the official tests included in the
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

## Unicode references

Mojibake's Unicode data and algorithm references are scoped to
[The Unicode Standard, Version 17.0.0](https://www.unicode.org/versions/Unicode17.0.0/)
and the [Unicode Character Database 17.0.0](https://www.unicode.org/Public/17.0.0/).
Normative algorithm references here and in [https://github.com/zaerl/mojibake/blob/main/API.md](API.md)
use the archived Unicode 17.0.0 versions of the applicable annexes and synchronized technical standards:

- [UAX #44: Unicode Character Database, Unicode 17.0.0](https://www.unicode.org/reports/tr44/tr44-36.html)
- [UAX #9: Unicode Bidirectional Algorithm, Unicode 17.0.0](https://www.unicode.org/reports/tr9/tr9-51.html)
- [UAX #11: East Asian Width, Unicode 17.0.0](https://www.unicode.org/reports/tr11/tr11-44.html)
- [UAX #14: Unicode Line Breaking Algorithm, Unicode 17.0.0](https://www.unicode.org/reports/tr14/tr14-55.html)
- [UAX #15: Unicode Normalization Forms, Unicode 17.0.0](https://www.unicode.org/reports/tr15/tr15-57.html)
- [UAX #29: Unicode Text Segmentation, Unicode 17.0.0](https://www.unicode.org/reports/tr29/tr29-47.html)
- [UAX #31: Unicode Identifiers and Syntax, Unicode 17.0.0](https://www.unicode.org/reports/tr31/tr31-43.html)
- [UTS #10: Unicode Collation Algorithm, Unicode 17.0.0](https://www.unicode.org/reports/tr10/tr10-53.html)
- [UTS #39: Unicode Security Mechanisms, Unicode 17.0.0](https://www.unicode.org/reports/tr39/tr39-32.html)
- [UTS #51: Unicode Emoji, Unicode 17.0.0](https://www.unicode.org/reports/tr51/tr51-29.html)

Generic Unicode links, when present, are informational or download links rather than normative
conformance references.

## Unicode Conformance Requirements
Mojibake satisfy the Unicode Conformance Requirements. See
[CONFORMANCE-REQUIREMENTS.md](https://github.com/zaerl/mojibake/blob/main/CONFORMANCE-REQUIREMENTS.md) for details.

## Unicode tailoring

Unless listed here, Mojibake applies the referenced Unicode 17.0.0 algorithms without
higher-level protocol tailoring.

- **Case conversion and case folding**: `mjb_case` is locale-sensitive through the process-global
  locale set by `mjb_locale_set`. The default locale is `MJB_LOCALE_EN`. `MJB_LOCALE_TR` and
  `MJB_LOCALE_AZ` apply the Turkish/Azerbaijani dotted-I rules from `SpecialCasing.txt` for
  uppercase, lowercase, and titlecase, and the Turkic `T` mappings from `CaseFolding.txt` for full
  and simple case folding. `MJB_LOCALE_LT` applies Lithuanian dot-above rules from
  `SpecialCasing.txt` for uppercase, lowercase, and titlecase; case folding remains the default
  non-Turkic mapping.
- **Collation**: `mjb_string_compare` and `mjb_collation_key` use DUCET without locale collation
  tailoring. The `mjb_collation_mode` argument only selects the UCA variable weighting strategy.
- **Display width**: `mjb_display_width` has an explicit `mjb_width_context` policy for East Asian
  Width `Ambiguous` characters. `mjb_codepoint_east_asian_width` itself reports the Unicode 17.0.0
  property value without tailoring.
- **Other Unicode algorithms**: normalization, bidirectional processing, grapheme/word/sentence/line
  breaking, identifier validation, confusable skeletons, and emoji sequence checks are not
  locale-tailored by Mojibake.

## Unicode conformance inventory

Mojibake interprets Unicode text only through the public APIs and supported UTF encodings listed in
this documentation. It does not implement rendering, font shaping, locale collation tailoring, or
higher-level protocol behavior beyond the documented locale-sensitive casing and display-width
policy. The table below maps the advertised Unicode algorithm and data claims to their Unicode
17.0.0 reference and test evidence.

| Claim | Public surface | Unicode reference | Evidence |
| --- | --- | --- | --- |
| Unicode Character Database data and derived properties | `mjb_codepoint_character`, `mjb_codepoint_property_value`, script/block/category/numeric helpers | [UAX #44](https://www.unicode.org/reports/tr44/tr44-36.html), UCD 17.0.0 | Generated from UCD data files including `UnicodeData.txt`, `Blocks.txt`, `Scripts.txt`, `PropList.txt`, `DerivedCoreProperties.txt`, `PropertyAliases.txt`, and `PropertyValueAliases.txt`; covered by local UCD/property tests. |
| Unicode Normalization Forms and quick check | `mjb_normalize`, `mjb_string_is_normalized` | [UAX #15](https://www.unicode.org/reports/tr15/tr15-57.html) | `NormalizationTest.txt`, `DerivedNormalizationProps.txt`, `tests/normalization.c`, and `tests/quick-check.c`. |
| Default case conversion and caseless matching | `mjb_case`, simple codepoint case helpers | [Unicode Core Section 3.13](https://www.unicode.org/versions/Unicode17.0.0/core-spec/chapter-3/#G33992), [UAX #29](https://www.unicode.org/reports/tr29/tr29-47.html) for titlecase word boundaries | `SpecialCasing.txt`, `CaseFolding.txt`, `WordBreakTest.txt`, `tests/special-case.c`, `tests/case.c`, and `tests/break-word.c`. |
| Grapheme, word, and sentence boundaries | `mjb_break_grapheme_cluster`, `mjb_break_word`, `mjb_break_sentence`, related truncation helpers | [UAX #29](https://www.unicode.org/reports/tr29/tr29-47.html) | `GraphemeBreakTest.txt`, `WordBreakTest.txt`, `SentenceBreakTest.txt`, `tests/segmentation.c`, `tests/break-word.c`, and `tests/break-sentence.c`. |
| Line breaking | `mjb_break_line` | [UAX #14](https://www.unicode.org/reports/tr14/tr14-55.html) | `LineBreakTest.txt` and `tests/break-line.c`. |
| Bidirectional Algorithm | `mjb_bidi_resolve`, `mjb_bidi_reorder_line`, `mjb_bidi_line_runs` | [UAX #9](https://www.unicode.org/reports/tr9/tr9-51.html) | `BidiCharacterTest.txt`, `BidiTest.txt`, `tests/bidi.c`, and `tests/bidi-class.c`. |
| Unicode Collation Algorithm, DUCET | `mjb_string_compare`, `mjb_collation_key` | [UTS #10](https://www.unicode.org/reports/tr10/tr10-53.html) | `CollationTest_NON_IGNORABLE.txt`, `CollationTest_SHIFTED.txt`, and `tests/collation.c`; surrogate-code-point rows are filtered because public string input rejects ill-formed surrogate code points. |
| Unicode identifiers and pattern syntax data | ID/XID/pattern predicates and `mjb_string_is_identifier` | [UAX #31](https://www.unicode.org/reports/tr31/tr31-43.html) | UCD ID/XID and pattern properties from `DerivedCoreProperties.txt` and `PropList.txt`; covered by `tests/identifier.c`. |
| Confusable skeleton matching | `mjb_string_is_confusable` | [UTS #39](https://www.unicode.org/reports/tr39/tr39-32.html) | `confusables.txt`, `intentional.txt`, and `tests/security.c`. |
| Emoji properties and sequence data | Emoji property predicates, `mjb_string_emoji_sequence`, RGI checks | [UTS #51](https://www.unicode.org/reports/tr51/tr51-29.html) | `emoji-data.txt`, `emoji-sequences.txt`, `emoji-zwj-sequences.txt`, `emoji-variation-sequences.txt`, `emoji-test.txt`, and `tests/emoji.c`. |
| East Asian Width property | `mjb_codepoint_east_asian_width`; consumed by `mjb_display_width` | [UAX #11](https://www.unicode.org/reports/tr11/tr11-44.html) | `EastAsianWidth.txt`, `tests/east-asian-width.c`, and property tests; display column counts are a documented local policy over that property. |

## Thanks

Mojibake is built using the work of extraordinary individuals and teams.

1. Unicode Character Database - Copyright © 1991-2026 Unicode, Inc.
(see [license.txt](https://www.unicode.org/license.txt))
2. Unicode CLDR Project - Copyright © 2004-2026 Unicode, Inc.
(see [LICENSE](https://raw.githubusercontent.com/unicode-org/cldr/refs/heads/main/LICENSE))

[^1]: **Mojibake** (Japanese: 文字化け 'character transformation') is the garbled text that is the
result of text being decoded using an unintended character encoding. I created this library because
I don't like any of the existing one.
