/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#pragma once

#include <stdint.h>

#include "mojibake-internal.h"

static inline uint8_t MJB_USED mjb_utf32_decode_step(uint8_t state, uint8_t unit_1, uint8_t unit_2,
    uint8_t unit_3, uint8_t unit_4, uint32_t *cpp, bool is_big_endian) {
    uint32_t codepoint = is_big_endian ? (unit_1 << 24) | (unit_2 << 16) | (unit_3 << 8) | unit_4 :
        unit_1 | (unit_2 << 8) | (unit_3 << 16) | (unit_4 << 24);

    if(codepoint > 0x10FFFF || (codepoint >= 0xD800 && codepoint <= 0xDFFF)) {
        *cpp = MJB_CODEPOINT_REPLACEMENT;

        return 2;
    }

    *cpp = codepoint;

    return MJB_UTF_ACCEPT;
}
