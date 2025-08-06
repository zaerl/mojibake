/**
 * The Mojibake library
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

/**
 * Return the encoding from the BOM (if possible).
 */
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
            bom_encoding = MJB_ENCODING_UTF_32 | MJB_ENCODING_UTF_32_BE;
        } else if(memcmp(buffer, MJB_ENCODING_UTF_32_LE_BOM, 4) == 0) {
            // A UTF-32-LE document is also valid UTF-16-LE
            bom_encoding = MJB_ENCODING_UTF_32 | MJB_ENCODING_UTF_32_LE | MJB_ENCODING_UTF_16_LE;
        }
    }

    if(length >= 2) {
        if(memcmp(buffer, MJB_ENCODING_UTF_16_BE_BOM, 2) == 0) {
            bom_encoding = MJB_ENCODING_UTF_16 | MJB_ENCODING_UTF_16_BE;
        } else if(memcmp(buffer, MJB_ENCODING_UTF_16_LE_BOM, 2) == 0) {
            bom_encoding |= MJB_ENCODING_UTF_16 | MJB_ENCODING_UTF_16_LE;
        }
    }

    return bom_encoding;
}

/**
 * Return the string encoding (the most probable).
 */
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

/**
 * Return true if the string is encoded in UTF-8.
 */
MJB_EXPORT bool mjb_string_is_utf8(const char *buffer, size_t size) {
    size_t next = 0;
    const char *current = buffer;
    size_t remaining = size;

    if(buffer == 0) {
        return false;
    }

    if(size == 0) {
        return true;
    }

    while(current < buffer + size) {
        mjb_codepoint codepoint = mjb_string_next_codepoint(current, remaining, &next);

        if(codepoint == MJB_CODEPOINT_NOT_VALID) {
            return false;
        }

        current += next;
        remaining -= next;
    }

    return true;
}

/**
 * Return true if the string is encoded in ASCII.
 */
MJB_EXPORT bool mjb_string_is_ascii(const char *buffer, size_t size) {
    if(buffer == 0 || size == 0) {
        return false;
    }

    const char *end = buffer + size;

    for(; buffer != end; ++buffer) {
        // Every character must have leading bit at zero.
        if(*buffer & 0x80) {
            return false;
        }
    }

    return 1;
}

// Deprecated. Use mjb_utf8_decode_step instead.
mjb_codepoint mjb_string_next_codepoint(const char *buffer, size_t size, size_t *next) {
    const char *end = buffer + size;
    unsigned char byte;
    unsigned int code_length;
    mjb_codepoint codepoint = MJB_CODEPOINT_NOT_VALID;

    if(buffer == 0) {
        return MJB_CODEPOINT_NOT_VALID;
    }

    if(size == 0) {
        return MJB_CODEPOINT_NOT_VALID;
    }

    byte = *buffer;

    if(byte <= 0x7F) {
        // 0b0xxxxxxx, 1 byte sequence
        *next = 1;

        return byte;
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
        // Invalid first byte of a multibyte character
        return MJB_CODEPOINT_NOT_VALID;
    }

    if(buffer + (code_length - 1) >= end) {
        // Truncated string or invalid byte sequence
        return MJB_CODEPOINT_NOT_VALID;
    }

    // Check continuation bytes: bit 7 should be set, bit 6 should be unset (b10xxxxxx).
    for(unsigned int i = 1; i < code_length; ++i) {
        if((buffer[i] & 0xC0) != 0x80) {
            return MJB_CODEPOINT_NOT_VALID;
        }
    }

    if(code_length == 2) {
        // 2 bytes sequence: U+0080..U+07FF
        codepoint = ((buffer[0] & 0x1f) << 6) + (buffer[1] & 0x3f);
        // buffer[0] >= 0xC2, so codepoint >= 0x0080.
        // buffer[0] <= 0xDF, (buffer[1] & 0x3f) <= 0x3f
        // So then codepoint <= 0x07ff
    } else if(code_length == 3) {
        // 3 bytes sequence: U+0800..U+FFFF
        codepoint = ((buffer[0] & 0x0f) << 12) + ((buffer[1] & 0x3f) << 6) +
        (buffer[2] & 0x3f);
        // (0xff & 0x0f) << 12 | (0xff & 0x3f) << 6 | (0xff & 0x3f) = 0xffff,
        // So then codepoint <= 0xffff
        if(codepoint < 0x0800) {
            return MJB_CODEPOINT_NOT_VALID;
        }

        // Surrogates (U+D800-U+DFFF) are invalid in UTF-8:
        // Test if (0xD800 <= codepoint && codepoint <= 0xDFFF)
        if((codepoint >> 11) == 0x1b) {
            return MJB_CODEPOINT_NOT_VALID;
        }
    } else if(code_length == 4) {
        // 4 bytes sequence: U+10000..U+10FFFF
        codepoint = ((buffer[0] & 0x07) << 18) + ((buffer[1] & 0x3f) << 12) +
        ((buffer[2] & 0x3f) << 6) + (buffer[3] & 0x3f);

        if((codepoint < 0x10000) || (0x10FFFF < codepoint)) {
            return MJB_CODEPOINT_NOT_VALID;
        }
    }

    *next = code_length;

    return codepoint;
}

MJB_EXPORT unsigned int mjb_codepoint_encode(mjb_codepoint codepoint, char *buffer, size_t size, mjb_encoding encoding) {
    if(buffer == NULL || size < 2 || encoding != MJB_ENCODING_UTF_8) {
        return 0;
    }

    if(codepoint <= 0x7F) {
        // 0b0x|xx|xx|xx, 1 byte sequence (ASCII)
        buffer[0] = (char)codepoint;
        buffer[1] = '\0';

        return 1;
    } else if(codepoint <= 0x7FF) {
        if(size < 3) {
            return 0;
        }

        // 0b11|0x|xx|xx: 2 bytes sequence
        buffer[0] = (char)(0xC0 | ((codepoint >> 6) & 0x1F));
        buffer[1] = (char)(0x80 | (codepoint & 0x3F));
        buffer[2] = '\0';

        return 2;
    } else if(codepoint <= 0xFFFF) {
        if(size < 4) {
            return 0;
        }

        // 0b11|10|xx|xx: 3 bytes sequence
        buffer[0] = (char)(0xE0 | ((codepoint >> 12) & 0x0F));
        buffer[1] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        buffer[2] = (char)(0x80 | (codepoint & 0x3F));
        buffer[3] = '\0';

        return 3;
    } else if(codepoint <= 0x10FFFF) {
        if(size < 5) {
            return 0;
        }

        // 0b11|11|0x|xx: 4 bytes sequence
        buffer[0] = (char)(0xF0 | ((codepoint >> 18) & 0x07));
        buffer[1] = (char)(0x80 | ((codepoint >> 12) & 0x3F));
        buffer[2] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        buffer[3] = (char)(0x80 | (codepoint & 0x3F));
        buffer[4] = '\0';

        return 4;
    }

    return 0;
}

/**
 * Return UTF-8 length of a string.
 */
MJB_EXPORT size_t mjb_string_utf8_length(const char *buffer, size_t max_length) {
    if(buffer == 0 || max_length == 0) {
        return 0;
    }

    uint8_t state = MJB_UTF8_ACCEPT;
    mjb_codepoint codepoint;
    size_t count = 0;
    const char *current = buffer;

    while(*current && (size_t)(current - buffer) < max_length) {
        state = mjb_utf8_decode_step(state, *current, &codepoint);

        if(state == MJB_UTF8_ACCEPT) {
            ++count;
        }

        ++current;
    }

    return count;
}
