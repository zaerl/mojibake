/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#pragma once

#ifndef MJB_BUFFER_H
#define MJB_BUFFER_H

#include "mojibake.h"

// A smaller version of mjb_character that only contains the information needed for the
// normalization process.
typedef struct mjb_n_character {
    mjb_codepoint codepoint;
    uint8_t combining;
    uint8_t decomposition;
    uint16_t quick_check;
} mjb_n_character;

/**
 * A smaller version of mjb_codepoint_character() that only returns the character information.
 * This is used to avoid the overhead of the full normalization process.
 */
MJB_EXPORT bool mjb_n_codepoint_character(mjb_codepoint codepoint, mjb_n_character *character);

#endif // MJB_BUFFER_H
