/**
 * The UCX library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <string.h>

#include "ucx.h"
#include "version.h"
#include "unicode_data.h"

#ifndef UCX_EXTERN
#define UCX_EXTERN extern
#endif

/* awk -F ';' '{ print $2 }' UnicodeData.txt | tr -s ' ' '\n' | sort | uniq -c | sort -r | head -n 100 */
/* awk -F ';' '{ print $2 }' UnicodeData.txt */

#ifndef UCX_EXPORT
#define UCX_EXPORT __attribute__((visibility("default")))
#endif

#define UCX_CONSTRUCTOR __attribute__((constructor))
#define UCX_DESTRUCTOR __attribute__((destructor))

#define UCX_ENCODING_UTF_8_BOM "\xEF\xBB\xBF"
#define UCX_ENCODING_UTF_16_BE_BOM "\xFE\xFF"
#define UCX_ENCODING_UTF_16_LE_BOM "\xFF\xFE"
#define UCX_ENCODING_UTF_32_BE_BOM "\x00\x00\xFE\xFF"
#define UCX_ENCODING_UTF_32_LE_BOM "\xFF\xFE\x00\x00"

/* Initializer. */
UCX_CONSTRUCTOR static void initializer(void) {

}

/* Finalizer. */
UCX_DESTRUCTOR static void finalizer(void) {

}

static ucx_encoding ucx_get_encoding_from_bom(const char *string,
    size_t length) {
    const unsigned char *buffer = (const unsigned char*)string;

    if(length < 2) {
        /* BOM are at least 2 characters */
        return UCX_ENCODING_UNKNOWN;
    }

    if(length >= 3) {
        if(memcmp(buffer, UCX_ENCODING_UTF_8_BOM, 3) == 0) {
            return UCX_ENCODING_UTF_8;
        }
    }

    ucx_encoding bom_encoding = UCX_ENCODING_UNKNOWN;

    if(length >= 4) {
        if(memcmp(buffer, UCX_ENCODING_UTF_32_BE_BOM, 4) == 0) {
            bom_encoding = UCX_ENCODING_UTF_32_BE;
        } else if(memcmp(buffer, UCX_ENCODING_UTF_32_LE_BOM, 4) == 0) {
            bom_encoding = UCX_ENCODING_UTF_32_LE;
        }
    }

    if(length >= 2) {
        if(memcmp(buffer, UCX_ENCODING_UTF_16_BE_BOM, 2) == 0) {
            bom_encoding = UCX_ENCODING_UTF_16_BE;
        } else if(memcmp(buffer, UCX_ENCODING_UTF_16_LE_BOM, 2) == 0) {
            /* A UTF-32-LE document is also valid UTF-16-LE */
            bom_encoding |= UCX_ENCODING_UTF_16_LE;
        }
    }

    return bom_encoding;
}

UCX_EXPORT char* ucx_get_version() {
    return UCX_VERSION;
}

UCX_EXPORT unsigned int ucx_get_version_number(void) {
    return UCX_VERSION_NUMBER;
}

UCX_EXPORT char* ucx_get_unicode_version() {
    return UCX_UNICODE_VERSION;
}

UCX_EXPORT int ucx_codepoint_is_valid(ucx_codepoint codepoint) {
    if(codepoint < UCX_CODEPOINT_MIN || codepoint > UCX_CODEPOINT_MAX ||
       (codepoint >= 0xFDD0 && codepoint <= 0xFDEF) ||
       (codepoint & 0xFFFE) == 0xFFFE || (codepoint & 0xFFFF) == 0xFFFF) {
        return 0;
    }

    return 1;
}

UCX_EXPORT int ucx_codespace_plane_is_valid(ucx_codespace_plane plane) {
    return plane >= UCX_CODESPACE_PLANE_MIN &&
        plane <= UCX_CODESPACE_PLANE_MAX;
}

UCX_EXPORT ucx_encoding ucx_string_get_encoding(const char *buffer,
    size_t size) {
    if(buffer == 0) {
        return UCX_ERRNO;
    }

    if(size == 0) {
        return UCX_ENCODING_UNKNOWN;
    }

    ucx_encoding bom_encoding = ucx_get_encoding_from_bom(buffer, size);

    if(bom_encoding != UCX_ENCODING_UNKNOWN) {
        return bom_encoding;
    }

    /* No BOM, let's try UTF-8 */
    if(ucx_string_is_utf8(buffer, size)) {
        bom_encoding |= UCX_ENCODING_UTF_8;
    }

    /* No BOM, let's try ASCII */
    if(ucx_string_is_ascii(buffer, size)) {
        bom_encoding |= UCX_ENCODING_ASCII;
    }

    return bom_encoding;
}

