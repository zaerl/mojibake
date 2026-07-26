/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "mojibake-internal.h"
#include "unicode-tables.h"

/**
 * A smaller version of mjb_codepoint_info() that only returns the character information.
 * This is used to avoid the overhead of the full normalization process.
 */
bool mjb_n_codepoint_character(mjb_codepoint codepoint, mjb_n_character *character) {
    if(character == NULL || codepoint > MJB_CODEPOINT_MAX ||
        (codepoint >= 0xD800 && codepoint <= 0xDFFF)) {
        return false;
    }

    // Check for Hangul syllables and CJK ideographs first, as they are common and can be handled
    // quickly.
    if(mjb_codepoint_is_hangul_syllable(codepoint) || mjb_codepoint_is_cjk_ideograph(codepoint)) {
        character->codepoint = codepoint;
        character->combining = MJB_CCC_NOT_REORDERED;
        character->decomposition = MJB_DECOMPOSITION_NONE;
        character->quick_check = MJB_QC_YES;

        return true;
    }

    if(mjb_unicode_n_character_lookup(codepoint, character)) {
        return true;
    }

    // Unassigned codepoints are inert normalization starters. They still have to be preserved:
    // normalization must never delete a valid scalar value merely because it has no UCD record.
    character->codepoint = codepoint;
    character->combining = MJB_CCC_NOT_REORDERED;
    character->decomposition = MJB_DECOMPOSITION_NONE;
    character->quick_check = MJB_QC_YES;

    return true;
}
