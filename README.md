# Mojibake

[![Test](https://github.com/zaerl/mojibake/actions/workflows/test.yml/badge.svg)](https://github.com/zaerl/mojibake/actions/workflows/test.yml)
[![OpenSSF Best Practices](https://www.bestpractices.dev/projects/13557/badge)](https://www.bestpractices.dev/projects/13557)

**Mojibake** is a low-level Unicode 17 text-processing library written in C11 and compatible
with C++17. It is released under the MIT License.

## Usage

You don't need to install anything. There are two files (`mojibake.c`, `mojibake.h`) to add to your
C/C++ project. Download it here [mojibake-amalgamation-026.zip](https://github.com/zaerl/mojibake/releases/download/v0.2.6/mojibake-amalgamation-026.zip)

Examples of normalization, characters count and NFKC casefold.

```c
int main(int argc, char *const argv[]) {
    const char *input = "Cafe\xCC\x81";
    size_t length = strlen(input);
    mjb_result result;

    // Normalize example: in NFC e + ◌́ -> é (U+00E9)
    if(mjb_normalize(input, length, MJB_ENC_UTF_8, MJB_NORMALIZATION_NFC, MJB_ENC_UTF_8,
        &result) != MJB_STATUS_OK) {
        return 1;
    }

    // Cafe + ◌́ (U+0301, COMBINING ACUTE ACCENT) -> Café
    print_string(input, length);

    // Caf + é (U+00E9, LATIN SMALL LETTER E WITH ACUTE) -> Café
    print_string(result.output, result.output_size);

    const char *mojibake = "文字化け";
    length = strlen(mojibake);

    // String length example: mjb_string_length counts the number of characters in a
    // string, not the number of bytes.
    printf("\"%s\" encoded in UTF-8 is %zu bytes long, and %zu characters long\n",
        mojibake, length, mjb_string_length(mojibake, length, MJB_ENC_UTF_8));

    mjb_result_free(&result);

    const char *case_input = "Straße";

    // NFKC casefold example: in NFKC casefold, ß -> ss
    if(mjb_nfkc_casefold(case_input, strlen(case_input), MJB_ENC_UTF_8, MJB_ENC_UTF_8,
        &result) != MJB_STATUS_OK) {
        return 1;
    }

    printf("%s -> %.*s\n", case_input, (int)result.output_size, result.output);
    mjb_result_free(&result);

    return 0;
}
```

This output:

```
Cafe<CC><81>
Caf<C3><A9>
"文字化け" encoded in UTF-8 is 12 bytes long, but instead is 4 characters long
Straße -> strasse
```

Mojibake aims to be:

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
- **Fuzz** Mojibake is fuzzed with [libFuzzer](https://llvm.org/docs/LibFuzzer.html) over untrusted
byte input
- `AddressSanitizer` and `UBSan` clean


### Build-time features

Mojibake can compile out optional feature tables to reduce binary size. Feature macros default to
enabled.

- `#define MJB_FEATURE_CHARACTER_NAMES` controls the Unicode character-name tables used by
`mjb_codepoint_character(...)` to fill `mjb_character.name`. When disabled, the tables are not
compiled and `mjb_character.name` is reported as `Codepoint U+XXXX`. This will redude the output
of **~30%**.

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

See [API.md](https://github.com/zaerl/mojibake/blob/main/API.md) or the site for the detailed
documentation.

### CLI

The `src/shell` directory builds the `mojibake` CLI used to test the library. Example usage:

```bash
# This outputs "NFC: Café", e + ◌́ -> é
mojibake nfc $'Cafe\u0301'

# The output an emoji sequence [1] Basic, [2] Fully-qualified of two characters U+263A U+FE0F
mojibake emoji "☺️"
```

## Building from source and contributing

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
