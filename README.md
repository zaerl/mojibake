# Mojibake

[![Test](https://github.com/zaerl/mojibake/actions/workflows/test.yml/badge.svg)](https://github.com/zaerl/mojibake/actions/workflows/test.yml)

**Mojibake**[^1] is a low-level Unicode 17 library written in C11. It can be compiled as C++17 as well.
It aims to be:

1. Small
2. Easy to use
3. Fast
4. Self-contained
5. Pass all Unicode Standard tests
6. Run in all modern OSes (Linux, macOS, FreeBSD, OpenBSD, NetBSD, Windows 11)

## Feature highlights

All the C files, together with the Unicode data tables, are concatenated into a single large file
and header: `mojibake.c` and `mojibake.h`. Zero dependencies.

**Text transformation**

- **Normalization**: NFC/NFD/NFKC/NFKD (`mjb_normalize`), plus a fast quick-check
  (`mjb_string_is_normalized`) ([UAX #15](https://www.unicode.org/reports/tr15/))
- **Case conversion**: uppercase, lowercase, titlecase, and case folding with full special-casing
  and conditional mappings (`mjb_case`)
- **Filtering**: strip controls, spaces, or numeric characters while normalizing
  (`mjb_string_filter`)

**Text analysis**

- **Character database**: every Unicode Character Database property: category, script, block,
  plane, numeric value, name (`mjb_codepoint_character`)
- **Segmentation**: grapheme clusters, words, sentences, and line-break opportunities
  ([UAX #29](https://www.unicode.org/reports/tr29/), [UAX #14](https://www.unicode.org/reports/tr14/))
- **Bidirectional text**: full Unicode Bidirectional Algorithm: paragraph resolution, line
  reordering, runs ([UAX #9](https://www.unicode.org/reports/tr9/))
- **Emoji**: codepoint properties, sequence analysis, RGI emoji detection
- **Display width**: East Asian width and terminal display width, with width-aware truncation
  (`mjb_display_width`, `mjb_truncate_width`)

**Sorting and comparison**

- **Collation**: Unicode Collation Algorithm string comparison and sort keys, in shifted and
  non-ignorable modes (`mjb_string_compare`, `mjb_collation_key`, [UTS #10](https://www.unicode.org/reports/tr10/))

**Security**

- **Confusable detection**: check if a string is visually confusable with another
  (`mjb_string_is_confusable`, [UTS #39](https://www.unicode.org/reports/tr39/))
- **Identifier validation**: XID/ID checks for parser and compiler authors
  (`mjb_string_is_identifier`, [UAX #31](https://www.unicode.org/reports/tr31/))

**Integration**

- **Encodings**: the API accepts and outputs UTF-8, UTF-16LE, UTF-16BE, UTF-32LE, UTF-32BE
  strings, with encoding detection and conversion (`mjb_string_encoding`,
  `mjb_string_convert_encoding`)
- **Parsing and string functions**: character-by-character iteration (`mjb_next_character`) and
  standard C `string.h`-style helpers (`mjb_strnlen`, and others)
- **Locales**: strict BCP 47 language tag parsing (`mjb_locale_parse`)
- **Embeddable**: custom allocators (`mjb_set_memory_functions`), build-time feature flags to trim
  table size, a C++17 wrapper (`src/cpp/mojibake.hpp`), a CLI tool (`src/shell`), and a
  WASM + TypeScript API (`src/api`)
- **Tested**: 1.5M+ assertions including every official Unicode conformance suite; fuzzed with
  `libFuzzer`; `AddressSanitizer` and `UBSan` clean

## Usage

You don't need to install anything. Add the C source and header to your build.

1. Download it here
[mojibake-amalgamation-023.zip](https://github.com/zaerl/mojibake/releases/download/v0.2.2/mojibake-amalgamation-023.zip)
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

    if(mjb_normalize(input, strlen(input), MJB_ENCODING_UTF_8, MJB_NORMALIZATION_NFC,
           MJB_ENCODING_UTF_8, &result) != MJB_STATUS_OK) {
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

```sh
cmake -S . -B build-no-name -DMJB_FEATURE_CHARACTER_NAMES=OFF
cmake --build build-no-name
```

With the provided Makefile:

```sh
make build BUILD_DIR=build-no-name FEATURE_CHARACTER_NAMES=OFF
make test-no-names
```

### Minimal API documentation

Following an incomplete documentation of current API. See [API.md](API.md) for the full list.

APIs that fill an output struct or allocated `mjb_result` return `mjb_status` and should be checked
against `MJB_STATUS_OK`. Predicate APIs, such as `mjb_string_is_utf8` and `mjb_codepoint_is_valid`,
return `bool` because the boolean is the result.

## String normalization

Mojibake let you normalize a string in NFC/NFKC/NFD/NFKD form.

```c
#include <stdio.h>
#include <string.h>
#include "mojibake.h"

int main(int argc, char * const argv[]) {
    // The string to normalize
    const char *hello = "Hello, World!";
    mjb_encoding encoding = MJB_ENCODING_UTF_8;
    mjb_result result;

    mjb_status status = mjb_normalize(hello, strlen(hello), encoding, MJB_NORMALIZATION_NFC,
        encoding, &result);

    if(status != MJB_STATUS_OK) {
        return 1;
    }

    printf("Normalized: %s\nSize: %zu\n", result.output, result.output_size);
    // Normalized: Hello, World!
    // Size: 13

    // Remember to free() the string if needed
    if(result.transformed) {
        mjb_free(result.output);
    }

    return 0;
}
```

## Codepoint information

You can retrieve information about codepoints. This example uses fields that are available
regardless of whether Unicode character names are compiled in.

```c
#include <stdio.h>
#include "mojibake.h"

int main(int argc, char * const argv[]) {
    mjb_character character;

    if(mjb_codepoint_character(0x022A, &character) != MJB_STATUS_OK) {
        return 1;
    }

    printf("U+%04X lowercase: U+%04X\n", character.codepoint, character.lowercase);
    printf("Graphic: %s\n", mjb_category_is_graphic(character.category) ? "yes" : "no");
    // See the `mojibake` struct for other fields

    return 0;
}
```

Output:

```
U+022A lowercase: U+022B
Graphic: yes
```

### CLI

The `src/shell` directory builds the `mojibake` CLI used to test the library. Example usage:

```sh
mojibake nfc $'Cafe\u0301'
```

Plain text output:

```
Café
```

Emoji sequence analysis:

```sh
mojibake emoji "☺️"
mojibake -c emoji 263A FE0F
```

### Coverage

Mojibake run a total of **1,560,807** tests including all the official tests included in the
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

```sh
make fuzz # 60 seconds by default
make fuzz FUZZ_TIME=300
```

## Building from source

See [CONTRIBUTING.md](CONTRIBUTING.md) for instructions.

## Licenses

This project is released under the MIT License (see [LICENSE](LICENSE) file).

## Thanks

Mojibake is built using the work of extraordinary individuals and teams.

1. Unicode Character Database - Copyright © 1991-2026 Unicode, Inc.
(see [license.txt](https://www.unicode.org/license.txt))
2. Unicode CLDR Project - Copyright © 2004-2026 Unicode, Inc.
(see [LICENSE](https://raw.githubusercontent.com/unicode-org/cldr/refs/heads/main/LICENSE))

[^1]: **Mojibake** (Japanese: 文字化け 'character transformation') is the garbled text that is the
result of text being decoded using an unintended character encoding. I created this library because
I don't like any of the existing one. It aims to be, in order of importance:
