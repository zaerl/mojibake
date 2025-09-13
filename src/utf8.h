/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

/*-
 * Copyright (c) 2014 Taylor R Campbell
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <stdint.h>

#include "mojibake-internal.h"

static const uint32_t utf8_classtab[0x10] = {
    0X88888888UL, 0X88888888UL, 0X99999999UL, 0X99999999UL,
    0XAAAAAAAAUL, 0XAAAAAAAAUL, 0XAAAAAAAAUL, 0XAAAAAAAAUL,
    0X222222FFUL, 0X22222222UL, 0X22222222UL, 0X22222222UL,
    0X3333333BUL, 0X33433333UL, 0XFFF5666CUL, 0XFFFFFFFFUL,
};

static const uint32_t utf8_statetab[0x10] = {
    0XFFFFFFF0UL, 0XFFFFFFFFUL, 0XFFFFFFF1UL, 0XFFFFFFF3UL,
    0XFFFFFFF4UL, 0XFFFFFFF7UL, 0XFFFFFFF6UL, 0XFFFFFFFFUL,
    0X33F11F0FUL, 0XF3311F0FUL, 0XF33F110FUL, 0XFFFFFFF2UL,
    0XFFFFFFF5UL, 0XFFFFFFFFUL, 0XFFFFFFFFUL, 0XFFFFFFFFUL,
};

static inline uint8_t MJB_USED mjb_utf8_decode_step(uint8_t state, uint8_t octet, uint32_t *cpp) {
    const uint8_t reject = (state >> 3);
    const uint8_t nonascii = (octet >> 7);
    const uint8_t c_class = (!nonascii? 0 :
        (0xF & (utf8_classtab[(octet >> 3) & 0xF] >> (4 * (octet & 7)))));

    *cpp = (state == MJB_UTF_ACCEPT
        ? (octet & (0xFFU >> c_class))
        : ((octet & 0x3FU) | (*cpp << 6)));

    return (reject? MJB_UTF_REJECT : (0xF & (utf8_statetab[c_class] >> (4 * (state & 7)))));
}
