/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include "mojibake.h"

// See: https://www.unicode.org/versions/Unicode16.0.0/core-spec/chapter-3/#G61399

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

// Return hangul syllable name
MJB_EXPORT bool mjb_hangul_syllable_name(mjb_codepoint codepoint, char *buffer, size_t size) {
    if(!mjb_codepoint_is_hangul_syllable(codepoint)) {
        return false;
    }

    unsigned int s_index = codepoint - MJB_CP_HANGUL_S_BASE;
    unsigned int l_index = s_index / MJB_CP_HANGUL_N_COUNT;
    unsigned int v_index = (s_index % MJB_CP_HANGUL_N_COUNT) / MJB_CP_HANGUL_T_COUNT;
    unsigned int t_index = s_index % MJB_CP_HANGUL_T_COUNT;

    // Print the full name of the syllable
    snprintf(buffer, size, "HANGUL SYLLABLE %s%s%s", mjb_choseong_names[l_index],
        mjb_jungseong_names[v_index], mjb_jongseong_names[t_index]);

    return true;
}

// Hangul syllable decomposition
bool mjb_hangul_syllable_decomposition(mjb_codepoint codepoint, mjb_codepoint *codepoints) {
    if(!mjb_codepoint_is_hangul_syllable(codepoint)) {
        return false;
    }

    unsigned int s_index = codepoint - MJB_CP_HANGUL_S_BASE;
    unsigned int l = MJB_CP_HANGUL_L_BASE + s_index / MJB_CP_HANGUL_N_COUNT;
    unsigned int v = MJB_CP_HANGUL_V_BASE + (s_index % MJB_CP_HANGUL_N_COUNT) / MJB_CP_HANGUL_T_COUNT;
    unsigned int t = MJB_CP_HANGUL_T_BASE + s_index % MJB_CP_HANGUL_T_COUNT;

    codepoints[0] = l;
    codepoints[1] = v;

    if(t != MJB_CP_HANGUL_T_BASE) {
        codepoints[2] = t;
    } else {
        codepoints[2] = 0;
    }

    return true;
}

// Return if the codepoint is an hangul syllable
MJB_EXPORT bool mjb_codepoint_is_hangul_syllable(mjb_codepoint codepoint) {
    unsigned int syllable_index = codepoint - MJB_CP_HANGUL_S_BASE;

    if(syllable_index < 0 || syllable_index >= MJB_CP_HANGUL_S_COUNT) {
        return false;
    }

    return true;
}
