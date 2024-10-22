/**
 * The mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include "mojibake.h"

// Arrays holding the names of the individual components
static const char* mjb_choseong_names[] = {
    "G", "GG", "N", "D", "DD", "R", "M", "B", "BB", "S", "SS", "NG", "J", "JJ", "C", "K", "T", "P", "H"
};

static const char* mjb_jungseong_names[] = {
    "A", "AE", "YA", "YAE", "EO", "E", "YEO", "YE", "O", "WA", "WAE", "OE", "YO", "U", "WEO", "WE", "WI",
    "YU", "EU", "YI", "I"
};

static const char* mjb_jongseong_names[] = {
    "", "G", "GG", "GS", "N", "NJ", "NH", "D", "R", "RG", "RM", "RB", "RS", "RT", "RP", "RH", "M", "B",
    "BS", "S", "SS", "NG", "J", "CH", "K", "T", "P", "H"
};

// Hangul syllable name
MJB_EXPORT bool mjb_hangul_syllable_name(mjb_codepoint codepoint, char *buffer, size_t size) {
    if(!mjb_codepoint_is_hangul_syllable(codepoint)) {
        return false;
    }

    // Calculate the index in the Hangul syllable block
    unsigned int syllable_index = codepoint - MJB_CODEPOINT_HANGUL_START;
    unsigned int choseong_index = syllable_index / 588;
    unsigned int jungseong_index = (syllable_index % 588) / 28;
    unsigned int jongseong_index = syllable_index % 28;

    // Print the full name of the syllable
    snprintf(buffer, size, "HANGUL SYLLABLE %s%s%s", mjb_choseong_names[choseong_index],
        mjb_jungseong_names[jungseong_index], mjb_jongseong_names[jongseong_index]);

    return false;
}

// Hangul syllable decomposition
bool mjb_hangul_syllable_decomposition(mjb_codepoint codepoint, mjb_codepoint *codepoints) {
    if(!mjb_codepoint_is_hangul_syllable(codepoint)) {
        return false;
    }

    // Calculate the index in the Hangul syllable block
    unsigned int syllable_index = codepoint - MJB_CODEPOINT_HANGUL_START;
    unsigned int choseong_index = syllable_index / 588;
    unsigned int jungseong_index = (syllable_index % 588) / 28;
    unsigned int jongseong_index = syllable_index % 28;

    // Print NFD Decomposition
    // Cho-seong component
    codepoints[0] = MJB_CODEPOINT_CHOSEONG_BASE + choseong_index;

    // Jung-seong component
    codepoints[1] = MJB_CODEPOINT_JUNGSEONG_BASE + jungseong_index;

    // Jong-seong component (if present)
    if(jongseong_index != 0) {
        codepoints[2] = MJB_CODEPOINT_JONGSEONG_BASE + jongseong_index;
    } else {
        codepoints[2] = 0;
    }

    return true;
}

// Return the codepoint titlecase codepoint
MJB_EXPORT bool mjb_codepoint_is_hangul_syllable(mjb_codepoint codepoint) {
    return codepoint >= MJB_CODEPOINT_HANGUL_START && codepoint <= MJB_CODEPOINT_HANGUL_END;
}
