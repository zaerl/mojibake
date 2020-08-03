/**
 * The mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#ifndef MB_H
#define MB_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/*
 A unicode codepoint
 [see: https://www.unicode.org/glossary/#code_point]
 */
typedef uint32_t mb_codepoint;

#define MB_CODEPOINT_MIN 0x0
#define MB_CODEPOINT_MAX 0x10FFFF

/*
 A unicode codepoint general category
 [see: https://www.unicode.org/glossary/#general_category]
 */
typedef struct mb_character {
    mb_codepoint codepoint;
    int general_category;
    char* name;
} mb_character;

/*
 A unicode block
 [see: https://www.unicode.org/glossary/#block]
 */
typedef struct mb_block {
    uint32_t start;
    uint32_t size;
    char* name;
} mb_block;

/* General categories */
#define MB_GENERAL_CATEGORY_LU  0 /* Lu Letter, Uppercase */
#define MB_GENERAL_CATEGORY_LL  1 /* Ll Letter, Lowercase */
#define MB_GENERAL_CATEGORY_LT  2 /* Lt Letter, Titlecase */
#define MB_GENERAL_CATEGORY_LM  3 /* Lm Letter, Modifier */
#define MB_GENERAL_CATEGORY_LO  4 /* Lo Letter, Other */
#define MB_GENERAL_CATEGORY_MN  5 /* Mn Mark, Non-Spacing */
#define MB_GENERAL_CATEGORY_MC  6 /* Mc Mark, Spacing Combining */
#define MB_GENERAL_CATEGORY_ME  7 /* Me Mark, Enclosing */
#define MB_GENERAL_CATEGORY_ND  8 /* Nd Number, Decimal Digit */
#define MB_GENERAL_CATEGORY_NL  9 /* Nl Number, Letter */
#define MB_GENERAL_CATEGORY_NO 10 /* No Number, Other */
#define MB_GENERAL_CATEGORY_PC 11 /* Pc Punctuation, Connector */
#define MB_GENERAL_CATEGORY_PD 12 /* Pd Punctuation, Dash */
#define MB_GENERAL_CATEGORY_PS 13 /* Ps Punctuation, Open */
#define MB_GENERAL_CATEGORY_PE 14 /* Pe Punctuation, Close */
#define MB_GENERAL_CATEGORY_PI 15 /* Pi Punctuation, Initial quote (may behave like Ps or Pe depending on usage) */
#define MB_GENERAL_CATEGORY_PF 16 /* Pf Punctuation, Final quote (may behave like Ps or Pe depending on usage) */
#define MB_GENERAL_CATEGORY_PO 17 /* Po Punctuation, Other */
#define MB_GENERAL_CATEGORY_SM 18 /* Sm Symbol, Math */
#define MB_GENERAL_CATEGORY_SC 19 /* Sc Symbol, Currency */
#define MB_GENERAL_CATEGORY_SK 20 /* Sk Symbol, Modifier */
#define MB_GENERAL_CATEGORY_SO 21 /* So Symbol, Other */
#define MB_GENERAL_CATEGORY_ZS 22 /* Zs Separator, Space */
#define MB_GENERAL_CATEGORY_ZL 23 /* Zl Separator, Line */
#define MB_GENERAL_CATEGORY_ZP 24 /* Zp Separator, Paragraph */
#define MB_GENERAL_CATEGORY_CC 25 /* Cc Other, Control */
#define MB_GENERAL_CATEGORY_CF 26 /* Cf Other, Format */
#define MB_GENERAL_CATEGORY_CS 27 /* Cs Other, Surrogate */
#define MB_GENERAL_CATEGORY_CO 28 /* Co Other, Private Use */
#define MB_GENERAL_CATEGORY_CN 29 /* Cn Other, Not Assigned (no characters in the file have this property) */

/* Unicode planes */
typedef uint8_t mb_codespace_plane;

#define MB_CODESPACE_PLANE_NUM 17 /* 17 planes */
#define MB_CODESPACE_PLANE_SIZE 65536 /* 2^16 code points per plane */

/*
 A unicode encoding
 [see: https://www.unicode.org/glossary/#code_point]
 */
typedef uint32_t mb_encoding;

#define MB_ENCODING_UNKNOWN 0
#define MB_ENCODING_ASCII 0x1
#define MB_ENCODING_UTF_8 0x2
#define MB_ENCODING_UTF_16_BE 0x4
#define MB_ENCODING_UTF_16_LE 0x8
#define MB_ENCODING_UTF_32_BE 0x10
#define MB_ENCODING_UTF_32_LE 0x20

#ifdef __cplusplus
extern "C" {
#endif

/* Output the current library version (MB_VERSION) */
char* mb_get_version(void);

/* Output the current library version number (MB_VERSION_NUMBER) */
unsigned int mb_get_version_number(void);

/* Output the current supported unicode version (MB_UNICODE_VERSION) */
char* mb_get_unicode_version(void);

/* Return true if the codepoint is valid */
bool mb_codepoint_is_valid(mb_codepoint codepoint);

/* Return true if the plane is valid */
bool mb_codespace_plane_is_valid(mb_codespace_plane plane);

/* Return the name of a plane, NULL if the place specified is not valid */
const char* mb_codespace_plane_name(mb_codespace_plane plane, bool abbreviation);

/* Return the string encoding (the most probable) */
mb_encoding mb_string_get_encoding(const char *buffer, size_t size);

/* Return 1 if the string is encoded in UTF-8 */
bool mb_string_is_utf8(const char *buffer, size_t size);

/* Return 1 if the string is encoded in ASCII */
bool mb_string_is_ascii(const char *buffer, size_t size);

/* Return the codepoint general category */
const mb_character* mb_codepoint_get_character(mb_codepoint codepoint);

/* Return true if the codepoint is an high-surrogate or a low-surrogate */
/*int mb_codepoint_is_surrogate(mb_codepoint);*/

#ifdef __cplusplus
}
#endif

#endif /* MB_UNICODEX_H */
