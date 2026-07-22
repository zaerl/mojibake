# API

Welcome to the Unicode world. Before starting, you can find a demo site where you can find the API
documentation and test the functions by using WASM here:
[https://mojibake.zaerl.com](https://mojibake.zaerl.com)

> [!NOTE]
> When we refer to "Unicode" or "Unicode 18" we refer to Unicode 18 **beta**

## Unicode glossary

You will need to have a basic glossary to understand the terminology.

A **Code Point** (`typedef uint32_t mjb_codepoint` in the code) in Unicode is an integer from 0 to
0x10FFFF. It's used to identify a **Character** in the list. For example `U+20A0`, the € currency.

A **Character** (`typedef struct mjb_character`) is the "smallest component of written language that
has semantic value". In Mojibake, an `mjb_character` is a struct with all the associated values.
For example `U+00BD` (**½**):

```
Codepoint: U+U+00BD
Name: VULGAR FRACTION ONE HALF
Category: [11] Number, other
Combining: [0] Not Reordered
Bidirectional: [14] Other neutrals
Decomposition: [6] Fraction
Decimal: N/A
Digit: N/A
Numeric: 1/2
Mirrored: N
Uppercase: N/A
Lowercase: N/A
Titlecase: N/A
```

> [!NOTE]
> The Uppercase, Lowercase, Titlecase fields [you find](https://github.com/zaerl/mojibake/blob/main/src/mojibake.h#L322-L324)
> in the `mjb_character` struct are not the real uppercase version of the codepoint you passed to
> the function but its "simple case uppercase", a 1-to-1 character transformation. To have the real
> case version of a string, use `mjb_map_case` function. Codepoint as `ß` transforms to a `SS` in
> uppercase.

An **encoding** is a way of storing a list of codepoints in memory. Nowadays, the UTF-8 encoding is
by far the most used one when we are talking about moving data around, but there are important
contexts where, for example, UTF-16 is used, such as: Windows APIs, Java, .NET, JavaScript, and
others.

> [!NOTE]
> In Mojibake `MJB_ENC_UTF_16` doesn't mean `MJB_ENC_UTF_16LE` (Little Endian, used by all modern
> CPUs) by default. To follow the [C8 requirement](https://github.com/zaerl/mojibake/blob/main/CONFORMANCE_REQUIREMENTS.md#c8),
> if you declare a generic `UTF-16` this means that the algorithm will check if there is a BOM at
> the start of the string and decide whether to use Little Endian or Big Endian. Usually it's not a
> good idea; use the generic `MJB_ENC_UTF_16` only if you are reading a file that you know has a
> BOM. Otherwise use `MJB_ENC_UTF_8` or `MJB_ENC_UTF_16LE`.

A **normalization** is the "process of removing alternate representations of equivalent sequences
from textual data". This means transforming a string to another one called its normalized form by
replacing parts with others. For example, `U+00C0` (LATIN CAPITAL LETTER A WITH GRAVE) is normalized
in NFD as a list of two characters: `U+0041` (LATIN CAPITAL LETTER A) + `U+0300` (COMBINING GRAVE
ACCENT).

## Basis

Here is the basis for using the library:

1. Mojibake does not have a default input encoding or output decoding; you must decide what to use
2. Input and output encodings can be different
3. Every string passed is simply a stream of bytes. Specify its exact byte length to process U+0000
   like any other codepoint, or pass `MJB_NUL_TERMINATED` when the input has an encoding-aware NUL
   terminator
4. `MJB_NUL_TERMINATED` scans one-byte code units for ASCII/UTF-8, two-byte code units for UTF-16,
   and four-byte code units for UTF-32. The terminator is excluded from the input. As with standard
   C string functions, the caller must provide a correctly terminated buffer
5. The major part of the functions return a `mjb_status` and should be checked against the
`MJB_STATUS_OK` constant. The `MJB_STATUS_OK` enum value is `0 (zero)` so don't check for truthy
6. Predicate APIs, such as `mjb_is_utf8` and `mjb_codepoint_is_valid`, return `bool` because
the boolean is the result

> [!IMPORTANT]
> `mjb_status` is an enum and if a function **succeeds** returns
> [MJB_STATUS_OK](https://github.com/zaerl/mojibake/blob/main/src/mojibake.h#L265) that is _zero_,
> so _false_. Always check for that value `if(mjb_normalize(...) == MJB_STATUS_OK)`. I've chosen
> this to follow other libraries' de facto standards.

## API signatures

All functions follow the same signature. And there are a few sets of types together with other very
specialized functions you will find in the API list below.

### Functions that handle strings

The functions return a `_mjb_status_` and accept these arguments:

1. The input string
2. The _length_ of the input string (`byte_length`)
3. The encoding of the input string (`encoding`)
4. The needed _arguments_ of the function, if any
5. The encoding you want to be used for the output string
6. A `mjb_result` pointer to store the result

See for example the [`mjb_normalize`](#mjb_normalize), [`mjb_filter`](#mjb_filter)
functions.

### Functions that handle a codepoint

The functions return a `mjb_status` and accept these arguments:

1. The codepoint to check
2. The needed _arguments_ of the function, if any
3. A pointer to a structure to save the result

See for example [`mjb_codepoint_info`](#mjb_codepoint_info),
[`mjb_codepoint_numeric_value`](#mjb_codepoint_numeric_value).

### Predicate functions

Those are the `mjb_(something)_is_(this)` kind of functions, which return a `bool`.

1. The thing to check
2. The needed _arguments_ of the function, if any

See for example [`mjb_is_utf8`](#mjb_is_utf8),
[`mjb_codepoint_is_valid`](#mjb_codepoint_is_valid).

## Strings encoding and generation

Mojibake is encoding agnostic. It can accept and output `uint8_t` (ASCII, UTF-8),
`uint16_t` (UTF-16), `uint32_t` (UTF-32) bytes of memory. The output strings can have different
encodings of the input strings.

> [!IMPORTANT]
> The _length_ of the input string (`byte_length`) **must** be the exact payload length in bytes.
> `strlen(...)` provides that length for a terminated UTF-8 string. For terminated `uint16_t` or
> `uint32_t` arrays, exclude the final code unit when computing an explicit length; otherwise it is
> processed as U+0000. Alternatively, use `MJB_NUL_TERMINATED`. If an array _decays_ to a pointer
> (for example when passed to a function), `sizeof(...)` returns the size of the pointer instead of
> the array.

Example of the [`mjb_normalize`](#mjb_normalize) function.

```c
mjb_status mjb_normalize(const char *buffer, size_t byte_length, mjb_encoding encoding,
mjb_normalization form, mjb_encoding output_encoding, mjb_result *result);
```

1. `buffer`: a block of memory, `uint8_t` (ASCII, UTF-8), `uint16_t` (UTF-16), `uint32_t` (UTF-32)
2. `byte_length`: the length in _bytes_ of `buffer`
3. `form`: the normalization
4. `encoding`: the encoding of `buffer`
5. `output_encoding`: the encoding of _output_ you want.
6. `results`: a pointer to a struct the function will fill

If you want to normalize the UTF-8 encoded `Cafe\xCC\x81` string to `NFC`, this is what you need to
do:

```c
const char *input = "Cafe\xCC\x81"; // "Cafe" + U+0301 COMBINING ACUTE ACCENT

mjb_result result;

if(mjb_normalize(input, MJB_NUL_TERMINATED, MJB_ENC_UTF_8, MJB_NORMALIZATION_NFC, MJB_ENC_UTF_8,
    &result) != MJB_STATUS_OK) {
    return 1;
}

printf("%s -> %.*s\n", (int)result.output_size, result.output);

mjb_result_free(&result);
```

1. The `length` of the input string is **six** because the input buffer is encoded in UTF-8 and so
`strlen` returns six.
2. The function can potentially return something different from `MJB_STATUS_OK`. In this situation,
you don't need to do anything. If a function fails, it will never leave data behind.
3. If the output string has been `transformed`, it means the function has allocated a result, and
you need to `mjb_result_free` it.

This way the output buffer will be encoded in UTF-16LE.

```c
if(mjb_normalize(input, strlen(input), MJB_ENC_UTF_8, MJB_NORMALIZATION_NFC, MJB_ENC_UTF_16LE,
    &result) != MJB_STATUS_OK) {
    return 1;
}
```

A summary for `Héllö`, encoded this way:

- UTF-8: `H\xC3\xA9ll\xC3\xB6`, 7 bytes
- UTF-16LE: `H\0\xE9\0l\0l\0\xF6\0`, 10 bytes
- UTF-16BE: `\0H\0\xE9\0l\0l\0\xF6`, 10 bytes
- UTF-32LE: `H\0\0\0\xE9\0\0\0l\0\0\0l\0\0\0\xF6\0\0\0`, 20 bytes
- UTF-32BE: `\0\0\0H\0\0\0\xE9\0\0\0l\0\0\0l\0\0\0\xF6`, 20 bytes

To count the Unicode codepoints, use `mjb_count_codepoints`.

```c
mjb_count_codepoints("H\xC3\xA9ll\xC3\xB6", 7, MJB_ENC_UTF_8); // 5 characters
mjb_count_codepoints("H\0\xE9\0l\0l\0\xF6\0", 10, MJB_ENC_UTF_16LE); // 5 characters
mjb_count_codepoints("\0H\0\xE9\0l\0l\0\xF6", 10, MJB_ENC_UTF_16BE); // 5 characters
mjb_count_codepoints("H\0\0\0\xE9\0\0\0l\0\0\0l\0\0\0\xF6\0\0\0", 20, MJB_ENC_UTF_32LE); // 5 characters
mjb_count_codepoints("\0\0\0H\0\0\0\xE9\0\0\0l\0\0\0l\0\0\0\xF6", 20, MJB_ENC_UTF_32BE); // 5 characters
```

Functions that handle a string, as `mjb_normalize`, `mjb_convert_encoding` has always a `_into(...)`
alternative that do not allocate the results for you, but you provide the buffer.

Example for the function:

```
mjb_status mjb_convert_encoding_into(const char *buffer, size_t byte_length, mjb_encoding encoding,
mjb_encoding output_encoding, void *output, size_t *output_size);
```

These are the rules, all the functions are equal:

1. Set `output` to NULL to query the required size you will get back stored in the  `output_size`
  pointer
2. If `output` is non-NULL, `*output_size` supplies its capacity; on return it contains the required
   size when the buffer is too small (`MJB_STATUS_OUTPUT_TOO_SMALL`), or the written size on success
3. **Terminators are excluded from the byte count and are not written**
4. No bytes are written when capacity is insufficient

> [!IMPORTANT]
> Explicitly sized Mojibake inputs are length-delimited and binary-safe. Output sizes never include
> a terminator, and `_into` functions do not write one. `MJB_NUL_TERMINATED` changes input length
> discovery only; it does not change output semantics.

```c
const char *input = "caf\xC3\xA9";
size_t required = 0;

// Get the length.
if(mjb_convert_encoding_into(input, strlen(input), MJB_ENC_UTF_8,
    MJB_ENC_UTF_16LE, NULL, &required) != MJB_STATUS_OK) {
    return 1;
}

char *output = (char *)malloc(required);
size_t capacity = required;

if(mjb_convert_encoding_into(input, strlen(input), MJB_ENC_UTF_8,
    MJB_ENC_UTF_16LE, output, &capacity) != MJB_STATUS_OK) {
    return 1;
}

// UTF-16LE payload bytes (no terminator): 8
printf("UTF-16LE payload bytes (no terminator): %zu", output_size)

free(output);
```

# Functions

## `mjb_codepoint_info`

Return the codepoint character.

```c
mjb_status mjb_codepoint_info(
    mjb_codepoint codepoint,
    mjb_character *character
);
```

Fill `character` with the Unicode Character Database record of a codepoint: name, category, combining class, bidirectional category, decomposition, numeric values, mirrored flag, and simple case mappings. When the library is compiled with `MJB_FEATURE_CHARACTER_NAMES=OFF` the name field is reported as `Codepoint U+XXXX`.

- `codepoint` - The codepoint to check
- `character` - The character to store the result

**Returns**

- `MJB_STATUS_OK` - The character was found and filled
- `MJB_STATUS_INVALID_ARGUMENT` - `character` is NULL or the codepoint is not valid
- `MJB_STATUS_NOT_FOUND` - The codepoint is not assigned

**Example**

```c
mjb_character character;

if(mjb_codepoint_info(0x022A, &character) != MJB_STATUS_OK) {
    return 1;
}

// U+022A lowercase: U+022B
printf("U+%04X lowercase: U+%04X", character.codepoint, character.lowercase);
```

See also: [`mjb_codepoint_block`](#mjb_codepoint_block), [`mjb_codepoint_script`](#mjb_codepoint_script), [`mjb_codepoint_property_binary`](#mjb_codepoint_property_binary), [`mjb_codepoint_property_int`](#mjb_codepoint_property_int).

Specifications: [UAX #44: Unicode Character Database, Unicode 18.0.0](https://www.unicode.org/reports/tr44/tr44-36.html).

## `mjb_normalize`

Normalize a string to NFC/NFKC/NFD/NFKD form.

```c
mjb_status mjb_normalize(
    const char *buffer,
    size_t byte_length,
    mjb_encoding encoding,
    mjb_normalization form,
    mjb_encoding output_encoding,
    mjb_result *result
);
```

Normalize a string to the requested Unicode normalization form. If the input is already normalized and no encoding conversion is needed, the input buffer is returned as-is in `result->output` with `result->transformed` set to false, without allocating.

- `buffer` - The string to normalize
- `byte_length` - The length of the string in bytes, or `MJB_NUL_TERMINATED` to determine it from an encoding-aware NUL code unit
- `encoding` - The encoding of the string
- `form` - The normalization form to use
- `output_encoding` - The output encoding of the string
- `result` - The pointer to store the result. If `result->transformed` is true, `result->output` is library-allocated and must be freed with `mjb_result_free(result)`

**Returns**

- `MJB_STATUS_OK` - The string was normalized (or already normal)
- `MJB_STATUS_INVALID_ARGUMENT` - `result` is NULL, or `buffer` is NULL with a non-zero size
- `MJB_STATUS_INVALID_FORM` - `form` is not NFC, NFD, NFKC, or NFKD
- `MJB_STATUS_OVERFLOW` - The output size would overflow
- `MJB_STATUS_NO_MEMORY` - Allocation failed

**Example**

```c
const char *input = "Cafe\xCC\x81"; // "Cafe" + U+0301 COMBINING ACUTE ACCENT
mjb_result result;

if(mjb_normalize(input, MJB_NUL_TERMINATED, MJB_ENC_UTF_8, MJB_NORMALIZATION_NFC, MJB_ENC_UTF_8,
    &result) != MJB_STATUS_OK) {
    return 1;
}

// NFC: Café
printf("NFC: %.*s", (int)result.output_size, result.output);

mjb_result_free(&result);
```

See also: [`mjb_normalize_into`](#mjb_normalize_into), [`mjb_normalization_quick_check`](#mjb_normalization_quick_check), [`mjb_filter`](#mjb_filter), [`mjb_filter_into`](#mjb_filter_into).

Specifications: [UAX #15: Unicode Normalization Forms, Unicode 18.0.0](https://www.unicode.org/reports/tr15/tr15-57.html).

## `mjb_normalize_into`

Normalize a string into a caller-provided buffer.

```c
mjb_status mjb_normalize_into(
    const char *buffer,
    size_t byte_length,
    mjb_encoding encoding,
    mjb_normalization form,
    mjb_encoding output_encoding,
    void *output,
    size_t *output_size
);
```

Normalize a string using the same Unicode normalization forms and encoding rules as `mjb_normalize`. Set `output` to NULL to query the required size. If `output` is non-NULL, `*output_size` supplies its capacity; on return it contains the required size when the buffer is too small, or the written size on success. Terminators are excluded from the byte count and are not written. No bytes are written when capacity is insufficient. NFD and NFKD write without allocation. NFC and NFKC may allocate temporary composition storage, including during a size query.

- `buffer` - The string to normalize
- `byte_length` - The length of the string in bytes, or `MJB_NUL_TERMINATED` to determine it from an encoding-aware NUL code unit
- `encoding` - The encoding of the string
- `form` - The normalization form to use
- `output_encoding` - The output encoding of the string
- `output` - The caller-provided output buffer, or NULL to query the required size. The caller retains ownership
- `output_size` - The input capacity and output required or written byte count

**Returns**

- `MJB_STATUS_OK` - The required size was returned or the normalized string was written
- `MJB_STATUS_INVALID_ARGUMENT` - `output_size` is NULL, or `buffer` is NULL with a non-zero size
- `MJB_STATUS_INVALID_ENCODING` - An encoding is invalid or lacks byte-order information
- `MJB_STATUS_INVALID_FORM` - `form` is not NFC, NFD, NFKC, or NFKD
- `MJB_STATUS_MALFORMED_INPUT` - The input contains an ill-formed code-unit sequence
- `MJB_STATUS_UNSUPPORTED` - The requested output encoding cannot represent a normalized codepoint
- `MJB_STATUS_OVERFLOW` - The required output size would overflow
- `MJB_STATUS_NO_MEMORY` - Temporary composition allocation failed
- `MJB_STATUS_OUTPUT_TOO_SMALL` - The output capacity is smaller than the required byte count

**Example**

```c
const char *input = "Cafe\xCC\x81"; // "Cafe" + U+0301 COMBINING ACUTE ACCENT
size_t output_size = 0;

if(mjb_normalize_into(input, strlen(input), MJB_ENC_UTF_8, MJB_NORMALIZATION_NFC,
    MJB_ENC_UTF_8, NULL, &output_size) != MJB_STATUS_OK) {
    return 1;
}

char output[5];

if(output_size > sizeof(output) || mjb_normalize_into(input, strlen(input), MJB_ENC_UTF_8,
    MJB_NORMALIZATION_NFC, MJB_ENC_UTF_8, output, &output_size) != MJB_STATUS_OK) {
    return 1;
}

// NFC payload (no terminator): Café
printf("NFC payload (no terminator): %.*s", (int)output_size, output);
```

See also: [`mjb_normalize`](#mjb_normalize), [`mjb_normalization_quick_check`](#mjb_normalization_quick_check), [`mjb_filter_into`](#mjb_filter_into), [`mjb_nfkc_casefold_into`](#mjb_nfkc_casefold_into).

Specifications: [UAX #15: Unicode Normalization Forms, Unicode 18.0.0](https://www.unicode.org/reports/tr15/tr15-57.html).

## `mjb_filter`

Filter a string with the selected mjb_filter_flags.

```c
mjb_status mjb_filter(
    const char *buffer,
    size_t byte_length,
    mjb_encoding encoding,
    mjb_filter_flags filters,
    mjb_encoding output_encoding,
    mjb_result *result
);
```

`MJB_FILTER_LIMIT_COMBINING` removes combining marks after the first `MJB_FILTER_MAX_COMBINING_MARKS` consecutive marks in an emitted run. This is useful for reducing Zalgo-style text while keeping ordinary accents and stacked marks.

- `buffer` - The string to filter
- `byte_length` - The length of the string in bytes, or `MJB_NUL_TERMINATED` to determine it from an encoding-aware NUL code unit
- `encoding` - The encoding of the string
- `filters` - The filters to use
- `output_encoding` - The output encoding of the string
- `result` - The pointer to store the result. If `result->transformed` is true, `result->output` is library-allocated and must be freed with `mjb_result_free(result)`

**Example**

```c
const char *mixed_whitespace = "Hello\t\t\n\nworld";
mjb_result result;

if(mjb_filter(mixed_whitespace, strlen(mixed_whitespace), MJB_ENC_UTF_8,
    MJB_FILTER_COLLAPSE_SPACES, MJB_ENC_UTF_8, &result) != MJB_STATUS_OK) {
    return 1;
}

// Filtered: Hello world
printf("Filtered: %.*s", (int)result.output_size, result.output);

mjb_result_free(&result);

const char *controls = "\x1\x2\t\n\v\f\r\x1f";

if(mjb_filter(controls, strlen(controls), MJB_ENC_UTF_8, MJB_FILTER_CONTROLS,
    MJB_ENC_UTF_8, &result) != MJB_STATUS_OK) {
    return 1;
}

// Filtered: \t\n\v\f\r
printf("Filtered: %.*s", (int)result.output_size, result.output);

mjb_result_free(&result);
```

See also: [`mjb_filter_into`](#mjb_filter_into), [`mjb_normalize`](#mjb_normalize).

## `mjb_filter_into`

Filter a string into a caller-provided buffer.

```c
mjb_status mjb_filter_into(
    const char *buffer,
    size_t byte_length,
    mjb_encoding encoding,
    mjb_filter_flags filters,
    mjb_encoding output_encoding,
    void *output,
    size_t *output_size
);
```

Apply the same filters as `mjb_filter` without allocating the final output buffer. Set `output` to NULL to query the required size. If `output` is non-NULL, `*output_size` supplies its capacity; on return it contains the required size when the buffer is too small, or the written size on success. Terminators are excluded from the byte count and are not written. No bytes are written when capacity is insufficient. Filtering itself does not allocate, but `MJB_FILTER_NORMALIZE` may allocate temporary normalization storage. `MJB_FILTER_LIMIT_COMBINING` keeps the first `MJB_FILTER_MAX_COMBINING_MARKS` consecutive marks in each emitted run.

- `buffer` - The string to filter
- `byte_length` - The length of the string in bytes, or `MJB_NUL_TERMINATED` to determine it from an encoding-aware NUL code unit
- `encoding` - The encoding of the string
- `filters` - The filters to use
- `output_encoding` - The output encoding of the string
- `output` - The caller-provided output buffer, or NULL to query the required size. The caller retains ownership
- `output_size` - The input capacity and output required or written byte count

**Returns**

- `MJB_STATUS_OK` - The required size was returned or the filtered string was written
- `MJB_STATUS_INVALID_ARGUMENT` - `output_size` is NULL, or `buffer` is NULL with a non-zero size
- `MJB_STATUS_INVALID_ENCODING` - An encoding is invalid or lacks byte-order information
- `MJB_STATUS_UNSUPPORTED` - The requested output encoding cannot represent a filtered codepoint
- `MJB_STATUS_OVERFLOW` - The required output size would overflow
- `MJB_STATUS_NO_MEMORY` - Temporary normalization allocation failed
- `MJB_STATUS_OUTPUT_TOO_SMALL` - The output capacity is smaller than the required byte count

**Example**

```c
const char *input = "Hello\t\t\nworld";
size_t output_size = 0;

if(mjb_filter_into(input, strlen(input), MJB_ENC_UTF_8, MJB_FILTER_COLLAPSE_SPACES,
    MJB_ENC_UTF_8, NULL, &output_size) != MJB_STATUS_OK) {
    return 1;
}

char output[11];

if(output_size > sizeof(output) || mjb_filter_into(input, strlen(input), MJB_ENC_UTF_8,
    MJB_FILTER_COLLAPSE_SPACES, MJB_ENC_UTF_8, output, &output_size) != MJB_STATUS_OK) {
    return 1;
}

// Filtered payload (no terminator): Hello world
printf("Filtered payload (no terminator): %.*s", (int)output_size, output);
```

See also: [`mjb_filter`](#mjb_filter), [`mjb_normalize`](#mjb_normalize).

## `mjb_nfkc_casefold`

Apply the Unicode NFKC_Casefold transform to a string.

```c
mjb_status mjb_nfkc_casefold(
    const char *buffer,
    size_t byte_length,
    mjb_encoding encoding,
    mjb_encoding output_encoding,
    mjb_result *result
);
```

Apply the normative `NFKC_Casefold` mapping and normalize the result to NFC. This transform performs compatibility folding, full default case folding, and removal of default-ignorable codepoints. It is intended for identifier comparison and is not locale-sensitive.

- `buffer` - The string to transform
- `byte_length` - The length of the string in bytes, or `MJB_NUL_TERMINATED` to determine it from an encoding-aware NUL code unit
- `encoding` - The encoding of the string
- `output_encoding` - The output encoding of the string
- `result` - The pointer to store the result. If `result->transformed` is true, `result->output` is library-allocated and must be freed with `mjb_result_free(result)`

**Returns**

- `MJB_STATUS_OK` - The transformed string was returned
- `MJB_STATUS_INVALID_ARGUMENT` - `result` is NULL, or `buffer` is NULL with a non-zero size
- `MJB_STATUS_OVERFLOW` - The output size would overflow
- `MJB_STATUS_UNSUPPORTED` - The transform did not stabilize
- `MJB_STATUS_NO_MEMORY` - Allocation failed

**Example**

```c
const char *input = "Stra\xC3\x9F" "e\xC2\xAD";
mjb_result result;

if(mjb_nfkc_casefold(input, strlen(input), MJB_ENC_UTF_8, MJB_ENC_UTF_8,
    &result) != MJB_STATUS_OK) {
    return 1;
}

// strasse
printf("%.*s", (int)result.output_size, result.output);
mjb_result_free(&result);
```

See also: [`mjb_nfkc_casefold_into`](#mjb_nfkc_casefold_into), [`mjb_normalize`](#mjb_normalize), [`mjb_map_case`](#mjb_map_case), [`mjb_is_identifier`](#mjb_is_identifier).

Specifications: [The Unicode Standard, Version 18.0.0, Section 3.13: Default Case Algorithms](https://www.unicode.org/versions/Unicode18.0.0/core-spec/chapter-3/#G33992), [UAX #31: Unicode Identifiers and Syntax, Unicode 18.0.0](https://www.unicode.org/reports/tr31/tr31-44.html), [UAX #44: Unicode Character Database, Unicode 18.0.0](https://www.unicode.org/reports/tr44/tr44-36.html).

## `mjb_nfkc_casefold_into`

Apply the Unicode NFKC_Casefold transform into a caller-provided buffer.

```c
mjb_status mjb_nfkc_casefold_into(
    const char *buffer,
    size_t byte_length,
    mjb_encoding encoding,
    mjb_encoding output_encoding,
    void *output,
    size_t *output_size
);
```

Apply the same normative `NFKC_Casefold` transform as `mjb_nfkc_casefold`. Set `output` to NULL to query the required size. If `output` is non-NULL, `*output_size` supplies its capacity; on return it contains the required size when the buffer is too small, or the written size on success. Terminators are excluded from the byte count and are not written. No bytes are written when capacity is insufficient. The final output uses caller-provided storage, but the normalization and folding passes require temporary allocations, including during a size query.

- `buffer` - The string to transform
- `byte_length` - The length of the string in bytes, or `MJB_NUL_TERMINATED` to determine it from an encoding-aware NUL code unit
- `encoding` - The encoding of the string
- `output_encoding` - The output encoding of the string
- `output` - The caller-provided output buffer, or NULL to query the required size. The caller retains ownership
- `output_size` - The input capacity and output required or written byte count

**Returns**

- `MJB_STATUS_OK` - The required size was returned or the transformed string was written
- `MJB_STATUS_INVALID_ARGUMENT` - `output_size` is NULL, or `buffer` is NULL with a non-zero size
- `MJB_STATUS_OVERFLOW` - The required output size would overflow
- `MJB_STATUS_UNSUPPORTED` - The transform did not stabilize
- `MJB_STATUS_NO_MEMORY` - Temporary allocation failed
- `MJB_STATUS_OUTPUT_TOO_SMALL` - The output capacity is smaller than the required byte count

**Example**

```c
const char *input = "Stra\xC3\x9F" "e\xC2\xAD";
size_t output_size = 0;

if(mjb_nfkc_casefold_into(input, strlen(input), MJB_ENC_UTF_8, MJB_ENC_UTF_8,
    NULL, &output_size) != MJB_STATUS_OK) {
    return 1;
}

char output[7];

if(output_size > sizeof(output) || mjb_nfkc_casefold_into(input, strlen(input), MJB_ENC_UTF_8,
    MJB_ENC_UTF_8, output, &output_size) != MJB_STATUS_OK) {
    return 1;
}

// NFKC casefold payload (no terminator): strasse
printf("NFKC casefold payload (no terminator): %.*s", (int)output_size, output);
```

See also: [`mjb_nfkc_casefold`](#mjb_nfkc_casefold), [`mjb_normalize`](#mjb_normalize), [`mjb_map_case`](#mjb_map_case), [`mjb_is_identifier`](#mjb_is_identifier).

Specifications: [The Unicode Standard, Version 18.0.0, Section 3.13: Default Case Algorithms](https://www.unicode.org/versions/Unicode18.0.0/core-spec/chapter-3/#G33992), [UAX #31: Unicode Identifiers and Syntax, Unicode 18.0.0](https://www.unicode.org/reports/tr31/tr31-44.html), [UAX #44: Unicode Character Database, Unicode 18.0.0](https://www.unicode.org/reports/tr44/tr44-36.html).

## `mjb_normalization_quick_check`

Check if a string is normalized to NFC/NFKC/NFD/NFKD form.

```c
mjb_status mjb_normalization_quick_check(
    const char *buffer,
    size_t byte_length,
    mjb_encoding encoding,
    mjb_normalization form,
    mjb_quick_check_result *quick_check
);
```

Run the normalization quick-check on a string without allocating. `MJB_QC_MAYBE` means the string may still be normalized, and only a full normalization pass with `mjb_normalize` can decide.

- `buffer` - The string to check
- `byte_length` - The length of the string in bytes, or `MJB_NUL_TERMINATED` to determine it from an encoding-aware NUL code unit
- `encoding` - The encoding of the string
- `form` - The normalization form to check
- `quick_check` - The quick-check result to store

**Returns**

- `MJB_STATUS_OK` - `quick_check` contains `MJB_QC_YES`, `MJB_QC_NO`, or `MJB_QC_MAYBE`
- `MJB_STATUS_INVALID_ARGUMENT` - `quick_check` is NULL, or `buffer` is NULL with a non-zero size
- `MJB_STATUS_INVALID_ENCODING` - `encoding` is invalid or a generic UTF-16/UTF-32 encoding has no byte-order information
- `MJB_STATUS_INVALID_FORM` - `form` is not NFC, NFD, NFKC, or NFKD
- `MJB_STATUS_MALFORMED_INPUT` - The input contains an ill-formed code-unit sequence

**Example**

```c
const char *input = "caf\xC3\xA9";
mjb_quick_check_result check;

if(mjb_normalization_quick_check(input, strlen(input), MJB_ENC_UTF_8,
    MJB_NORMALIZATION_NFC, &check) != MJB_STATUS_OK) {
    return 1;
}

// NFC normalized: yes
printf("NFC normalized: %s", check == MJB_QC_YES ? "yes" : "no");
```

See also: [`mjb_normalize`](#mjb_normalize).

Specifications: [UAX #15: Unicode Normalization Forms, Unicode 18.0.0](https://www.unicode.org/reports/tr15/tr15-57.html).

## `mjb_detect_encoding`

Return the string encoding (the most probable).

```c
mjb_encoding mjb_detect_encoding(
    const char *buffer,
    size_t byte_length
);
```

`mjb_detect_encoding` reports BOM-derived UTF-16/UTF-32 schemes with the generic family bit plus the resolved endian bit. Passing that detected value consumes the leading BOM as a signature. Passing an explicit-endian encoding such as `MJB_ENC_UTF_16BE` preserves an initial U+FEFF as text. When flags overlap, as with a UTF-32LE BOM that also has the UTF-16LE BOM prefix, decoding gives UTF-32 precedence. `MJB_NUL_TERMINATED` is not accepted because the encoding, and therefore the NUL code-unit width, is unknown.

- `buffer` - The string to check
- `byte_length` - The explicit length of the string, in bytes

**Example**

```c
const char utf16le[] = "\xFF\xFEH\0i\0";
mjb_encoding detected = mjb_detect_encoding(utf16le, sizeof(utf16le) - 1);
bool is_utf16le = detected == (MJB_ENC_UTF_16 | MJB_ENC_UTF_16LE);

// UTF-16LE detected: yes
printf("UTF-16LE detected: %s", is_utf16le ? "yes" : "no");
```

## `mjb_is_ascii`

Return true if the string is encoded in ASCII.

```c
bool mjb_is_ascii(
    const char *buffer,
    size_t byte_length
);
```

- `buffer` - The string to check
- `byte_length` - The length of the string in bytes, or `MJB_NUL_TERMINATED` to determine it from an encoding-aware NUL code unit

**Example**

```c
const char *input = "Plain ASCII";

// ASCII: yes
printf("ASCII: %s", mjb_is_ascii(input, strlen(input)) ? "yes" : "no");
```

## `mjb_is_utf8`

Return true if the string is encoded in UTF-8.

```c
bool mjb_is_utf8(
    const char *buffer,
    size_t byte_length
);
```

- `buffer` - The string to check
- `byte_length` - The length of the string in bytes, or `MJB_NUL_TERMINATED` to determine it from an encoding-aware NUL code unit

**Example**

```c
const char *input = "caf\xC3\xA9";

// Valid UTF-8: yes
printf("Valid UTF-8: %s", mjb_is_utf8(input, strlen(input)) ? "yes" : "no");
```

## `mjb_is_utf16`

Return true if the string is encoded in UTF-16BE or UTF-16LE.

```c
bool mjb_is_utf16(
    const char *buffer,
    size_t byte_length
);
```

- `buffer` - The string to check
- `byte_length` - The length of the string in bytes, or `MJB_NUL_TERMINATED` to determine it from an encoding-aware NUL code unit

**Example**

```c
const char utf16be[] = "\xFE\xFF\0H\0i"; // BOM + "Hi" in UTF-16BE

// UTF-16: yes
printf("UTF-16: %s", mjb_is_utf16(utf16be, sizeof(utf16be) - 1) ? "yes" : "no");
```

## `mjb_count_codepoints`

Return the length of a string.

```c
size_t mjb_count_codepoints(
    const char *buffer,
    size_t max_length,
    mjb_encoding encoding
);
```

Return the number of Unicode codepoints in a string, up to `max_length` bytes.

- `buffer` - The string to check
- `max_length` - The maximum length of the string in bytes, or `MJB_NUL_TERMINATED`
- `encoding` - The encoding of the string

**Example**

```c
// The "Héllö" string is five Unicode characters, but has different byte lengths in different encodings.

const char *utf8 = "H\xC3\xA9ll\xC3\xB6"; // 7 bytes
const char utf16le[] = "H\0\xE9\0l\0l\0\xF6\0"; // 10 bytes
const char utf16be[] = "\0H\0\xE9\0l\0l\0\xF6"; // 10 bytes
const char utf32le[] = "H\0\0\0\xE9\0\0\0l\0\0\0l\0\0\0\xF6\0\0\0"; // 20 bytes
const char utf32be[] = "\0\0\0H\0\0\0\xE9\0\0\0l\0\0\0l\0\0\0\xF6"; // 20 bytes

// 5 UTF-8 characters
printf("%zu UTF-8 characters", mjb_count_codepoints(utf8, 7, MJB_ENC_UTF_8));
// 5 UTF-16LE characters
printf("%zu UTF-16LE characters", mjb_count_codepoints(utf16le, 10, MJB_ENC_UTF_16LE));
// 5 UTF-16BE characters
printf("%zu UTF-16BE characters", mjb_count_codepoints(utf16be, 10, MJB_ENC_UTF_16BE));
// 5 UTF-32LE characters
printf("%zu UTF-32LE characters", mjb_count_codepoints(utf32le, 20, MJB_ENC_UTF_32LE));
// 5 UTF-32BE characters
printf("%zu UTF-32BE characters", mjb_count_codepoints(utf32be, 20, MJB_ENC_UTF_32BE));
```

## `mjb_for_each_character`

Run a callback for each character of a string.

```c
mjb_status mjb_for_each_character(
    const char *buffer,
    size_t byte_length,
    mjb_encoding encoding,
    mjb_for_each_character_fn callback
);
```

- `buffer` - The string to check
- `byte_length` - The length of the string in bytes, or `MJB_NUL_TERMINATED` to determine it from an encoding-aware NUL code unit
- `encoding` - The encoding of the string
- `callback` - The function to call for each character

**Example**

```c
mjb_status status = mjb_for_each_character("ABC", 3, MJB_ENC_UTF_8, NULL);

// A callback is required: yes
bool callback_required = status == MJB_STATUS_INVALID_ARGUMENT;

// A callback is required: yes
printf("A callback is required: %s", callback_required ? "yes" : "no");
```

## `mjb_codepoint_property_binary`

Return the value of a binary Unicode property.

```c
mjb_status mjb_codepoint_property_binary(
    mjb_codepoint codepoint,
    mjb_property property,
    bool *value
);
```

Return `true` when the codepoint has the binary property and `false` when it does not. Passing an enumerated property is a type mismatch and returns `MJB_STATUS_INVALID_ARGUMENT`.

- `codepoint` - The codepoint to check
- `property` - The binary property to query
- `value` - Where to store the binary property value

**Example**

```c
bool is_alphabetic;

if(mjb_codepoint_property_binary('A', MJB_PR_ALPHABETIC,
    &is_alphabetic) != MJB_STATUS_OK) {
    return 1;
}

// U+0041 is alphabetic: yes
printf("U+0041 is alphabetic: %s", is_alphabetic ? "yes" : "no");
```

See also: [`mjb_codepoint_property_int`](#mjb_codepoint_property_int).

Specifications: [UAX #44: Unicode Character Database, Unicode 18.0.0](https://www.unicode.org/reports/tr44/tr44-36.html).

## `mjb_codepoint_property_int`

Return the value of an enumerated or integer Unicode property.

```c
mjb_status mjb_codepoint_property_int(
    mjb_codepoint codepoint,
    mjb_property property,
    int32_t *value
);
```

Passing a binary property is a type mismatch and returns `MJB_STATUS_INVALID_ARGUMENT`. `MJB_STATUS_NOT_FOUND` means that the codepoint has no stored value for the requested property.

- `codepoint` - The codepoint to check
- `property` - The enumerated or integer property to query
- `value` - Where to store the property value

**Example**

```c
int32_t script;

if(mjb_codepoint_property_int('A', MJB_PR_SCRIPT, &script) != MJB_STATUS_OK) {
    return 1;
}

// U+0041 uses the Latin script: yes
printf("U+0041 uses the Latin script: %s", script == MJB_SC_LATN ? "yes" : "no");
```

See also: [`mjb_codepoint_property_binary`](#mjb_codepoint_property_binary).

Specifications: [UAX #44: Unicode Character Database, Unicode 18.0.0](https://www.unicode.org/reports/tr44/tr44-36.html).

## `mjb_codepoint_numeric_value`

Return the numeric value of a codepoint.

```c
mjb_status mjb_codepoint_numeric_value(
    mjb_codepoint codepoint,
    mjb_numeric_value *value
);
```

Return the numeric value of a codepoint, if any. If the codepoint has no numeric value, `value->decimal` and `value->digit` are set to `MJB_NUMBER_NOT_VALID` (-1).

- `codepoint` - The codepoint to check
- `value` - The numeric value to store the result

**Returns**

- `MJB_STATUS_OK` - The character was found and filled
- `MJB_STATUS_INVALID_ARGUMENT` - `value` is NULL or the codepoint is not valid

**Example**

```c
mjb_numeric_value num;

if(mjb_codepoint_numeric_value(0x0031, &num) != MJB_STATUS_OK) { // U+0031 = 1
    return 1;
}

// decimal=1, digit=1, numeric=1
printf("decimal=%d, digit=%d, numeric=%s", num.decimal, num.digit, num.numeric);

if(mjb_codepoint_numeric_value(0x00BD, &num) != MJB_STATUS_OK) { // U+00BD = '½'
    return 1;
}

// decimal=-1, digit=-1, numeric=1/2
printf("decimal=%d, digit=%d, numeric=%s", num.decimal, num.digit, num.numeric);
```

Specifications: [UAX #44: Unicode Character Database, Unicode 18.0.0](https://www.unicode.org/reports/tr44/tr44-36.html).

## `mjb_codepoint_block`

Return the character block.

```c
mjb_status mjb_codepoint_block(
    mjb_codepoint codepoint,
    mjb_block_info *block
);
```

- `codepoint` - The codepoint to check
- `block` - The block to store the result

**Example**

```c
mjb_block_info block;

if(mjb_codepoint_block('A', &block) != MJB_STATUS_OK) {
    return 1;
}

// Block: Basic Latin
printf("Block: %s", block.name);
```

Specifications: [UAX #44: Unicode Character Database, Unicode 18.0.0](https://www.unicode.org/reports/tr44/tr44-36.html).

## `mjb_codepoint_script`

Return the script of a codepoint.

```c
mjb_script mjb_codepoint_script(
    mjb_codepoint codepoint
);
```

- `codepoint` - The codepoint to check

**Example**

```c
mjb_script script = mjb_codepoint_script(0x03A9); // Greek capital omega

// Greek script: yes
printf("Greek script: %s", script == MJB_SC_GREK ? "yes" : "no");
```

Specifications: [UAX #44: Unicode Character Database, Unicode 18.0.0](https://www.unicode.org/reports/tr44/tr44-36.html).

## `mjb_codepoint_script_extensions`

Return the Script_Extensions set of a codepoint.

```c
mjb_status mjb_codepoint_script_extensions(
    mjb_codepoint codepoint,
    mjb_script *scripts,
    size_t *count
);
```

Return the explicit Script_Extensions set, or the ordinary Script value when the codepoint has no explicit Script_Extensions entry. Call first with `scripts` set to NULL to obtain the required count.

- `codepoint` - The codepoint to check
- `scripts` - The caller-provided script buffer, or NULL to query the required count
- `count` - The input capacity and output script count

**Example**

```c
size_t count = 0;

if(mjb_codepoint_script_extensions(0x30FC, NULL, &count) != MJB_STATUS_OK) {
    return 1;
}

mjb_script scripts[3];

if(count > 3 || mjb_codepoint_script_extensions(0x30FC, scripts,
    &count) != MJB_STATUS_OK) {
    return 1;
}

// U+30FC has 2 Script_Extensions
printf("U+30FC has %zu Script_Extensions", count);
```

See also: [`mjb_codepoint_script`](#mjb_codepoint_script).

Specifications: [UAX #24: Unicode Script Property, Unicode 18.0.0](https://www.unicode.org/reports/tr24/tr24-40.html).

## `mjb_codepoint_encode`

Encode a codepoint to a string.

```c
unsigned int mjb_codepoint_encode(
    mjb_codepoint codepoint,
    char *buffer,
    size_t byte_length,
    mjb_encoding encoding
);
```

- `codepoint` - The codepoint to encode
- `buffer` - The buffer to encode the codepoint to
- `byte_length` - The length of the buffer, in bytes
- `encoding` - The encoding to use

**Example**

```c
char encoded[4];
unsigned int size = mjb_codepoint_encode(0x20AC, encoded, sizeof(encoded), MJB_ENC_UTF_8);

// € sign uses 3 UTF-8 bytes
printf("%.*s sign uses %u UTF-8 bytes", (int)size, encoded, size);
```

## `mjb_convert_encoding`

Convert from one encoding to another.

```c
mjb_status mjb_convert_encoding(
    const char *buffer,
    size_t byte_length,
    mjb_encoding encoding,
    mjb_encoding output_encoding,
    mjb_result *result
);
```

Convert a string between the supported encodings (UTF-8, UTF-16LE/BE, UTF-32LE/BE). Generic UTF-16/UTF-32 input consumes a leading BOM as the encoding scheme signature and uses it to resolve byte order. Explicit-endian input preserves an initial U+FEFF as text. Generic UTF-16/UTF-32 without a BOM, and generic UTF-16/UTF-32 output, are rejected because the byte order is not specified.

- `buffer` - The string to convert
- `byte_length` - The length of the string in bytes, or `MJB_NUL_TERMINATED` to determine it from an encoding-aware NUL code unit
- `encoding` - The input encoding of the string
- `output_encoding` - The output encoding of the string
- `result` - The pointer to store the result. If `result->transformed` is true, `result->output` is library-allocated and must be freed with `mjb_result_free(result)`

**Returns**

- `MJB_STATUS_OK` - The string was converted
- `MJB_STATUS_INVALID_ARGUMENT` - `result` is NULL, `buffer` is NULL with a non-zero size, or the input is not valid in the source encoding
- `MJB_STATUS_INVALID_ENCODING` - A generic UTF-16/UTF-32 encoding did not provide enough byte order information
- `MJB_STATUS_UNSUPPORTED` - The requested encoding conversion is not supported
- `MJB_STATUS_OVERFLOW` - The output size would overflow
- `MJB_STATUS_NO_MEMORY` - Allocation failed

**Example**

```c
const char *input = "caf\xC3\xA9";
mjb_result result;

if(mjb_convert_encoding(input, strlen(input), MJB_ENC_UTF_8,
    MJB_ENC_UTF_16LE, &result) != MJB_STATUS_OK) {
    return 1;
}

// UTF-16LE bytes: 8
printf("UTF-16LE bytes: %zu", result.output_size);
mjb_result_free(&result);
```

See also: [`mjb_convert_encoding_into`](#mjb_convert_encoding_into), [`mjb_detect_encoding`](#mjb_detect_encoding), [`mjb_codepoint_encode`](#mjb_codepoint_encode).

## `mjb_convert_encoding_into`

Convert from one encoding to another into a caller-provided buffer.

```c
mjb_status mjb_convert_encoding_into(
    const char *buffer,
    size_t byte_length,
    mjb_encoding encoding,
    mjb_encoding output_encoding,
    void *output,
    size_t *output_size
);
```

Convert a string using the same encoding and BOM rules as `mjb_convert_encoding`, without allocating memory. Set `output` to NULL to query the required size. If `output` is non-NULL, `*output_size` supplies its capacity; on return it contains the required size when the buffer is too small, or the written size on success. The size is the encoded payload byte count: terminators are excluded, and this function does not write a terminator. No bytes are written when capacity is insufficient.

- `buffer` - The string to convert
- `byte_length` - The length of the string in bytes, or `MJB_NUL_TERMINATED` to determine it from an encoding-aware NUL code unit
- `encoding` - The input encoding of the string
- `output_encoding` - The output encoding of the string
- `output` - The caller-provided output buffer, or NULL to query the required size. The caller retains ownership
- `output_size` - The input capacity and output required or written byte count

**Returns**

- `MJB_STATUS_OK` - The required size was returned or the string was converted
- `MJB_STATUS_INVALID_ARGUMENT` - `output_size` is NULL, or `buffer` is NULL with a non-zero size
- `MJB_STATUS_INVALID_ENCODING` - A generic UTF-16/UTF-32 encoding did not provide enough byte order information
- `MJB_STATUS_UNSUPPORTED` - The requested encoding conversion is not supported
- `MJB_STATUS_OVERFLOW` - The required output size would overflow
- `MJB_STATUS_OUTPUT_TOO_SMALL` - The output capacity is smaller than the required byte count

**Example**

```c
const char *input = "caf\xC3\xA9";
size_t output_size = 0;

if(mjb_convert_encoding_into(input, strlen(input), MJB_ENC_UTF_8,
    MJB_ENC_UTF_16LE, NULL, &output_size) != MJB_STATUS_OK) {
    return 1;
}

unsigned char output[8];

if(output_size > sizeof(output) || mjb_convert_encoding_into(input, strlen(input),
    MJB_ENC_UTF_8, MJB_ENC_UTF_16LE, output, &output_size) != MJB_STATUS_OK) {
    return 1;
}

// UTF-16LE payload bytes (no terminator): 8
printf("UTF-16LE payload bytes (no terminator): %zu", output_size);
```

See also: [`mjb_convert_encoding`](#mjb_convert_encoding), [`mjb_detect_encoding`](#mjb_detect_encoding), [`mjb_codepoint_encode`](#mjb_codepoint_encode).

## `mjb_collation_compare`

Compare two strings using UCA.

```c
mjb_status mjb_collation_compare(
    const char *s1,
    size_t s1_byte_length,
    mjb_encoding s1_encoding,
    const char *s2,
    size_t s2_byte_length,
    mjb_encoding s2_encoding,
    mjb_collation_mode mode,
    int *order
);
```

Compare two strings using the Unicode Collation Algorithm and the default collation element table (DUCET), with `strcmp`-style semantics.

- `s1` - The first string to compare
- `s1_byte_length` - The length of the first string in bytes, or `MJB_NUL_TERMINATED`
- `s1_encoding` - The encoding of the first string
- `s2` - The second string to compare
- `s2_byte_length` - The length of the second string in bytes, or `MJB_NUL_TERMINATED`
- `s2_encoding` - The encoding of the second string
- `mode` - The variable weighting strategy
- `order` - The strcmp-style comparison result to store

**Returns**

- `MJB_STATUS_OK` - `order` is negative, zero, or positive according to the collation order
- `MJB_STATUS_INVALID_ARGUMENT` - `order` is NULL, or an input buffer is NULL with a non-zero size
- `MJB_STATUS_INVALID_ENCODING` - An input encoding is invalid or lacks byte-order information
- `MJB_STATUS_MALFORMED_INPUT` - An input contains an ill-formed code-unit sequence
- `MJB_STATUS_OVERFLOW` - An intermediate size would overflow
- `MJB_STATUS_NO_MEMORY` - Allocation failed

**Example**

```c
int order;

if(mjb_collation_compare("apple", 5, MJB_ENC_UTF_8,
    "banana", 6, MJB_ENC_UTF_8, MJB_COLLATION_NON_IGNORABLE, &order) != MJB_STATUS_OK) {
    return 1;
}

// apple sorts before banana: yes
printf("apple sorts before banana: %s", order < 0 ? "yes" : "no");
```

See also: [`mjb_collation_key`](#mjb_collation_key), [`mjb_collation_key_into`](#mjb_collation_key_into).

Specifications: [UTS #10: Unicode Collation Algorithm, Unicode 18.0.0](https://www.unicode.org/reports/tr10/tr10-54.html).

## `mjb_collation_key`

Generate a UCA sort key for a string.

```c
mjb_status mjb_collation_key(
    const char *buffer,
    size_t byte_length,
    mjb_encoding encoding,
    mjb_collation_mode mode,
    mjb_result *result
);
```

Generate a binary sort key for a string. Sort keys of different strings can be compared with `memcmp` and yield the same order as `mjb_collation_compare`. Useful when the same strings are compared many times, such as sorting or database indexing.

- `buffer` - The string to generate the sort key for
- `byte_length` - The length of the string in bytes, or `MJB_NUL_TERMINATED` to determine it from an encoding-aware NUL code unit
- `encoding` - The encoding of the string
- `mode` - The variable weighting strategy
- `result` - The pointer to store the binary sort key. If `result->transformed` is true, `result->output` is library-allocated and must be freed with `mjb_result_free(result)`

**Returns**

- `MJB_STATUS_OK` - The sort key was generated
- `MJB_STATUS_INVALID_ARGUMENT` - `result` is NULL, or `buffer` is NULL with a non-zero size
- `MJB_STATUS_OVERFLOW` - The sort key size would overflow
- `MJB_STATUS_NO_MEMORY` - Allocation failed

**Example**

```c
mjb_result key;

if(mjb_collation_key("r\xC3\xA9sum\xC3\xA9", 8, MJB_ENC_UTF_8,
    MJB_COLLATION_NON_IGNORABLE, &key) != MJB_STATUS_OK) {
    return 1;
}

// Sort key is non-empty: yes
printf("Sort key is non-empty: %s", key.output_size > 0 ? "yes" : "no");
mjb_result_free(&key);
```

See also: [`mjb_collation_key_into`](#mjb_collation_key_into), [`mjb_collation_compare`](#mjb_collation_compare).

Specifications: [UTS #10: Unicode Collation Algorithm, Unicode 18.0.0](https://www.unicode.org/reports/tr10/tr10-54.html).

## `mjb_collation_key_into`

Generate a binary collation key into a caller-provided buffer.

```c
mjb_status mjb_collation_key_into(
    const char *buffer,
    size_t byte_length,
    mjb_encoding encoding,
    mjb_collation_mode mode,
    void *output,
    size_t *output_size
);
```

Generate the same binary sort key as `mjb_collation_key` without allocating the final key buffer. Set `output` to NULL to query the required byte count. If `output` is non-NULL, `*output_size` supplies its capacity; on return it contains the required size when the buffer is too small, or the written size on success. A collation key is binary: no terminator is included or written, and no bytes are written when capacity is insufficient. Collation processing still uses temporary allocations, including during a size query.

- `buffer` - The string to generate the sort key for
- `byte_length` - The length of the string in bytes, or `MJB_NUL_TERMINATED` to determine it from an encoding-aware NUL code unit
- `encoding` - The encoding of the string
- `mode` - The variable weighting strategy
- `output` - The caller-provided binary output buffer, or NULL to query its size. The caller retains ownership
- `output_size` - The input capacity and output required or written byte count

**Returns**

- `MJB_STATUS_OK` - The required size was returned or the binary sort key was written
- `MJB_STATUS_INVALID_ARGUMENT` - `output_size` is NULL, or `buffer` is NULL with a non-zero size
- `MJB_STATUS_INVALID_ENCODING` - The input encoding is invalid
- `MJB_STATUS_MALFORMED_INPUT` - The input contains an ill-formed code-unit sequence
- `MJB_STATUS_OVERFLOW` - The required key size would overflow
- `MJB_STATUS_NO_MEMORY` - Temporary allocation failed
- `MJB_STATUS_OUTPUT_TOO_SMALL` - The output capacity is smaller than the required byte count

**Example**

```c
const char *input = "r\xC3\xA9sum\xC3\xA9";
size_t output_size = 0;

if(mjb_collation_key_into(input, 8, MJB_ENC_UTF_8, MJB_COLLATION_NON_IGNORABLE,
    NULL, &output_size) != MJB_STATUS_OK) {
    return 1;
}

unsigned char output[64];

if(output_size > sizeof(output) || mjb_collation_key_into(input, 8, MJB_ENC_UTF_8,
    MJB_COLLATION_NON_IGNORABLE, output, &output_size) != MJB_STATUS_OK) {
    return 1;
}

// Sort key is non-empty: yes
printf("Sort key is non-empty: %s", output_size > 0 ? "yes" : "no");
```

See also: [`mjb_collation_key`](#mjb_collation_key), [`mjb_collation_compare`](#mjb_collation_compare).

Specifications: [UTS #10: Unicode Collation Algorithm, Unicode 18.0.0](https://www.unicode.org/reports/tr10/tr10-54.html).

## `mjb_map_case`

Change string case.

```c
mjb_status mjb_map_case(
    const char *buffer,
    size_t byte_length,
    mjb_encoding encoding,
    mjb_map_case_type type,
    mjb_encoding output_encoding,
    mjb_result *result
);
```

Convert a string to uppercase, lowercase, titlecase, or its case-folded form. Full case mappings are applied, including special casing and conditional mappings, so the output may have a different length than the input. Titlecase uses UAX #29 word boundaries: the first cased character in each word segment is titlecased, and subsequent characters in that segment are lowercased. Casing is tailored by the process-global locale set with `mjb_set_locale`: the default `MJB_LOCALE_EN` uses default non-Turkic mappings. `MJB_LOCALE_TR` and `MJB_LOCALE_AZ` apply Turkish/Azerbaijani dotted-I casing and Turkic `T` case-folding mappings. `MJB_LOCALE_LT` applies Lithuanian dot-above casing rules, while case folding remains the default non-Turkic mapping.

- `buffer` - The string to change case
- `byte_length` - The length of the string in bytes, or `MJB_NUL_TERMINATED` to determine it from an encoding-aware NUL code unit
- `encoding` - The encoding of the string
- `type` - The type of case change
- `output_encoding` - The output encoding of the string
- `result` - The pointer to store the result. If `result->transformed` is true, `result->output` is library-allocated and must be freed with `mjb_result_free(result)`

**Returns**

- `MJB_STATUS_OK` - The case conversion succeeded
- `MJB_STATUS_INVALID_ARGUMENT` - `result` is NULL, `buffer` is NULL with a non-zero size, or `type` is not a valid case type
- `MJB_STATUS_NO_MEMORY` - Allocation failed

**Example**

```c
const char *input = "Stra\xC3\x9F""e"; // "Straße"
mjb_result result;

if(mjb_map_case(input, strlen(input), MJB_ENC_UTF_8, MJB_CASE_UPPER, MJB_ENC_UTF_8,
    &result) != MJB_STATUS_OK) {
    return 1;
}

// Upper: STRASSE
printf("Upper: %.*s", (int)result.output_size, result.output);

mjb_result_free(&result);
```

See also: [`mjb_map_case_into`](#mjb_map_case_into), [`mjb_set_locale`](#mjb_set_locale), [`mjb_get_locale`](#mjb_get_locale).

Specifications: [The Unicode Standard, Version 18.0.0, Section 3.13: Default Case Algorithms](https://www.unicode.org/versions/Unicode18.0.0/core-spec/chapter-3/#G33992).

## `mjb_map_case_into`

Change string case into a caller-provided buffer.

```c
mjb_status mjb_map_case_into(
    const char *buffer,
    size_t byte_length,
    mjb_encoding encoding,
    mjb_map_case_type type,
    mjb_encoding output_encoding,
    void *output,
    size_t *output_size
);
```

Apply the same full, special, conditional, titlecase, locale-sensitive, and case folding mappings as `mjb_map_case` without allocating memory. Set `output` to NULL to query the required size. If `output` is non-NULL, `*output_size` supplies its capacity; on return it contains the required size when the buffer is too small, or the written size on success. Terminators are excluded from the byte count and are not written. No bytes are written when capacity is insufficient.

- `buffer` - The string to change case
- `byte_length` - The length of the string in bytes, or `MJB_NUL_TERMINATED` to determine it from an encoding-aware NUL code unit
- `encoding` - The encoding of the string
- `type` - The type of case change
- `output_encoding` - The output encoding of the string
- `output` - The caller-provided output buffer, or NULL to query the required size. The caller retains ownership
- `output_size` - The input capacity and output required or written byte count

**Returns**

- `MJB_STATUS_OK` - The required size was returned or the case-mapped string was written
- `MJB_STATUS_INVALID_ARGUMENT` - `output_size` is NULL, `buffer` is NULL with a non-zero size, or `type` is invalid
- `MJB_STATUS_UNSUPPORTED` - The requested output encoding cannot represent a mapped codepoint
- `MJB_STATUS_OVERFLOW` - The required output size would overflow
- `MJB_STATUS_OUTPUT_TOO_SMALL` - The output capacity is smaller than the required byte count

**Example**

```c
const char *input = "Stra\xC3\x9F""e"; // "Straße"
size_t output_size = 0;

if(mjb_map_case_into(input, strlen(input), MJB_ENC_UTF_8, MJB_CASE_UPPER, MJB_ENC_UTF_8,
    NULL, &output_size) != MJB_STATUS_OK) {
    return 1;
}

char output[7];

if(output_size > sizeof(output) || mjb_map_case_into(input, strlen(input), MJB_ENC_UTF_8,
    MJB_CASE_UPPER, MJB_ENC_UTF_8, output, &output_size) != MJB_STATUS_OK) {
    return 1;
}

// Upper payload (no terminator): STRASSE
printf("Upper payload (no terminator): %.*s", (int)output_size, output);
```

See also: [`mjb_map_case`](#mjb_map_case), [`mjb_set_locale`](#mjb_set_locale), [`mjb_get_locale`](#mjb_get_locale).

Specifications: [The Unicode Standard, Version 18.0.0, Section 3.13: Default Case Algorithms](https://www.unicode.org/versions/Unicode18.0.0/core-spec/chapter-3/#G33992).

## `mjb_codepoint_is_valid`

Return true if the codepoint is valid.

```c
bool mjb_codepoint_is_valid(
    mjb_codepoint codepoint
);
```

- `codepoint` - The codepoint to check

**Example**

```c
// U+10FFFD valid: yes
printf("U+10FFFD valid: %s", mjb_codepoint_is_valid(0x10FFFD) ? "yes" : "no");
```

## `mjb_codepoint_is_graphic`

Return true if the codepoint is graphic.

```c
bool mjb_codepoint_is_graphic(
    mjb_codepoint codepoint
);
```

- `codepoint` - The codepoint to check

**Example**

```c
// Letter A is graphic: yes
printf("Letter A is graphic: %s", mjb_codepoint_is_graphic('A') ? "yes" : "no");
```

## `mjb_codepoint_is_combining`

Return true if the codepoint is combining.

```c
bool mjb_codepoint_is_combining(
    mjb_codepoint codepoint
);
```

- `codepoint` - The codepoint to check

**Example**

```c
// U+0301 is combining: yes
printf("U+0301 is combining: %s", mjb_codepoint_is_combining(0x0301) ? "yes" : "no");
```

## `mjb_codepoint_is_hangul_leading_jamo`

Return if the codepoint is a hangul L.

```c
bool mjb_codepoint_is_hangul_leading_jamo(
    mjb_codepoint codepoint
);
```

- `codepoint` - The codepoint to check

**Example**

```c
// U+1100 is a leading Jamo: yes
printf("U+1100 is a leading Jamo: %s", mjb_codepoint_is_hangul_leading_jamo(0x1100) ? "yes" : "no");
```

## `mjb_codepoint_is_hangul_vowel_jamo`

Return if the codepoint is a hangul V.

```c
bool mjb_codepoint_is_hangul_vowel_jamo(
    mjb_codepoint codepoint
);
```

- `codepoint` - The codepoint to check

**Example**

```c
// U+1161 is a vowel Jamo: yes
printf("U+1161 is a vowel Jamo: %s", mjb_codepoint_is_hangul_vowel_jamo(0x1161) ? "yes" : "no");
```

## `mjb_codepoint_is_hangul_trailing_jamo`

Return if the codepoint is a hangul T.

```c
bool mjb_codepoint_is_hangul_trailing_jamo(
    mjb_codepoint codepoint
);
```

- `codepoint` - The codepoint to check

**Example**

```c
// U+11A8 is a trailing Jamo: yes
printf("U+11A8 is a trailing Jamo: %s", mjb_codepoint_is_hangul_trailing_jamo(0x11A8) ? "yes" : "no");
```

## `mjb_codepoint_is_hangul_jamo`

Return if the codepoint is a hangul jamo.

```c
bool mjb_codepoint_is_hangul_jamo(
    mjb_codepoint codepoint
);
```

- `codepoint` - The codepoint to check

**Example**

```c
// U+1100 is Hangul Jamo: yes
printf("U+1100 is Hangul Jamo: %s", mjb_codepoint_is_hangul_jamo(0x1100) ? "yes" : "no");
```

## `mjb_codepoint_is_hangul_syllable`

Return if the codepoint is a hangul syllable.

```c
bool mjb_codepoint_is_hangul_syllable(
    mjb_codepoint codepoint
);
```

- `codepoint` - The codepoint to check

**Example**

```c
// U+AC00 is a Hangul syllable: yes
printf("U+AC00 is a Hangul syllable: %s", mjb_codepoint_is_hangul_syllable(0xAC00) ? "yes" : "no");
```

## `mjb_codepoint_is_cjk_ideograph`

Return if the codepoint is CJK ideograph.

```c
bool mjb_codepoint_is_cjk_ideograph(
    mjb_codepoint codepoint
);
```

- `codepoint` - The codepoint to check

**Example**

```c
// U+4E00 is a CJK ideograph: yes
printf("U+4E00 is a CJK ideograph: %s", mjb_codepoint_is_cjk_ideograph(0x4E00) ? "yes" : "no");
```

## `mjb_codepoint_is_cjk_extension_ideograph`

Return if the codepoint is CJK extension.

```c
bool mjb_codepoint_is_cjk_extension_ideograph(
    mjb_codepoint codepoint
);
```

- `codepoint` - The codepoint to check

**Example**

```c
// U+20000 is a CJK extension ideograph: yes
printf("U+20000 is a CJK extension ideograph: %s", mjb_codepoint_is_cjk_extension_ideograph(0x20000) ? "yes" : "no");
```

## `mjb_category_is_graphic`

Return true if the category is graphic.

```c
bool mjb_category_is_graphic(
    mjb_category category
);
```

- `category` - The category to check

**Example**

```c
// Uppercase letters are graphic: yes
bool graphic = mjb_category_is_graphic(MJB_CATEGORY_LU);

// Uppercase letters are graphic: yes
printf("Uppercase letters are graphic: %s", graphic ? "yes" : "no");
```

## `mjb_category_is_combining`

Return true if the category is combining.

```c
bool mjb_category_is_combining(
    mjb_category category
);
```

- `category` - The category to check

**Example**

```c
// Nonspacing marks are combining: yes
bool combining = mjb_category_is_combining(MJB_CATEGORY_MN);

// Nonspacing marks are combining: yes
printf("Nonspacing marks are combining: %s", combining ? "yes" : "no");
```

## `mjb_next_line_break`

Unicode line break algorithm.

```c
mjb_break_type mjb_next_line_break(
    const char *buffer,
    size_t byte_length,
    mjb_encoding encoding,
    mjb_next_line_state *state
);
```

- `buffer` - The string to check
- `byte_length` - The explicit length of the string, in bytes
- `encoding` - The encoding of the string
- `state` - The state to store the result

**Example**

```c
mjb_next_line_state state;
state.index = 0;
mjb_break_type type = mjb_next_line_break("Hello world", 11, MJB_ENC_UTF_8, &state);

// First line-break result is set: yes
printf("First line-break result is set: %s", type != MJB_BT_NOT_SET ? "yes" : "no");
```

See also: [`mjb_next_grapheme_break`](#mjb_next_grapheme_break), [`mjb_next_word_break`](#mjb_next_word_break), [`mjb_next_sentence_break`](#mjb_next_sentence_break).

Specifications: [UAX #14: Unicode Line Breaking Algorithm, Unicode 18.0.0](https://www.unicode.org/reports/tr14/tr14-56.html).

## `mjb_next_word_break`

Word cluster breaking.

```c
mjb_break_type mjb_next_word_break(
    const char *buffer,
    size_t byte_length,
    mjb_encoding encoding,
    mjb_next_word_state *state
);
```

- `buffer` - The string to check
- `byte_length` - The explicit length of the string, in bytes
- `encoding` - The encoding of the string
- `state` - The state to store the result

**Example**

```c
mjb_next_word_state state;
state.index = 0;
size_t boundaries = 0;

while(mjb_next_word_break("Hello world", 11, MJB_ENC_UTF_8, &state) != MJB_BT_NOT_SET) {
    ++boundaries;
}

// Word-break positions: 11
printf("Word-break positions: %zu", boundaries);
```

See also: [`mjb_next_grapheme_break`](#mjb_next_grapheme_break), [`mjb_next_sentence_break`](#mjb_next_sentence_break), [`mjb_truncate_word`](#mjb_truncate_word).

Specifications: [UAX #29: Unicode Text Segmentation, Unicode 18.0.0](https://www.unicode.org/reports/tr29/tr29-48.html).

## `mjb_next_sentence_break`

Sentence boundaries breaking.

```c
mjb_break_type mjb_next_sentence_break(
    const char *buffer,
    size_t byte_length,
    mjb_encoding encoding,
    mjb_next_sentence_state *state
);
```

- `buffer` - The string to check
- `byte_length` - The explicit length of the string, in bytes
- `encoding` - The encoding of the string
- `state` - The state to store the result

**Example**

```c
mjb_next_sentence_state state;
state.index = 0;
size_t boundaries = 0;
const char *input = "Hello. Goodbye.";

while(mjb_next_sentence_break(input, strlen(input), MJB_ENC_UTF_8, &state) != MJB_BT_NOT_SET) {
    ++boundaries;
}

// Sentence-break positions: 15
printf("Sentence-break positions: %zu", boundaries);
```

See also: [`mjb_next_grapheme_break`](#mjb_next_grapheme_break), [`mjb_next_word_break`](#mjb_next_word_break).

Specifications: [UAX #29: Unicode Text Segmentation, Unicode 18.0.0](https://www.unicode.org/reports/tr29/tr29-48.html).

## `mjb_next_grapheme_break`

Grapheme cluster breaking.

```c
mjb_break_type mjb_next_grapheme_break(
    const char *buffer,
    size_t byte_length,
    mjb_encoding encoding,
    mjb_next_state *state
);
```

Iterate the grapheme cluster (user-perceived character) boundaries of a string. Call repeatedly with the same state until it reports the end of the string. Stateful break functions require an explicit length and do not accept `MJB_NUL_TERMINATED`; determine the length once before iteration.

- `buffer` - The string to check
- `byte_length` - The explicit length of the string, in bytes
- `encoding` - The encoding of the string
- `state` - The state to store the result

**Example**

```c
const char *input = "e\xCC\x81"; // e + combining acute accent
mjb_next_state state;
state.index = 0;
size_t codepoints = 0;

while(mjb_next_grapheme_break(input, strlen(input), MJB_ENC_UTF_8,
    &state) != MJB_BT_NOT_SET) {
    ++codepoints;
}

// Codepoints examined: 2
printf("Codepoints examined: %zu", codepoints);
```

See also: [`mjb_next_word_break`](#mjb_next_word_break), [`mjb_next_sentence_break`](#mjb_next_sentence_break), [`mjb_next_line_break`](#mjb_next_line_break), [`mjb_truncate_grapheme`](#mjb_truncate_grapheme).

Specifications: [UAX #29: Unicode Text Segmentation, Unicode 18.0.0](https://www.unicode.org/reports/tr29/tr29-48.html).

## `mjb_truncate_grapheme`

Return the number of bytes that form the first `max_graphemes` grapheme cluster segments.

```c
size_t mjb_truncate_grapheme(
    const char *buffer,
    size_t byte_length,
    mjb_encoding encoding,
    size_t max_graphemes
);
```

- `buffer` - The string to check
- `byte_length` - The length of the string in bytes, or `MJB_NUL_TERMINATED` to determine it from an encoding-aware NUL code unit
- `encoding` - The encoding of the string
- `max_graphemes` - The maximum number of graphemes to return

**Example**

```c
const char *input = "A\xF0\x9F\x87\xAE\xF0\x9F\x87\xB9Z"; // A🇮🇹Z
size_t bytes = mjb_truncate_grapheme(input, strlen(input), MJB_ENC_UTF_8, 2);

// First two graphemes use 9 bytes
printf("First two graphemes use %zu bytes", bytes);
```

## `mjb_truncate_grapheme_width`

Return the number of bytes whose grapheme clusters fit within max_columns display columns.

```c
size_t mjb_truncate_grapheme_width(
    const char *buffer,
    size_t byte_length,
    mjb_encoding encoding,
    mjb_width_context context,
    size_t max_columns
);
```

- `buffer` - The string to check
- `byte_length` - The length of the string in bytes, or `MJB_NUL_TERMINATED` to determine it from an encoding-aware NUL code unit
- `encoding` - The encoding of the string
- `context` - The width context
- `max_columns` - The maximum number of columns to return

**Example**

```c
const char *input = "A\xE7\x95\x8C"; // A界
size_t bytes = mjb_truncate_grapheme_width(input, strlen(input), MJB_ENC_UTF_8,
    MJB_WIDTH_CONTEXT_WESTERN, 2);

// Two columns include 1 byte
printf("Two columns include %zu byte", bytes);
```

## `mjb_truncate_word`

Return the number of bytes that form the first max_segments word-break segments.

```c
size_t mjb_truncate_word(
    const char *buffer,
    size_t byte_length,
    mjb_encoding encoding,
    size_t max_segments
);
```

- `buffer` - The string to check
- `byte_length` - The length of the string in bytes, or `MJB_NUL_TERMINATED` to determine it from an encoding-aware NUL code unit
- `encoding` - The encoding of the string
- `max_segments` - The maximum number of segments to return

**Example**

```c
const char *input = "Hello world";
size_t bytes = mjb_truncate_word(input, strlen(input), MJB_ENC_UTF_8, 1);

// First word segment uses 5 bytes
printf("First word segment uses %zu bytes", bytes);
```

## `mjb_truncate_word_width`

Return the number of bytes whose word-break segments fit within max_columns display columns.

```c
size_t mjb_truncate_word_width(
    const char *buffer,
    size_t byte_length,
    mjb_encoding encoding,
    mjb_width_context context,
    size_t max_columns
);
```

- `buffer` - The string to check
- `byte_length` - The length of the string in bytes, or `MJB_NUL_TERMINATED` to determine it from an encoding-aware NUL code unit
- `encoding` - The encoding of the string
- `context` - The width context
- `max_columns` - The maximum number of columns to return

**Example**

```c
const char *input = "Hello world";
size_t bytes = mjb_truncate_word_width(input, strlen(input), MJB_ENC_UTF_8,
    MJB_WIDTH_CONTEXT_WESTERN, 6);

// Six columns include 6 bytes
printf("Six columns include %zu bytes", bytes);
```

## `mjb_bidi_resolve`

Resolve bidirectional text (TR9) for a paragraph.

```c
mjb_status mjb_bidi_resolve(
    const char *buffer,
    size_t byte_length,
    mjb_encoding encoding,
    mjb_direction direction,
    mjb_bidi_paragraph *result
);
```

Resolve the embedding levels of a paragraph following the Unicode Bidirectional Algorithm. The resolved paragraph can then be split into lines and reordered visually with `mjb_bidi_reorder_line` and `mjb_bidi_line_runs`.

- `buffer` - The input string
- `byte_length` - The length of the string in bytes, or `MJB_NUL_TERMINATED` to determine it from an encoding-aware NUL code unit
- `encoding` - The encoding of the string
- `direction` - The base paragraph direction (LTR, RTL, or AUTO for P2/P3)
- `result` - Output paragraph; chars is library-allocated. `result->chars` is library-allocated and must be freed with `mjb_bidi_paragraph_free()`

**Returns**

- `MJB_STATUS_OK` - The paragraph was resolved
- `MJB_STATUS_INVALID_ARGUMENT` - `result` is NULL, or `buffer` is NULL with a non-zero size
- `MJB_STATUS_OVERFLOW` - The paragraph size would overflow
- `MJB_STATUS_NO_MEMORY` - Allocation failed

**Example**

```c
const char *input = "abc \xD7\x90\xD7\x91\xD7\x92"; // abc אבג
mjb_bidi_paragraph paragraph;

if(mjb_bidi_resolve(input, strlen(input), MJB_ENC_UTF_8, MJB_DIRECTION_AUTO,
    &paragraph) != MJB_STATUS_OK) {
    return 1;
}

// Paragraph codepoints: 7
printf("Paragraph codepoints: %zu", paragraph.count);
mjb_bidi_paragraph_free(&paragraph);
```

See also: [`mjb_bidi_paragraph_free`](#mjb_bidi_paragraph_free), [`mjb_bidi_reorder_line`](#mjb_bidi_reorder_line), [`mjb_bidi_line_runs`](#mjb_bidi_line_runs).

Specifications: [UAX #9: Unicode Bidirectional Algorithm, Unicode 18.0.0](https://www.unicode.org/reports/tr9/tr9-51.html).

## `mjb_bidi_reorder_line`

Reorder a line visually (L1-L4); visual_order is caller-allocated.

```c
mjb_status mjb_bidi_reorder_line(
    const mjb_bidi_paragraph *paragraph,
    size_t line_start,
    size_t line_end,
    size_t *visual_order
);
```

- `paragraph` - The resolved paragraph
- `line_start` - Start index into paragraph->chars
- `line_end` - End index (exclusive) into paragraph->chars
- `visual_order` - Caller-allocated array of size (`line_end` - `line_start`). Caller-allocated; the library does not retain or free it

**Example**

```c
const char *input = "\xD7\x90\xD7\x91\xD7\x92"; // אבג
mjb_bidi_paragraph paragraph;
size_t visual_order[3];

if(mjb_bidi_resolve(input, strlen(input), MJB_ENC_UTF_8, MJB_DIRECTION_AUTO,
    &paragraph) != MJB_STATUS_OK ||
    mjb_bidi_reorder_line(&paragraph, 0, paragraph.count,
        visual_order) != MJB_STATUS_OK) {
    return 1;
}

// First visual index: 2
printf("First visual index: %zu", visual_order[0]);
mjb_bidi_paragraph_free(&paragraph);
```

See also: [`mjb_bidi_resolve`](#mjb_bidi_resolve), [`mjb_bidi_line_runs`](#mjb_bidi_line_runs).

Specifications: [UAX #9: Unicode Bidirectional Algorithm, Unicode 18.0.0](https://www.unicode.org/reports/tr9/tr9-51.html).

## `mjb_bidi_line_runs`

Compute visual level runs; pass runs=NULL to count first.

```c
mjb_status mjb_bidi_line_runs(
    const mjb_bidi_paragraph *paragraph,
    const size_t *visual_order,
    size_t count,
    mjb_bidi_run *runs,
    size_t *run_count
);
```

- `paragraph` - The resolved paragraph
- `visual_order` - Visual order array from `mjb_bidi_reorder_line`
- `count` - Length of visual_order
- `runs` - Caller-allocated array, or NULL to only count
- `run_count` - On output: number of runs written (or total if `runs` = `NULL`)

**Example**

```c
mjb_bidi_paragraph paragraph;
size_t visual_order[3];
size_t run_count = 0;

if(mjb_bidi_resolve("abc", 3, MJB_ENC_UTF_8, MJB_DIRECTION_LTR,
    &paragraph) != MJB_STATUS_OK ||
    mjb_bidi_reorder_line(&paragraph, 0, 3, visual_order) != MJB_STATUS_OK ||
    mjb_bidi_line_runs(&paragraph, visual_order, 3, NULL,
        &run_count) != MJB_STATUS_OK) {
    return 1;
}

// Visual runs: 1
printf("Visual runs: %zu", run_count);
mjb_bidi_paragraph_free(&paragraph);
```

See also: [`mjb_bidi_resolve`](#mjb_bidi_resolve), [`mjb_bidi_reorder_line`](#mjb_bidi_reorder_line).

Specifications: [UAX #9: Unicode Bidirectional Algorithm, Unicode 18.0.0](https://www.unicode.org/reports/tr9/tr9-51.html).

## `mjb_bidi_paragraph_free`

Free a bidi paragraph allocated by mjb_bidi_resolve.

```c
void mjb_bidi_paragraph_free(
    mjb_bidi_paragraph *paragraph
);
```

- `paragraph` - The paragraph to free

**Example**

```c
mjb_bidi_paragraph paragraph;

if(mjb_bidi_resolve("abc", 3, MJB_ENC_UTF_8, MJB_DIRECTION_LTR,
    &paragraph) != MJB_STATUS_OK) {
    return 1;
}

mjb_bidi_paragraph_free(&paragraph);

// Paragraph released: yes
printf("Paragraph released: %s", paragraph.chars == NULL ? "yes" : "no");
```

See also: [`mjb_bidi_resolve`](#mjb_bidi_resolve).

## `mjb_codepoint_is_id_start`

Return true if the codepoint is a valid Unicode identifier start (Unicode 18.0.0 UAX #31 ID_Start).

```c
bool mjb_codepoint_is_id_start(
    mjb_codepoint codepoint
);
```

- `codepoint` - The codepoint to check

**Example**

```c
// Greek alpha starts an identifier: yes
bool starts = mjb_codepoint_is_id_start(0x03B1);

// Greek alpha starts an identifier: yes
printf("Greek alpha starts an identifier: %s", starts ? "yes" : "no");
```

Specifications: [UAX #31: Unicode Identifiers and Syntax, Unicode 18.0.0](https://www.unicode.org/reports/tr31/tr31-44.html).

## `mjb_codepoint_is_id_continue`

Return true if the codepoint is a valid Unicode identifier continuation (Unicode 18.0.0 UAX #31 ID_Continue).

```c
bool mjb_codepoint_is_id_continue(
    mjb_codepoint codepoint
);
```

- `codepoint` - The codepoint to check

**Example**

```c
// Digit 7 continues an identifier: yes
bool continues = mjb_codepoint_is_id_continue('7');

// Digit 7 continues an identifier: yes
printf("Digit 7 continues an identifier: %s", continues ? "yes" : "no");
```

Specifications: [UAX #31: Unicode Identifiers and Syntax, Unicode 18.0.0](https://www.unicode.org/reports/tr31/tr31-44.html).

## `mjb_codepoint_is_xid_start`

Return true if the codepoint is a valid NFKC identifier start (Unicode 18.0.0 UAX #31 XID_Start).

```c
bool mjb_codepoint_is_xid_start(
    mjb_codepoint codepoint
);
```

- `codepoint` - The codepoint to check

**Example**

```c
// Letter A is XID_Start: yes
printf("Letter A is XID_Start: %s", mjb_codepoint_is_xid_start('A') ? "yes" : "no");
```

Specifications: [UAX #31: Unicode Identifiers and Syntax, Unicode 18.0.0](https://www.unicode.org/reports/tr31/tr31-44.html).

## `mjb_codepoint_is_xid_continue`

Return true if the codepoint is a valid NFKC identifier continuation (Unicode 18.0.0 UAX #31 XID_Continue).

```c
bool mjb_codepoint_is_xid_continue(
    mjb_codepoint codepoint
);
```

- `codepoint` - The codepoint to check

**Example**

```c
// Underscore is XID_Continue: yes
bool continues = mjb_codepoint_is_xid_continue('_');

// Underscore is XID_Continue: yes
printf("Underscore is XID_Continue: %s", continues ? "yes" : "no");
```

Specifications: [UAX #31: Unicode Identifiers and Syntax, Unicode 18.0.0](https://www.unicode.org/reports/tr31/tr31-44.html).

## `mjb_codepoint_is_pattern_syntax`

Return true if the codepoint is reserved for use in patterns (Unicode 18.0.0 UAX #31 Pattern_Syntax).

```c
bool mjb_codepoint_is_pattern_syntax(
    mjb_codepoint codepoint
);
```

- `codepoint` - The codepoint to check

**Example**

```c
// Plus sign is Pattern_Syntax: yes
bool syntax = mjb_codepoint_is_pattern_syntax('+');

// Plus sign is Pattern_Syntax: yes
printf("Plus sign is Pattern_Syntax: %s", syntax ? "yes" : "no");
```

Specifications: [UAX #31: Unicode Identifiers and Syntax, Unicode 18.0.0](https://www.unicode.org/reports/tr31/tr31-44.html).

## `mjb_codepoint_is_pattern_white_space`

Return true if the codepoint is pattern whitespace (Unicode 18.0.0 UAX #31 Pattern_White_Space).

```c
bool mjb_codepoint_is_pattern_white_space(
    mjb_codepoint codepoint
);
```

- `codepoint` - The codepoint to check

**Example**

```c
// Space is Pattern_White_Space: yes
bool whitespace = mjb_codepoint_is_pattern_white_space(' ');

// Space is Pattern_White_Space: yes
printf("Space is Pattern_White_Space: %s", whitespace ? "yes" : "no");
```

Specifications: [UAX #31: Unicode Identifiers and Syntax, Unicode 18.0.0](https://www.unicode.org/reports/tr31/tr31-44.html).

## `mjb_is_identifier`

Return true if the string is a valid Unicode identifier (Unicode 18.0.0 UAX #31).

```c
bool mjb_is_identifier(
    const char *buffer,
    size_t byte_length,
    mjb_encoding encoding,
    mjb_identifier_profile profile
);
```

Validate a string as a Unicode identifier: the first character must be a valid identifier start and the following ones valid identifier continuations, using ID_Start/ID_Continue for the DEFAULT profile or XID_Start/XID_Continue for the NFKC profile.

- `buffer` - The string to validate
- `byte_length` - The length of the string in bytes, or `MJB_NUL_TERMINATED` to determine it from an encoding-aware NUL code unit
- `encoding` - The encoding of the string
- `profile` - The identifier profile (DEFAULT or NFKC)

**Example**

```c
const char *identifier = "delta_2";

bool valid = mjb_is_identifier(identifier, strlen(identifier), MJB_ENC_UTF_8,
    MJB_IDENTIFIER_NFKC);

// Valid identifier: yes
printf("Valid identifier: %s", valid ? "yes" : "no");
```

See also: [`mjb_codepoint_is_id_start`](#mjb_codepoint_is_id_start), [`mjb_codepoint_is_id_continue`](#mjb_codepoint_is_id_continue), [`mjb_codepoint_is_xid_start`](#mjb_codepoint_is_xid_start), [`mjb_codepoint_is_xid_continue`](#mjb_codepoint_is_xid_continue).

Specifications: [UAX #31: Unicode Identifiers and Syntax, Unicode 18.0.0](https://www.unicode.org/reports/tr31/tr31-44.html).

## `mjb_property_name`

Return the name of a property, NULL if the property specified is not valid.

```c
const char *mjb_property_name(
    mjb_property property
);
```

- `property` - The property to check

**Example**

```c
const char *name = mjb_property_name(MJB_PR_ALPHABETIC);

// Property: Alphabetic
printf("Property: %s", name);
```

Specifications: [UAX #44: Unicode Character Database, Unicode 18.0.0](https://www.unicode.org/reports/tr44/tr44-36.html).

## `mjb_confusable_skeleton`

Compute a Unicode confusable skeleton (Unicode 18.0.0 UTS #39 Section 4).

```c
mjb_status mjb_confusable_skeleton(
    const char *buffer,
    size_t byte_length,
    mjb_encoding encoding,
    mjb_encoding output_encoding,
    mjb_result *result
);
```

Compute the UTS #39 `bidiSkeleton(LTR, input)`: apply the Unicode Bidirectional Algorithm through L4, then NFD, remove default-ignorables, substitute prototypes from `confusables.txt`, and reapply NFD. Skeletons can be stored or indexed so future confusable checks can compare them directly.

- `buffer` - The string to transform
- `byte_length` - The length of the string in bytes, or `MJB_NUL_TERMINATED` to determine it from an encoding-aware NUL code unit
- `encoding` - The encoding of the string
- `output_encoding` - The output encoding of the skeleton
- `result` - The pointer to store the result. If `result->transformed` is true, `result->output` is library-allocated and must be freed with `mjb_result_free(result)`

**Returns**

- `MJB_STATUS_OK` - The confusable skeleton was returned
- `MJB_STATUS_INVALID_ARGUMENT` - `result` is NULL, or `buffer` is NULL with a non-zero size
- `MJB_STATUS_OVERFLOW` - The output size would overflow
- `MJB_STATUS_NO_MEMORY` - Allocation failed

**Example**

```c
const char *input = "h\xD0\xB5llo"; // Cyrillic U+0435 in place of e
mjb_result result;

if(mjb_confusable_skeleton(input, strlen(input), MJB_ENC_UTF_8, MJB_ENC_UTF_8,
    &result) != MJB_STATUS_OK) {
    return 1;
}

// hello
printf("%.*s", (int)result.output_size, result.output);
mjb_result_free(&result);
```

See also: [`mjb_confusable_skeleton_into`](#mjb_confusable_skeleton_into), [`mjb_are_confusable`](#mjb_are_confusable), [`mjb_is_identifier`](#mjb_is_identifier).

Specifications: [UTS #39: Unicode Security Mechanisms, Unicode 18.0.0](https://www.unicode.org/reports/tr39/tr39-33.html).

## `mjb_confusable_skeleton_into`

Compute a Unicode confusable skeleton into a caller-provided buffer.

```c
mjb_status mjb_confusable_skeleton_into(
    const char *buffer,
    size_t byte_length,
    mjb_encoding encoding,
    mjb_encoding output_encoding,
    void *output,
    size_t *output_size
);
```

Compute the same UTS #39 `bidiSkeleton(LTR, input)` as `mjb_confusable_skeleton` without allocating the final output buffer. Set `output` to NULL to query the required size. If `output` is non-NULL, `*output_size` supplies its capacity; on return it contains the required size when the buffer is too small, or the written size on success. Terminators are excluded and are not written. No bytes are written when capacity is insufficient. Bidirectional resolution, normalization, and skeleton mapping still require temporary allocations, including during a size query.

- `buffer` - The string to transform
- `byte_length` - The length of the string in bytes, or `MJB_NUL_TERMINATED` to determine it from an encoding-aware NUL code unit
- `encoding` - The encoding of the string
- `output_encoding` - The output encoding of the skeleton
- `output` - The caller-provided output buffer, or NULL to query the required size. The caller retains ownership
- `output_size` - The input capacity and output required or written byte count

**Returns**

- `MJB_STATUS_OK` - The required size was returned or the skeleton was written
- `MJB_STATUS_INVALID_ARGUMENT` - `output_size` is NULL, or `buffer` is NULL with a non-zero size
- `MJB_STATUS_INVALID_ENCODING` - An encoding is invalid
- `MJB_STATUS_MALFORMED_INPUT` - The input contains an ill-formed code-unit sequence
- `MJB_STATUS_UNSUPPORTED` - The output encoding cannot represent a skeleton codepoint
- `MJB_STATUS_OVERFLOW` - The required output size would overflow
- `MJB_STATUS_NO_MEMORY` - Temporary allocation failed
- `MJB_STATUS_OUTPUT_TOO_SMALL` - The output capacity is smaller than the required byte count

**Example**

```c
const char *input = "h\xD0\xB5llo"; // Cyrillic U+0435 in place of e
size_t output_size = 0;

if(mjb_confusable_skeleton_into(input, strlen(input), MJB_ENC_UTF_8, MJB_ENC_UTF_8,
    NULL, &output_size) != MJB_STATUS_OK) {
    return 1;
}

char output[5];

if(output_size > sizeof(output) || mjb_confusable_skeleton_into(input, strlen(input),
    MJB_ENC_UTF_8, MJB_ENC_UTF_8, output, &output_size) != MJB_STATUS_OK) {
    return 1;
}

// Skeleton payload (no terminator): hello
printf("Skeleton payload (no terminator): %.*s", (int)output_size, output);
```

See also: [`mjb_confusable_skeleton`](#mjb_confusable_skeleton), [`mjb_are_confusable`](#mjb_are_confusable), [`mjb_is_identifier`](#mjb_is_identifier).

Specifications: [UTS #39: Unicode Security Mechanisms, Unicode 18.0.0](https://www.unicode.org/reports/tr39/tr39-33.html).

## `mjb_are_confusable`

Determine whether two strings are visually confusable (Unicode 18.0.0 UTS #39 Section 4): skeleton(s1) == skeleton(s2).

```c
mjb_status mjb_are_confusable(
    const char *s1,
    size_t s1_byte_length,
    mjb_encoding s1_encoding,
    const char *s2,
    size_t s2_byte_length,
    mjb_encoding s2_encoding,
    bool *confusable
);
```

Compute the confusable skeleton of both strings and store true when the skeletons are equal, meaning the two strings are visually confusable, such as "good" and "gооd" with Cyrillic о.

- `s1` - The first string
- `s1_byte_length` - The length of the first string in bytes, or `MJB_NUL_TERMINATED`
- `s1_encoding` - The encoding of the first string
- `s2` - The second string
- `s2_byte_length` - The length of the second string in bytes, or `MJB_NUL_TERMINATED`
- `s2_encoding` - The encoding of the second string
- `confusable` - Whether the strings are visually confusable

**Returns**

- `MJB_STATUS_OK` - `confusable` contains the comparison result
- `MJB_STATUS_INVALID_ARGUMENT` - `confusable` is NULL, or an input buffer is NULL with a non-zero size
- `MJB_STATUS_INVALID_ENCODING` - An input encoding is invalid or lacks byte-order information
- `MJB_STATUS_MALFORMED_INPUT` - An input contains an ill-formed code-unit sequence
- `MJB_STATUS_OVERFLOW` - An intermediate size would overflow
- `MJB_STATUS_NO_MEMORY` - Allocation failed

**Example**

```c
const char *latin = "hello";
const char *mixed = "h\xD0\xB5llo"; // Cyrillic е
bool confusable;

if(mjb_are_confusable(latin, strlen(latin), MJB_ENC_UTF_8,
    mixed, strlen(mixed), MJB_ENC_UTF_8, &confusable) != MJB_STATUS_OK) {
    return 1;
}

// Visually confusable: yes
printf("Visually confusable: %s", confusable ? "yes" : "no");
```

See also: [`mjb_confusable_skeleton`](#mjb_confusable_skeleton), [`mjb_confusable_skeleton_into`](#mjb_confusable_skeleton_into), [`mjb_is_identifier`](#mjb_is_identifier).

Specifications: [UTS #39: Unicode Security Mechanisms, Unicode 18.0.0](https://www.unicode.org/reports/tr39/tr39-33.html).

## `mjb_codepoint_emoji_properties`

Return the emoji properties.

```c
mjb_status mjb_codepoint_emoji_properties(
    mjb_codepoint codepoint,
    mjb_emoji_properties *emoji
);
```

- `codepoint` - The codepoint to check
- `emoji` - The emoji properties to store the result

**Example**

```c
mjb_emoji_properties emoji;

if(mjb_codepoint_emoji_properties(0x1F600, &emoji) != MJB_STATUS_OK) {
    return 1;
}

// U+1F600 has Emoji_Presentation: yes
printf("U+1F600 has Emoji_Presentation: %s", emoji.presentation ? "yes" : "no");
```

See also: [`mjb_classify_emoji_sequence`](#mjb_classify_emoji_sequence), [`mjb_codepoint_is_emoji`](#mjb_codepoint_is_emoji).

Specifications: [UTS #51: Unicode Emoji, Unicode 18.0.0](https://www.unicode.org/reports/tr51/tr51-30.html).

## `mjb_codepoint_is_emoji`

Return true if the codepoint has the Unicode Emoji property.

```c
bool mjb_codepoint_is_emoji(
    mjb_codepoint codepoint
);
```

- `codepoint` - The codepoint to check

**Example**

```c
// Number sign has the Emoji property: yes
bool emoji = mjb_codepoint_is_emoji('#');

// Number sign has the Emoji property: yes
printf("Number sign has the Emoji property: %s", emoji ? "yes" : "no");
```

Specifications: [UTS #51: Unicode Emoji, Unicode 18.0.0](https://www.unicode.org/reports/tr51/tr51-30.html).

## `mjb_codepoint_is_emoji_presentation`

Return true if the codepoint has the Unicode Emoji_Presentation property.

```c
bool mjb_codepoint_is_emoji_presentation(
    mjb_codepoint codepoint
);
```

- `codepoint` - The codepoint to check

**Example**

```c
// Grinning face defaults to emoji presentation: yes
bool presentation = mjb_codepoint_is_emoji_presentation(0x1F600);

// Grinning face defaults to emoji presentation: yes
printf("Grinning face defaults to emoji presentation: %s", presentation ? "yes" : "no");
```

Specifications: [UTS #51: Unicode Emoji, Unicode 18.0.0](https://www.unicode.org/reports/tr51/tr51-30.html).

## `mjb_codepoint_is_emoji_modifier`

Return true if the codepoint has the Unicode Emoji_Modifier property.

```c
bool mjb_codepoint_is_emoji_modifier(
    mjb_codepoint codepoint
);
```

- `codepoint` - The codepoint to check

**Example**

```c
// Medium skin tone is an emoji modifier: yes
bool modifier = mjb_codepoint_is_emoji_modifier(0x1F3FD);

// Medium skin tone is an emoji modifier: yes
printf("Medium skin tone is an emoji modifier: %s", modifier ? "yes" : "no");
```

Specifications: [UTS #51: Unicode Emoji, Unicode 18.0.0](https://www.unicode.org/reports/tr51/tr51-30.html).

## `mjb_codepoint_is_emoji_modifier_base`

Return true if the codepoint has the Unicode Emoji_Modifier_Base property.

```c
bool mjb_codepoint_is_emoji_modifier_base(
    mjb_codepoint codepoint
);
```

- `codepoint` - The codepoint to check

**Example**

```c
// Waving hand accepts an emoji modifier: yes
bool modifier_base = mjb_codepoint_is_emoji_modifier_base(0x1F44B);

// Waving hand accepts an emoji modifier: yes
printf("Waving hand accepts an emoji modifier: %s", modifier_base ? "yes" : "no");
```

Specifications: [UTS #51: Unicode Emoji, Unicode 18.0.0](https://www.unicode.org/reports/tr51/tr51-30.html).

## `mjb_codepoint_is_emoji_component`

Return true if the codepoint has the Unicode Emoji_Component property.

```c
bool mjb_codepoint_is_emoji_component(
    mjb_codepoint codepoint
);
```

- `codepoint` - The codepoint to check

**Example**

```c
// Zero-width joiner is an emoji component: yes
bool component = mjb_codepoint_is_emoji_component(0x200D);

// Zero-width joiner is an emoji component: yes
printf("Zero-width joiner is an emoji component: %s", component ? "yes" : "no");
```

Specifications: [UTS #51: Unicode Emoji, Unicode 18.0.0](https://www.unicode.org/reports/tr51/tr51-30.html).

## `mjb_codepoint_is_extended_pictographic`

Return true if the codepoint has the Unicode Extended_Pictographic property.

```c
bool mjb_codepoint_is_extended_pictographic(
    mjb_codepoint codepoint
);
```

- `codepoint` - The codepoint to check

**Example**

```c
// Red heart is Extended_Pictographic: yes
bool pictographic = mjb_codepoint_is_extended_pictographic(0x2764);

// Red heart is Extended_Pictographic: yes
printf("Red heart is Extended_Pictographic: %s", pictographic ? "yes" : "no");
```

Specifications: [UTS #51: Unicode Emoji, Unicode 18.0.0](https://www.unicode.org/reports/tr51/tr51-30.html).

## `mjb_codepoint_plane`

Return the plane of the codepoint.

```c
mjb_plane mjb_codepoint_plane(
    mjb_codepoint codepoint
);
```

- `codepoint` - The codepoint to check

**Example**

```c
mjb_plane plane = mjb_codepoint_plane(0x1F600);

// U+1F600 is in the SMP: yes
printf("U+1F600 is in the SMP: %s", plane == MJB_PLANE_SMP ? "yes" : "no");
```

## `mjb_plane_is_valid`

Return true if the plane is valid.

```c
bool mjb_plane_is_valid(
    mjb_plane plane
);
```

- `plane` - The plane to check

**Example**

```c
// Plane 16 is valid: yes
printf("Plane 16 is valid: %s", mjb_plane_is_valid(MJB_PLANE_PUA_B) ? "yes" : "no");
```

## `mjb_plane_name`

Return the name of a plane, NULL if the plane specified is not valid.

```c
const char *mjb_plane_name(
    mjb_plane plane,
    bool abbreviation
);
```

- `plane` - The plane to check
- `abbreviation` - Whether to use an abbreviation

**Example**

```c
// Plane: Basic Multilingual Plane
printf("Plane: %s", mjb_plane_name(MJB_PLANE_BMP, false));
```

## `mjb_classify_emoji_sequence`

Return emoji sequence metadata for a complete string.

```c
mjb_status mjb_classify_emoji_sequence(
    const char *buffer,
    size_t byte_length,
    mjb_encoding encoding,
    mjb_emoji_sequence *emoji
);
```

- `buffer` - The string to check
- `byte_length` - The length of the string in bytes, or `MJB_NUL_TERMINATED` to determine it from an encoding-aware NUL code unit
- `encoding` - The encoding of the string
- `emoji` - The emoji sequence metadata to store the result

**Example**

```c
const char *flag = "\xF0\x9F\x87\xAE\xF0\x9F\x87\xB9"; // 🇮🇹
mjb_emoji_sequence emoji;

if(mjb_classify_emoji_sequence(flag, strlen(flag), MJB_ENC_UTF_8,
    &emoji) != MJB_STATUS_OK) {
    return 1;
}

// Sequence codepoints: 2
printf("Sequence codepoints: %zu", emoji.codepoint_count);
```

See also: [`mjb_is_emoji_sequence`](#mjb_is_emoji_sequence), [`mjb_is_rgi_emoji`](#mjb_is_rgi_emoji).

Specifications: [UTS #51: Unicode Emoji, Unicode 18.0.0](https://www.unicode.org/reports/tr51/tr51-30.html).

## `mjb_is_emoji_sequence`

Return true if the complete string is an emoji sequence listed by Unicode, including standardized emoji variation sequences.

```c
bool mjb_is_emoji_sequence(
    const char *buffer,
    size_t byte_length,
    mjb_encoding encoding
);
```

- `buffer` - The string to check
- `byte_length` - The length of the string in bytes, or `MJB_NUL_TERMINATED` to determine it from an encoding-aware NUL code unit
- `encoding` - The encoding of the string

**Example**

```c
const char *keycap = "1\xEF\xB8\x8F\xE2\x83\xA3"; // 1️⃣

bool listed = mjb_is_emoji_sequence(keycap, strlen(keycap), MJB_ENC_UTF_8);

// Listed emoji sequence: yes
printf("Listed emoji sequence: %s", listed ? "yes" : "no");
```

See also: [`mjb_is_rgi_emoji`](#mjb_is_rgi_emoji), [`mjb_classify_emoji_sequence`](#mjb_classify_emoji_sequence).

Specifications: [UTS #51: Unicode Emoji, Unicode 18.0.0](https://www.unicode.org/reports/tr51/tr51-30.html).

## `mjb_is_rgi_emoji`

Return true if the complete string is an RGI emoji sequence, excluding plain standardized variation sequences.

```c
bool mjb_is_rgi_emoji(
    const char *buffer,
    size_t byte_length,
    mjb_encoding encoding
);
```

- `buffer` - The string to check
- `byte_length` - The length of the string in bytes, or `MJB_NUL_TERMINATED` to determine it from an encoding-aware NUL code unit
- `encoding` - The encoding of the string

**Example**

```c
const char *flag = "\xF0\x9F\x87\xAE\xF0\x9F\x87\xB9"; // 🇮🇹

bool rgi = mjb_is_rgi_emoji(flag, strlen(flag), MJB_ENC_UTF_8);

// RGI emoji: yes
printf("RGI emoji: %s", rgi ? "yes" : "no");
```

See also: [`mjb_is_emoji_sequence`](#mjb_is_emoji_sequence), [`mjb_classify_emoji_sequence`](#mjb_classify_emoji_sequence).

Specifications: [UTS #51: Unicode Emoji, Unicode 18.0.0](https://www.unicode.org/reports/tr51/tr51-30.html).

## `mjb_hangul_syllable_name`

Return hangul syllable name.

```c
mjb_status mjb_hangul_syllable_name(
    mjb_codepoint codepoint,
    char *buffer,
    size_t byte_length
);
```

- `codepoint` - The codepoint to check
- `buffer` - The buffer to store the result
- `byte_length` - The capacity of the output buffer, in bytes

**Example**

```c
char name[32];

if(mjb_hangul_syllable_name(0xAC01, name, sizeof(name)) != MJB_STATUS_OK) {
    return 1;
}

// Name: HANGUL SYLLABLE GAG
printf("Name: %s", name);
```

## `mjb_hangul_syllable_decomposition`

Hangul syllable decomposition.

```c
mjb_status mjb_hangul_syllable_decomposition(
    mjb_codepoint codepoint,
    mjb_codepoint *codepoints
);
```

- `codepoint` - The codepoint to check
- `codepoints` - The codepoints to store the result

**Example**

```c
mjb_codepoint decomposition[3];

if(mjb_hangul_syllable_decomposition(0xAC01,
    decomposition) != MJB_STATUS_OK) {
    return 1;
}

// Decomposition starts with: U+1100
printf("Decomposition starts with: U+%04X", decomposition[0]);
```

## `mjb_hangul_syllable_composition`

Hangul syllable composition.

```c
size_t mjb_hangul_syllable_composition(
    mjb_buffer_character *characters,
    size_t characters_len
);
```

- `characters` - The characters to compose
- `characters_len` - The length of the characters

**Example**

```c
mjb_buffer_character characters[] = {
    { 0x1100, 0 }, // choseong kiyeok
    { 0x1161, 0 }, // jungseong a
    { 0x11A8, 0 }  // jongseong kiyeok
};
size_t length = mjb_hangul_syllable_composition(characters, 3);

// Composition: U+AC01
printf("Composition: U+%04X", length == 1 ? characters[0].codepoint : 0);
```

## `mjb_codepoint_east_asian_width`

Return the east asian width of a codepoint.

```c
mjb_status mjb_codepoint_east_asian_width(
    mjb_codepoint codepoint,
    mjb_east_asian_width *width
);
```

- `codepoint` - The codepoint to check
- `width` - The width to store the result

**Example**

```c
mjb_east_asian_width width;

if(mjb_codepoint_east_asian_width(0x754C, &width) != MJB_STATUS_OK) { // 界
    return 1;
}

// U+754C is wide: yes
printf("U+754C is wide: %s", width == MJB_EAW_WIDE ? "yes" : "no");
```

See also: [`mjb_display_width`](#mjb_display_width).

Specifications: [UAX #11: East Asian Width, Unicode 18.0.0](https://www.unicode.org/reports/tr11/tr11-45.html).

## `mjb_display_width`

Return the display width of a string.

```c
mjb_status mjb_display_width(
    const char *buffer,
    size_t byte_length,
    mjb_encoding encoding,
    mjb_width_context context,
    size_t *width
);
```

Compute the number of display columns a string occupies in a terminal, accounting for wide and ambiguous East Asian characters, combining marks, and emoji sequences.

- `buffer` - The string to normalize
- `byte_length` - The length of the string in bytes, or `MJB_NUL_TERMINATED` to determine it from an encoding-aware NUL code unit
- `encoding` - The encoding of the string
- `context` - The width context for ambiguous-width characters
- `width` - The width to store the result

**Returns**

- `MJB_STATUS_OK` - The width was computed
- `MJB_STATUS_INVALID_ARGUMENT` - `width` is NULL, or `buffer` is NULL with a non-zero size
- `MJB_STATUS_OVERFLOW` - The width would overflow

**Example**

```c
const char *input = "A\xE7\x95\x8C"; // A界
size_t width;

if(mjb_display_width(input, strlen(input), MJB_ENC_UTF_8,
    MJB_WIDTH_CONTEXT_WESTERN, &width) != MJB_STATUS_OK) {
    return 1;
}

// Display columns: 3
printf("Display columns: %zu", width);
```

See also: [`mjb_codepoint_east_asian_width`](#mjb_codepoint_east_asian_width), [`mjb_truncate_grapheme_width`](#mjb_truncate_grapheme_width).

Specifications: [UAX #11: East Asian Width, Unicode 18.0.0](https://www.unicode.org/reports/tr11/tr11-45.html).

## `mjb_locale_parse`

Parse a BCP 47 language tag.

```c
mjb_status mjb_locale_parse(
    const char *id,
    size_t byte_length,
    mjb_encoding encoding,
    mjb_locale_id *locale,
    mjb_error *error
);
```

Parse a BCP 47 language tag, such as `sr-Latn-RS`, into its components: language, extended language, script, region, variant, extensions, private use, and grandfathered tags. Parsing is strict: malformed tags are rejected and `error` is filled with the failure reason.

- `id` - The BCP 47 language tag to parse
- `byte_length` - The length of the locale identifier in bytes, or `MJB_NUL_TERMINATED`
- `encoding` - The encoding of the locale identifier
- `locale` - The locale structure to store the result
- `error` - The error to store when parsing fails

**Returns**

- `MJB_STATUS_OK` - The tag was parsed and `locale` filled
- `MJB_STATUS_INVALID_ARGUMENT` - An argument is NULL or the tag is not a valid BCP 47 language tag
- `MJB_STATUS_NO_MEMORY` - Allocation failed

**Example**

```c
mjb_locale_id locale;
mjb_error error;

if(mjb_locale_parse("sr-Latn-RS", 10, MJB_ENC_UTF_8, &locale,
    &error) != MJB_STATUS_OK) {
    return 1;
}

// Locale: sr Latn RS
printf("Locale: %s %s %s", locale.language, locale.script, locale.region);
```

See also: [`mjb_set_locale`](#mjb_set_locale).

Specifications: [BCP 47: Tags for Identifying Languages](https://www.rfc-editor.org/rfc/rfc5646).

## `mjb_set_locale`

Set the current process-global locale.

```c
mjb_status mjb_set_locale(
    mjb_locale locale
);
```

Set the process-global locale used by `mjb_map_case`. The default locale is `MJB_LOCALE_EN`, and `mjb_reset` resets it to `MJB_LOCALE_EN`. Only `MJB_LOCALE_TR`, `MJB_LOCALE_AZ`, and `MJB_LOCALE_LT` currently tailor casing. Other valid locale values are accepted but do not change Unicode algorithm behavior.

- `locale` - The locale to set

**Returns**

- `MJB_STATUS_OK` - The locale was set
- `MJB_STATUS_INVALID_ARGUMENT` - `locale` is not a valid `mjb_locale` value

**Example**

```c
if(mjb_set_locale(MJB_LOCALE_TR) != MJB_STATUS_OK) {
    return 1;
}

// Turkish locale selected: yes
printf("Turkish locale selected: yes");
if(mjb_set_locale(MJB_LOCALE_EN) != MJB_STATUS_OK) {
    return 1;
}
```

See also: [`mjb_get_locale`](#mjb_get_locale), [`mjb_map_case`](#mjb_map_case), [`mjb_map_case_into`](#mjb_map_case_into).

## `mjb_get_locale`

Return the current process-global locale.

```c
mjb_locale mjb_get_locale(void);
```

Return the process-global locale selected with `mjb_set_locale`. The default is `MJB_LOCALE_EN`, and `mjb_reset` restores that default.

**Returns**

- `mjb_locale` - The currently selected locale

**Example**

```c
mjb_locale locale = mjb_get_locale();

// Current locale is English: yes
printf("Current locale is English: %s", locale == MJB_LOCALE_EN ? "yes" : "no");
```

See also: [`mjb_set_locale`](#mjb_set_locale), [`mjb_map_case`](#mjb_map_case), [`mjb_map_case_into`](#mjb_map_case_into).

## `mjb_result_free`

Free a mjb_result.

```c
mjb_status mjb_result_free(
    mjb_result *result
);
```

Free the memory allocated for a `mjb_result`. The `result` pointer is set to NULL.

- `result` - The result to free

**Returns**

- `MJB_STATUS_OK` - The result was freed
- `MJB_STATUS_INVALID_ARGUMENT` - `result` is NULL

**Example**

```c
mjb_result result;

if(mjb_convert_encoding("A", 1, MJB_ENC_UTF_8, MJB_ENC_UTF_16LE,
    &result) != MJB_STATUS_OK || mjb_result_free(&result) != MJB_STATUS_OK) {
    return 1;
}

// Result released: yes
printf("Result released: %s", result.output == NULL ? "yes" : "no");
```

## `mjb_utf8_snprintf`

Format a UTF-8 string without leaving an incomplete trailing codepoint.

```c
int mjb_utf8_snprintf(
    char *buffer,
    size_t buffer_size,
    const char *format,
    ...
);
```

Use the C library formatting rules and return semantics of `snprintf`. If the destination buffer truncates a well-formed UTF-8 result, any incomplete trailing codepoint is removed before the terminating NULL. Truncation is at a codepoint boundary, not a grapheme-cluster boundary.

- `buffer` - The destination buffer, or NULL when buffer_size is zero
- `buffer_size` - The destination buffer capacity in bytes, including the terminating NULL
- `format` - The printf format string

**Returns**

- `A nonnegative value` - The number of bytes the complete result requires, excluding the terminating NULL
- `A negative value` - The underlying `vsnprintf` reported an encoding error

**Example**

```c
char buffer[4];
int required = mjb_utf8_snprintf(buffer, sizeof(buffer), "%s",
    "\xC3\xA9\xC3\xA9"); // éé

// 4: é
printf("%d: %s", required, buffer);
```

See also: [`mjb_utf8_vsnprintf`](#mjb_utf8_vsnprintf), [`mjb_utf8_grapheme_snprintf`](#mjb_utf8_grapheme_snprintf), [`mjb_is_utf8`](#mjb_is_utf8).

## `mjb_utf8_vsnprintf`

Format a UTF-8 string from a va_list without leaving an incomplete trailing codepoint.

```c
int mjb_utf8_vsnprintf(
    char *buffer,
    size_t buffer_size,
    const char *format,
    va_list args
);
```

The `va_list` counterpart of `mjb_utf8_snprintf`. It has the same UTF-8 input requirements, clipping behavior, and return semantics as `mjb_utf8_snprintf`.

- `buffer` - The destination buffer, or NULL when buffer_size is zero
- `buffer_size` - The destination buffer capacity in bytes, including the terminating NULL
- `format` - The printf format string
- `args` - The formatting arguments

**Returns**

- `A nonnegative value` - The number of bytes the complete result requires, excluding the terminating NULL
- `A negative value` - The underlying `vsnprintf` reported an encoding error

See also: [`mjb_utf8_snprintf`](#mjb_utf8_snprintf), [`mjb_utf8_grapheme_vsnprintf`](#mjb_utf8_grapheme_vsnprintf), [`mjb_is_utf8`](#mjb_is_utf8).

## `mjb_utf8_grapheme_snprintf`

Format UTF-8 without truncating an extended grapheme cluster.

```c
int mjb_utf8_grapheme_snprintf(
    char *buffer,
    size_t buffer_size,
    const char *format,
    ...
);
```

Use the C library formatting rules and return semantics of `snprintf`. If the destination buffer truncates a well-formed UTF-8 result, the output is shortened to the largest prefix that ends at an extended grapheme-cluster boundary in the complete result. Unlike `mjb_utf8_snprintf`, truncation can require temporary allocation and a second evaluation of the format. Do not use `%n` or arguments whose values can change as a formatting side effect. On allocation failure the destination is set to an empty string, the function returns a negative value, and `errno` is set to `ENOMEM`.

- `buffer` - The destination buffer, or NULL when buffer_size is zero
- `buffer_size` - The destination buffer capacity in bytes, including the terminating NULL
- `format` - The printf format string

**Returns**

- `A nonnegative value` - The number of bytes the complete result requires, excluding the terminating NULL
- `A negative value` - Formatting failed or the complete result could not be obtained

**Example**

```c
char buffer[4];
int required = mjb_utf8_grapheme_snprintf(buffer, sizeof(buffer), "%s",
    "Ae\xCC\x81" "B"); // A, e + combining acute accent, B

// 5: A
printf("%d: %s", required, buffer);
```

See also: [`mjb_utf8_grapheme_vsnprintf`](#mjb_utf8_grapheme_vsnprintf), [`mjb_utf8_snprintf`](#mjb_utf8_snprintf), [`mjb_truncate_grapheme`](#mjb_truncate_grapheme).

Specifications: [UAX #29: Unicode Text Segmentation, Unicode 18.0.0](https://www.unicode.org/reports/tr29/tr29-48.html).

## `mjb_utf8_grapheme_vsnprintf`

Format UTF-8 from a va_list without truncating an extended grapheme cluster.

```c
int mjb_utf8_grapheme_vsnprintf(
    char *buffer,
    size_t buffer_size,
    const char *format,
    va_list args
);
```

The `va_list` counterpart of `mjb_utf8_grapheme_snprintf`. It has the same grapheme-safe clipping behavior, allocation requirements, and return semantics as `mjb_utf8_grapheme_snprintf`.

- `buffer` - The destination buffer, or NULL when buffer_size is zero
- `buffer_size` - The destination buffer capacity in bytes, including the terminating NULL
- `format` - The printf format string
- `args` - The formatting arguments

**Returns**

- `A nonnegative value` - The number of bytes the complete result requires, excluding the terminating NULL
- `A negative value` - Formatting failed or the complete result could not be obtained

See also: [`mjb_utf8_grapheme_snprintf`](#mjb_utf8_grapheme_snprintf), [`mjb_utf8_vsnprintf`](#mjb_utf8_vsnprintf), [`mjb_truncate_grapheme`](#mjb_truncate_grapheme).

Specifications: [UAX #29: Unicode Text Segmentation, Unicode 18.0.0](https://www.unicode.org/reports/tr29/tr29-48.html).

## `mjb_version`

Output the current library version (MJB_VERSION).

```c
const char *mjb_version(void);
```

Output the current library version as a string, such as "1.0.0".

**Example**

```c
const char *version = mjb_version();

// Version is available: yes
printf("Version is available: %s", version[0] != '\0' ? "yes" : "no");
```

See also: [`mjb_version_number`](#mjb_version_number), [`mjb_unicode_version`](#mjb_unicode_version).

## `mjb_version_number`

Output the current library version number (MJB_VERSION_NUMBER).

```c
unsigned int mjb_version_number(void);
```

Output the current library version number as an unsigned integer.

**Example**

```c
unsigned int version = mjb_version_number();

// Version number is positive: yes
printf("Version number is positive: %s", version > 0 ? "yes" : "no");
```

See also: [`mjb_version`](#mjb_version), [`mjb_unicode_version`](#mjb_unicode_version).

## `mjb_unicode_version`

Output the current supported Unicode version (MJB_UNICODE_VERSION).

```c
const char *mjb_unicode_version(void);
```

Output the current supported Unicode version as a string, such as "15.0.0".

**Example**

```c
const char *version = mjb_unicode_version();

// Unicode version: 18.0.0
printf("Unicode version: %s", version);
```

See also: [`mjb_version`](#mjb_version), [`mjb_version_number`](#mjb_version_number).

## `mjb_set_memory_functions`

Set the library memory functions.

```c
mjb_status mjb_set_memory_functions(
    mjb_alloc_fn alloc_fn,
    mjb_realloc_fn realloc_fn,
    mjb_free_fn free_fn
);
```

Replace the allocator used by the library for all internal allocations and for the buffers returned in `mjb_result`. Must be called before any other library call.

- `alloc_fn` - The function to allocate memory
- `realloc_fn` - The function to reallocate memory
- `free_fn` - The function to free memory

**Example**

```c
mjb_reset(); // Ensure no allocator is currently locked in.

if(mjb_set_memory_functions(malloc, realloc, free) != MJB_STATUS_OK) {
    return 1;
}

// Standard allocator installed: yes
printf("Standard allocator installed: yes");
mjb_reset();
```

See also: [`mjb_alloc`](#mjb_alloc), [`mjb_realloc`](#mjb_realloc), [`mjb_free`](#mjb_free).

## `mjb_reset`

Reset the library. Not needed to be called.

```c
void mjb_reset(void);
```

**Example**

```c
mjb_reset();

// Library state reset: yes
printf("Library state reset: yes");
```

## `mjb_alloc`

Allocate memory.

```c
void *mjb_alloc(
    size_t byte_length
);
```

Allocate memory using the allocator set by `mjb_set_memory_functions`. If no allocator is set, the default allocator is used.

- `byte_length` - The length of the memory to allocate

**Example**

```c
char *buffer = (char*)mjb_alloc(sizeof("allocated"));

if(buffer == NULL) {
    return 1;
}

memcpy(buffer, "allocated", sizeof("allocated"));

// Buffer: allocated
printf("Buffer: %s", buffer);
mjb_free(buffer);
```

See also: [`mjb_realloc`](#mjb_realloc), [`mjb_free`](#mjb_free).

## `mjb_realloc`

Reallocate memory.

```c
void *mjb_realloc(
    void *ptr,
    size_t new_size
);
```

Reallocate memory using the allocator set by `mjb_set_memory_functions`. If no allocator is set, the default allocator is used.

- `ptr` - The pointer to reallocate
- `new_size` - The new size of the memory

**Example**

```c
char *buffer = (char*)mjb_alloc(8);

if(buffer == NULL) {
    return 1;
}

char *larger = (char*)mjb_realloc(buffer, 32);

if(larger == NULL) {
    mjb_free(buffer);
    return 1;
}

// Reallocation succeeded: yes
printf("Reallocation succeeded: yes");
mjb_free(larger);
```

See also: [`mjb_alloc`](#mjb_alloc), [`mjb_free`](#mjb_free).

## `mjb_free`

Free memory.

```c
void mjb_free(
    void *ptr
);
```

Free memory using the allocator set by `mjb_set_memory_functions`. If no allocator is set, the default allocator is used.

- `ptr` - The pointer to free

**Example**

```c
void *memory = mjb_alloc(16);

if(memory == NULL) {
    return 1;
}

mjb_free(memory);

// Memory freed: yes
printf("Memory freed: yes");
```

See also: [`mjb_alloc`](#mjb_alloc), [`mjb_realloc`](#mjb_realloc).

# Unicode references

Mojibake targets [The Unicode Standard, Version 18.0.0](https://www.unicode.org/versions/Unicode18.0.0/)
and the [Unicode Character Database 18.0.0](https://www.unicode.org/Public/18.0.0/). Function-level
Unicode specification links below point to the archived Unicode 18.0.0 version of the applicable
Unicode Standard Annex or synchronized Unicode Technical Standard. Generic Unicode links, when
present, are informational or download links rather than normative conformance references.

# Unicode tailoring

Unless a function documents a tailoring, it uses the referenced Unicode 18.0.0 algorithm
without higher-level protocol tailoring.

- `mjb_map_case` is locale-sensitive through the process-global locale set by `mjb_set_locale`. The
  default locale is `MJB_LOCALE_EN`. Turkish and Azerbaijani apply dotted-I casing rules and Turkic
  case-folding mappings. Lithuanian applies dot-above casing rules; case folding remains the default
  non-Turkic mapping.
- `mjb_collation_compare` and `mjb_collation_key` use DUCET without locale collation tailoring.
  `mjb_collation_mode` only selects the UCA variable weighting strategy.
- `mjb_display_width` uses its `mjb_width_context` argument to choose how East Asian Width
  `Ambiguous` characters are counted. `mjb_codepoint_east_asian_width` returns the Unicode 18.0.0
  property value without tailoring.
- Normalization, NFKC case folding, bidirectional processing, grapheme/word/sentence/line breaking,
  identifier validation, confusable skeletons, and emoji sequence checks are not locale-tailored by
  Mojibake.

# Unicode conformance inventory

Mojibake interprets Unicode text only through the public APIs and supported UTF encodings listed in
this documentation. It does not implement rendering, font shaping, locale collation tailoring, or
higher-level protocol behavior beyond the documented locale-sensitive casing and display-width
policy. The table below maps the advertised Unicode algorithm and data claims to their Unicode
18.0.0 reference and test evidence.

| Claim | Public surface | Unicode reference | Evidence |
| ----- | -------------- | ----------------- | -------- |
| Unicode Character Database data and derived properties | `mjb_codepoint_info`, `mjb_codepoint_property_binary`, `mjb_codepoint_property_int`, `mjb_codepoint_script_extensions`, script/block/category/numeric helpers | [UAX #44](https://www.unicode.org/reports/tr44/tr44-36.html), [UAX #24](https://www.unicode.org/reports/tr24/tr24-40.html), UCD 18.0.0 | Generated from UCD data files including `UnicodeData.txt`, `Blocks.txt`, `Scripts.txt`, `ScriptExtensions.txt`, `PropList.txt`, `DerivedCoreProperties.txt`, `PropertyAliases.txt`, and `PropertyValueAliases.txt`; every explicit Script_Extensions range is covered by `tests/properties.c`. |
| Unicode Normalization Forms and quick check | `mjb_normalize`, `mjb_normalization_quick_check` | [UAX #15](https://www.unicode.org/reports/tr15/tr15-57.html) | `NormalizationTest.txt`, `DerivedNormalizationProps.txt`, `tests/normalization.c`, and `tests/quick-check.c`. |
| Default case conversion and caseless matching | `mjb_map_case`, `mjb_nfkc_casefold`, simple codepoint case helpers | [Unicode Core Section 3.13](https://www.unicode.org/versions/Unicode18.0.0/core-spec/chapter-3/#G33992), [UAX #29](https://www.unicode.org/reports/tr29/tr29-48.html) for titlecase word boundaries, [UAX #31](https://www.unicode.org/reports/tr31/tr31-44.html) for identifier caseless matching | `SpecialCasing.txt`, `CaseFolding.txt`, `WordBreakTest.txt`, every explicit `NFKC_CF` mapping in `DerivedNormalizationProps.txt`, `tests/special-case.c`, `tests/case.c`, `tests/normalization.c`, and `tests/break-word.c`. |
| Grapheme, word, and sentence boundaries | `mjb_next_grapheme_break`, `mjb_next_word_break`, `mjb_next_sentence_break`, related truncation helpers | [UAX #29](https://www.unicode.org/reports/tr29/tr29-48.html) | `GraphemeBreakTest.txt`, `WordBreakTest.txt`, `SentenceBreakTest.txt`, `tests/segmentation.c`, `tests/break-word.c`, and `tests/break-sentence.c`. |
| Line breaking | `mjb_next_line_break` | [UAX #14](https://www.unicode.org/reports/tr14/tr14-56.html) | `LineBreakTest.txt` and `tests/break-line.c`. |
| Bidirectional Algorithm | `mjb_bidi_resolve`, `mjb_bidi_reorder_line`, `mjb_bidi_line_runs` | [UAX #9](https://www.unicode.org/reports/tr9/tr9-51.html) | `BidiCharacterTest.txt`, `BidiTest.txt`, `tests/bidi.c`, and `tests/bidi-class.c`. |
| Unicode Collation Algorithm, DUCET | `mjb_collation_compare`, `mjb_collation_key` | [UTS #10](https://www.unicode.org/reports/tr10/tr10-54.html) | `CollationTest_NON_IGNORABLE.txt`, `CollationTest_SHIFTED.txt`, and `tests/collation.c`; surrogate-code-point rows are filtered because public string input rejects ill-formed surrogate code points. |
| Unicode identifiers and pattern syntax data | ID/XID/pattern predicates and `mjb_is_identifier` | [UAX #31](https://www.unicode.org/reports/tr31/tr31-44.html) | UCD ID/XID and pattern properties from `DerivedCoreProperties.txt` and `PropList.txt`; covered by `tests/identifier.c`. |
| Confusable skeleton generation and matching | `mjb_confusable_skeleton`, `mjb_are_confusable` | [UTS #39](https://www.unicode.org/reports/tr39/tr39-33.html) | Every mapping in `confusables.txt`, every pair in `intentional.txt`, and `tests/security.c`. |
| Emoji properties and sequence data | Emoji property predicates, `mjb_classify_emoji_sequence`, RGI checks | [UTS #51](https://www.unicode.org/reports/tr51/tr51-30.html) | `emoji-data.txt`, `emoji-sequences.txt`, `emoji-zwj-sequences.txt`, `emoji-variation-sequences.txt`, `emoji-test.txt`, and `tests/emoji.c`. |
| East Asian Width property | `mjb_codepoint_east_asian_width`; consumed by `mjb_display_width` | [UAX #11](https://www.unicode.org/reports/tr11/tr11-45.html) | `EastAsianWidth.txt`, `tests/east-asian-width.c`, and property tests; display column counts are a documented local policy over that property. |
