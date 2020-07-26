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

#define UCX_NULL null

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

#define UCX_GC_L 0
#define UCX_GC_MARK 1
#define UCX_GC_NUMBER 2
#define UCX_GC_PUNCTUATION 3
#define UCX_GC_SYMBOL 4
#define UCX_GC_SEPARATOR 5
#define UCX_GC_OTHER 6

/* A unicode plane [see: https://www.unicode.org/glossary/#plane] */
typedef uint8_t ucx_codespace_plane;

#define UCX_CODESPACE_PLANE_MIN 0
#define UCX_CODESPACE_PLANE_MAX 16

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
