## API

Return the codepoint character

```c
bool mjb_codepoint_character(mjb_codepoint codepoint, mjb_character *character);
```

Normalize a string to NFC/NFKC/NFD/NFKD form

```c
bool mjb_normalize(const char *buffer, size_t size, mjb_encoding encoding, mjb_normalization form, mjb_encoding output_encoding, mjb_result *result);
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
bool mjb_codepoint_has_property(mjb_codepoint codepoint, mjb_property property, uint8_t *value);
```

Return all properties of a codepoint

```c
bool mjb_codepoint_properties(mjb_codepoint codepoint, uint8_t *buffer);
```

Return a property value

```c
uint8_t mjb_codepoint_property(uint8_t *properties, mjb_property property);
```

Return the script of a codepoint

```c
mjb_script mjb_codepoint_script(mjb_codepoint codepoint);
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

Compare two strings using UCA

```c
int mjb_string_compare(const char *s1, size_t s1_length, const char *s2, size_t s2_length, mjb_encoding encoding, mjb_collation_mode mode);
```

Generate a UCA sort key for a string

```c
bool mjb_collation_key(const char *buffer, size_t size, mjb_encoding encoding, mjb_collation_mode mode, mjb_result*result);
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

Return if the codepoint is CJK extension

```c
bool mjb_codepoint_is_cjk_ext(mjb_codepoint codepoint);
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
mjb_break_type mjb_break_line(const char *buffer, size_t size, mjb_encoding encoding, mjb_next_line_state *state);
```

Word cluster breaking

```c
mjb_break_type mjb_break_word(const char *buffer, size_t size, mjb_encoding encoding, mjb_next_word_state *state);
```

Return the number of bytes that form the first max_segments word-break segments

```c
size_t mjb_truncate_word(const char *buffer, size_t size, mjb_encoding encoding, size_t max_segments);
```

Return the number of bytes whose word-break segments fit within max_columns display columns

```c
size_t mjb_truncate_word_width(const char *buffer, size_t size, mjb_encoding encoding, mjb_width_context context, size_t max_columns);
```

Sentence boundaries breaking

```c
mjb_break_type mjb_break_sentence(const char *buffer, size_t size, mjb_encoding encoding, mjb_next_sentence_state *state);
```

Grapheme cluster breaking

```c
mjb_break_type mjb_segmentation(const char *buffer, size_t size, mjb_encoding encoding, mjb_next_state *state);
```

Return the number of bytes that form the first max_graphemes grapheme cluster segments

```c
size_t mjb_truncate(const char *buffer, size_t size, mjb_encoding encoding, size_t max_graphemes);
```

Return the number of bytes whose grapheme clusters fit within max_columns display columns

```c
size_t mjb_truncate_width(const char *buffer, size_t size, mjb_encoding encoding, mjb_width_context context, size_t max_columns);
```

Resolve bidirectional text (TR9) for a paragraph

```c
bool mjb_bidi_resolve(const char *buffer, size_t size, mjb_encoding encoding, mjb_direction direction, mjb_bidi_paragraph *result);
```

Free a bidi paragraph allocated by mjb_bidi_resolve

```c
void mjb_bidi_free(mjb_bidi_paragraph *paragraph);
```

Reorder a line visually (L1-L4); visual_order is caller-allocated

```c
bool mjb_bidi_reorder_line(const mjb_bidi_paragraph *paragraph, size_t line_start, size_t line_end, size_t *visual_order);
```

Compute visual level runs; pass runs=NULL to count first

```c
bool mjb_bidi_line_runs(const mjb_bidi_paragraph *paragraph, const size_t *visual_order, size_t count, mjb_bidi_run *runs, size_t *run_count);
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

Return true if the codepoint is a valid Unicode identifier start (UAX#31 ID_Start)

```c
bool mjb_codepoint_is_id_start(mjb_codepoint codepoint);
```

Return true if the codepoint is a valid Unicode identifier continuation (UAX#31 ID_Continue)

```c
bool mjb_codepoint_is_id_continue(mjb_codepoint codepoint);
```

Return true if the codepoint is a valid NFKC identifier start (UAX#31 XID_Start)

```c
bool mjb_codepoint_is_xid_start(mjb_codepoint codepoint);
```

Return true if the codepoint is a valid NFKC identifier continuation (UAX#31 XID_Continue)

```c
bool mjb_codepoint_is_xid_continue(mjb_codepoint codepoint);
```

Return true if the codepoint is reserved for use in patterns (UAX#31 Pattern_Syntax)

```c
bool mjb_codepoint_is_pattern_syntax(mjb_codepoint codepoint);
```

Return true if the codepoint is pattern whitespace (UAX#31 Pattern_White_Space)

```c
bool mjb_codepoint_is_pattern_white_space(mjb_codepoint codepoint);
```

Return true if the string is a valid Unicode identifier (UAX#31)

```c
bool mjb_string_is_identifier(const char *buffer, size_t size, mjb_encoding encoding, mjb_identifier_profile profile);
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

Return the east asian width of a codepoint

```c
bool mjb_codepoint_east_asian_width(mjb_codepoint codepoint, mjb_east_asian_width *width);
```

Return the display width of a string

```c
bool mjb_display_width(const char *buffer, size_t size, mjb_encoding encoding, mjb_width_context context, size_t *width);
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