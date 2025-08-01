/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#pragma once

#ifndef MJB_MOJIBAKE_H
#define MJB_MOJIBAKE_H

#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 201112L
#error "C11 or a later version is required"
#endif

#if defined(_WIN32) || defined(_WIN64)
#error "Windows is not supported"
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "sqlite3/sqlite3.h"
#include "unicode.h"
#include "utf8.h"

#ifdef __cplusplus
extern "C" {
#endif

// Static assertions for important constants
_Static_assert(sizeof(uint32_t) == 4, "uint32_t must be 4 bytes");
_Static_assert(sizeof(char) == 1, "char must be 1 byte");

#define MJB_VERSION          "1.0.0"
#define MJB_VERSION_NUMBER   0x100 // MAJOR << 8 && MINOR << 4 && REVISION
#define MJB_VERSION_MAJOR    1
#define MJB_VERSION_MINOR    0
#define MJB_VERSION_REVISION 0

#define MJB_UNICODE_VERSION       "16.0"
#define MJB_UNICODE_VERSION_MAJOR 16
#define MJB_UNICODE_VERSION_MINOR 0

#ifndef MJB_EXTERN
#define MJB_EXTERN extern
#endif

#ifndef MJB_EXPORT
#define MJB_EXPORT __attribute__((visibility("default")))
#endif

// Modern compiler attributes with fallbacks
#if defined(__GNUC__) || defined(__clang__)
    #define MJB_NODISCARD __attribute__((warn_unused_result))
    #define MJB_NONNULL(...) __attribute__((nonnull(__VA_ARGS__)))
    #define MJB_PURE __attribute__((pure))
    #define MJB_CONST __attribute__((const))
#elif defined(__cplusplus) && __cplusplus >= 201703L
    #define MJB_NODISCARD [[nodiscard]]
    #define MJB_NONNULL(...)
    #define MJB_PURE
    #define MJB_CONST
#else
    #define MJB_NODISCARD
    #define MJB_NONNULL(...)
    #define MJB_PURE
    #define MJB_CONST
#endif

// See c standard memory allocation functions
typedef void *(*mjb_alloc_fn)(size_t size);
typedef void *(*mjb_realloc_fn)(void *ptr, size_t new_size);
typedef void (*mjb_free_fn)(void *ptr);

/**
 * Mojibake is represented by a pointer to an instance of the opaque structure
 * named "mojibake". The [mjb_initialize()] and [mjb_initialize_v2()] functions
 * are its constructor. Every function accept an instance to this allocated
 * pointer. This is used to ensure reentrancy.
 */
typedef struct mojibake {
    bool ok;
    mjb_alloc_fn memory_alloc;
    mjb_realloc_fn memory_realloc;
    mjb_free_fn memory_free;
    sqlite3 *db;
    sqlite3_stmt *stmt_get_codepoint;
    sqlite3_stmt *stmt_get_block;
    sqlite3_stmt *stmt_is_combining;
    sqlite3_stmt *stmt_decompose;
    sqlite3_stmt *stmt_compatibility_decompose;
    sqlite3_stmt *stmt_compose;
    sqlite3_stmt *stmt_compatibility_compose;
} mojibake;

/**
 * A unicode codepoint, a value in the range 0 to 0x10FFFF
 * [see: https://www.unicode.org/glossary/#code_point]
 */
typedef uint32_t mjb_codepoint;

#define MJB_CODEPOINT_MIN         0x0
#define MJB_CODEPOINT_MAX         0x10FFFF // Maximum valid unicode code point
#define MJB_CODEPOINT_REPLACEMENT 0xFFFD   // The character used when there is invalid data
#define MJB_CODEPOINT_NOT_VALID   0x110000 // Not a valid codepoint

// Corean Hangul Syllables
#define MJB_CODEPOINT_HANGUL_START 0xAC00
#define MJB_CODEPOINT_HANGUL_END   0xD7A3
#define MJB_CODEPOINT_CHOSEONG_BASE 0x1100
#define MJB_CODEPOINT_JUNGSEONG_BASE 0x1161
#define MJB_CODEPOINT_JONGSEONG_BASE 0x11A7

// CJK Ideographs
#define MJB_CODEPOINT_CJK_IDEOGRAPH_START 0x4E00
#define MJB_CODEPOINT_CJK_IDEOGRAPH_END   0x9FFF
#define MJB_CODEPOINT_CJK_EXTENSION_A_START 0x3400
#define MJB_CODEPOINT_CJK_EXTENSION_A_END 0x4DBF
#define MJB_CODEPOINT_CJK_EXTENSION_B_START 0x20000
#define MJB_CODEPOINT_CJK_EXTENSION_B_END 0x2A6DF
#define MJB_CODEPOINT_CJK_EXTENSION_C_START 0x2A700
#define MJB_CODEPOINT_CJK_EXTENSION_C_END 0x2B739
#define MJB_CODEPOINT_CJK_EXTENSION_D_START 0x2B740
#define MJB_CODEPOINT_CJK_EXTENSION_D_END 0x2B81D
#define MJB_CODEPOINT_CJK_EXTENSION_E_START 0x2B820
#define MJB_CODEPOINT_CJK_EXTENSION_E_END 0x2CEA1
#define MJB_CODEPOINT_CJK_EXTENSION_F_START 0x2CEB0
#define MJB_CODEPOINT_CJK_EXTENSION_F_END 0x2EBE0
#define MJB_CODEPOINT_CJK_EXTENSION_I_START 0x2EBF0
#define MJB_CODEPOINT_CJK_EXTENSION_I_END 0x2EE5D

// Tangut Ideographs
#define MJB_CODEPOINT_TANGUT_IDEOGRAPH_START 0x17000
#define MJB_CODEPOINT_TANGUT_IDEOGRAPH_END 0x187F7

// Numeric values, to be used when the decimal and digit mjb_character fields are not valid
#define MJB_NUMBER_NOT_VALID -1

/**
 * Unicode encoding
 * [see: https://www.unicode.org/glossary/#character_encoding_scheme]
 */
typedef enum mjb_encoding {
    MJB_ENCODING_UNKNOWN =   0x0,
    MJB_ENCODING_ASCII =     0x1,
    MJB_ENCODING_UTF_8 =     0x2,
    MJB_ENCODING_UTF_16 =    0x4,
    MJB_ENCODING_UTF_16_BE = 0x8,
    MJB_ENCODING_UTF_16_LE = 0x10,
    MJB_ENCODING_UTF_32 =    0x20,
    MJB_ENCODING_UTF_32_BE = 0x40,
    MJB_ENCODING_UTF_32_LE = 0x80
} mjb_encoding;

/**
 * Normalization form
 * [see: https://www.unicode.org/glossary/#normalization_form]
 */
typedef enum mjb_normalization {
    MJB_NORMALIZATION_NFC,  // Canonical decomposition followed by canonical composition
    MJB_NORMALIZATION_NFD,  // Canonical decomposition without recomposition
    MJB_NORMALIZATION_NFKC, // Compatibility decomposition followed by canonical composition
    MJB_NORMALIZATION_NFKD  // Compatibility decomposition without recomposition
} mjb_normalization;

/**
 * Unicode block
 * [see: https://www.unicode.org/glossary/#block]
 */
typedef struct mjb_codepoint_block {
    mjb_block id;
    char name[128];
    uint32_t start;
    uint32_t end;
} mjb_codepoint_block;

/**
 * A unicode character
 * [see: https://www.unicode.org/glossary/#character]
 */
typedef struct mjb_character {
    mjb_codepoint codepoint;
    char name[128];
    mjb_category category;
    mjb_canonical_combining_class combining;
    unsigned short bidirectional;
    mjb_decomposition decomposition;
    int decimal;
    int digit;
    char numeric[16];
    bool mirrored;
    mjb_codepoint uppercase;
    mjb_codepoint lowercase;
    mjb_codepoint titlecase;
} mjb_character;

typedef enum mjb_next_character_type {
    MJB_NEXT_CHAR_NONE = 0x0,
    MJB_NEXT_CHAR_FIRST = 0x1,
    MJB_NEXT_CHAR_LAST = 0x2
} mjb_next_character_type;

typedef bool (*mjb_next_character_fn)(mjb_character *character, mjb_next_character_type type);

// Return the string encoding (the most probable)
MJB_PURE mjb_encoding mjb_string_encoding(const char *buffer, size_t size);

// Return true if the string is encoded in UTF-8
MJB_PURE bool mjb_string_is_utf8(const char *buffer, size_t size);

// Return true if the string is encoded in ASCII
MJB_PURE bool mjb_string_is_ascii(const char *buffer, size_t size);

// Return next codepoint in the string
MJB_PURE mjb_codepoint mjb_string_next_codepoint(const char *buffer, size_t size, size_t *next);

// Encode a codepoint to a string
unsigned int mjb_codepoint_encode(mjb_codepoint codepoint, char *buffer, size_t size, mjb_encoding encoding);

// Return true if the codepoint is valid
MJB_CONST bool mjb_codepoint_is_valid(mjb_codepoint codepoint);

// Return the codepoint character
MJB_NONNULL(1) bool mjb_codepoint_character(mjb_character *character, mjb_codepoint codepoint);

// Return true if the codepoint has the category
MJB_CONST bool mjb_codepoint_category_is(mjb_codepoint codepoint, mjb_category category);

// Return true if the codepoint is graphic
MJB_CONST bool mjb_codepoint_is_graphic(mjb_codepoint codepoint);

// Return true if the codepoint is combining
MJB_CONST bool mjb_codepoint_is_combining(mjb_codepoint codepoint);

// Return true if the category is combining
MJB_CONST bool mjb_category_is_combining(mjb_category category);

// Return the character block
MJB_CONST bool mjb_character_block(mjb_codepoint codepoint, mjb_codepoint_block *block);

// Return the codepoint lowercase codepoint
MJB_CONST mjb_codepoint mjb_codepoint_to_lowercase(mjb_codepoint codepoint);

// Return the codepoint uppercase codepoint
MJB_CONST mjb_codepoint mjb_codepoint_to_uppercase(mjb_codepoint codepoint);

// Return the codepoint titlecase codepoint
MJB_CONST mjb_codepoint mjb_codepoint_to_titlecase(mjb_codepoint codepoint);

// Normalize a string
MJB_NONNULL(1, 3) char *mjb_normalize(const char *buffer, size_t size, size_t *output_size, mjb_encoding encoding, mjb_normalization form);

// Get the next character from the string
MJB_NONNULL(1, 4) bool mjb_next_character(const char *buffer, size_t size, mjb_encoding encoding, mjb_next_character_fn fn);

// Return the plane of the codepoint
MJB_CONST mjb_plane mjb_codepoint_plane(mjb_codepoint codepoint);

// Return true if the plane is valid
MJB_CONST bool mjb_plane_is_valid(mjb_plane plane);

// Return the name of a plane, NULL if the place specified is not valid
MJB_CONST const char *mjb_plane_name(mjb_plane plane, bool abbreviation);

// Return hangul syllable name
MJB_NONNULL(2) bool mjb_hangul_syllable_name(mjb_codepoint codepoint, char *buffer, size_t size);

// Hangul syllable decomposition
MJB_NONNULL(2) bool mjb_hangul_syllable_decomposition(mjb_codepoint codepoint, mjb_codepoint *codepoints);

// Return if the codepoint is an hangul syllable
MJB_CONST bool mjb_codepoint_is_hangul_syllable(mjb_codepoint codepoint);

// Return if the codepoint is CJK ideograph
MJB_CONST bool mjb_codepoint_is_cjk_ideograph(mjb_codepoint codepoint);

// Sort
MJB_NONNULL(1) void mjb_sort(mjb_character arr[], size_t size);

// Output the current library version (MJB_VERSION)
MJB_CONST const char *mjb_version(void);

// Output the current library version number (MJB_VERSION_NUMBER)
MJB_CONST unsigned int mjb_version_number(void);

// Output the current supported unicode version (MJB_UNICODE_VERSION)
MJB_CONST const char *mjb_unicode_version(void);

// Initialize the library. Not needed to be called
MJB_NODISCARD bool mjb_initialize(void);

// Initialize the library with custom values. Not needed to be called
MJB_NODISCARD bool mjb_initialize_v2(mjb_alloc_fn alloc_fn, mjb_realloc_fn realloc_fn, mjb_free_fn free_fn, sqlite3_mem_methods *db_mem_methods);

// Shutdown the library. Not needed to be called
void mjb_shutdown(void);

// Allocate and zero memory
MJB_NODISCARD void *mjb_alloc(size_t size);

// Reallocate memory
MJB_NODISCARD MJB_NONNULL(1) void *mjb_realloc(void *ptr, size_t new_size);

// Free memory
MJB_NONNULL(1) void mjb_free(void *ptr);

#ifdef __cplusplus
}
#endif

#endif // MJB_MOJIBAKE_H
