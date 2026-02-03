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
    if(codepoint > MJB_CODEPOINT_MAX ||
        (codepoint >= 0xFDD0 && codepoint <= 0xFDEF) || // Noncharacter
        (codepoint & 0xFFFE) == 0xFFFE || (codepoint & 0xFFFF) == 0xFFFF) { // Noncharacter
        return false;
    }

    return true;
}


static bool mjb_codepoint_cjk_th_character(mjb_codepoint codepoint, mjb_character *character) {
    character->name[0] = '\0';
    const char *format = NULL;
    mjb_codepoint format_codepoint = codepoint;

    if(mjb_codepoint_is_hangul_syllable(codepoint)) {
        // Hangul syllable
        mjb_hangul_syllable_name(codepoint, character->name, 128);
    } else if(mjb_codepoint_is_cjk_ideograph(codepoint)) {
        format = "CJK UNIFIED IDEOGRAPH-%X";
    } else if(
        (codepoint >= MJB_TANGUT_IDEOGRAPH_START && codepoint <= MJB_TANGUT_IDEOGRAPH_END) ||
        (codepoint >= MJB_TANGUT_IDEOGRAPH_SUPPLEMENT_START && codepoint <= MJB_TANGUT_IDEOGRAPH_SUPPLEMENT_END)
    ) {
        format = "TANGUT IDEOGRAPH-%X";
    } else if(codepoint >= MJB_TANGUT_COMPONENT_START && codepoint <= MJB_TANGUT_COMPONENT_END) {
        format_codepoint = codepoint - MJB_TANGUT_COMPONENT_START + 1;
        format = "TANGUT COMPONENT-%03d";
    } else if(codepoint >= MJB_TANGUT_COMPONENT_SUPPLEMENT_START && codepoint <= MJB_TANGUT_COMPONENT_SUPPLEMENT_END) {
        format_codepoint = codepoint - MJB_TANGUT_COMPONENT_SUPPLEMENT_START + 769;
        format = "TANGUT COMPONENT-%03d";
    } else if(codepoint >= MJB_KHITAN_SMALL_SCRIPT_CHARACTER_START && codepoint <= MJB_KHITAN_SMALL_SCRIPT_CHARACTER_END) {
        format = "KHITAN SMALL SCRIPT CHARACTER-%X";
    } else if(codepoint >= MJB_EGYPTIAN_H_FORMAT_EXT_START && codepoint <= MJB_EGYPTIAN_H_EXT_END) {
        if(codepoint >= 0x143FF) {
            // Last valid is EGYPTIAN HIEROGLYPH-143FA
            return false;
        }

        format = "EGYPTIAN HIEROGLYPH-%X";
    } else {
        return false;
    }

    if(format != NULL) {
        snprintf(character->name, 128, format, format_codepoint);
    }

    character->codepoint = codepoint;
    character->category = MJB_CATEGORY_LO;
    character->combining = MJB_CCC_NOT_REORDERED;
    character->bidirectional = MJB_BIDI_L;
    character->decomposition = MJB_DECOMPOSITION_NONE;
    character->decimal = MJB_NUMBER_NOT_VALID;
    character->digit = MJB_NUMBER_NOT_VALID;
    character->numeric[0] = '\0';
    character->mirrored = false;
    character->uppercase = 0;
    character->lowercase = 0;
    character->titlecase = 0;

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

    sqlite3_reset(mjb_global.stmt_get_codepoint);
    // sqlite3_clear_bindings(mjb_global.stmt_get_codepoint);

    int rc = sqlite3_bind_int(mjb_global.stmt_get_codepoint, 1, codepoint);

    if(rc != SQLITE_OK) {
        return false;
    }

    rc = sqlite3_step(mjb_global.stmt_get_codepoint);

    if(rc != SQLITE_ROW) {
        // Try CJK or ancient scripts characters before returning false
        return mjb_codepoint_cjk_th_character(codepoint, character);
    }

    character->codepoint = (mjb_codepoint)sqlite3_column_int(mjb_global.stmt_get_codepoint, 0);

    char *name = (char*)sqlite3_column_text(mjb_global.stmt_get_codepoint, 1);

    // Egyptian Hieroglyphs
    // Egyptian Hieroglyph Format Controls
    if(codepoint >= MJB_EGYPTIAN_H_START && codepoint < MJB_EGYPTIAN_H_FORMAT_EXT_START) {
        snprintf(character->name, 128, "EGYPTIAN HIEROGLYPH %s", name);
    } else if(
        (codepoint >= MJB_CJK_COMPATIBILITY_IDEOGRAPH_START &&
        codepoint <= MJB_CJK_COMPATIBILITY_IDEOGRAPH_END) ||
        (codepoint >= MJB_CJK_COMPATIBILITY_IDEOGRAPH_SUPPLEMENT_START &&
        codepoint <= MJB_CJK_COMPATIBILITY_IDEOGRAPH_SUPPLEMENT_END)) {
        // CJK Compatibility Ideographs or CJK Compatibility Ideographs Supplement
        snprintf(character->name, 128, "CJK COMPATIBILITY IDEOGRAPH-%X", codepoint);
    } else if(codepoint >= 0x14400 && codepoint <= 0x1467F) {
        // Anatolian Hieroglyphs
        snprintf(character->name, 128, "ANATOLIAN HIEROGLYPH A%s", name);
    } else if((codepoint >= 0x12000 && codepoint <= 0x12399) ||
        (codepoint >= 0x12480 && codepoint <= 0x12543)) {
        // Cuneiform signs
        snprintf(character->name, 128, "CUNEIFORM SIGN %s", name);
    } else {
        if(name != NULL) {
            size_t len = strlen(name);

            if(len > 127) {
                len = 127;
            }

            memcpy(character->name, name, len);
            character->name[len] = '\0';
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
        size_t len = strlen(numeric);

        if(len > 15) {
            len = 15;
        }

        memcpy(character->numeric, numeric, len);
        character->numeric[len] = '\0';
    } else {
        character->numeric[0] = '\0';
    }

    character->mirrored = sqlite3_column_int(mjb_global.stmt_get_codepoint, 9) == 1;
    character->uppercase = (mjb_codepoint)sqlite3_column_int(mjb_global.stmt_get_codepoint, 10);
    character->lowercase = (mjb_codepoint)sqlite3_column_int(mjb_global.stmt_get_codepoint, 11);
    character->titlecase = (mjb_codepoint)sqlite3_column_int(mjb_global.stmt_get_codepoint, 12);

    return true;
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

MJB_EXPORT bool mjb_category_is_combining(mjb_category category) {
    return category == MJB_CATEGORY_MN || category == MJB_CATEGORY_MC ||
    category == MJB_CATEGORY_ME;
}

// Return the character block
MJB_EXPORT bool mjb_codepoint_block(mjb_codepoint codepoint, mjb_block_info *block) {
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
        size_t len = strlen(name);

        if(len > 127) {
            len = 127;
        }

        memcpy(block->name, name, len);
        block->name[len] = '\0';
    } else {
        block->name[0] = '\0';
    }

    block->start = (mjb_codepoint)raw_start;
    block->end = (mjb_codepoint)raw_end;

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

// Return true if the codepoint is combining
MJB_EXPORT bool mjb_codepoint_is_combining(mjb_codepoint codepoint) {
    mjb_character character;

    if(!mjb_codepoint_character(codepoint, &character)) {
        return false;
    }

    return mjb_category_is_combining(character.category);
}
