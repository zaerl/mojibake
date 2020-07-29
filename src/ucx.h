/**
 * The UCX library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#ifndef UCX_H
#define UCX_H

#include <stddef.h>
#include <stdint.h>

#define UCX_OK 0
#define UCX_ERRNO -1

/*
 A unicode codepoint
 [see: https://www.unicode.org/glossary/#code_point]
 */
typedef uint32_t ucx_codepoint;

#define UCX_CODEPOINT_MIN 0x0
#define UCX_CODEPOINT_MAX 0x10FFFF

/*
 A unicode codepoint general category
 [see: https://www.unicode.org/glossary/#general_category]
 */
typedef struct ucx_character {
    ucx_codepoint codepoint;
    int general_category;
    char* name;
} ucx_character;

/* General categories */
#define UCX_GENERAL_CATEGORY_LU  0 /* Lu Letter, Uppercase */
#define UCX_GENERAL_CATEGORY_LL  1 /* Ll Letter, Lowercase */
#define UCX_GENERAL_CATEGORY_LT  2 /* Lt Letter, Titlecase */
#define UCX_GENERAL_CATEGORY_LM  3 /* Lm Letter, Modifier */
#define UCX_GENERAL_CATEGORY_LO  4 /* Lo Letter, Other */
#define UCX_GENERAL_CATEGORY_MN  5 /* Mn Mark, Non-Spacing */
#define UCX_GENERAL_CATEGORY_MC  6 /* Mc Mark, Spacing Combining */
#define UCX_GENERAL_CATEGORY_ME  7 /* Me Mark, Enclosing */
#define UCX_GENERAL_CATEGORY_ND  8 /* Nd Number, Decimal Digit */
#define UCX_GENERAL_CATEGORY_NL  9 /* Nl Number, Letter */
#define UCX_GENERAL_CATEGORY_NO 10 /* No Number, Other */
#define UCX_GENERAL_CATEGORY_PC 11 /* Pc Punctuation, Connector */
#define UCX_GENERAL_CATEGORY_PD 12 /* Pd Punctuation, Dash */
#define UCX_GENERAL_CATEGORY_PS 13 /* Ps Punctuation, Open */
#define UCX_GENERAL_CATEGORY_PE 14 /* Pe Punctuation, Close */
#define UCX_GENERAL_CATEGORY_PI 15 /* Pi Punctuation, Initial quote (may behave like Ps or Pe depending on usage) */
#define UCX_GENERAL_CATEGORY_PF 16 /* Pf Punctuation, Final quote (may behave like Ps or Pe depending on usage) */
#define UCX_GENERAL_CATEGORY_PO 17 /* Po Punctuation, Other */
#define UCX_GENERAL_CATEGORY_SM 18 /* Sm Symbol, Math */
#define UCX_GENERAL_CATEGORY_SC 19 /* Sc Symbol, Currency */
#define UCX_GENERAL_CATEGORY_SK 20 /* Sk Symbol, Modifier */
#define UCX_GENERAL_CATEGORY_SO 21 /* So Symbol, Other */
#define UCX_GENERAL_CATEGORY_ZS 22 /* Zs Separator, Space */
#define UCX_GENERAL_CATEGORY_ZL 23 /* Zl Separator, Line */
#define UCX_GENERAL_CATEGORY_ZP 24 /* Zp Separator, Paragraph */
#define UCX_GENERAL_CATEGORY_CC 25 /* Cc Other, Control */
#define UCX_GENERAL_CATEGORY_CF 26 /* Cf Other, Format */
#define UCX_GENERAL_CATEGORY_CS 27 /* Cs Other, Surrogate */
#define UCX_GENERAL_CATEGORY_CO 28 /* Co Other, Private Use */
#define UCX_GENERAL_CATEGORY_CN 29 /* Cn Other, Not Assigned (no characters in the file have this property) */

/* A unicode plane [see: https://www.unicode.org/glossary/#plane] */
typedef uint8_t ucx_codespace_plane;

#define UCX_CODESPACE_PLANE_MIN 0
#define UCX_CODESPACE_PLANE_MAX 16
#define UCX_CODESPACE_PLANE_SIZE 65536 /* 2^16 code points */

/*
 A unicode encoding
 [see: https://www.unicode.org/glossary/#code_point]
 */
typedef uint32_t ucx_encoding;

#define UCX_ENCODING_UNKNOWN 0
#define UCX_ENCODING_ASCII 0x1
#define UCX_ENCODING_UTF_8 0x2
#define UCX_ENCODING_UTF_16_BE 0x4
#define UCX_ENCODING_UTF_16_LE 0x8
#define UCX_ENCODING_UTF_32_BE 0x10
#define UCX_ENCODING_UTF_32_LE 0x20

#ifdef __cplusplus
extern "C" {
#endif

/* Output the current library version (UCX_VERSION) */
char* ucx_get_version(void);

/* Output the current library version number (UCX_VERSION_NUMBER) */
unsigned int ucx_get_version_number(void);

/* Output the current supported unicode version (UCX_UNICODE_VERSION) */
char* ucx_get_unicode_version(void);

/* Return 1 (true) if the codepoint is valid */
int ucx_codepoint_is_valid(ucx_codepoint codepoint);

/* Return 1 (true) if the plane is valid */
int ucx_codespace_plane_is_valid(ucx_codespace_plane plane);

/* Return the string encoding (the most probable) */
ucx_encoding ucx_string_get_encoding(const char *buffer, size_t size);

/* Return 1 if the string is encoded in UTF-8 */
int ucx_string_is_utf8(const char *buffer, size_t size);

/* Return 1 if the string is encoded in ASCII */
int ucx_string_is_ascii(const char *buffer, size_t size);

/* Return the codepoint general category */
const ucx_character* ucx_codepoint_get_character(ucx_codepoint codepoint);

/* Return true if the codepoint is an high-surrogate or a low-surrogate */
/*int ucx_codepoint_is_surrogate(ucx_codepoint);*/

#ifdef __cplusplus
}
#endif

#endif /* UCX_UNICODEX_H */
