/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <string.h>

#include "mojibake.h"
#include "utf8.h"

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
    if(buffer == 0 || size == 0) {
        return false;
    }

    const char *index = buffer;
    uint8_t state = MJB_UTF8_ACCEPT;
    mjb_codepoint codepoint = MJB_CODEPOINT_NOT_VALID;

    // Loop through the string.
    for(; *index; ++index) {
        // Find next codepoint.
        state = mjb_utf8_decode_step(state, *index, &codepoint);

        if(state == MJB_UTF8_REJECT) {
            // The string is not well-formed.
            return false;
        }
    }

    return state == MJB_UTF8_ACCEPT;
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
