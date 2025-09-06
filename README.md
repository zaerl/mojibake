# Mojibake

> **Note:**
> This project is an **experimental library**. It is not designed for production use, and there may
> be bugs, limitations, or incomplete features. The API can change from one commit to another.
>
> Use at your own discretion, and feel free to collaborate.

**Mojibake** is a low-level Unicode 16 library written in C11.

**Mojibake** (Japanese: 文字化け 'character transformation') is the garbled text that is the result
of text being decoded using an unintended character encoding. I created this library because I don't
like any of the existing one. It aims to be, in order of importance:

1. Small
2. Easy to use
3. Fast
4. Pass all Unicode Standard tests
5. Self-contained

It consists in a `mojibake.c` file, a `mojibake.h` file and a `mojibake.db` file (a SQLite database
file). A `shell.c` file is also provided that let you build a `mojibake` CLI, if you want.

An online demo can be found at https://mojibake.zaerl.com/. It is a WASM-compiled version you can
use to preview the API.

It has a `0.0.0` version because the various parts I consider critical have not 100% coverage. These
are the major parts:

**Normalization**: `mjb_normalize`, and other, let you normalize a string to `NFC/NFKC/NFD/NFKD` form.

**Full character properties**: `mjb_codepoint_character`, and other, let you obtain all the
properties found in the Unicode Character Database.

**Normalization**: `mjb_case`, and other, let you normalize a string to uppercase, lowercase,
titlecase. Etc.

**Parsing**: `mjb_next_character`, and other, let you parse a UTF-8, UTF-16(BE, LE), UTF-32(BE, LE)
string.

**Segmentation and line breaking**: `mjb_break_line`, and other, let you break a string by line, and
segment it.

**Base string functions**: `mjb_strncmp`, and other, aim to have a full coverage of standard C
library `string.h` header.

Following an incomplete documentation of current API.

## String normalization

Mojibake let you normalize a string in NFC/NFKC/NFD/NFKD form.

```c
#include "mojibake.h"

// The string to normalize
const char *hello = "Hello, World!"
mjb_normalization_result result;

char *normalized = mjb_normalize(hello, 13, MJB_ENCODING_UTF_8, MJB_NORMALIZATION_NFC, &result);
printf("Normalized: %s\nSize: %zu\n", result.output, result.output_size);

// Remember to free() the string
mjb_free(result.output);

```

## Codepoints informations

You can retrieved informations about codepoints. Example for `U+022A LATIN CAPITAL LETTER O WITH DIAERESIS AND MACRON`

```c
#include "mojibake.h"

mjb_character character;

mjb_codepoint_character(0x022A, &character);

printf("Name: %s", character.codepoint, character.name);
// See the `mojibake` struct for other fields
```

Output:

```
Name: LATIN CAPITAL LETTER O WITH DIAERESIS AND MACRON
```

### CLI

A `shell.c` file is provided that let you have a CLI to test the library. Example usage:

```sh
mojibake char $'\U022A'
```

Plain text output:

```
Codepoint: U+022A
Name: LATIN CAPITAL LETTER O WITH DIAERESIS AND MACRON
Character: Ȫ
Hex UTF-8: C8 AA
NFD: Ȫ
NFD normalization: U+004F U+0308 U+0304
NFC: Ȫ
NFC normalization: U+022A
NFKD: Ȫ
NFKD normalization: U+004F U+0308 U+0304
NFKC: Ȫ
NFKC normalization: U+004F U+0308 U+0304
Category: [0] Letter, uppercase
Combining: [0] Not reordered
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
mojibake -o json char $'\U022A'
```

Output:

```json
[
  {
    "codepoint": "U+022A",
    "name": "LATIN CAPITAL LETTER O WITH DIAERESIS AND MACRON",
    "character": "Ȫ",
    "hex_utf-8": [200, 170],
    "nfd": "Ȫ",
    "nfd_normalization": [79, 776, 772],
    "nfc": "Ȫ",
    "nfc_normalization": [554],
    "nfkd": "Ȫ",
    "nfkd_normalization": [79, 776, 772],
    "nfkc": "Ȫ",
    "nfkc_normalization": [79, 776, 772],
    "category": {
      "code": 0,
      "value": "Letter, uppercase"
    },
    "combining": {
      "code": 0,
      "value": "Not reordered"
    },
    "bidirectional": {
      "code": 1,
      "value": "Left-to-right"
    },
    "plane": {
      "code": 0,
      "value": "Basic Multilingual Plane"
    },
    "block": {
      "code": 3,
      "value": "Latin Extended-B"
    },
    "decomposition": {
      "code": 1,
      "value": "Canonical"
    },
    "decimal": null,
    "digit": null,
    "numeric": null,
    "mirrored": false,
    "uppercase": null,
    "lowercase": 555,
    "titlecase": null
  }
]
```

JSON is by default indented with two characters. But you can change it, if you need to parse
multiple codepoints. E.g. `./mojibake.sh -j 0 -o json char $'\U61\U62'`

### Coverage

Mojibake run all the normalization tests found in the standard
[NormalizationTest.txt](https://www.unicode.org/Public/16.0.0/ucd/NormalizationTest.txt) suite of
tests.

## WebAssembly

An experimental WASM build can be used.

```
make wasm
cd build-wasm/src
python3 -m http.server # or similar
# Open http://[::1]:8000/wasm.html. It will show the Mojibake version on console
```

## Thanks

Mojibake is built using the work of extraordinary individuals and teams.

1. The `utf8.h` file is Copyright (c) 2014 Taylor R Campbell
2. SQLite is in the [public domain](https://sqlite.org/copyright.html).
3. modern-normalize is under the MIT License. Copyright (c) Sindre Sorhus, Jonathan Neal, Nicolas
   Gallagher