UCX_EXPORT int ucx_string_is_utf8(const char *string, size_t size) {
    const unsigned char *buffer = (const unsigned char*)string;
    const unsigned char *end = buffer + size;
    unsigned char byte;
    unsigned int code_length, i;
    uint32_t ch;

    if(string == 0) {
        return UCX_ERRNO;
    }

    if(size == 0) {
        return 0;
    }

    while(buffer != end) {
        byte = *buffer;

        if(byte <= 0x7F) {
            /* 0b0xxxxxxx, 1 byte sequence */
            buffer += 1;
            continue;
        }

        if(0xC2 <= byte && byte <= 0xDF) {
            /* 0b110xxxxx: 2 bytes sequence */
            code_length = 2;
        } else if(0xE0 <= byte && byte <= 0xEF) {
            /* 0b1110xxxx: 3 bytes sequence */
            code_length = 3;
        } else if(0xF0 <= byte && byte <= 0xF4) {
            /* 0b11110xxx: 4 bytes sequence */
            code_length = 4;
        } else {
            /* invalid first byte of a multibyte character */
            return 0;
        }

        if(buffer + (code_length - 1) >= end) {
            /* truncated string or invalid byte sequence */
            return 0;
        }

        /* Check continuation bytes: bit 7 should be set, bit 6 should be
         * unset (b10xxxxxx). */
        for(i = 1; i < code_length; ++i) {
            if((buffer[i] & 0xC0) != 0x80) {
                return 0;
            }
        }

        if(code_length == 2) {
            /* 2 bytes sequence: U+0080..U+07FF */
            ch = ((buffer[0] & 0x1f) << 6) + (buffer[1] & 0x3f);
            /* buffer[0] >= 0xC2, so ch >= 0x0080.
             buffer[0] <= 0xDF, (buffer[1] & 0x3f) <= 0x3f, so ch <= 0x07ff */
        } else if(code_length == 3) {
            /* 3 bytes sequence: U+0800..U+FFFF */
            ch = ((buffer[0] & 0x0f) << 12) + ((buffer[1] & 0x3f) << 6) +
            (buffer[2] & 0x3f);
            /* (0xff & 0x0f) << 12 | (0xff & 0x3f) << 6 | (0xff & 0x3f) = 0xffff,
             so ch <= 0xffff */
            if(ch < 0x0800) {
                return 0;
            }

            /* surrogates (U+D800-U+DFFF) are invalid in UTF-8:
             test if (0xD800 <= ch && ch <= 0xDFFF) */
            if((ch >> 11) == 0x1b) {
                return 0;
            }
        } else if(code_length == 4) {
            /* 4 bytes sequence: U+10000..U+10FFFF */
            ch = ((buffer[0] & 0x07) << 18) + ((buffer[1] & 0x3f) << 12) +
            ((buffer[2] & 0x3f) << 6) + (buffer[3] & 0x3f);

            if((ch < 0x10000) || (0x10FFFF < ch)) {
                return 0;
            }
        }

        buffer += code_length;
    }

    return 1;
}

UCX_EXPORT int ucx_string_is_ascii(const char *string, size_t size) {
    const unsigned char *buffer = (const unsigned char*)string;
    const unsigned char *end = buffer + size;

    if(string == 0) {
        return UCX_ERRNO;
    }

    if(size == 0) {
        return 0;
    }

    for(; buffer != end; ++buffer) {
        /* every character must have leading bit at zero */
        if(*buffer & 0x80) {
            return 0;
        }
    }

    return 1;
}

UCX_EXPORT ucx_character* ucx_get_codepoint_character(ucx_codepoint codepoint) {
    if(codepoint > UCX_CHARACTER_MAX) {
        return 0;
    }

    return &ucx_characters[codepoint];
}

UCX_EXPORT const ucx_character* ucx_codepoint_get_character(ucx_codepoint codepoint) {
    if(!ucx_codepoint_is_valid(codepoint)) {
        return NULL;
    }

    return &ucx_characters[codepoint];
}

/* UCX_EXPORT const unsigned char* ucx_convert_encoding(const unsigned char *buffer,
 unsigned int size, ucx_encoding encoding) {
 if(buffer == 0 || size == 0 || encoding > UCX_ENCODING_UTF_32_LE) {
 return 0;
 }

 return "";
 }*/
