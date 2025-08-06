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

    ATT_ASSERT(mjb_codepoint_is_hangul_jamo(0x1100), true, "Hangul Jamo - Choseong Kiyeok")
    ATT_ASSERT(mjb_codepoint_is_hangul_jamo(0x11A7), true, "Hangul Jamo - Jongseong Nieun")
    ATT_ASSERT(mjb_codepoint_is_hangul_jamo(0x1161), true, "Hangul Jamo - Jungseong A")
    ATT_ASSERT(mjb_codepoint_is_hangul_jamo(0x0000), false, "Not a Hangul Jamo")

    ATT_ASSERT(mjb_codepoint_is_hangul_l(0x1100), true, "Hangul L - Choseong Kiyeok")
    ATT_ASSERT(mjb_codepoint_is_hangul_l(0x1112), true, "Hangul L - Choseong Hieuh")
    ATT_ASSERT(mjb_codepoint_is_hangul_l(0x1161), false, "Not a Hangul L")

    ATT_ASSERT(mjb_codepoint_is_hangul_v(0x1161), true, "Hangul V - Jungseong A")
    ATT_ASSERT(mjb_codepoint_is_hangul_v(0x1175), true, "Hangul V - Jungseong I")
    ATT_ASSERT(mjb_codepoint_is_hangul_v(0x1100), false, "Not a Hangul V")

    ATT_ASSERT(mjb_codepoint_is_hangul_t(0x11A8), true, "Hangul T - Jongseong Kiyeok")
    ATT_ASSERT(mjb_codepoint_is_hangul_t(0x11C2), true, "Hangul T - Jongseong Hieuh")
    ATT_ASSERT(mjb_codepoint_is_hangul_t(0x1100), false, "Not a Hangul T")

    mjb_codepoint codepoints[3];
    ATT_ASSERT(mjb_hangul_syllable_decomposition(0xAC01, codepoints), true, "Hangul syllable decomposition")
    ATT_ASSERT(codepoints[0], 0x1100, "Hangul L - Choseong Kiyeok")
    ATT_ASSERT(codepoints[1], 0x1161, "Hangul V - Jungseong A")
    ATT_ASSERT(codepoints[2], 0x11A8, "Hangul T - Jongseong Kiyeok")

    return NULL;
}
