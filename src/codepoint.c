/**
 * The mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <string.h>
#include "mojibake.h"

static const mjb_character empty_character;

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
    mjb_initialize();

    if(character == NULL || !mjb_codepoint_is_valid(codepoint)) {
        return false;
    }

    // Reset character
    *character = empty_character;
    // *character = mjb_characters[codepoint];

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
