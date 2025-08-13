/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "mojibake-internal.h"
#include "utf8.h"
#include "utf16.h"

extern mojibake mjb_global;

MJB_EXPORT bool mjb_break(const char *buffer, size_t length, mjb_encoding encoding) {
    return true;
}
