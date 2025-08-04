/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "../src/mojibake.h"
#include "test.h"

void *test_hangul(void *arg) {
    mjb_character character;

    // CURRENT_ASSERT mjb_hangul_syllable_name
    mjb_hangul_syllable_name(MJB_CP_HANGUL_S_BASE, character.name, 128);
    ATT_ASSERT(character.name, "HANGUL SYLLABLE GA", "First syllable")

    mjb_hangul_syllable_name(0xAC01, character.name, 128);
    ATT_ASSERT(character.name, "HANGUL SYLLABLE GAG", "GAG")

    mjb_hangul_syllable_name(0xB155, character.name, 128);
    ATT_ASSERT(character.name, "HANGUL SYLLABLE NYEONG", "NYEONG")

    mjb_hangul_syllable_name(0xC1A4, character.name, 128);
    ATT_ASSERT(character.name, "HANGUL SYLLABLE SOK", "SOK")

    mjb_hangul_syllable_name(0xCE59, character.name, 128);
    ATT_ASSERT(character.name, "HANGUL SYLLABLE CIG", "CIG")

    mjb_hangul_syllable_name(0xCE58, character.name, 128);
    ATT_ASSERT(character.name, "HANGUL SYLLABLE CI", "CI")

    mjb_hangul_syllable_name(0xD3C9, character.name, 128);
    ATT_ASSERT(character.name, "HANGUL SYLLABLE PYEONG", "PYEONG")

    mjb_hangul_syllable_name(MJB_CP_HANGUL_S_BASE + MJB_CP_HANGUL_S_COUNT - 1, character.name, 128);
    ATT_ASSERT(character.name, "HANGUL SYLLABLE HIH", "Last syllable")

    ATT_ASSERT(mjb_codepoint_is_hangul_syllable(MJB_CP_HANGUL_S_BASE), true, "Hangul start")

    return NULL;
}
