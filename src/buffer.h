/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#pragma once

#ifndef MJB_BUFFER_H
#define MJB_BUFFER_H

#include "mojibake.h"

/**
 * A smaller version of mjb_normalize() that only returns the character information.
 * This is used to avoid the overhead of the full normalization process.
 */
MJB_EXPORT bool mjb_get_buffer_character(mjb_codepoint codepoint,
    mjb_normalization_character *character);

#endif // MJB_BUFFER_H
