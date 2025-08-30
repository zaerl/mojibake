/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "mojibake-internal.h"

extern mojibake mjb_global;

// Word and Grapheme Cluster Breaking
// See: https://unicode.org/reports/tr29/
MJB_EXPORT bool mjb_segmentation(const char *buffer, size_t length, mjb_encoding encoding) {
    return true;
}
