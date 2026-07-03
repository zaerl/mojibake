# Mojibake

[![Test](https://github.com/zaerl/mojibake/actions/workflows/test.yml/badge.svg)](https://github.com/zaerl/mojibake/actions/workflows/test.yml)

**Mojibake** is a low-level Unicode 17 library written in C11. It can be compiled as C++17 as well.

## Introduction

**Mojibake** (Japanese: 文字化け 'character transformation') is the garbled text that is the result
of text being decoded using an unintended character encoding. I created this library because I don't
like any of the existing one. It aims to be, in order of importance:

1. Small
2. Easy to use
3. Fast
4. Pass all Unicode Standard tests
5. Self-contained

All the C files, together with the Unicode data tables are concatenaded into a single large file and
and header:

1. `mojibake.h`
2. `mojibake.c`

A CLI implementation is provided in `src/shell`, and a C++ wrapper is in `src/cpp/mojibake.hpp`.

## Usage

You don't need to install anything. Add the C source and header to your build.

1. Download it here
[mojibake-amalgamation-022.zip](https://github.com/zaerl/mojibake/releases/download/v0.2.2/mojibake-amalgamation-022.zip)
2. Unzip it
3. Add `mojibake.c` and `mojibake.h` to your project

Example:

```c
#include <stdio.h>
#include "mojibake.h"

// This is a simple example of how to use the Mojibake library.
int main(int argc, char * const argv[]) {
    printf("This is an example of Mojibake v%s\n", mjb_version());
    printf("Unicode version: %s\n", mjb_unicode_version());

    mjb_character character;
    mjb_codepoint_character(0x022A, &character);

    // This will print: "U+022A: LATIN CAPITAL LETTER O WITH DIAERESIS AND MACRON"
    printf("U+%04X: %s\n", character.codepoint, character.name);

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

### Minimal API documentation

1. **Normalization**: `mjb_normalize`, and other, let you normalize a string to `NFC/NFKC/NFD/NFKD` form.
2. **Full character properties**: `mjb_codepoint_character`, and other, let you obtain all the
properties found in the Unicode Character Database.
3. **Case**: `mjb_case`, and other, let you normalize a string to uppercase, lowercase,
titlecase. Etc.
4. **Parsing**: `mjb_next_character`, and other, let you parse a UTF-8, UTF-16(BE, LE), UTF-32(BE, LE)
string.
5. **Segmentation and line breaking**: `mjb_break_line`, and other, let you break a string by line, and
segment it.
6. **Bidirectional reordering**: `mjb_bidi_resolve`, apply the Unicode Bidirectional Algorithm
7. **Collation comparing**: `mjb_string_compare`, compare two string using the Unicode Collation Algorithm
8. **Base string functions**: `mjb_strnlen`, and other, aim to have a full coverage of standard C
library `string.h` header.

Following an incomplete documentation of current API.

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

    if(!mjb_normalize(hello, strlen(hello), encoding, MJB_NORMALIZATION_NFC, encoding, &result)) {
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

## Codepoints informations

You can retrieved informations about codepoints. Example for `U+022A LATIN CAPITAL LETTER O WITH DIAERESIS AND MACRON`

```c
#include <stdio.h>
#include "mojibake.h"

int main(int argc, char * const argv[]) {
    mjb_character character;
    mjb_codepoint_character(0x022A, &character);

    printf("U+%04X: %s\n", character.codepoint, character.name);
    // See the `mojibake` struct for other fields
}
```

Output:

```
U+022A: LATIN CAPITAL LETTER O WITH DIAERESIS AND MACRON
```

### CLI

The `src/shell` directory builds the `mojibake` CLI used to test the library. Example usage:

```sh
mojibake -v char $'\U022A'
# Similar to
# mojibake -vv codepoint 022A
```

Plain text output:

```
Codepoint: U+022A
Name: LATIN CAPITAL LETTER O WITH DIAERESIS AND MACRON
Character: Ȫ
Hex UTF-8: C8 AA
Hex UTF-16BE: 02 2A
Hex UTF-16LE: 2A 02
Hex UTF-32BE: 00 00 02 2A
Hex UTF-32LE: 2A 02 00 00
NFD: Ȫ
NFD normalization: U+004F U+0308 U+0304
NFC: Ȫ
NFC normalization: U+022A
NFKD: Ȫ
NFKD normalization: U+004F U+0308 U+0304
NFKC: Ȫ
NFKC normalization: U+004F U+0308 U+0304
Category: [0] Letter, uppercase
Combining: [0] Not Reordered
Bidirectional: [1] Left-to-right
Plane: [0] Basic Multilingual Plane
Block: [3] Latin Extended-B
Decomposition: [1] Canonical
Decimal: N/A
Digit: N/A
Numeric: N/A
Mirrored: N
Uppercase: N/A
Lowercase: U+022B
Titlecase: N/A
```

JSON format:

```sh
mojibake -vv -o json -j 2 char $'\U022A'
# Similar to
# mojibake -vv -o json -j 2 codepoint 022A
```

Emoji sequence analysis:

```sh
mojibake emoji "☺️"
mojibake -c emoji 263A FE0F
```

### Coverage

Mojibake run a total of **1,469,058** tests including all the official tests included in the standard:

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
