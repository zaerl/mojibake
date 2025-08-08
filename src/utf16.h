/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#pragma once

#include <stdint.h>

#define	MJB_UTF16_ACCEPT 0
#define	MJB_UTF16_REJECT 1

static inline uint8_t __used mjb_utf16_decode_step(uint8_t state, uint16_t unit, uint32_t *cpp) {
    if(state == MJB_UTF16_ACCEPT) {
        // Starting fresh or continuing after a complete codepoint
        if(unit < 0xD800 || unit > 0xDFFF) {
            // Basic Multilingual Plane character (U+0000 to U+D7FF, U+E000 to U+FFFF)
            *cpp = unit;

            return MJB_UTF16_ACCEPT;
        } else if(unit >= 0xD800 && unit <= 0xDBFF) {
            // High surrogate (U+D800 to U+DBFF) - expect low surrogate next
            *cpp = (uint32_t)(unit & 0x3FF) << 10;  // Store high 10 bits

            return MJB_UTF16_REJECT;
        } else {
            // Low surrogate without high surrogate (U+DC00 to U+DFFF) - error
            *cpp = 0xFFFD;  // Replacement character

            return 2;
        }
    } else if(state == MJB_UTF16_REJECT) {
        // Expecting low surrogate
        if(unit >= 0xDC00 && unit <= 0xDFFF) {
            // Valid low surrogate - combine with stored high surrogate
            *cpp = 0x10000 + (*cpp | (unit & 0x3FF));

            return MJB_UTF16_ACCEPT;
        } else {
            // Expected low surrogate but got something else - error
            *cpp = 0xFFFD;

            return 2;
        }
    }

    // Invalid state
    *cpp = 0xFFFD;  // Replacement character

    return 2;
}
