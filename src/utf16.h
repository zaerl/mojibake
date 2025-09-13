/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#pragma once

#include <stdint.h>

#include "mojibake-internal.h"

static inline uint8_t MJB_USED mjb_utf16_decode_step(uint8_t state, uint16_t unit, uint32_t *cpp) {
    if(state == MJB_UTF_ACCEPT) {
        if(unit < 0xD800 || unit > 0xDFFF) {
            // BMP
            *cpp = unit;

            return MJB_UTF_ACCEPT;
        } else if(unit >= 0xD800 && unit <= 0xDBFF) {
            // High surrogate (U+D800 to U+DBFF). Expect low surrogate next
            *cpp = (uint32_t)(unit & 0x3FF) << 10;

            return MJB_UTF_REJECT;
        } else {
            // Low surrogate without high surrogate
            *cpp = 0xFFFD;

            return 2;
        }
    } else if(state == MJB_UTF_REJECT) {
        // Expecting low surrogate
        if(unit >= 0xDC00 && unit <= 0xDFFF) {
            // Valid low surrogate
            *cpp = 0x10000 + (*cpp | (unit & 0x3FF));

            return MJB_UTF_ACCEPT;
        } else {
            *cpp = 0xFFFD;

            return 2;
        }
    }

    // Invalid state
    *cpp = 0xFFFD;

    return 2;
}
