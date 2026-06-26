/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>

#include "mojibake-internal.h"
#include "unicode-tables.h"

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
    character->bidirectional = MJB_PR_BIDI_CLASS_L;
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
    if(character == NULL || !mjb_codepoint_is_valid(codepoint)) {
        return false;
    }

    mjb_category category;

    if(!mjb_unicode_category_lookup(codepoint, &category)) {
        // Try CJK or ancient scripts characters before returning false
        return mjb_codepoint_cjk_th_character(codepoint, character);
    }

    character->codepoint = codepoint;

    char name[128];
    name[0] = '\0';

    mjb_unicode_name_lookup(codepoint, name, sizeof(name));

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
        snprintf(character->name, 128, "%s", name);
    }

    character->category = category;
    character->combining = MJB_CCC_NOT_REORDERED;
    character->bidirectional = MJB_PR_BIDI_CLASS_L;
    character->decomposition = MJB_DECOMPOSITION_NONE;
    character->decimal = MJB_NUMBER_NOT_VALID;
    character->digit = MJB_NUMBER_NOT_VALID;
    character->numeric[0] = '\0';
    character->mirrored = false;
    character->uppercase = 0;
    character->lowercase = 0;
    character->titlecase = 0;

    mjb_n_character n_character;

    if(mjb_unicode_n_character_lookup(codepoint, &n_character)) {
        character->combining = (mjb_canonical_combining_class)n_character.combining;
        character->decomposition = (mjb_decomposition)n_character.decomposition;
    }

    bool mirrored;
    mjb_bidi_class bidi;

    if(mjb_unicode_bidi_lookup(codepoint, &bidi, &mirrored)) {
        character->bidirectional = (unsigned short)bidi;
        character->mirrored = mirrored;
    }

    mjb_numeric_value numeric;

    if(mjb_unicode_numeric_value_lookup(codepoint, &numeric)) {
        character->decimal = numeric.decimal;
        character->digit = numeric.digit;
        snprintf(character->numeric, sizeof(character->numeric), "%s", numeric.numeric);
    }

    mjb_unicode_case_mapping case_mapping;

    if(mjb_unicode_case_lookup(codepoint, &case_mapping)) {
        character->uppercase = case_mapping.uppercase;
        character->lowercase = case_mapping.lowercase;
        character->titlecase = case_mapping.titlecase;
    }

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
}

MJB_EXPORT bool mjb_category_is_combining(mjb_category category) {
    return category == MJB_CATEGORY_MN || category == MJB_CATEGORY_MC ||
        category == MJB_CATEGORY_ME;
}

// Return the character block
MJB_EXPORT bool mjb_codepoint_block(mjb_codepoint codepoint, mjb_block_info *block) {
    if(block == NULL || !mjb_codepoint_is_valid(codepoint)) {
        return false;
    }

    return mjb_unicode_block_lookup(codepoint, block);
}

// Return true if the codepoint is graphic
MJB_EXPORT bool mjb_codepoint_is_graphic(mjb_codepoint codepoint) {
    mjb_character character;

    if(!mjb_codepoint_character(codepoint, &character)) {
        return false;
    }

    return mjb_category_is_graphic(character.category);
}

// Return the numeric value of a codepoint (decimal, digit, numeric string)
MJB_EXPORT bool mjb_codepoint_numeric_value(mjb_codepoint codepoint, mjb_numeric_value *value) {
    if(value == NULL || !mjb_codepoint_is_valid(codepoint)) {
        return false;
    }

    if(mjb_unicode_numeric_value_lookup(codepoint, value)) {
        return true;
    }

    // Can potentially be a CJK or ancient script character.
    value->decimal = MJB_NUMBER_NOT_VALID;
    value->digit = MJB_NUMBER_NOT_VALID;
    value->numeric[0] = '\0';

    return true;
}

// Return true if the codepoint is combining
MJB_EXPORT bool mjb_codepoint_is_combining(mjb_codepoint codepoint) {
    mjb_category category;

    if(!mjb_codepoint_is_valid(codepoint) ||
        !mjb_unicode_category_lookup(codepoint, &category)) {
        return false;
    }

    return mjb_category_is_combining(category);
}
