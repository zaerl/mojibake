/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#pragma once

#include <stdint.h>

#include "mojibake-internal.h"

static const uint32_t utf8_classtab[16] = {
    0X88888888UL, 0X88888888UL, 0X99999999UL, 0X99999999UL,
    0XAAAAAAAAUL, 0XAAAAAAAAUL, 0XAAAAAAAAUL, 0XAAAAAAAAUL,
    0X222222FFUL, 0X22222222UL, 0X22222222UL, 0X22222222UL,
    0X3333333BUL, 0X33433333UL, 0XFFF5666CUL, 0XFFFFFFFFUL,
};

static const uint32_t utf8_statetab[16] = {
    0XFFFFFFF0UL, 0XFFFFFFFFUL, 0XFFFFFFF1UL, 0XFFFFFFF3UL,
    0XFFFFFFF4UL, 0XFFFFFFF7UL, 0XFFFFFFF6UL, 0XFFFFFFFFUL,
    0X33F11F0FUL, 0XF3311F0FUL, 0XF33F110FUL, 0XFFFFFFF2UL,
    0XFFFFFFF5UL, 0XFFFFFFFFUL, 0XFFFFFFFFUL, 0XFFFFFFFFUL,
};

static inline uint8_t MJB_USED mjb_utf8_decode_step(uint8_t state, uint8_t octet, uint32_t *cpp) {
    // ASCII character in accept state (most common case)
    if(state == MJB_UTF_ACCEPT && octet < 0x80) {
        *cpp = octet;
        return MJB_UTF_ACCEPT;
    }

    // ASCII chars (< 0x80) have class 0, others require table lookup
    const uint8_t c_class = (octet < 0x80) ? 0 :
        ((utf8_classtab[(octet >> 3) & 0xF] >> ((octet & 7) << 2)) & 0xF);

    // Update codepoint: either start a new sequence or continue existing
    *cpp = (state == MJB_UTF_ACCEPT)
        ? (octet & (0xFFU >> c_class))
        : ((octet & 0x3FU) | (*cpp << 6));

    // Compute next state: reject if high bit set, otherwise table lookup
    if(state & 0x8) {
        return MJB_UTF_REJECT;
    }

    return (utf8_statetab[c_class] >> ((state & 7) << 2)) & 0xF;
}
