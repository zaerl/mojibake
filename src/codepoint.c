/**
 * The mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <string.h>
#include "mojibake.h"

extern struct mojibake mjb_global;

// Return true if the codepoint is valid
MJB_EXPORT bool mjb_codepoint_is_valid(mjb_codepoint codepoint) {
    if(codepoint < MJB_CODEPOINT_MIN || codepoint > MJB_CODEPOINT_MAX ||
        (codepoint >= 0xFDD0 && codepoint <= 0xFDEF) || // Noncharacter
        (codepoint & 0xFFFE) == 0xFFFE || (codepoint & 0xFFFF) == 0xFFFF) { // Noncharacter
        return false;
    }

    return true;
}

// Return the codepoint character
MJB_EXPORT bool mjb_codepoint_character(mjb_character *character, mjb_codepoint codepoint) {
    if(!mjb_initialize()) {
        return false;
    }

    if(character == NULL || !mjb_codepoint_is_valid(codepoint)) {
        return false;
    }

    sqlite3_reset(mjb_global.stmt_get_codepoint);
    sqlite3_clear_bindings(mjb_global.stmt_get_codepoint);

    int rc = sqlite3_bind_int(mjb_global.stmt_get_codepoint, 1, codepoint);

    if(rc != SQLITE_OK) {
        return false;
    }

    rc = sqlite3_step(mjb_global.stmt_get_codepoint);

    if(rc != SQLITE_ROW) {
        return false;
    }

    character->codepoint = (mjb_codepoint)sqlite3_column_int(mjb_global.stmt_get_codepoint, 0);
    character->name = (char*)sqlite3_column_text(mjb_global.stmt_get_codepoint, 1);
    character->category = (mjb_category)sqlite3_column_int(mjb_global.stmt_get_codepoint, 2);
    character->combining = (mjb_canonical_combining_class)sqlite3_column_int(mjb_global.stmt_get_codepoint, 3);
    character->bidirectional = (unsigned short)sqlite3_column_int(mjb_global.stmt_get_codepoint, 4);
    character->decomposition = (mjb_decomposition)sqlite3_column_int(mjb_global.stmt_get_codepoint, 5);
    character->decimal = sqlite3_column_int(mjb_global.stmt_get_codepoint, 6);
    character->digit = sqlite3_column_int(mjb_global.stmt_get_codepoint, 7);
    character->numeric = (char*)sqlite3_column_text(mjb_global.stmt_get_codepoint, 8);
    character->mirrored = sqlite3_column_int(mjb_global.stmt_get_codepoint, 9) == 1;
    character->uppercase = (mjb_codepoint)sqlite3_column_int(mjb_global.stmt_get_codepoint, 10);
    character->lowercase = (mjb_codepoint)sqlite3_column_int(mjb_global.stmt_get_codepoint, 11);
    character->titlecase = (mjb_codepoint)sqlite3_column_int(mjb_global.stmt_get_codepoint, 12);

    return true;
}

// Return true if the codepoint has the category
MJB_EXPORT bool mjb_codepoint_is(mjb_codepoint codepoint, mjb_category category) {
    mjb_character character;

    if(!mjb_codepoint_character(&character, codepoint)) {
        return false;
    }

    return character.category == category;
}

// Return true if the codepoint is graphic
MJB_EXPORT bool mjb_codepoint_is_graphic(mjb_codepoint codepoint) {
    mjb_character character;

    if(!mjb_codepoint_character(&character, codepoint)) {
        return false;
    }

    // All C categories can be printed
    switch(character.category) {
        case MJB_CATEGORY_CC:
        case MJB_CATEGORY_CF:
        case MJB_CATEGORY_CS:
        case MJB_CATEGORY_CO:
        case MJB_CATEGORY_CN:
            return false;
        default:
            return true;
    }
}

MJB_EXPORT bool mjb_codepoint_block_is(mjb_codepoint codepoint, mjb_block block) {
    if(!mjb_initialize()) {
        return false;
    }

    if(block < 0 || block >= MJB_BLOCK_NUM || !mjb_codepoint_is_valid(codepoint)) {
        return false;
    }

    sqlite3_reset(mjb_global.stmt_get_block);
    sqlite3_clear_bindings(mjb_global.stmt_get_block);

    int rc = sqlite3_bind_int(mjb_global.stmt_get_block, 1, codepoint);

    rc = sqlite3_step(mjb_global.stmt_get_block);

    if(rc != SQLITE_ROW) {
        return false;
    }

    mjb_block stmt_get_block = (mjb_block)sqlite3_column_int(mjb_global.stmt_get_block, 0);

    return stmt_get_block == block;
}

// Return the codepoint lowercase codepoint
MJB_EXPORT mjb_codepoint mjb_codepoint_to_lowercase(mjb_codepoint codepoint) {
    mjb_character character;

    if(!mjb_codepoint_character(&character, codepoint)) {
        return codepoint;
    }

    return character.lowercase == 0 ? codepoint : character.lowercase;
}

// Return the codepoint uppercase codepoint
MJB_EXPORT mjb_codepoint mjb_codepoint_to_uppercase(mjb_codepoint codepoint) {
    mjb_character character;

    if(!mjb_codepoint_character(&character, codepoint)) {
        return codepoint;
    }

    return character.uppercase == 0 ? codepoint : character.uppercase;
}

// Return the codepoint titlecase codepoint
MJB_EXPORT mjb_codepoint mjb_codepoint_to_titlecase(mjb_codepoint codepoint) {
    mjb_character character;

    if(!mjb_codepoint_character(&character, codepoint)) {
        return codepoint;
    }

    return character.titlecase == 0 ? codepoint : character.titlecase;
}
