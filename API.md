## API

Return the codepoint character

```c
bool mjb_codepoint_character(mjb_codepoint codepoint, mjb_character *character);
```

Normalize a string to NFC/NFKC/NFD/NFKD form

```c
bool mjb_normalize(const char *buffer, size_t size, mjb_encoding encoding, mjb_normalization form, mjb_result *result);
```

Return the next character from a string

```c
bool mjb_next_character(const char *buffer, size_t size, mjb_encoding encoding, mjb_next_character_fn fn);
```

Check if a string is normalized to NFC/NFKC/NFD/NFKD form

```c
mjb_quick_check_result mjb_string_is_normalized(const char *buffer, size_t size, mjb_encoding encoding, mjb_normalization form);
```

Filter a string to remove invalid characters

```c
bool mjb_string_filter(const char *buffer, size_t size, mjb_encoding encoding, mjb_encoding output_encoding, mjb_filter filters, mjb_result *result);
```

Return if a codepoint has a property

```c
bool mjb_codepoint_has_property(mjb_codepoint codepoint, mjb_property property);
```

Return the string encoding (the most probable)

```c
mjb_encoding mjb_string_encoding(const char *buffer, size_t size);
```

Return true if the string is encoded in UTF-8

```c
bool mjb_string_is_utf8(const char *buffer, size_t size);
```

Return true if the string is encoded in UTF-16BE or UTF-16LE

```c
bool mjb_string_is_utf16(const char *buffer, size_t size);
```

Return true if the string is encoded in ASCII

```c
bool mjb_string_is_ascii(const char *buffer, size_t size);
```

Encode a codepoint to a string

```c
unsigned int mjb_codepoint_encode(mjb_codepoint codepoint, char *buffer, size_t size, mjb_encoding encoding);
```

Convert from an encoding to another

```c
bool mjb_string_convert_encoding(const char *buffer, size_t size, mjb_encoding encoding, mjb_encoding output_encoding, mjb_result *result);
```

Return the length of a string

```c
size_t mjb_strnlen(const char *buffer, size_t max_length, mjb_encoding encoding);
```

Compare two strings

```c
int mjb_string_compare(const char *s1, size_t s1_length, mjb_encoding s1_encoding, const char *s2, size_t s2_length, mjb_encoding s2_encoding);
```

Change string case

```c
char *mjb_case(const char *buffer, size_t size, mjb_case_type type, mjb_encoding encoding);
```

Return true if the codepoint is valid

```c
bool mjb_codepoint_is_valid(mjb_codepoint codepoint);
```

Return true if the codepoint is graphic

```c
bool mjb_codepoint_is_graphic(mjb_codepoint codepoint);
```

Return true if the codepoint is combining

```c
bool mjb_codepoint_is_combining(mjb_codepoint codepoint);
```

Return if the codepoint is an hangul L

```c
bool mjb_codepoint_is_hangul_l(mjb_codepoint codepoint);
```

Return if the codepoint is an hangul V

```c
bool mjb_codepoint_is_hangul_v(mjb_codepoint codepoint);
```

Return if the codepoint is an hangul T

```c
bool mjb_codepoint_is_hangul_t(mjb_codepoint codepoint);
```

Return if the codepoint is an hangul jamo

```c
bool mjb_codepoint_is_hangul_jamo(mjb_codepoint codepoint);
```

Return if the codepoint is an hangul syllable

```c
bool mjb_codepoint_is_hangul_syllable(mjb_codepoint codepoint);
```

Return if the codepoint is CJK ideograph

```c
bool mjb_codepoint_is_cjk_ideograph(mjb_codepoint codepoint);
```

Return true if the category is graphic

```c
bool mjb_category_is_graphic(mjb_category category);
```

Return true if the category is combining

```c
bool mjb_category_is_combining(mjb_category category);
```

Return the character block

```c
bool mjb_codepoint_block(mjb_codepoint codepoint, mjb_block_info *block);
```

Return the codepoint lowercase codepoint

```c
mjb_codepoint mjb_codepoint_to_lowercase(mjb_codepoint codepoint);
```

Return the codepoint uppercase codepoint

```c
mjb_codepoint mjb_codepoint_to_uppercase(mjb_codepoint codepoint);
```

Return the codepoint titlecase codepoint

```c
mjb_codepoint mjb_codepoint_to_titlecase(mjb_codepoint codepoint);
```

Unicode line break algorithm

```c
mjb_line_break *mjb_break_line(const char *buffer, size_t size, mjb_encoding encoding, size_t *output_size);
```

Word and grapheme cluster breaking

```c
bool mjb_segmentation(const char *buffer, size_t size, mjb_encoding encoding);
```

Return the plane of the codepoint

```c
mjb_plane mjb_codepoint_plane(mjb_codepoint codepoint);
```

Return true if the plane is valid

```c
bool mjb_plane_is_valid(mjb_plane plane);
```

Return the name of a plane, NULL if the place specified is not valid

```c
const char *mjb_plane_name(mjb_plane plane, bool abbreviation);
```

Return the emoji properties

```c
bool mjb_codepoint_emoji(mjb_codepoint codepoint, mjb_emoji_properties *emoji);
```

Return hangul syllable name

```c
bool mjb_hangul_syllable_name(mjb_codepoint codepoint, char *buffer, size_t size);
```

Hangul syllable decomposition

```c
bool mjb_hangul_syllable_decomposition(mjb_codepoint codepoint, mjb_codepoint *codepoints);
```

Hangul syllable composition

```c
size_t mjb_hangul_syllable_composition(mjb_buffer_character *characters, size_t characters_len);
```

Output the current library version (MJB_VERSION)

```c
const char *mjb_version(void);
```

Output the current library version number (MJB_VERSION_NUMBER)

```c
unsigned int mjb_version_number(void);
```

Output the current supported unicode version (MJB_UNICODE_VERSION)

```c
const char *mjb_unicode_version(void);
```

Initialize the library. Not needed to be called

```c
bool mjb_initialize(void);
```

Initialize the library with custom values. Not needed to be called

```c
bool mjb_initialize_v2(mjb_alloc_fn alloc_fn, mjb_realloc_fn realloc_fn, mjb_free_fn free_fn, const char *db, size_t db_size);
```

Shutdown the library. Not needed to be called

```c
void mjb_shutdown(void);
```

Allocate and zero memory

```c
void *mjb_alloc(size_t size);
```

Reallocate memory

```c
void *mjb_realloc(void *ptr, size_t new_size);
```

Free memory

```c
void mjb_free(void *ptr);
```