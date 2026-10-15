/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#pragma once

#ifndef MJB_PUNYCODE_H
#define MJB_PUNYCODE_H

#include "mojibake-internal.h"

#if MJB_FEATURE_IDNA
mjb_status mjb_punycode_encode(const mjb_codepoint *codepoints, size_t count, mjb_output *output);
// On success, *codepoints holds `byte_length` elements, of which the first *count are decoded.
// Free it with a size of `byte_length * sizeof(mjb_codepoint)`.
mjb_status mjb_punycode_decode(const char *buffer, size_t byte_length, mjb_codepoint **codepoints,
    size_t *count);
#endif

#endif // MJB_PUNYCODE_H
