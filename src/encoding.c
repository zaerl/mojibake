/*
 * The mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <string.h>
#include "mojibake.h"

#define MJB_ENCODING_UTF_8_BOM "\xEF\xBB\xBF"
#define MJB_ENCODING_UTF_16_BE_BOM "\xFE\xFF"
#define MJB_ENCODING_UTF_16_LE_BOM "\xFF\xFE"
#define MJB_ENCODING_UTF_32_BE_BOM "\x00\x00\xFE\xFF"
#define MJB_ENCODING_UTF_32_LE_BOM "\xFF\xFE\x00\x00"

static mjb_encoding mjb_encoding_from_bom(const char *buffer, size_t length) {
    if(length < 2) {
        // BOM are at least 2 characters
        return MJB_ENCODING_UNKNOWN;
    }

    if(length >= 3) {
        if(memcmp(buffer, MJB_ENCODING_UTF_8_BOM, 3) == 0) {
            return MJB_ENCODING_UTF_8;
        }
    }

    mjb_encoding bom_encoding = MJB_ENCODING_UNKNOWN;

    if(length >= 4) {
        if(memcmp(buffer, MJB_ENCODING_UTF_32_BE_BOM, 4) == 0) {
            bom_encoding = MJB_ENCODING_UTF_32_BE;
        } else if(memcmp(buffer, MJB_ENCODING_UTF_32_LE_BOM, 4) == 0) {
            bom_encoding = MJB_ENCODING_UTF_32_LE;
        }
    }

    if(length >= 2) {
        if(memcmp(buffer, MJB_ENCODING_UTF_16_BE_BOM, 2) == 0) {
            bom_encoding = MJB_ENCODING_UTF_16_BE;
        } else if(memcmp(buffer, MJB_ENCODING_UTF_16_LE_BOM, 2) == 0) {
            // A UTF-32-LE document is also valid UTF-16-LE
            bom_encoding |= MJB_ENCODING_UTF_16_LE;
        }
    }

    return bom_encoding;
}

// Return the string encoding (the most probable)
MJB_EXPORT mjb_encoding mjb_string_encoding(const char *buffer, size_t size) {
    if(buffer == 0 || size == 0) {
        return MJB_ENCODING_UNKNOWN;
    }

    mjb_encoding bom_encoding = mjb_encoding_from_bom(buffer, size);

    if(bom_encoding != MJB_ENCODING_UNKNOWN) {
        return bom_encoding;
    }

    // No BOM, let's try UTF-8
    if(mjb_string_is_utf8(buffer, size)) {
        bom_encoding |= MJB_ENCODING_UTF_8;
    }

    // No BOM, let's try ASCII
    if(mjb_string_is_ascii(buffer, size)) {
        bom_encoding |= MJB_ENCODING_ASCII;
    }

    return bom_encoding;
}

// Return true if the string is encoded in UTF-8
MJB_EXPORT bool mjb_string_is_utf8(const char *buffer, size_t size) {
    const char *end = buffer + size;
    unsigned char byte;
    unsigned int code_length, i;
    uint32_t ch;

    if(buffer == 0 || size == 0) {
        return false;
    }

    while(buffer != end) {
        byte = *buffer;

        if(byte <= 0x7F) {
            // 0b0xxxxxxx, 1 byte sequence
            buffer += 1;
            continue;
        }

        if(0xC2 <= byte && byte <= 0xDF) {
            // 0b110xxxxx: 2 bytes sequence
            code_length = 2;
        } else if(0xE0 <= byte && byte <= 0xEF) {
            // 0b1110xxxx: 3 bytes sequence
            code_length = 3;
        } else if(0xF0 <= byte && byte <= 0xF4) {
            // 0b11110xxx: 4 bytes sequence
            code_length = 4;
        } else {
            // invalid first byte of a multibyte character
            return false;
        }

        if(buffer + (code_length - 1) >= end) {
            // truncated string or invalid byte sequence
            return false;
        }

        // Check continuation bytes: bit 7 should be set, bit 6 should be unset (b10xxxxxx).
        for(i = 1; i < code_length; ++i) {
            if((buffer[i] & 0xC0) != 0x80) {
                return false;
            }
        }

        if(code_length == 2) {
            // 2 bytes sequence: U+0080..U+07FF
            ch = ((buffer[0] & 0x1f) << 6) + (buffer[1] & 0x3f);
            /* buffer[0] >= 0xC2, so ch >= 0x0080.
             buffer[0] <= 0xDF, (buffer[1] & 0x3f) <= 0x3f, so ch <= 0x07ff */
        } else if(code_length == 3) {
            // 3 bytes sequence: U+0800..U+FFFF
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
            // 4 bytes sequence: U+10000..U+10FFFF
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

// Return true if the string is encoded in ASCII
MJB_EXPORT bool mjb_string_is_ascii(const char *buffer, size_t size) {
    const char *end = buffer + size;

    if(buffer == 0 || size == 0) {
        return false;
    }

    for(; buffer != end; ++buffer) {
        // every character must have leading bit at zero
        if(*buffer & 0x80) {
            return false;
        }
    }

    return 1;
}
