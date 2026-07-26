/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#pragma once

#ifndef MJB_PUNYCODE_H
#define MJB_PUNYCODE_H

#include "mojibake-internal.h"

mjb_status mjb_punycode_encode(const mjb_codepoint *codepoints, size_t count, mjb_output *output);
mjb_status mjb_punycode_decode(const char *buffer, size_t byte_length, mjb_codepoint **codepoints,
    size_t *count);

#endif // MJB_PUNYCODE_H
