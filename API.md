# API

You can find a demo site where you can find the API documentation and test the functions by using
WASM here: [https://mojibake.zaerl.com](https://mojibake.zaerl.com)

Here is the basis for using the library:

1. Mojibake does not have a default input encoding or output decoding; you must decide what to use
2. Input and output encodings can be different
3. Every string passed is simply a stream of bytes, and you must specify how many bytes there are
4. For safety reasons, the functions stop when they encounter a `\0` byte in the input strings even
if the `length` is bigger. This is _unless_ you declare `MJB_DANGEROUSLY_ALLOW_EMBEDDED_NULLS` or
you use UTF-16/UTF-32 that can have `\0` bytes
5. The major part of the functions return a `mjb_status` and should be checked against the
`MJB_STATUS_OK` constant. The `MJB_STATUS_OK` enum value is `0 (zero)` so don't check for truthy
6. Predicate APIs, such as `mjb_string_is_utf8` and `mjb_codepoint_is_valid`, return `bool` because
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

See for example the [`mjb_normalize`](#mjb_normalize), [`mjb_string_filter`](#mjb_string_filter)
functions.

### Functions that handle a codepoint

The functions return a `mjb_status` and accept these arguments:

1. The codepoint to check
2. The needed _arguments_ of the function, if any
3. A pointer to a structure to save the result

See for example [`mjb_codepoint_character`](#mjb_codepoint_character),
[`mjb_codepoint_numeric_value`](#mjb_codepoint_numeric_value).

### Predicate functions

Those are the `mjb_(something)_is_(this)` kind of functions, which return a `bool`.

1. The thing to check
2. The needed _arguments_ of the function, if any

See for example [`mjb_string_is_utf8`](#mjb_string_is_utf8),
[`mjb_codepoint_is_valid`](#mjb_codepoint_is_valid).

## Strings encoding and generation

Mojibake is encoding agnostic. It can accept and output `uint8_t` (ASCII, UTF-8),
`uint16_t` (UTF-16), `uint32_t` (UTF-32) bytes of memory. The output strings can have different
encodings of the input strings.

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

If you want to normalize the UTF-8 encoded `Cafe\xCC\x81` string to `NFC` this is what you need to
do:

```c
const char *input = "Cafe\xCC\x81"; // "Cafe" + U+0301 COMBINING ACUTE ACCENT

mjb_result result;

if(mjb_normalize(input, strlen(input), MJB_ENC_UTF_8, MJB_NORMALIZATION_NFC, MJB_ENC_UTF_8,
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

To find the real number of characters, you will later use `mjb_string_length`.

```c
mjb_string_length("H\xC3\xA9ll\xC3\xB6", 7, MJB_ENC_UTF_8); // 5 characters
mjb_string_length("H\0\xE9\0l\0l\0\xF6\0", 10, MJB_ENC_UTF_16LE); // 5 characters
mjb_string_length("\0H\0\xE9\0l\0l\0\xF6", 10, MJB_ENC_UTF_16BE); // 5 characters
mjb_string_length("H\0\0\0\xE9\0\0\0l\0\0\0l\0\0\0\xF6\0\0\0", 20, MJB_ENC_UTF_32LE); // 5 characters
mjb_string_length("\0\0\0H\0\0\0\xE9\0\0\0l\0\0\0l\0\0\0\xF6", 20, MJB_ENC_UTF_32BE); // 5 characters
```

# Functions

## `mjb_codepoint_character`

Return the codepoint character.

```c
mjb_status mjb_codepoint_character(
    mjb_codepoint codepoint,
    mjb_character *character
);
```

Fill `character` with the Unicode Character Database record of a codepoint: name, category, combining class, bidirectional category, decomposition, numeric values, mirrored flag, and simple case mappings. When the library is compiled with `MJB_FEATURE_CHARACTER_NAMES=OFF` the name field is reported as `Codepoint U+XXXX`.

- `codepoint` — The codepoint to check
- `character` — The character to store the result

**Returns**

- `MJB_STATUS_OK` — The character was found and filled
- `MJB_STATUS_INVALID_ARGUMENT` — `character` is NULL or the codepoint is not valid
- `MJB_STATUS_NOT_FOUND` — The codepoint is not assigned

**Example**

```c
mjb_character character;

if(mjb_codepoint_character(0x022A, &character) != MJB_STATUS_OK) {
    return 1;
}

// U+022A lowercase: U+022B
printf("U+%04X lowercase: U+%04X", character.codepoint, character.lowercase);
```

See also: [`mjb_codepoint_block`](#mjb_codepoint_block), [`mjb_codepoint_script`](#mjb_codepoint_script), [`mjb_codepoint_property_binary`](#mjb_codepoint_property_binary), [`mjb_codepoint_property_int`](#mjb_codepoint_property_int).

Specifications: [UAX #44: Unicode Character Database, Unicode 17.0.0](https://www.unicode.org/reports/tr44/tr44-36.html).

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

- `buffer` — The string to normalize
- `byte_length` — The length of the string, in bytes
- `encoding` — The encoding of the string
- `form` — The normalization form to use
- `output_encoding` — The output encoding of the string
- `result` — The pointer to store the result. If `result->transformed` is true, `result->output` is library-allocated and must be freed with `mjb_result_free(result)`

**Returns**

- `MJB_STATUS_OK` — The string was normalized (or already normal)
- `MJB_STATUS_INVALID_ARGUMENT` — `result` is NULL, or `buffer` is NULL with a non-zero size
- `MJB_STATUS_INVALID_FORM` — `form` is not NFC, NFD, NFKC, or NFKD
- `MJB_STATUS_OVERFLOW` — The output size would overflow
- `MJB_STATUS_NO_MEMORY` — Allocation failed

**Example**

```c
const char *input = "Cafe\xCC\x81"; // "Cafe" + U+0301 COMBINING ACUTE ACCENT
mjb_result result;

if(mjb_normalize(input, strlen(input), MJB_ENC_UTF_8, MJB_NORMALIZATION_NFC, MJB_ENC_UTF_8,
    &result) != MJB_STATUS_OK) {
    return 1;
}

// NFC: Café
printf("NFC: %.*s", (int)result.output_size, result.output);

if(result.transformed) {
    mjb_free(result.output);
}
```

See also: [`mjb_string_is_normalized`](#mjb_string_is_normalized), [`mjb_string_filter`](#mjb_string_filter).

Specifications: [UAX #15: Unicode Normalization Forms, Unicode 17.0.0](https://www.unicode.org/reports/tr15/tr15-57.html).

## `mjb_string_filter`

Filter a string with the selected mjb_filter flags.

```c
mjb_status mjb_string_filter(
    const char *buffer,
    size_t byte_length,
    mjb_encoding encoding,
    mjb_filter filters,
    mjb_encoding output_encoding,
    mjb_result *result
);
```

`MJB_FILTER_LIMIT_COMBINING` removes combining marks after the first `MJB_FILTER_MAX_COMBINING_MARKS` consecutive marks in an emitted run. This is useful for reducing Zalgo-style text while keeping ordinary accents and stacked marks.

- `buffer` — The string to filter
- `byte_length` — The length of the string, in bytes
- `encoding` — The encoding of the string
- `filters` — The filters to use
- `output_encoding` — The output encoding of the string
- `result` — The pointer to store the result. If `result->transformed` is true, `result->output` is library-allocated and must be freed with `mjb_result_free(result)`

**Example**

```c
const char *mixed_whitespace = "Hello\t\t\n\nworld";
mjb_result result;

if(mjb_string_filter(mixed_whitespace, strlen(mixed_whitespace), MJB_ENC_UTF_8,
    MJB_FILTER_COLLAPSE_SPACES, MJB_ENC_UTF_8, &result) != MJB_STATUS_OK) {
    return 1;
}

// Filtered: Hello world
printf("Filtered: %.*s", (int)result.output_size, result.output);

if(result.transformed) {
    mjb_free(result.output);
}

const char *controls = "\x1\x2\t\n\v\f\r\x1f";

if(mjb_string_filter(controls, strlen(controls), MJB_ENC_UTF_8, MJB_FILTER_CONTROLS,
    MJB_ENC_UTF_8, &result) != MJB_STATUS_OK) {
    return 1;
}

// Filtered: \t\n\v\f\r
printf("Filtered: %.*s", (int)result.output_size, result.output);

if(result.transformed) {
    mjb_free(result.output);
}
```

See also: [`mjb_normalize`](#mjb_normalize).

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

- `buffer` — The string to transform
- `byte_length` — The length of the string, in bytes
- `encoding` — The encoding of the string
- `output_encoding` — The output encoding of the string
- `result` — The pointer to store the result. If `result->transformed` is true, `result->output` is library-allocated and must be freed with `mjb_result_free(result)`

**Returns**

- `MJB_STATUS_OK` — The transformed string was returned
- `MJB_STATUS_INVALID_ARGUMENT` — `result` is NULL, or `buffer` is NULL with a non-zero size
- `MJB_STATUS_OVERFLOW` — The output size would overflow
- `MJB_STATUS_NO_MEMORY` — Allocation failed

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

See also: [`mjb_normalize`](#mjb_normalize), [`mjb_case`](#mjb_case), [`mjb_string_is_identifier`](#mjb_string_is_identifier).

Specifications: [The Unicode Standard, Version 17.0.0, Section 3.13: Default Case Algorithms](https://www.unicode.org/versions/Unicode17.0.0/core-spec/chapter-3/#G33992), [UAX #31: Unicode Identifiers and Syntax, Unicode 17.0.0](https://www.unicode.org/reports/tr31/tr31-43.html), [UAX #44: Unicode Character Database, Unicode 17.0.0](https://www.unicode.org/reports/tr44/tr44-36.html).

## `mjb_string_is_normalized`

Check if a string is normalized to NFC/NFKC/NFD/NFKD form.

```c
mjb_quick_check_result mjb_string_is_normalized(
    const char *buffer,
    size_t byte_length,
    mjb_encoding encoding,
    mjb_normalization form
);
```

Run the normalization quick-check on a string without allocating. `MJB_QC_MAYBE` means the string may still be normalized, and only a full normalization pass with `mjb_normalize` can decide.

- `buffer` — The string to check
- `byte_length` — The length of the string, in bytes
- `encoding` — The encoding of the string
- `form` — The normalization form to check

**Returns**

- `MJB_QC_YES` — The string is normalized to the requested form
- `MJB_QC_NO` — The string is not normalized
- `MJB_QC_MAYBE` — Inconclusive: a full normalization is needed to decide

**Example**

```c
const char *input = "caf\xC3\xA9";
mjb_quick_check_result check = mjb_string_is_normalized(input, strlen(input),
    MJB_ENC_UTF_8, MJB_NORMALIZATION_NFC);

// NFC normalized: yes
printf("NFC normalized: %s", check == MJB_QC_YES ? "yes" : "no");
```

See also: [`mjb_normalize`](#mjb_normalize).

Specifications: [UAX #15: Unicode Normalization Forms, Unicode 17.0.0](https://www.unicode.org/reports/tr15/tr15-57.html).

## `mjb_string_encoding`

Return the string encoding (the most probable).

```c
mjb_encoding mjb_string_encoding(
    const char *buffer,
    size_t byte_length
);
```

`mjb_string_encoding` reports BOM-derived UTF-16/UTF-32 schemes with the generic family bit plus the resolved endian bit. Passing that detected value consumes the leading BOM as a signature. Passing an explicit-endian encoding such as `MJB_ENC_UTF_16BE` preserves an initial U+FEFF as text. When flags overlap, as with a UTF-32LE BOM that also has the UTF-16LE BOM prefix, decoding gives UTF-32 precedence.

- `buffer` — The string to check
- `byte_length` — The length of the string, in bytes

**Example**

```c
const char utf16le[] = "\xFF\xFEH\0i\0";
mjb_encoding detected = mjb_string_encoding(utf16le, sizeof(utf16le) - 1);
bool is_utf16le = detected == (MJB_ENC_UTF_16 | MJB_ENC_UTF_16LE);

// UTF-16LE detected: yes
printf("UTF-16LE detected: %s", is_utf16le ? "yes" : "no");
```

## `mjb_string_is_ascii`

Return true if the string is encoded in ASCII.

```c
bool mjb_string_is_ascii(
    const char *buffer,
    size_t byte_length
);
```

- `buffer` — The string to check
- `byte_length` — The length of the string, in bytes

**Example**

```c
const char *input = "Plain ASCII";

// ASCII: yes
printf("ASCII: %s", mjb_string_is_ascii(input, strlen(input)) ? "yes" : "no");
```

## `mjb_string_is_utf8`

Return true if the string is encoded in UTF-8.

```c
bool mjb_string_is_utf8(
    const char *buffer,
    size_t byte_length
);
```

- `buffer` — The string to check
- `byte_length` — The length of the string, in bytes

**Example**

```c
const char *input = "caf\xC3\xA9";

// Valid UTF-8: yes
printf("Valid UTF-8: %s", mjb_string_is_utf8(input, strlen(input)) ? "yes" : "no");
```

## `mjb_string_is_utf16`

Return true if the string is encoded in UTF-16BE or UTF-16LE.

```c
bool mjb_string_is_utf16(
    const char *buffer,
    size_t byte_length
);
```

- `buffer` — The string to check
- `byte_length` — The length of the string, in bytes

**Example**

```c
const char utf16be[] = "\xFE\xFF\0H\0i"; // BOM + "Hi" in UTF-16BE

// UTF-16: yes
printf("UTF-16: %s", mjb_string_is_utf16(utf16be, sizeof(utf16be) - 1) ? "yes" : "no");
```

## `mjb_string_length`

Return the length of a string.

```c
size_t mjb_string_length(
    const char *buffer,
    size_t max_length,
    mjb_encoding encoding
);
```

Return the number of Unicode codepoints in a string, up to `max_length` bytes.

- `buffer` — The string to check
- `max_length` — The maximum length of the string, in bytes
- `encoding` — The encoding of the string

**Example**

```c
// The "Héllö" string is five Unicode characters, but has different byte lengths in different encodings.

const char *utf8 = "H\xC3\xA9ll\xC3\xB6"; // 7 bytes
const char utf16le[] = "H\0\xE9\0l\0l\0\xF6\0"; // 10 bytes
const char utf16be[] = "\0H\0\xE9\0l\0l\0\xF6"; // 10 bytes
const char utf32le[] = "H\0\0\0\xE9\0\0\0l\0\0\0l\0\0\0\xF6\0\0\0"; // 20 bytes
const char utf32be[] = "\0\0\0H\0\0\0\xE9\0\0\0l\0\0\0l\0\0\0\xF6"; // 20 bytes

// 5 UTF-8 characters
printf("%zu UTF-8 characters", mjb_string_length(utf8, 7, MJB_ENC_UTF_8));
// 5 UTF-16LE characters
printf("%zu UTF-16LE characters", mjb_string_length(utf16le, 10, MJB_ENC_UTF_16LE));
// 5 UTF-16BE characters
printf("%zu UTF-16BE characters", mjb_string_length(utf16be, 10, MJB_ENC_UTF_16BE));
// 5 UTF-32LE characters
printf("%zu UTF-32LE characters", mjb_string_length(utf32le, 20, MJB_ENC_UTF_32LE));
// 5 UTF-32BE characters
printf("%zu UTF-32BE characters", mjb_string_length(utf32be, 20, MJB_ENC_UTF_32BE));
```

## `mjb_string_each_character`

Run a callback for each character of a string.

```c
mjb_status mjb_string_each_character(
    const char *buffer,
    size_t byte_length,
    mjb_encoding encoding,
    mjb_string_each_character_fn callback
);
```

- `buffer` — The string to check
- `byte_length` — The length of the string, in bytes
- `encoding` — The encoding of the string
- `callback` — The function to call for each character

**Example**

```c
mjb_status status = mjb_string_each_character("ABC", 3, MJB_ENC_UTF_8, NULL);

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

- `codepoint` — The codepoint to check
- `property` — The binary property to query
- `value` — Where to store the binary property value

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

Specifications: [UAX #44: Unicode Character Database, Unicode 17.0.0](https://www.unicode.org/reports/tr44/tr44-36.html).

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

- `codepoint` — The codepoint to check
- `property` — The enumerated or integer property to query
- `value` — Where to store the property value

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

Specifications: [UAX #44: Unicode Character Database, Unicode 17.0.0](https://www.unicode.org/reports/tr44/tr44-36.html).

## `mjb_codepoint_numeric_value`

Return the numeric value of a codepoint.

```c
mjb_status mjb_codepoint_numeric_value(
    mjb_codepoint codepoint,
    mjb_numeric_value *value
);
```

Return the numeric value of a codepoint, if any. If the codepoint has no numeric value, `value->decimal` and `value->digit` are set to `MJB_NUMBER_NOT_VALID` (-1).

- `codepoint` — The codepoint to check
- `value` — The numeric value to store the result

**Returns**

- `MJB_STATUS_OK` — The character was found and filled
- `MJB_STATUS_INVALID_ARGUMENT` — `value` is NULL or the codepoint is not valid

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

Specifications: [UAX #44: Unicode Character Database, Unicode 17.0.0](https://www.unicode.org/reports/tr44/tr44-36.html).

## `mjb_codepoint_block`

Return the character block.

```c
mjb_status mjb_codepoint_block(
    mjb_codepoint codepoint,
    mjb_block_info *block
);
```

- `codepoint` — The codepoint to check
- `block` — The block to store the result

**Example**

```c
mjb_block_info block;

if(mjb_codepoint_block('A', &block) != MJB_STATUS_OK) {
    return 1;
}

// Block: Basic Latin
printf("Block: %s", block.name);
```

Specifications: [UAX #44: Unicode Character Database, Unicode 17.0.0](https://www.unicode.org/reports/tr44/tr44-36.html).

## `mjb_codepoint_script`

Return the script of a codepoint.

```c
mjb_script mjb_codepoint_script(
    mjb_codepoint codepoint
);
```

- `codepoint` — The codepoint to check

**Example**

```c
mjb_script script = mjb_codepoint_script(0x03A9); // Greek capital omega

// Greek script: yes
printf("Greek script: %s", script == MJB_SC_GREK ? "yes" : "no");
```

Specifications: [UAX #44: Unicode Character Database, Unicode 17.0.0](https://www.unicode.org/reports/tr44/tr44-36.html).

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

- `codepoint` — The codepoint to check
- `scripts` — The caller-provided script buffer, or NULL to query the required count
- `count` — The input capacity and output script count

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

Specifications: [UAX #24: Unicode Script Property, Unicode 17.0.0](https://www.unicode.org/reports/tr24/tr24-39.html).

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

- `codepoint` — The codepoint to encode
- `buffer` — The buffer to encode the codepoint to
- `byte_length` — The length of the buffer, in bytes
- `encoding` — The encoding to use

**Example**

```c
char encoded[4];
unsigned int size = mjb_codepoint_encode(0x20AC, encoded, sizeof(encoded), MJB_ENC_UTF_8);

// € sign uses 3 UTF-8 bytes
printf("%.*s sign uses %u UTF-8 bytes", (int)size, encoded, size);
```

## `mjb_string_convert_encoding`

Convert from one encoding to another.

```c
mjb_status mjb_string_convert_encoding(
    const char *buffer,
    size_t byte_length,
    mjb_encoding encoding,
    mjb_encoding output_encoding,
    mjb_result *result
);
```

Convert a string between the supported encodings (UTF-8, UTF-16LE/BE, UTF-32LE/BE). Generic UTF-16/UTF-32 input consumes a leading BOM as the encoding scheme signature and uses it to resolve byte order. Explicit-endian input preserves an initial U+FEFF as text. Generic UTF-16/UTF-32 without a BOM, and generic UTF-16/UTF-32 output, are rejected because the byte order is not specified.

- `buffer` — The string to convert
- `byte_length` — The length of the string, in bytes
- `encoding` — The input encoding of the string
- `output_encoding` — The output encoding of the string
- `result` — The pointer to store the result. If `result->transformed` is true, `result->output` is library-allocated and must be freed with `mjb_result_free(result)`

**Returns**

- `MJB_STATUS_OK` — The string was converted
- `MJB_STATUS_INVALID_ARGUMENT` — `result` is NULL, `buffer` is NULL with a non-zero size, or the input is not valid in the source encoding
- `MJB_STATUS_INVALID_ENCODING` — A generic UTF-16/UTF-32 encoding did not provide enough byte order information
- `MJB_STATUS_UNSUPPORTED` — The requested encoding conversion is not supported
- `MJB_STATUS_OVERFLOW` — The output size would overflow
- `MJB_STATUS_NO_MEMORY` — Allocation failed

**Example**

```c
const char *input = "caf\xC3\xA9";
mjb_result result;

if(mjb_string_convert_encoding(input, strlen(input), MJB_ENC_UTF_8,
    MJB_ENC_UTF_16LE, &result) != MJB_STATUS_OK) {
    return 1;
}

// UTF-16LE bytes: 8
printf("UTF-16LE bytes: %zu", result.output_size);
mjb_result_free(&result);
```

See also: [`mjb_string_encoding`](#mjb_string_encoding), [`mjb_codepoint_encode`](#mjb_codepoint_encode).

## `mjb_string_compare`

Compare two strings using UCA.

```c
int mjb_string_compare(
    const char *s1,
    size_t s1_byte_length,
    mjb_encoding s1_encoding,
    const char *s2,
    size_t s2_byte_length,
    mjb_encoding s2_encoding,
    mjb_collation_mode mode
);
```

Compare two strings using the Unicode Collation Algorithm and the default collation element table (DUCET), with `strcmp`-style semantics.

- `s1` — The first string to compare
- `s1_byte_length` — The length of the first string, in bytes
- `s1_encoding` — The encoding of the first string
- `s2` — The second string to compare
- `s2_byte_length` — The length of the second string, in bytes
- `s2_encoding` — The encoding of the second string
- `mode` — The variable weighting strategy

**Returns**

- `< 0` — The first string collates before the second
- `0` — The strings are equal under UCA
- `> 0` — The first string collates after the second

**Example**

```c
int order = mjb_string_compare("apple", 5, MJB_ENC_UTF_8,
    "banana", 6, MJB_ENC_UTF_8, MJB_COLLATION_NON_IGNORABLE);

// apple sorts before banana: yes
printf("apple sorts before banana: %s", order < 0 ? "yes" : "no");
```

See also: [`mjb_collation_key`](#mjb_collation_key).

Specifications: [UTS #10: Unicode Collation Algorithm, Unicode 17.0.0](https://www.unicode.org/reports/tr10/tr10-53.html).

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

Generate a binary sort key for a string. Sort keys of different strings can be compared with `memcmp` and yield the same order as `mjb_string_compare`. Useful when the same strings are compared many times, such as sorting or database indexing.

- `buffer` — The string to generate the sort key for
- `byte_length` — The length of the string, in bytes
- `encoding` — The encoding of the string
- `mode` — The variable weighting strategy
- `result` — The pointer to store the binary sort key. If `result->transformed` is true, `result->output` is library-allocated and must be freed with `mjb_result_free(result)`

**Returns**

- `MJB_STATUS_OK` — The sort key was generated
- `MJB_STATUS_INVALID_ARGUMENT` — `result` is NULL, or `buffer` is NULL with a non-zero size
- `MJB_STATUS_OVERFLOW` — The sort key size would overflow
- `MJB_STATUS_NO_MEMORY` — Allocation failed

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

See also: [`mjb_string_compare`](#mjb_string_compare).

Specifications: [UTS #10: Unicode Collation Algorithm, Unicode 17.0.0](https://www.unicode.org/reports/tr10/tr10-53.html).

## `mjb_case`

Change string case.

```c
mjb_status mjb_case(
    const char *buffer,
    size_t byte_length,
    mjb_encoding encoding,
    mjb_case_type type,
    mjb_encoding output_encoding,
    mjb_result *result
);
```

Convert a string to uppercase, lowercase, titlecase, or its case-folded form. Full case mappings are applied, including special casing and conditional mappings, so the output may have a different length than the input. Titlecase uses UAX #29 word boundaries: the first cased character in each word segment is titlecased, and subsequent characters in that segment are lowercased. Casing is tailored by the process-global locale set with `mjb_locale_set`: the default `MJB_LOCALE_EN` uses default non-Turkic mappings. `MJB_LOCALE_TR` and `MJB_LOCALE_AZ` apply Turkish/Azerbaijani dotted-I casing and Turkic `T` case-folding mappings. `MJB_LOCALE_LT` applies Lithuanian dot-above casing rules, while case folding remains the default non-Turkic mapping.

- `buffer` — The string to change case
- `byte_length` — The length of the string, in bytes
- `encoding` — The encoding of the string
- `type` — The type of case change
- `output_encoding` — The output encoding of the string
- `result` — The pointer to store the result. If `result->transformed` is true, `result->output` is library-allocated and must be freed with `mjb_result_free(result)`

**Returns**

- `MJB_STATUS_OK` — The case conversion succeeded
- `MJB_STATUS_INVALID_ARGUMENT` — `result` is NULL, `buffer` is NULL with a non-zero size, or `type` is not a valid case type
- `MJB_STATUS_NO_MEMORY` — Allocation failed

**Example**

```c
const char *input = "Stra\xC3\x9F""e"; // "Straße"
mjb_result result;

if(mjb_case(input, strlen(input), MJB_ENC_UTF_8, MJB_CASE_UPPER, MJB_ENC_UTF_8,
    &result) != MJB_STATUS_OK) {
    return 1;
}

// Upper: STRASSE
printf("Upper: %.*s", (int)result.output_size, result.output);

if(result.transformed) {
    mjb_free(result.output);
}
```

See also: [`mjb_locale_set`](#mjb_locale_set), [`mjb_codepoint_to_uppercase`](#mjb_codepoint_to_uppercase), [`mjb_codepoint_to_lowercase`](#mjb_codepoint_to_lowercase), [`mjb_codepoint_to_titlecase`](#mjb_codepoint_to_titlecase).

Specifications: [The Unicode Standard, Version 17.0.0, Section 3.13: Default Case Algorithms](https://www.unicode.org/versions/Unicode17.0.0/core-spec/chapter-3/#G33992).

## `mjb_codepoint_is_valid`

Return true if the codepoint is valid.

```c
bool mjb_codepoint_is_valid(
    mjb_codepoint codepoint
);
```

- `codepoint` — The codepoint to check

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

- `codepoint` — The codepoint to check

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

- `codepoint` — The codepoint to check

**Example**

```c
// U+0301 is combining: yes
printf("U+0301 is combining: %s", mjb_codepoint_is_combining(0x0301) ? "yes" : "no");
```

## `mjb_codepoint_is_hangul_l`

Return if the codepoint is a hangul L.

```c
bool mjb_codepoint_is_hangul_l(
    mjb_codepoint codepoint
);
```

- `codepoint` — The codepoint to check

**Example**

```c
// U+1100 is a leading Jamo: yes
printf("U+1100 is a leading Jamo: %s", mjb_codepoint_is_hangul_l(0x1100) ? "yes" : "no");
```

## `mjb_codepoint_is_hangul_v`

Return if the codepoint is a hangul V.

```c
bool mjb_codepoint_is_hangul_v(
    mjb_codepoint codepoint
);
```

- `codepoint` — The codepoint to check

**Example**

```c
// U+1161 is a vowel Jamo: yes
printf("U+1161 is a vowel Jamo: %s", mjb_codepoint_is_hangul_v(0x1161) ? "yes" : "no");
```

## `mjb_codepoint_is_hangul_t`

Return if the codepoint is a hangul T.

```c
bool mjb_codepoint_is_hangul_t(
    mjb_codepoint codepoint
);
```

- `codepoint` — The codepoint to check

**Example**

```c
// U+11A8 is a trailing Jamo: yes
printf("U+11A8 is a trailing Jamo: %s", mjb_codepoint_is_hangul_t(0x11A8) ? "yes" : "no");
```

## `mjb_codepoint_is_hangul_jamo`

Return if the codepoint is a hangul jamo.

```c
bool mjb_codepoint_is_hangul_jamo(
    mjb_codepoint codepoint
);
```

- `codepoint` — The codepoint to check

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

- `codepoint` — The codepoint to check

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

- `codepoint` — The codepoint to check

**Example**

```c
// U+4E00 is a CJK ideograph: yes
printf("U+4E00 is a CJK ideograph: %s", mjb_codepoint_is_cjk_ideograph(0x4E00) ? "yes" : "no");
```

## `mjb_codepoint_is_cjk_ext`

Return if the codepoint is CJK extension.

```c
bool mjb_codepoint_is_cjk_ext(
    mjb_codepoint codepoint
);
```

- `codepoint` — The codepoint to check

**Example**

```c
// U+20000 is a CJK extension ideograph: yes
printf("U+20000 is a CJK extension ideograph: %s", mjb_codepoint_is_cjk_ext(0x20000) ? "yes" : "no");
```

## `mjb_category_is_graphic`

Return true if the category is graphic.

```c
bool mjb_category_is_graphic(
    mjb_category category
);
```

- `category` — The category to check

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

- `category` — The category to check

**Example**

```c
// Nonspacing marks are combining: yes
bool combining = mjb_category_is_combining(MJB_CATEGORY_MN);

// Nonspacing marks are combining: yes
printf("Nonspacing marks are combining: %s", combining ? "yes" : "no");
```

## `mjb_codepoint_to_lowercase`

Return the codepoint lowercase codepoint.

```c
mjb_codepoint mjb_codepoint_to_lowercase(
    mjb_codepoint codepoint
);
```

Return the lowercase codepoint of a codepoint. If the codepoint has no lowercase equivalent, the original codepoint is returned.

- `codepoint` — The codepoint to check

**Returns**

- `codepoint` — The lowercase codepoint, or the original codepoint

**Example**

```c
mjb_codepoint codepoint;

codepoint = mjb_codepoint_to_lowercase(0x0041); // U+0041 = 'A'

// A > a
printf("%c > %c", 'A', codepoint);

codepoint = mjb_codepoint_to_lowercase(0x03A3); // U+03A3 = 'Σ'

// U+03A3 > U+03C3, Σ > σ
printf("U+%04X > U+%04X, %s > %s",  0x03A3, codepoint, "Σ", "σ");
```

See also: [`mjb_codepoint_to_uppercase`](#mjb_codepoint_to_uppercase), [`mjb_codepoint_to_titlecase`](#mjb_codepoint_to_titlecase).

## `mjb_codepoint_to_uppercase`

Return the codepoint uppercase codepoint.

```c
mjb_codepoint mjb_codepoint_to_uppercase(
    mjb_codepoint codepoint
);
```

Return the uppercase codepoint of a codepoint. If the codepoint has no uppercase equivalent, the original codepoint is returned.

- `codepoint` — The codepoint to check

**Returns**

- `codepoint` — The uppercase codepoint, or the original codepoint

**Example**

```c
mjb_codepoint uppercase = mjb_codepoint_to_uppercase(0x00E9); // é

// Uppercase: U+00C9
printf("Uppercase: U+%04X", uppercase);
```

See also: [`mjb_codepoint_to_lowercase`](#mjb_codepoint_to_lowercase), [`mjb_codepoint_to_titlecase`](#mjb_codepoint_to_titlecase).

## `mjb_codepoint_to_titlecase`

Return the codepoint titlecase codepoint.

```c
mjb_codepoint mjb_codepoint_to_titlecase(
    mjb_codepoint codepoint
);
```

Return the titlecase codepoint of a codepoint. If the codepoint has no titlecase equivalent, the original codepoint is returned.

- `codepoint` — The codepoint to check

**Returns**

- `codepoint` — The titlecase codepoint, or the original codepoint

**Example**

```c
mjb_codepoint titlecase = mjb_codepoint_to_titlecase(0x01F3); // dz

// Titlecase: U+01F2
printf("Titlecase: U+%04X", titlecase);
```

See also: [`mjb_codepoint_to_lowercase`](#mjb_codepoint_to_lowercase), [`mjb_codepoint_to_uppercase`](#mjb_codepoint_to_uppercase).

## `mjb_break_line`

Unicode line break algorithm.

```c
mjb_break_type mjb_break_line(
    const char *buffer,
    size_t byte_length,
    mjb_encoding encoding,
    mjb_next_line_state *state
);
```

- `buffer` — The string to check
- `byte_length` — The length of the string, in bytes
- `encoding` — The encoding of the string
- `state` — The state to store the result

**Example**

```c
mjb_next_line_state state;
state.index = 0;
mjb_break_type type = mjb_break_line("Hello world", 11, MJB_ENC_UTF_8, &state);

// First line-break result is set: yes
printf("First line-break result is set: %s", type != MJB_BT_NOT_SET ? "yes" : "no");
```

See also: [`mjb_break_grapheme_cluster`](#mjb_break_grapheme_cluster), [`mjb_break_word`](#mjb_break_word), [`mjb_break_sentence`](#mjb_break_sentence).

Specifications: [UAX #14: Unicode Line Breaking Algorithm, Unicode 17.0.0](https://www.unicode.org/reports/tr14/tr14-55.html).

## `mjb_break_word`

Word cluster breaking.

```c
mjb_break_type mjb_break_word(
    const char *buffer,
    size_t byte_length,
    mjb_encoding encoding,
    mjb_next_word_state *state
);
```

- `buffer` — The string to check
- `byte_length` — The length of the string, in bytes
- `encoding` — The encoding of the string
- `state` — The state to store the result

**Example**

```c
mjb_next_word_state state;
state.index = 0;
size_t boundaries = 0;

while(mjb_break_word("Hello world", 11, MJB_ENC_UTF_8, &state) != MJB_BT_NOT_SET) {
    ++boundaries;
}

// Word-break positions: 11
printf("Word-break positions: %zu", boundaries);
```

See also: [`mjb_break_grapheme_cluster`](#mjb_break_grapheme_cluster), [`mjb_break_sentence`](#mjb_break_sentence), [`mjb_truncate_word`](#mjb_truncate_word).

Specifications: [UAX #29: Unicode Text Segmentation, Unicode 17.0.0](https://www.unicode.org/reports/tr29/tr29-47.html).

## `mjb_break_sentence`

Sentence boundaries breaking.

```c
mjb_break_type mjb_break_sentence(
    const char *buffer,
    size_t byte_length,
    mjb_encoding encoding,
    mjb_next_sentence_state *state
);
```

- `buffer` — The string to check
- `byte_length` — The length of the string, in bytes
- `encoding` — The encoding of the string
- `state` — The state to store the result

**Example**

```c
mjb_next_sentence_state state;
state.index = 0;
size_t boundaries = 0;
const char *input = "Hello. Goodbye.";

while(mjb_break_sentence(input, strlen(input), MJB_ENC_UTF_8, &state) != MJB_BT_NOT_SET) {
    ++boundaries;
}

// Sentence-break positions: 15
printf("Sentence-break positions: %zu", boundaries);
```

See also: [`mjb_break_grapheme_cluster`](#mjb_break_grapheme_cluster), [`mjb_break_word`](#mjb_break_word).

Specifications: [UAX #29: Unicode Text Segmentation, Unicode 17.0.0](https://www.unicode.org/reports/tr29/tr29-47.html).

## `mjb_break_grapheme_cluster`

Grapheme cluster breaking.

```c
mjb_break_type mjb_break_grapheme_cluster(
    const char *buffer,
    size_t byte_length,
    mjb_encoding encoding,
    mjb_next_state *state
);
```

Iterate the grapheme cluster (user-perceived character) boundaries of a string. Call repeatedly with the same state until it reports the end of the string.

- `buffer` — The string to check
- `byte_length` — The length of the string, in bytes
- `encoding` — The encoding of the string
- `state` — The state to store the result

**Example**

```c
const char *input = "e\xCC\x81"; // e + combining acute accent
mjb_next_state state;
state.index = 0;
size_t codepoints = 0;

while(mjb_break_grapheme_cluster(input, strlen(input), MJB_ENC_UTF_8,
    &state) != MJB_BT_NOT_SET) {
    ++codepoints;
}

// Codepoints examined: 2
printf("Codepoints examined: %zu", codepoints);
```

See also: [`mjb_break_word`](#mjb_break_word), [`mjb_break_sentence`](#mjb_break_sentence), [`mjb_break_line`](#mjb_break_line), [`mjb_truncate`](#mjb_truncate).

Specifications: [UAX #29: Unicode Text Segmentation, Unicode 17.0.0](https://www.unicode.org/reports/tr29/tr29-47.html).

## `mjb_truncate`

Return the number of bytes that form the first `max_graphemes` grapheme cluster segments.

```c
size_t mjb_truncate(
    const char *buffer,
    size_t byte_length,
    mjb_encoding encoding,
    size_t max_graphemes
);
```

- `buffer` — The string to check
- `byte_length` — The length of the string, in bytes
- `encoding` — The encoding of the string
- `max_graphemes` — The maximum number of graphemes to return

**Example**

```c
const char *input = "A\xF0\x9F\x87\xAE\xF0\x9F\x87\xB9Z"; // A🇮🇹Z
size_t bytes = mjb_truncate(input, strlen(input), MJB_ENC_UTF_8, 2);

// First two graphemes use 9 bytes
printf("First two graphemes use %zu bytes", bytes);
```

## `mjb_truncate_width`

Return the number of bytes whose grapheme clusters fit within max_columns display columns.

```c
size_t mjb_truncate_width(
    const char *buffer,
    size_t byte_length,
    mjb_encoding encoding,
    mjb_width_context context,
    size_t max_columns
);
```

- `buffer` — The string to check
- `byte_length` — The length of the string, in bytes
- `encoding` — The encoding of the string
- `context` — The width context
- `max_columns` — The maximum number of columns to return

**Example**

```c
const char *input = "A\xE7\x95\x8C"; // A界
size_t bytes = mjb_truncate_width(input, strlen(input), MJB_ENC_UTF_8,
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

- `buffer` — The string to check
- `byte_length` — The length of the string, in bytes
- `encoding` — The encoding of the string
- `max_segments` — The maximum number of segments to return

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

- `buffer` — The string to check
- `byte_length` — The length of the string, in bytes
- `encoding` — The encoding of the string
- `context` — The width context
- `max_columns` — The maximum number of columns to return

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

- `buffer` — The input string
- `byte_length` — The length of the string, in bytes
- `encoding` — The encoding of the string
- `direction` — The base paragraph direction (LTR, RTL, or AUTO for P2/P3)
- `result` — Output paragraph; chars is library-allocated. `result->chars` is library-allocated and must be freed with `mjb_bidi_free()`

**Returns**

- `MJB_STATUS_OK` — The paragraph was resolved
- `MJB_STATUS_INVALID_ARGUMENT` — `result` is NULL, or `buffer` is NULL with a non-zero size
- `MJB_STATUS_OVERFLOW` — The paragraph size would overflow
- `MJB_STATUS_NO_MEMORY` — Allocation failed

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
mjb_bidi_free(&paragraph);
```

See also: [`mjb_bidi_free`](#mjb_bidi_free), [`mjb_bidi_reorder_line`](#mjb_bidi_reorder_line), [`mjb_bidi_line_runs`](#mjb_bidi_line_runs).

Specifications: [UAX #9: Unicode Bidirectional Algorithm, Unicode 17.0.0](https://www.unicode.org/reports/tr9/tr9-51.html).

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

- `paragraph` — The resolved paragraph
- `line_start` — Start index into paragraph->chars
- `line_end` — End index (exclusive) into paragraph->chars
- `visual_order` — Caller-allocated array of size (`line_end` - `line_start`). Caller-allocated; the library does not retain or free it

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
mjb_bidi_free(&paragraph);
```

See also: [`mjb_bidi_resolve`](#mjb_bidi_resolve), [`mjb_bidi_line_runs`](#mjb_bidi_line_runs).

Specifications: [UAX #9: Unicode Bidirectional Algorithm, Unicode 17.0.0](https://www.unicode.org/reports/tr9/tr9-51.html).

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

- `paragraph` — The resolved paragraph
- `visual_order` — Visual order array from `mjb_bidi_reorder_line`
- `count` — Length of visual_order
- `runs` — Caller-allocated array, or NULL to only count
- `run_count` — On output: number of runs written (or total if `runs` = `NULL`)

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
mjb_bidi_free(&paragraph);
```

See also: [`mjb_bidi_resolve`](#mjb_bidi_resolve), [`mjb_bidi_reorder_line`](#mjb_bidi_reorder_line).

Specifications: [UAX #9: Unicode Bidirectional Algorithm, Unicode 17.0.0](https://www.unicode.org/reports/tr9/tr9-51.html).

## `mjb_bidi_free`

Free a bidi paragraph allocated by mjb_bidi_resolve.

```c
void mjb_bidi_free(
    mjb_bidi_paragraph *paragraph
);
```

- `paragraph` — The paragraph to free

**Example**

```c
mjb_bidi_paragraph paragraph;

if(mjb_bidi_resolve("abc", 3, MJB_ENC_UTF_8, MJB_DIRECTION_LTR,
    &paragraph) != MJB_STATUS_OK) {
    return 1;
}

mjb_bidi_free(&paragraph);

// Paragraph released: yes
printf("Paragraph released: %s", paragraph.chars == NULL ? "yes" : "no");
```

See also: [`mjb_bidi_resolve`](#mjb_bidi_resolve).

## `mjb_codepoint_is_id_start`

Return true if the codepoint is a valid Unicode identifier start (Unicode 17.0.0 UAX #31 ID_Start).

```c
bool mjb_codepoint_is_id_start(
    mjb_codepoint codepoint
);
```

- `codepoint` — The codepoint to check

**Example**

```c
// Greek alpha starts an identifier: yes
bool starts = mjb_codepoint_is_id_start(0x03B1);

// Greek alpha starts an identifier: yes
printf("Greek alpha starts an identifier: %s", starts ? "yes" : "no");
```

Specifications: [UAX #31: Unicode Identifiers and Syntax, Unicode 17.0.0](https://www.unicode.org/reports/tr31/tr31-43.html).

## `mjb_codepoint_is_id_continue`

Return true if the codepoint is a valid Unicode identifier continuation (Unicode 17.0.0 UAX #31 ID_Continue).

```c
bool mjb_codepoint_is_id_continue(
    mjb_codepoint codepoint
);
```

- `codepoint` — The codepoint to check

**Example**

```c
// Digit 7 continues an identifier: yes
bool continues = mjb_codepoint_is_id_continue('7');

// Digit 7 continues an identifier: yes
printf("Digit 7 continues an identifier: %s", continues ? "yes" : "no");
```

Specifications: [UAX #31: Unicode Identifiers and Syntax, Unicode 17.0.0](https://www.unicode.org/reports/tr31/tr31-43.html).

## `mjb_codepoint_is_xid_start`

Return true if the codepoint is a valid NFKC identifier start (Unicode 17.0.0 UAX #31 XID_Start).

```c
bool mjb_codepoint_is_xid_start(
    mjb_codepoint codepoint
);
```

- `codepoint` — The codepoint to check

**Example**

```c
// Letter A is XID_Start: yes
printf("Letter A is XID_Start: %s", mjb_codepoint_is_xid_start('A') ? "yes" : "no");
```

Specifications: [UAX #31: Unicode Identifiers and Syntax, Unicode 17.0.0](https://www.unicode.org/reports/tr31/tr31-43.html).

## `mjb_codepoint_is_xid_continue`

Return true if the codepoint is a valid NFKC identifier continuation (Unicode 17.0.0 UAX #31 XID_Continue).

```c
bool mjb_codepoint_is_xid_continue(
    mjb_codepoint codepoint
);
```

- `codepoint` — The codepoint to check

**Example**

```c
// Underscore is XID_Continue: yes
bool continues = mjb_codepoint_is_xid_continue('_');

// Underscore is XID_Continue: yes
printf("Underscore is XID_Continue: %s", continues ? "yes" : "no");
```

Specifications: [UAX #31: Unicode Identifiers and Syntax, Unicode 17.0.0](https://www.unicode.org/reports/tr31/tr31-43.html).

## `mjb_codepoint_is_pattern_syntax`

Return true if the codepoint is reserved for use in patterns (Unicode 17.0.0 UAX #31 Pattern_Syntax).

```c
bool mjb_codepoint_is_pattern_syntax(
    mjb_codepoint codepoint
);
```

- `codepoint` — The codepoint to check

**Example**

```c
// Plus sign is Pattern_Syntax: yes
bool syntax = mjb_codepoint_is_pattern_syntax('+');

// Plus sign is Pattern_Syntax: yes
printf("Plus sign is Pattern_Syntax: %s", syntax ? "yes" : "no");
```

Specifications: [UAX #31: Unicode Identifiers and Syntax, Unicode 17.0.0](https://www.unicode.org/reports/tr31/tr31-43.html).

## `mjb_codepoint_is_pattern_white_space`

Return true if the codepoint is pattern whitespace (Unicode 17.0.0 UAX #31 Pattern_White_Space).

```c
bool mjb_codepoint_is_pattern_white_space(
    mjb_codepoint codepoint
);
```

- `codepoint` — The codepoint to check

**Example**

```c
// Space is Pattern_White_Space: yes
bool whitespace = mjb_codepoint_is_pattern_white_space(' ');

// Space is Pattern_White_Space: yes
printf("Space is Pattern_White_Space: %s", whitespace ? "yes" : "no");
```

Specifications: [UAX #31: Unicode Identifiers and Syntax, Unicode 17.0.0](https://www.unicode.org/reports/tr31/tr31-43.html).

## `mjb_string_is_identifier`

Return true if the string is a valid Unicode identifier (Unicode 17.0.0 UAX #31).

```c
bool mjb_string_is_identifier(
    const char *buffer,
    size_t byte_length,
    mjb_encoding encoding,
    mjb_identifier_profile profile
);
```

Validate a string as a Unicode identifier: the first character must be a valid identifier start and the following ones valid identifier continuations, using ID_Start/ID_Continue for the DEFAULT profile or XID_Start/XID_Continue for the NFKC profile.

- `buffer` — The string to validate
- `byte_length` — The length of the string, in bytes
- `encoding` — The encoding of the string
- `profile` — The identifier profile (DEFAULT or NFKC)

**Example**

```c
const char *identifier = "delta_2";

bool valid = mjb_string_is_identifier(identifier, strlen(identifier), MJB_ENC_UTF_8,
    MJB_IDENTIFIER_NFKC);

// Valid identifier: yes
printf("Valid identifier: %s", valid ? "yes" : "no");
```

See also: [`mjb_codepoint_is_id_start`](#mjb_codepoint_is_id_start), [`mjb_codepoint_is_id_continue`](#mjb_codepoint_is_id_continue), [`mjb_codepoint_is_xid_start`](#mjb_codepoint_is_xid_start), [`mjb_codepoint_is_xid_continue`](#mjb_codepoint_is_xid_continue).

Specifications: [UAX #31: Unicode Identifiers and Syntax, Unicode 17.0.0](https://www.unicode.org/reports/tr31/tr31-43.html).

## `mjb_property_name`

Return the name of a property, NULL if the property specified is not valid.

```c
const char *mjb_property_name(
    mjb_property property
);
```

- `property` — The property to check

**Example**

```c
const char *name = mjb_property_name(MJB_PR_ALPHABETIC);

// Property: Alphabetic
printf("Property: %s", name);
```

Specifications: [UAX #44: Unicode Character Database, Unicode 17.0.0](https://www.unicode.org/reports/tr44/tr44-36.html).

## `mjb_confusable_skeleton`

Compute a Unicode confusable skeleton (Unicode 17.0.0 UTS #39 Section 4).

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

- `buffer` — The string to transform
- `byte_length` — The length of the string, in bytes
- `encoding` — The encoding of the string
- `output_encoding` — The output encoding of the skeleton
- `result` — The pointer to store the result. If `result->transformed` is true, `result->output` is library-allocated and must be freed with `mjb_result_free(result)`

**Returns**

- `MJB_STATUS_OK` — The confusable skeleton was returned
- `MJB_STATUS_INVALID_ARGUMENT` — `result` is NULL, or `buffer` is NULL with a non-zero size
- `MJB_STATUS_OVERFLOW` — The output size would overflow
- `MJB_STATUS_NO_MEMORY` — Allocation failed

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

See also: [`mjb_string_is_confusable`](#mjb_string_is_confusable), [`mjb_string_is_identifier`](#mjb_string_is_identifier).

Specifications: [UTS #39: Unicode Security Mechanisms, Unicode 17.0.0](https://www.unicode.org/reports/tr39/tr39-32.html).

## `mjb_string_is_confusable`

Return true if two strings are visually confusable (Unicode 17.0.0 UTS #39 Section 4): skeleton(s1) == skeleton(s2).

```c
bool mjb_string_is_confusable(
    const char *s1,
    size_t s1_byte_length,
    mjb_encoding s1_encoding,
    const char *s2,
    size_t s2_byte_length,
    mjb_encoding s2_encoding
);
```

Compute the confusable skeleton of both strings and return true when the skeletons are equal, meaning the two strings are visually confusable, such as "good" and "gооd" with Cyrillic о.

- `s1` — The first string
- `s1_byte_length` — The length of the first string, in bytes
- `s1_encoding` — The encoding of the first string
- `s2` — The second string
- `s2_byte_length` — The length of the second string, in bytes
- `s2_encoding` — The encoding of the second string

**Example**

```c
const char *latin = "hello";
const char *mixed = "h\xD0\xB5llo"; // Cyrillic е

bool confusable = mjb_string_is_confusable(latin, strlen(latin), MJB_ENC_UTF_8,
    mixed, strlen(mixed), MJB_ENC_UTF_8);

// Visually confusable: yes
printf("Visually confusable: %s", confusable ? "yes" : "no");
```

See also: [`mjb_confusable_skeleton`](#mjb_confusable_skeleton), [`mjb_string_is_identifier`](#mjb_string_is_identifier).

Specifications: [UTS #39: Unicode Security Mechanisms, Unicode 17.0.0](https://www.unicode.org/reports/tr39/tr39-32.html).

## `mjb_codepoint_emoji`

Return the emoji properties.

```c
mjb_status mjb_codepoint_emoji(
    mjb_codepoint codepoint,
    mjb_emoji_properties *emoji
);
```

- `codepoint` — The codepoint to check
- `emoji` — The emoji properties to store the result

**Example**

```c
mjb_emoji_properties emoji;

if(mjb_codepoint_emoji(0x1F600, &emoji) != MJB_STATUS_OK) {
    return 1;
}

// U+1F600 has Emoji_Presentation: yes
printf("U+1F600 has Emoji_Presentation: %s", emoji.presentation ? "yes" : "no");
```

See also: [`mjb_string_emoji_sequence`](#mjb_string_emoji_sequence), [`mjb_codepoint_is_emoji`](#mjb_codepoint_is_emoji).

Specifications: [UTS #51: Unicode Emoji, Unicode 17.0.0](https://www.unicode.org/reports/tr51/tr51-29.html).

## `mjb_codepoint_is_emoji`

Return true if the codepoint has the Unicode Emoji property.

```c
bool mjb_codepoint_is_emoji(
    mjb_codepoint codepoint
);
```

- `codepoint` — The codepoint to check

**Example**

```c
// Number sign has the Emoji property: yes
bool emoji = mjb_codepoint_is_emoji('#');

// Number sign has the Emoji property: yes
printf("Number sign has the Emoji property: %s", emoji ? "yes" : "no");
```

Specifications: [UTS #51: Unicode Emoji, Unicode 17.0.0](https://www.unicode.org/reports/tr51/tr51-29.html).

## `mjb_codepoint_is_emoji_presentation`

Return true if the codepoint has the Unicode Emoji_Presentation property.

```c
bool mjb_codepoint_is_emoji_presentation(
    mjb_codepoint codepoint
);
```

- `codepoint` — The codepoint to check

**Example**

```c
// Grinning face defaults to emoji presentation: yes
bool presentation = mjb_codepoint_is_emoji_presentation(0x1F600);

// Grinning face defaults to emoji presentation: yes
printf("Grinning face defaults to emoji presentation: %s", presentation ? "yes" : "no");
```

Specifications: [UTS #51: Unicode Emoji, Unicode 17.0.0](https://www.unicode.org/reports/tr51/tr51-29.html).

## `mjb_codepoint_is_emoji_modifier`

Return true if the codepoint has the Unicode Emoji_Modifier property.

```c
bool mjb_codepoint_is_emoji_modifier(
    mjb_codepoint codepoint
);
```

- `codepoint` — The codepoint to check

**Example**

```c
// Medium skin tone is an emoji modifier: yes
bool modifier = mjb_codepoint_is_emoji_modifier(0x1F3FD);

// Medium skin tone is an emoji modifier: yes
printf("Medium skin tone is an emoji modifier: %s", modifier ? "yes" : "no");
```

Specifications: [UTS #51: Unicode Emoji, Unicode 17.0.0](https://www.unicode.org/reports/tr51/tr51-29.html).

## `mjb_codepoint_is_emoji_modifier_base`

Return true if the codepoint has the Unicode Emoji_Modifier_Base property.

```c
bool mjb_codepoint_is_emoji_modifier_base(
    mjb_codepoint codepoint
);
```

- `codepoint` — The codepoint to check

**Example**

```c
// Waving hand accepts an emoji modifier: yes
bool modifier_base = mjb_codepoint_is_emoji_modifier_base(0x1F44B);

// Waving hand accepts an emoji modifier: yes
printf("Waving hand accepts an emoji modifier: %s", modifier_base ? "yes" : "no");
```

Specifications: [UTS #51: Unicode Emoji, Unicode 17.0.0](https://www.unicode.org/reports/tr51/tr51-29.html).

## `mjb_codepoint_is_emoji_component`

Return true if the codepoint has the Unicode Emoji_Component property.

```c
bool mjb_codepoint_is_emoji_component(
    mjb_codepoint codepoint
);
```

- `codepoint` — The codepoint to check

**Example**

```c
// Zero-width joiner is an emoji component: yes
bool component = mjb_codepoint_is_emoji_component(0x200D);

// Zero-width joiner is an emoji component: yes
printf("Zero-width joiner is an emoji component: %s", component ? "yes" : "no");
```

Specifications: [UTS #51: Unicode Emoji, Unicode 17.0.0](https://www.unicode.org/reports/tr51/tr51-29.html).

## `mjb_codepoint_is_extended_pictographic`

Return true if the codepoint has the Unicode Extended_Pictographic property.

```c
bool mjb_codepoint_is_extended_pictographic(
    mjb_codepoint codepoint
);
```

- `codepoint` — The codepoint to check

**Example**

```c
// Red heart is Extended_Pictographic: yes
bool pictographic = mjb_codepoint_is_extended_pictographic(0x2764);

// Red heart is Extended_Pictographic: yes
printf("Red heart is Extended_Pictographic: %s", pictographic ? "yes" : "no");
```

Specifications: [UTS #51: Unicode Emoji, Unicode 17.0.0](https://www.unicode.org/reports/tr51/tr51-29.html).

## `mjb_codepoint_plane`

Return the plane of the codepoint.

```c
mjb_plane mjb_codepoint_plane(
    mjb_codepoint codepoint
);
```

- `codepoint` — The codepoint to check

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

- `plane` — The plane to check

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

- `plane` — The plane to check
- `abbreviation` — Whether to use an abbreviation

**Example**

```c
// Plane: Basic Multilingual Plane
printf("Plane: %s", mjb_plane_name(MJB_PLANE_BMP, false));
```

## `mjb_string_emoji_sequence`

Return emoji sequence metadata for a complete string.

```c
mjb_status mjb_string_emoji_sequence(
    const char *buffer,
    size_t byte_length,
    mjb_encoding encoding,
    mjb_emoji_sequence *emoji
);
```

- `buffer` — The string to check
- `byte_length` — The length of the string, in bytes
- `encoding` — The encoding of the string
- `emoji` — The emoji sequence metadata to store the result

**Example**

```c
const char *flag = "\xF0\x9F\x87\xAE\xF0\x9F\x87\xB9"; // 🇮🇹
mjb_emoji_sequence emoji;

if(mjb_string_emoji_sequence(flag, strlen(flag), MJB_ENC_UTF_8,
    &emoji) != MJB_STATUS_OK) {
    return 1;
}

// Sequence codepoints: 2
printf("Sequence codepoints: %zu", emoji.codepoint_count);
```

See also: [`mjb_string_is_emoji_sequence`](#mjb_string_is_emoji_sequence), [`mjb_string_is_rgi_emoji`](#mjb_string_is_rgi_emoji).

Specifications: [UTS #51: Unicode Emoji, Unicode 17.0.0](https://www.unicode.org/reports/tr51/tr51-29.html).

## `mjb_string_is_emoji_sequence`

Return true if the complete string is an emoji sequence listed by Unicode, including standardized emoji variation sequences.

```c
bool mjb_string_is_emoji_sequence(
    const char *buffer,
    size_t byte_length,
    mjb_encoding encoding
);
```

- `buffer` — The string to check
- `byte_length` — The length of the string, in bytes
- `encoding` — The encoding of the string

**Example**

```c
const char *keycap = "1\xEF\xB8\x8F\xE2\x83\xA3"; // 1️⃣

bool listed = mjb_string_is_emoji_sequence(keycap, strlen(keycap), MJB_ENC_UTF_8);

// Listed emoji sequence: yes
printf("Listed emoji sequence: %s", listed ? "yes" : "no");
```

See also: [`mjb_string_is_rgi_emoji`](#mjb_string_is_rgi_emoji), [`mjb_string_emoji_sequence`](#mjb_string_emoji_sequence).

Specifications: [UTS #51: Unicode Emoji, Unicode 17.0.0](https://www.unicode.org/reports/tr51/tr51-29.html).

## `mjb_string_is_rgi_emoji`

Return true if the complete string is an RGI emoji sequence, excluding plain standardized variation sequences.

```c
bool mjb_string_is_rgi_emoji(
    const char *buffer,
    size_t byte_length,
    mjb_encoding encoding
);
```

- `buffer` — The string to check
- `byte_length` — The length of the string, in bytes
- `encoding` — The encoding of the string

**Example**

```c
const char *flag = "\xF0\x9F\x87\xAE\xF0\x9F\x87\xB9"; // 🇮🇹

bool rgi = mjb_string_is_rgi_emoji(flag, strlen(flag), MJB_ENC_UTF_8);

// RGI emoji: yes
printf("RGI emoji: %s", rgi ? "yes" : "no");
```

See also: [`mjb_string_is_emoji_sequence`](#mjb_string_is_emoji_sequence), [`mjb_string_emoji_sequence`](#mjb_string_emoji_sequence).

Specifications: [UTS #51: Unicode Emoji, Unicode 17.0.0](https://www.unicode.org/reports/tr51/tr51-29.html).

## `mjb_hangul_syllable_name`

Return hangul syllable name.

```c
mjb_status mjb_hangul_syllable_name(
    mjb_codepoint codepoint,
    char *buffer,
    size_t byte_length
);
```

- `codepoint` — The codepoint to check
- `buffer` — The buffer to store the result
- `byte_length` — The length of the string, in bytes

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

- `codepoint` — The codepoint to check
- `codepoints` — The codepoints to store the result

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

- `characters` — The characters to compose
- `characters_len` — The length of the characters

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

- `codepoint` — The codepoint to check
- `width` — The width to store the result

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

Specifications: [UAX #11: East Asian Width, Unicode 17.0.0](https://www.unicode.org/reports/tr11/tr11-44.html).

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

- `buffer` — The string to normalize
- `byte_length` — The length of the string, in bytes
- `encoding` — The encoding of the string
- `context` — The width context for ambiguous-width characters
- `width` — The width to store the result

**Returns**

- `MJB_STATUS_OK` — The width was computed
- `MJB_STATUS_INVALID_ARGUMENT` — `width` is NULL, or `buffer` is NULL with a non-zero size
- `MJB_STATUS_OVERFLOW` — The width would overflow

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

See also: [`mjb_codepoint_east_asian_width`](#mjb_codepoint_east_asian_width), [`mjb_truncate_width`](#mjb_truncate_width).

Specifications: [UAX #11: East Asian Width, Unicode 17.0.0](https://www.unicode.org/reports/tr11/tr11-44.html).

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

- `id` — The BCP 47 language tag to parse
- `byte_length` — The length of the locale identifier, in bytes
- `encoding` — The encoding of the locale identifier
- `locale` — The locale structure to store the result
- `error` — The error to store when parsing fails

**Returns**

- `MJB_STATUS_OK` — The tag was parsed and `locale` filled
- `MJB_STATUS_INVALID_ARGUMENT` — An argument is NULL or the tag is not a valid BCP 47 language tag
- `MJB_STATUS_NO_MEMORY` — Allocation failed

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

See also: [`mjb_locale_set`](#mjb_locale_set).

Specifications: [BCP 47: Tags for Identifying Languages](https://www.rfc-editor.org/rfc/rfc5646).

## `mjb_locale_set`

Set current locale used by locale-sensitive casing.

```c
mjb_status mjb_locale_set(
    unsigned int locale
);
```

Set the process-global locale used by `mjb_case`. The default locale is `MJB_LOCALE_EN`, and `mjb_shutdown` resets it to `MJB_LOCALE_EN`. Only `MJB_LOCALE_TR`, `MJB_LOCALE_AZ`, and `MJB_LOCALE_LT` currently tailor casing. Other valid locale values are accepted but do not change Unicode algorithm behavior.

- `locale` — The locale to set

**Returns**

- `MJB_STATUS_OK` — The locale was set
- `MJB_STATUS_INVALID_ARGUMENT` — `locale` is not a valid `mjb_locale` value

**Example**

```c
if(mjb_locale_set(MJB_LOCALE_TR) != MJB_STATUS_OK) {
    return 1;
}

// Turkish locale selected: yes
printf("Turkish locale selected: yes");
if(mjb_locale_set(MJB_LOCALE_EN) != MJB_STATUS_OK) {
    return 1;
}
```

See also: [`mjb_case`](#mjb_case).

## `mjb_result_free`

Free a mjb_result.

```c
mjb_status mjb_result_free(
    mjb_result *result
);
```

Free the memory allocated for a `mjb_result`. The `result` pointer is set to NULL.

- `result` — The result to free

**Returns**

- `MJB_STATUS_OK` — The result was freed
- `MJB_STATUS_INVALID_ARGUMENT` — `result` is NULL

**Example**

```c
mjb_result result;

if(mjb_string_convert_encoding("A", 1, MJB_ENC_UTF_8, MJB_ENC_UTF_16LE,
    &result) != MJB_STATUS_OK || mjb_result_free(&result) != MJB_STATUS_OK) {
    return 1;
}

// Result released: yes
printf("Result released: %s", result.output == NULL ? "yes" : "no");
```

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

// Unicode version: 17.0.0
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

- `alloc_fn` — The function to allocate memory
- `realloc_fn` — The function to reallocate memory
- `free_fn` — The function to free memory

**Example**

```c
mjb_shutdown(); // Ensure no allocator is currently locked in.

if(mjb_set_memory_functions(malloc, realloc, free) != MJB_STATUS_OK) {
    return 1;
}

// Standard allocator installed: yes
printf("Standard allocator installed: yes");
mjb_shutdown();
```

See also: [`mjb_alloc`](#mjb_alloc), [`mjb_realloc`](#mjb_realloc), [`mjb_free`](#mjb_free).

## `mjb_shutdown`

Shutdown the library. Not needed to be called.

```c
void mjb_shutdown(void);
```

**Example**

```c
mjb_shutdown();

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

- `byte_length` — The length of the memory to allocate

**Example**

```c
char *buffer = (char*)mjb_alloc(32);

if(buffer == NULL) {
    return 1;
}

strcpy(buffer, "allocated");

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

- `ptr` — The pointer to reallocate
- `new_size` — The new size of the memory

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

- `ptr` — The pointer to free

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

Mojibake targets [The Unicode Standard, Version 17.0.0](https://www.unicode.org/versions/Unicode17.0.0/)
and the [Unicode Character Database 17.0.0](https://www.unicode.org/Public/17.0.0/). Function-level
Unicode specification links below point to the archived Unicode 17.0.0 version of the applicable
Unicode Standard Annex or synchronized Unicode Technical Standard. Generic Unicode links, when
present, are informational or download links rather than normative conformance references.

# Unicode tailoring

Unless a function documents a tailoring, it uses the referenced Unicode 17.0.0 algorithm
without higher-level protocol tailoring.

- `mjb_case` is locale-sensitive through the process-global locale set by `mjb_locale_set`. The
  default locale is `MJB_LOCALE_EN`. Turkish and Azerbaijani apply dotted-I casing rules and Turkic
  case-folding mappings. Lithuanian applies dot-above casing rules; case folding remains the default
  non-Turkic mapping.
- `mjb_string_compare` and `mjb_collation_key` use DUCET without locale collation tailoring.
  `mjb_collation_mode` only selects the UCA variable weighting strategy.
- `mjb_display_width` uses its `mjb_width_context` argument to choose how East Asian Width
  `Ambiguous` characters are counted. `mjb_codepoint_east_asian_width` returns the Unicode 17.0.0
  property value without tailoring.
- Normalization, NFKC case folding, bidirectional processing, grapheme/word/sentence/line breaking,
  identifier validation, confusable skeletons, and emoji sequence checks are not locale-tailored by
  Mojibake.

# Unicode conformance inventory

Mojibake interprets Unicode text only through the public APIs and supported UTF encodings listed in
this documentation. It does not implement rendering, font shaping, locale collation tailoring, or
higher-level protocol behavior beyond the documented locale-sensitive casing and display-width
policy. The table below maps the advertised Unicode algorithm and data claims to their Unicode
17.0.0 reference and test evidence.

| Claim | Public surface | Unicode reference | Evidence |
| ----- | -------------- | ----------------- | -------- |
| Unicode Character Database data and derived properties | `mjb_codepoint_character`, `mjb_codepoint_property_binary`, `mjb_codepoint_property_int`, `mjb_codepoint_script_extensions`, script/block/category/numeric helpers | [UAX #44](https://www.unicode.org/reports/tr44/tr44-36.html), [UAX #24](https://www.unicode.org/reports/tr24/tr24-39.html), UCD 17.0.0 | Generated from UCD data files including `UnicodeData.txt`, `Blocks.txt`, `Scripts.txt`, `ScriptExtensions.txt`, `PropList.txt`, `DerivedCoreProperties.txt`, `PropertyAliases.txt`, and `PropertyValueAliases.txt`; every explicit Script_Extensions range is covered by `tests/properties.c`. |
| Unicode Normalization Forms and quick check | `mjb_normalize`, `mjb_string_is_normalized` | [UAX #15](https://www.unicode.org/reports/tr15/tr15-57.html) | `NormalizationTest.txt`, `DerivedNormalizationProps.txt`, `tests/normalization.c`, and `tests/quick-check.c`. |
| Default case conversion and caseless matching | `mjb_case`, `mjb_nfkc_casefold`, simple codepoint case helpers | [Unicode Core Section 3.13](https://www.unicode.org/versions/Unicode17.0.0/core-spec/chapter-3/#G33992), [UAX #29](https://www.unicode.org/reports/tr29/tr29-47.html) for titlecase word boundaries, [UAX #31](https://www.unicode.org/reports/tr31/tr31-43.html) for identifier caseless matching | `SpecialCasing.txt`, `CaseFolding.txt`, `WordBreakTest.txt`, every explicit `NFKC_CF` mapping in `DerivedNormalizationProps.txt`, `tests/special-case.c`, `tests/case.c`, `tests/normalization.c`, and `tests/break-word.c`. |
| Grapheme, word, and sentence boundaries | `mjb_break_grapheme_cluster`, `mjb_break_word`, `mjb_break_sentence`, related truncation helpers | [UAX #29](https://www.unicode.org/reports/tr29/tr29-47.html) | `GraphemeBreakTest.txt`, `WordBreakTest.txt`, `SentenceBreakTest.txt`, `tests/segmentation.c`, `tests/break-word.c`, and `tests/break-sentence.c`. |
| Line breaking | `mjb_break_line` | [UAX #14](https://www.unicode.org/reports/tr14/tr14-55.html) | `LineBreakTest.txt` and `tests/break-line.c`. |
| Bidirectional Algorithm | `mjb_bidi_resolve`, `mjb_bidi_reorder_line`, `mjb_bidi_line_runs` | [UAX #9](https://www.unicode.org/reports/tr9/tr9-51.html) | `BidiCharacterTest.txt`, `BidiTest.txt`, `tests/bidi.c`, and `tests/bidi-class.c`. |
| Unicode Collation Algorithm, DUCET | `mjb_string_compare`, `mjb_collation_key` | [UTS #10](https://www.unicode.org/reports/tr10/tr10-53.html) | `CollationTest_NON_IGNORABLE.txt`, `CollationTest_SHIFTED.txt`, and `tests/collation.c`; surrogate-code-point rows are filtered because public string input rejects ill-formed surrogate code points. |
| Unicode identifiers and pattern syntax data | ID/XID/pattern predicates and `mjb_string_is_identifier` | [UAX #31](https://www.unicode.org/reports/tr31/tr31-43.html) | UCD ID/XID and pattern properties from `DerivedCoreProperties.txt` and `PropList.txt`; covered by `tests/identifier.c`. |
| Confusable skeleton generation and matching | `mjb_confusable_skeleton`, `mjb_string_is_confusable` | [UTS #39](https://www.unicode.org/reports/tr39/tr39-32.html) | Every mapping in `confusables.txt`, every pair in `intentional.txt`, and `tests/security.c`. |
| Emoji properties and sequence data | Emoji property predicates, `mjb_string_emoji_sequence`, RGI checks | [UTS #51](https://www.unicode.org/reports/tr51/tr51-29.html) | `emoji-data.txt`, `emoji-sequences.txt`, `emoji-zwj-sequences.txt`, `emoji-variation-sequences.txt`, `emoji-test.txt`, and `tests/emoji.c`. |
| East Asian Width property | `mjb_codepoint_east_asian_width`; consumed by `mjb_display_width` | [UAX #11](https://www.unicode.org/reports/tr11/tr11-44.html) | `EastAsianWidth.txt`, `tests/east-asian-width.c`, and property tests; display column counts are a documented local policy over that property. |
