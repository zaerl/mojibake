# Mojibake

Mojibake is a low-level Unicode library written in C99. THIS IS AN EXPERIMENTAL LIBRARY. DO NOT USE.

## API

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

Output the current library version (MJB_VERSION)
```c
char *mjb_version(void);
```

Output the current library version number (MJB_VERSION_NUMBER)
```c
unsigned int mjb_version_number(void);
```

Output the current supported unicode version (MJB_UNICODE_VERSION)
```c
char *mjb_unicode_version(void);
```

Return true if the plane is valid
```c
bool mjb_plane_is_valid(mjb_plane plane);
```

Return the name of a plane, NULL if the place specified is not valid
```c
const char *mjb_plane_name(mjb_plane plane, bool abbreviation);
```

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

Encode a codepoint to a string
```c
bool mjb_codepoint_encode(mjb_codepoint codepoint, char *buffer, size_t size, mjb_encoding encoding);
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
bool mjb_codepoint_is(mjb_codepoint codepoint, mjb_category category);
```

Return true if the codepoint has the block
```c
bool mjb_codepoint_block_is(mjb_codepoint codepoint, mjb_block block);
```

Return true if the codepoint is graphic
```c
bool mjb_codepoint_is_graphic(mjb_codepoint codepoint);
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
void *mjb_normalize(void *source, size_t source_size, size_t *output_size, mjb_encoding encoding, mjb_normalization form);
```
