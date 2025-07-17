# Mojibake

> **Note:**
> This project is an **experimental library**. It is not designed for production
> use, and there may be bugs, limitations, or incomplete features. Use at your
> own discretion, and feel free to collaborate

Mojibake is a low-level Unicode library written in C99.

## API

Return the string encoding (the most probable)

```c
mjb_encoding mjb_string_encoding(const char *buffer, size_t size);
```

Return true if the string is encoded in UTF-8

```c
bool mjb_string_is_utf8(const char *buffer, size_t size);
```

Return true if the string is encoded in ASCII

```c
bool mjb_string_is_ascii(const char *buffer, size_t size);
```

Return next codepoint in the string

```c
mjb_codepoint mjb_string_next_codepoint(const char *buffer, size_t size, size_t *next);
```

Encode a codepoint to a string

```c
unsigned int mjb_codepoint_encode(mjb_codepoint codepoint, char *buffer, size_t size, mjb_encoding encoding);
```

Return true if the codepoint is valid

```c
bool mjb_codepoint_is_valid(mjb_codepoint codepoint);
```

Return the codepoint character

```c
bool mjb_codepoint_character(mjb_character *character, mjb_codepoint codepoint);
```

Return true if the codepoint has the category

```c
bool mjb_codepoint_category_is(mjb_codepoint codepoint, mjb_category category);
```

Return true if the codepoint is graphic

```c
bool mjb_codepoint_is_graphic(mjb_codepoint codepoint);
```

Return true if the codepoint is combining

```c
bool mjb_codepoint_is_combining(mjb_codepoint codepoint);
```

Return true if the category is combining

```c
bool mjb_category_is_combining(mjb_category category);
```

Return the character block

```c
bool mjb_character_block(mjb_codepoint codepoint, mjb_codepoint_block *block);
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

Normalize a string

```c
char *mjb_normalize(char *buffer, size_t size, size_t *output_size, mjb_encoding encoding, mjb_normalization form);
```

Get the next character from the string

```c
bool mjb_next_character(const char *buffer, size_t size, mjb_encoding encoding, mjb_next_character_fn fn);
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

Return hangul syllable name

```c
bool mjb_hangul_syllable_name(mjb_codepoint codepoint, char *buffer, size_t size);
```

Hangul syllable decomposition

```c
bool mjb_hangul_syllable_decomposition(mjb_codepoint codepoint, mjb_codepoint *codepoints);
```

Return if the codepoint is an hangul syllable

```c
bool mjb_codepoint_is_hangul_syllable(mjb_codepoint codepoint);
```

Return if the codepoint is CJK ideograph

```c
bool mjb_codepoint_is_cjk_ideograph(mjb_codepoint codepoint);
```

Sort

```c
void mjb_sort(mjb_character arr[], size_t size);
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
bool mjb_initialize_v2(mjb_alloc_fn alloc_fn, mjb_realloc_fn realloc_fn, mjb_free_fn free_fn);
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
