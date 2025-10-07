/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#pragma once

#ifndef MJB_EAST_ASIAN_WIDTH_H
#define MJB_EAST_ASIAN_WIDTH_H

#include "mojibake.h"

/**
 * East Asian Width
 * [see: https://www.unicode.org/reports/tr11/#East_Asian_Width]
 */
typedef enum mjb_east_asian_width {
    MJB_EAW_AMBIGUOUS,
    MJB_EAW_FULL_WIDTH,
    MJB_EAW_HALF_WIDTH,
    MJB_EAW_NEUTRAL,
    MJB_EAW_NARROW,
    MJB_EAW_WIDE
} mjb_east_asian_width;

#define MJB_EAW_COUNT 6

MJB_NONNULL(2) bool mjb_codepoint_east_asian_width(mjb_codepoint codepoint,
    mjb_east_asian_width *width);

#endif // MJB_EAST_ASIAN_WIDTH_H
