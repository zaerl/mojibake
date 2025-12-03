/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "buffer.h"
#include "mojibake-internal.h"

extern mojibake mjb_global;

 /**
 * A smaller version of mjb_codepoint_character() that only returns the character information.
 * This is used to avoid the overhead of the full normalization process.
 */
MJB_EXPORT bool mjb_n_codepoint_character(mjb_codepoint codepoint, mjb_n_character *character) {
    if(!mjb_codepoint_is_valid(codepoint)) {
        return false;
    }

    if(mjb_codepoint_is_hangul_syllable(codepoint) || mjb_codepoint_is_cjk_ideograph(codepoint)) {
        character->codepoint = codepoint;
        character->combining = MJB_CCC_NOT_REORDERED;
        character->decomposition = MJB_DECOMPOSITION_NONE;
        character->quick_check = MJB_QC_YES;

        return true;
    }

    sqlite3_reset(mjb_global.stmt_buffer_character);
    // sqlite3_clear_bindings(mjb_global.stmt_buffer_character);

    int rc = sqlite3_bind_int(mjb_global.stmt_buffer_character, 1, codepoint);

    if(rc != SQLITE_OK) {
        return false;
    }

    rc = sqlite3_step(mjb_global.stmt_buffer_character);

    if(rc != SQLITE_ROW) {
        return false;
    }

    character->codepoint = codepoint;
    character->combining = (uint8_t)sqlite3_column_int(mjb_global.stmt_buffer_character, 1);
    character->decomposition = (uint8_t)sqlite3_column_int(mjb_global.stmt_buffer_character, 2);

    if(sqlite3_column_type(mjb_global.stmt_buffer_character, 3) == SQLITE_NULL) {
        character->quick_check = MJB_QC_YES;
    } else {
        character->quick_check = (uint16_t)sqlite3_column_int(mjb_global.stmt_buffer_character, 3);
    }

    return true;
}
