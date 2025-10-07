/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>

#include "mojibake-internal.h"

extern mojibake mjb_global;

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
MJB_EXPORT bool mjb_codepoint_character(mjb_codepoint codepoint, mjb_character *character) {
    if(!mjb_initialize()) {
        return false;
    }

    if(!mjb_codepoint_is_valid(codepoint)) {
        return false;
    }

    if(mjb_codepoint_is_hangul_syllable(codepoint)) {
        // Hangul syllable
        character->codepoint = codepoint;
        mjb_hangul_syllable_name(codepoint, character->name, 128);
        character->category = MJB_CATEGORY_LO;
        character->combining = MJB_CCC_NOT_REORDERED;
        character->bidirectional = MJB_BIDI_L;
        character->decomposition = MJB_DECOMPOSITION_NONE;
        character->decimal = 0;
        character->digit = 0;
        character->numeric[0] = '\0';
        character->mirrored = false;
        character->uppercase = 0;
        character->lowercase = 0;
        character->titlecase = 0;

        return true;
    } else if(mjb_codepoint_is_cjk_ideograph(codepoint)) {
        // CJK Ideograph
        character->codepoint = codepoint;
        snprintf(character->name, 128, "CJK UNIFIED IDEOGRAPH-%X", codepoint);
        character->category = MJB_CATEGORY_LO;
        character->combining = MJB_CCC_NOT_REORDERED;
        character->bidirectional = MJB_BIDI_L;
        character->decomposition = MJB_DECOMPOSITION_NONE;
        character->decimal = 0;
        character->digit = 0;
        character->numeric[0] = '\0';
        character->mirrored = false;
        character->uppercase = 0;
        character->lowercase = 0;
        character->titlecase = 0;

        return true;
    }

    sqlite3_reset(mjb_global.stmt_get_codepoint);
    // sqlite3_clear_bindings(mjb_global.stmt_get_codepoint);

    int rc = sqlite3_bind_int(mjb_global.stmt_get_codepoint, 1, codepoint);

    if(rc != SQLITE_OK) {
        return false;
    }

    rc = sqlite3_step(mjb_global.stmt_get_codepoint);

    if(rc != SQLITE_ROW) {
        return false;
    }

    character->codepoint = (mjb_codepoint)sqlite3_column_int(mjb_global.stmt_get_codepoint, 0);

    char *name = (char*)sqlite3_column_text(mjb_global.stmt_get_codepoint, 1);

    // Egyptian Hieroglyphs
    // Egyptian Hieroglyph Format Controls
    if(codepoint >= 0x13000 && codepoint <= 0x143FF) {
        // Egyptian Hieroglyphs Extended-A
        if(codepoint >= 0x13460) {
            snprintf(character->name, 128, "EGYPTIAN HIEROGLYPH-%X", codepoint);
        } else {
            snprintf(character->name, 128, "EGYPTIAN HIEROGLYPH %s", name);
        }
    } else if(codepoint >= MJB_CJK_COMPATIBILITY_IDEOGRAPH_START &&
        codepoint <= MJB_CJK_COMPATIBILITY_IDEOGRAPH_END) {
        // CJK Compatibility Ideographs
        snprintf(character->name, 128, "CJK COMPATIBILITY IDEOGRAPH-%X", codepoint);
    } else if(codepoint >= 0x14400 && codepoint <= 0x1467F) {
        // Anatolian Hieroglyphs
        snprintf(character->name, 128, "ANATOLIAN HIEROGLYPH A%s", name);
    } else {
        if(name != NULL) {
            strncpy(character->name, name, 128);
        } else {
            character->name[0] = '\0';
        }
    }

    character->category = (mjb_category)sqlite3_column_int(mjb_global.stmt_get_codepoint, 2);
    character->combining = (mjb_canonical_combining_class)sqlite3_column_int(
        mjb_global.stmt_get_codepoint, 3);
    character->bidirectional = (unsigned short)sqlite3_column_int(mjb_global.stmt_get_codepoint,
        4);
    character->decomposition = (mjb_decomposition)sqlite3_column_int(mjb_global.stmt_get_codepoint,
        5);

    if(sqlite3_column_type(mjb_global.stmt_get_codepoint, 6) == SQLITE_NULL) {
        character->decimal = MJB_NUMBER_NOT_VALID;
    } else {
        character->decimal = sqlite3_column_int(mjb_global.stmt_get_codepoint, 6);
    }

    if(sqlite3_column_type(mjb_global.stmt_get_codepoint, 7) == SQLITE_NULL) {
        character->digit = MJB_NUMBER_NOT_VALID;
    } else {
        character->digit = sqlite3_column_int(mjb_global.stmt_get_codepoint, 7);
    }

    char *numeric = (char*)sqlite3_column_text(mjb_global.stmt_get_codepoint, 8);

    if(numeric != NULL) {
        strncpy(character->numeric, numeric, 16);
    } else {
        character->numeric[0] = '\0';
    }

    character->mirrored = sqlite3_column_int(mjb_global.stmt_get_codepoint, 9) == 1;
    character->uppercase = (mjb_codepoint)sqlite3_column_int(mjb_global.stmt_get_codepoint, 10);
    character->lowercase = (mjb_codepoint)sqlite3_column_int(mjb_global.stmt_get_codepoint, 11);
    character->titlecase = (mjb_codepoint)sqlite3_column_int(mjb_global.stmt_get_codepoint, 12);

    return true;
}

