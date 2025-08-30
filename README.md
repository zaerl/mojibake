# Mojibake

> **Note:**
> This project is an **experimental library**. It is not designed for production use, and there may
> be bugs, limitations, or incomplete features. The API can change from one commit to another.

> The current version is 0.0.0. Use at your own discretion, and feel free to collaborate.

Mojibake is a low-level Unicode library written in C99. It aims to be fast and small. It consists in
a `mojibake.c` file, a `mojibake.h` file and a `mojibake.db` file. A `shell.c` file is also provided
that let you build a `mojibake` CLI.

## String normalization

Mojibake let you normalize a string in NFC/NFKC/NFD/NFKD form.

```c
#include "mojibake.h"

// The string to normalize
const char *hello = "Hello, World!"
// The length of the normalized string
size_t normalized_size = 0;

char *normalized = mjb_normalize(hello, 13, &normalized_size, MJB_NORMALIZATION_NFC);
printf("Normalized: %s\nSize: %zu\n", normalized, normalized_size);

// Remember to free() the string
mjb_free(normalized);

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

## Thanks

Mojibake is built using the work of extraordinary individuals and teams.

1. The `utf8.h` file is Copyright (c) 2014 Taylor R Campbell
2. SQLite is in the [public domain](https://sqlite.org/copyright.html).
