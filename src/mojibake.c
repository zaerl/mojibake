/**
 * The mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <string.h>

#include "mojibake.h"
#include "version.h"
#include "sqlite/sqlite3.h"

#ifndef MB_EXTERN
#define MB_EXTERN extern
#endif

#ifndef MB_EXPORT
#define MB_EXPORT __attribute__((visibility("default")))
#endif

#define MB_ENCODING_UTF_8_BOM "\xEF\xBB\xBF"
#define MB_ENCODING_UTF_16_BE_BOM "\xFE\xFF"
#define MB_ENCODING_UTF_16_LE_BOM "\xFF\xFE"
#define MB_ENCODING_UTF_32_BE_BOM "\x00\x00\xFE\xFF"
#define MB_ENCODING_UTF_32_LE_BOM "\xFF\xFE\x00\x00"

static mb_encoding mb_get_encoding_from_bom(const char *string,
    size_t length) {
    const unsigned char *buffer = (const unsigned char*)string;

    if(length < 2) {
        /* BOM are at least 2 characters */
        return MB_ENCODING_UNKNOWN;
    }

    if(length >= 3) {
        if(memcmp(buffer, MB_ENCODING_UTF_8_BOM, 3) == 0) {
            return MB_ENCODING_UTF_8;
        }
    }

    mb_encoding bom_encoding = MB_ENCODING_UNKNOWN;

    if(length >= 4) {
        if(memcmp(buffer, MB_ENCODING_UTF_32_BE_BOM, 4) == 0) {
            bom_encoding = MB_ENCODING_UTF_32_BE;
        } else if(memcmp(buffer, MB_ENCODING_UTF_32_LE_BOM, 4) == 0) {
            bom_encoding = MB_ENCODING_UTF_32_LE;
        }
    }

    if(length >= 2) {
        if(memcmp(buffer, MB_ENCODING_UTF_16_BE_BOM, 2) == 0) {
            bom_encoding = MB_ENCODING_UTF_16_BE;
        } else if(memcmp(buffer, MB_ENCODING_UTF_16_LE_BOM, 2) == 0) {
            /* A UTF-32-LE document is also valid UTF-16-LE */
            bom_encoding |= MB_ENCODING_UTF_16_LE;
        }
    }

    return bom_encoding;
}

MB_EXPORT char* mb_get_version() {
    return MB_VERSION;
}

MB_EXPORT unsigned int mb_get_version_number(void) {
    return MB_VERSION_NUMBER;
}

MB_EXPORT char* mb_get_unicode_version() {
    return MB_UNICODE_VERSION;
}

MB_EXPORT bool mb_codepoint_is_valid(mb_codepoint codepoint) {
    if(codepoint < MB_CODEPOINT_MIN || codepoint > MB_CODEPOINT_MAX ||
       (codepoint >= 0xFDD0 && codepoint <= 0xFDEF) ||
       (codepoint & 0xFFFE) == 0xFFFE || (codepoint & 0xFFFF) == 0xFFFF) {
        return false;
    }

    return true;
}

MB_EXPORT bool mb_plane_is_valid(mb_plane plane) {
    return plane >= 0 && plane < MB_PLANE_NUM;
}

MB_EXPORT const char* mb_plane_name(mb_plane plane, bool full) {
    if(!mb_plane_is_valid(plane)) {
        return NULL;
    }

    switch(plane) {
        case 0:
            return full ? "Basic Multilingual Plane" : "BMP";

        case 1:
            return full ? "Supplementary Multilingual Plane" : "SMP";

        case 2:
            return full ? "Supplementary Ideographic Plane" : "SIP";

        case 3:
            return full ? "Tertiary Ideographic Plane" : "TIP";

        case 14:
            return full ? "Supplementary Special-purpose Plane" : "SSP";

        case 15:
            return full ? "Supplementary Private Use Area-A" : "PUA-A";

        case 16:
            return full ? "Supplementary Private Use Area-B" : "PUA-B";
    }

    return "Unassigned";
}

MB_EXPORT mb_encoding mb_string_get_encoding(const char *buffer,
    size_t size) {
    if(buffer == 0 || size == 0) {
        return MB_ENCODING_UNKNOWN;
    }

    mb_encoding bom_encoding = mb_get_encoding_from_bom(buffer, size);

    if(bom_encoding != MB_ENCODING_UNKNOWN) {
        return bom_encoding;
    }

    /* No BOM, let's try UTF-8 */
    if(mb_string_is_utf8(buffer, size)) {
        bom_encoding |= MB_ENCODING_UTF_8;
    }

    /* No BOM, let's try ASCII */
    if(mb_string_is_ascii(buffer, size)) {
        bom_encoding |= MB_ENCODING_ASCII;
    }

    return bom_encoding;
}

MB_EXPORT bool mb_string_is_utf8(const char *string, size_t size) {
    const unsigned char *buffer = (const unsigned char*)string;
    const unsigned char *end = buffer + size;
    unsigned char byte;
    unsigned int code_length, i;
    uint32_t ch;

    if(string == 0 || size == 0) {
        return false;
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
            return false;
        }

        if(buffer + (code_length - 1) >= end) {
            /* truncated string or invalid byte sequence */
            return false;
        }

        /* Check continuation bytes: bit 7 should be set, bit 6 should be
         * unset (b10xxxxxx). */
        for(i = 1; i < code_length; ++i) {
            if((buffer[i] & 0xC0) != 0x80) {
                return false;
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
                return false;
            }

            /* surrogates (U+D800-U+DFFF) are invalid in UTF-8:
             test if (0xD800 <= ch && ch <= 0xDFFF) */
            if((ch >> 11) == 0x1b) {
                return false;
            }
        } else if(code_length == 4) {
            /* 4 bytes sequence: U+10000..U+10FFFF */
            ch = ((buffer[0] & 0x07) << 18) + ((buffer[1] & 0x3f) << 12) +
            ((buffer[2] & 0x3f) << 6) + (buffer[3] & 0x3f);

            if((ch < 0x10000) || (0x10FFFF < ch)) {
                return false;
            }
        }

        buffer += code_length;
    }

    return true;
}

MB_EXPORT bool mb_string_is_ascii(const char *string, size_t size) {
    const unsigned char *buffer = (const unsigned char*)string;
    const unsigned char *end = buffer + size;

    if(string == 0 || size == 0) {
        return false;
    }

    for(; buffer != end; ++buffer) {
        /* every character must have leading bit at zero */
        if(*buffer & 0x80) {
            return false;
        }
    }

    return 1;
}

MB_EXPORT const mb_character* mb_codepoint_get_character(mb_codepoint codepoint) {
    if(!mb_codepoint_is_valid(codepoint)) {
        return NULL;
    }
}

/* MB_EXPORT const unsigned char* mb_convert_encoding(const unsigned char *buffer,
 unsigned int size, mb_encoding encoding) {
 if(buffer == 0 || size == 0 || encoding > MB_ENCODING_UTF_32_LE) {
 return 0;
 }

 return "";
 }*/