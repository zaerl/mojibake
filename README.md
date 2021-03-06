# Mojibake

Mojibake is a low-level Unicode library written in C99. THIS IS AN EXPERIMENTAL LIBRARY. DO NOT USE.

## API

Initialize the library
```c
bool mjb_initialize(mojibake **mjb);
```

Initialize the library with custom values
```c
bool mjb_initialize_v2(mojibake **mjb, mjb_alloc_fn alloc_fn, mjb_realloc_fn realloc_fn, mjb_free_fn free_fn);
```

The library is ready
```c
bool mjb_ready(mojibake *mjb);
```

Allocate and zero memory
```c
void *mjb_alloc(mojibake *mjb, size_t size);
```

Reallocate memory
```c
void *mjb_realloc(mojibake *mjb, void *ptr, size_t new_size);
```

Free memory
```c
void mjb_free(mojibake *mjb, void *ptr);
```

Output the current library version (MJB_VERSION)
```c
char *mjb_version();
```

Output the current library version number (MJB_VERSION_NUMBER)
```c
unsigned int mjb_version_number();
```

Output the current supported unicode version (MJB_UNICODE_VERSION)
```c
char *mjb_unicode_version();
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

Return true if the codepoint is valid
```c
bool mjb_codepoint_is_valid(mojibake *mjb, mjb_codepoint codepoint);
```

Return the codepoint character
```c
bool mjb_codepoint_character(mojibake *mjb, mjb_character *character, mjb_codepoint codepoint);
```

Return true if the codepoint has the category
```c
bool mjb_codepoint_is(mojibake *mjb, mjb_codepoint codepoint, mjb_category category);
```

Return true if the codepoint is graphic
```c
bool mjb_codepoint_is_graphic(mojibake *mjb, mjb_codepoint codepoint);
```

Return the codepoint lowercase codepoint
```c
mjb_codepoint mjb_codepoint_to_lowercase(mojibake *mjb, mjb_codepoint codepoint);
```

Return the codepoint uppercase codepoint
```c
mjb_codepoint mjb_codepoint_to_uppercase(mojibake *mjb, mjb_codepoint codepoint);
```

Return the codepoint titlecase codepoint
```c
mjb_codepoint mjb_codepoint_to_titlecase(mojibake *mjb, mjb_codepoint codepoint);
```

Normalize a string
```c
void *mjb_normalize(mojibake *mjb, void *source, size_t source_size, size_t *output_size, mjb_encoding encoding, mjb_normalization form);
```
