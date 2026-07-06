/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "mojibake-internal.h"
#include "unicode-tables.h"

 /**
 * A smaller version of mjb_codepoint_character() that only returns the character information.
 * This is used to avoid the overhead of the full normalization process.
 */
bool mjb_n_codepoint_character(mjb_codepoint codepoint, mjb_n_character *character) {
    if(character == NULL || !mjb_codepoint_is_valid(codepoint)) {
        return false;
    }

    // Check for Hangul syllables and CJK ideographs first, as they are common and can be handled quickly.
    if(mjb_codepoint_is_hangul_syllable(codepoint) || mjb_codepoint_is_cjk_ideograph(codepoint)) {
        character->codepoint = codepoint;
        character->combining = MJB_CCC_NOT_REORDERED;
        character->decomposition = MJB_DECOMPOSITION_NONE;
        character->quick_check = MJB_QC_YES;

        return true;
    }

    return mjb_unicode_n_character_lookup(codepoint, character);
}
