/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "mojibake-internal.h"
#include "break.h"

extern mojibake mjb_global;

// Return the codepoint character
MJB_EXPORT bool mjb_codepoint_line_breaking_class(mjb_codepoint codepoint,
    mjb_line_breaking_class *line_breaking_class) {
    if(!mjb_initialize()) {
        return false;
    }

    if(!mjb_codepoint_is_valid(codepoint)) {
        return false;
    }

    sqlite3_reset(mjb_global.stmt_line_breaking_class);
    sqlite3_bind_int(mjb_global.stmt_line_breaking_class, 1, codepoint);

    if(sqlite3_step(mjb_global.stmt_line_breaking_class) != SQLITE_ROW) {
        return false;
    }

    *line_breaking_class = (mjb_line_breaking_class)sqlite3_column_int(mjb_global.stmt_line_breaking_class, 0);

    return true;
}

// Line breaking algorithm
// see: https://www.unicode.org/reports/tr14
// Word and Grapheme Cluster Breaking
// see: https://unicode.org/reports/tr29/
MJB_EXPORT bool mjb_break(const char *buffer, size_t length, mjb_encoding encoding) {
    return true;
}