// Return true if the codepoint is graphic
MJB_EXPORT bool mjb_codepoint_is_graphic(mjb_codepoint codepoint) {
    mjb_character character;

    if(!mjb_codepoint_character(codepoint, &character)) {
        return false;
    }

    return mjb_category_is_graphic(character.category);
}

MJB_EXPORT bool mjb_category_is_graphic(mjb_category category) {
    // All C categories can be printed
    switch(category) {
        case MJB_CATEGORY_CC:
        case MJB_CATEGORY_CF:
        case MJB_CATEGORY_CS:
        case MJB_CATEGORY_CO:
        case MJB_CATEGORY_CN:
            return false;
        default:
            return true;
    }

    return true;
}

// Return true if the codepoint is combining
MJB_EXPORT bool mjb_codepoint_is_combining(mjb_codepoint codepoint) {
    mjb_character character;

    if(!mjb_codepoint_character(codepoint, &character)) {
        return false;
    }

    return mjb_category_is_combining(character.category);
}

// Return true if the category is combining
MJB_EXPORT bool mjb_category_is_combining(mjb_category category) {
    return category == MJB_CATEGORY_MN || category == MJB_CATEGORY_MC ||
    category == MJB_CATEGORY_ME;
}

// Return the character block
MJB_EXPORT bool mjb_character_block(mjb_codepoint codepoint, mjb_codepoint_block *block) {
    if(!mjb_initialize()) {
        return false;
    }

    if(!mjb_codepoint_is_valid(codepoint)) {
        return false;
    }

    sqlite3_reset(mjb_global.stmt_get_block);
    //sqlite3_clear_bindings(mjb_global.stmt_get_block);

    int rc = sqlite3_bind_int(mjb_global.stmt_get_block, 1, codepoint);

    rc = sqlite3_step(mjb_global.stmt_get_block);

    if(rc != SQLITE_ROW) {
        return false;
    }

    int raw_id = sqlite3_column_int(mjb_global.stmt_get_block, 0);
    int raw_start = sqlite3_column_int(mjb_global.stmt_get_block, 1);
    int raw_end = sqlite3_column_int(mjb_global.stmt_get_block, 2);
    char *name = (char*)sqlite3_column_text(mjb_global.stmt_get_block, 3);

    block->id = (mjb_block)raw_id;

    if(name != NULL) {
        strncpy(block->name, name, 128);
    } else {
        block->name[0] = '\0';
    }

    block->start = (mjb_codepoint)raw_start;
    block->end = (mjb_codepoint)raw_end;

    return true;
}

// Return the codepoint lowercase codepoint
MJB_EXPORT mjb_codepoint mjb_codepoint_to_lowercase(mjb_codepoint codepoint) {
    mjb_character character;

    if(!mjb_codepoint_character(codepoint, &character)) {
        return codepoint;
    }

    return character.lowercase == 0 ? codepoint : character.lowercase;
}

// Return the codepoint uppercase codepoint
MJB_EXPORT mjb_codepoint mjb_codepoint_to_uppercase(mjb_codepoint codepoint) {
    mjb_character character;

    if(!mjb_codepoint_character(codepoint, &character)) {
        return codepoint;
    }

    return character.uppercase == 0 ? codepoint : character.uppercase;
}

// Return the codepoint titlecase codepoint
MJB_EXPORT mjb_codepoint mjb_codepoint_to_titlecase(mjb_codepoint codepoint) {
    mjb_character character;

    if(!mjb_codepoint_character(codepoint, &character)) {
        return codepoint;
    }

    return character.titlecase == 0 ? codepoint : character.titlecase;
}
