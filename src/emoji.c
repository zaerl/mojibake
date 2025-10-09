/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "mojibake-internal.h"

extern mojibake mjb_global;

MJB_EXPORT bool mjb_codepoint_emoji(mjb_codepoint codepoint, mjb_emoji_properties *emoji) {
    if(!mjb_initialize()) {
        return false;
    }

    if(!mjb_codepoint_is_valid(codepoint)) {
        return false;
    }

    sqlite3_reset(mjb_global.stmt_get_emoji);

    int rc = sqlite3_bind_int(mjb_global.stmt_get_emoji, 1, codepoint);

    if(rc != SQLITE_OK) {
        return false;
    }

    rc = sqlite3_step(mjb_global.stmt_get_emoji);

    if(rc != SQLITE_ROW) {
        return false;
    }

    emoji->codepoint = (mjb_codepoint)sqlite3_column_int(mjb_global.stmt_get_emoji, 0);
    emoji->emoji = (bool)sqlite3_column_int(mjb_global.stmt_get_emoji, 1);
    emoji->presentation = (bool)sqlite3_column_int(mjb_global.stmt_get_emoji, 2);
    emoji->modifier = (bool)sqlite3_column_int(mjb_global.stmt_get_emoji, 3);
    emoji->modifier_base = (bool)sqlite3_column_int(mjb_global.stmt_get_emoji, 4);
    emoji->component = (bool)sqlite3_column_int(mjb_global.stmt_get_emoji, 5);
    emoji->extended_pictographic = (bool)sqlite3_column_int(mjb_global.stmt_get_emoji, 6);

    return true;
}
