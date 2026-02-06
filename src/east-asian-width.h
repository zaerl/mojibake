/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#pragma once

#ifndef MJB_EAST_ASIAN_WIDTH_H
#define MJB_EAST_ASIAN_WIDTH_H

#include "mojibake.h"

MJB_NONNULL(2) bool mjb_codepoint_east_asian_width(mjb_codepoint codepoint,
    mjb_east_asian_width *width);

#endif // MJB_EAST_ASIAN_WIDTH_H
