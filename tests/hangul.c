/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"

ATT_TEST(hangul) {
    mjb_character character;

    MJB_TEST_COVERAGE(mjb_hangul_syllable_name);
    ATT_ASSERT_STATUS(mjb_hangul_syllable_name(MJB_CP_HANGUL_S_BASE, NULL, 128),
        MJB_STATUS_INVALID_ARGUMENT, "Hangul syllable name rejects NULL buffer");
    ATT_ASSERT_STATUS(mjb_hangul_syllable_name(MJB_CP_HANGUL_S_BASE, character.name, 0),
        MJB_STATUS_INVALID_ARGUMENT, "Hangul syllable name rejects zero buffer size");
    ATT_ASSERT_STATUS(mjb_hangul_syllable_name(MJB_CP_HANGUL_S_BASE, character.name, 128),
        MJB_STATUS_OK, "First syllable name");
    ATT_ASSERT((char *)character.name, (char *)"HANGUL SYLLABLE GA", "First syllable");

    ATT_ASSERT_STATUS(mjb_hangul_syllable_name(0xAC01, character.name, 128), MJB_STATUS_OK,
        "GAG name");
    ATT_ASSERT((char *)character.name, (char *)"HANGUL SYLLABLE GAG", "GAG");

    ATT_ASSERT_STATUS(mjb_hangul_syllable_name(0xB155, character.name, 128), MJB_STATUS_OK,
        "NYEONG name");
    ATT_ASSERT((char *)character.name, (char *)"HANGUL SYLLABLE NYEONG", "NYEONG");

    ATT_ASSERT_STATUS(mjb_hangul_syllable_name(0xC1A4, character.name, 128), MJB_STATUS_OK,
        "SOK name");
    ATT_ASSERT((char *)character.name, (char *)"HANGUL SYLLABLE SOK", "SOK");

    ATT_ASSERT_STATUS(mjb_hangul_syllable_name(0xCE59, character.name, 128), MJB_STATUS_OK,
        "CIG name");
    ATT_ASSERT((char *)character.name, (char *)"HANGUL SYLLABLE CIG", "CIG");

    ATT_ASSERT_STATUS(mjb_hangul_syllable_name(0xCE58, character.name, 128), MJB_STATUS_OK,
        "CI name");
    ATT_ASSERT((char *)character.name, (char *)"HANGUL SYLLABLE CI", "CI");

    ATT_ASSERT_STATUS(mjb_hangul_syllable_name(0xD3C9, character.name, 128), MJB_STATUS_OK,
        "PYEONG name");
    ATT_ASSERT((char *)character.name, (char *)"HANGUL SYLLABLE PYEONG", "PYEONG");

    ATT_ASSERT_STATUS(mjb_hangul_syllable_name(MJB_CP_HANGUL_S_BASE + MJB_CP_HANGUL_S_COUNT - 1,
                          character.name, 128),
        MJB_STATUS_OK, "Last syllable name");
    ATT_ASSERT((char *)character.name, (char *)"HANGUL SYLLABLE HIH", "Last syllable");

    ATT_ASSERT(mjb_codepoint_is_hangul_syllable(MJB_CP_HANGUL_S_BASE), true, "Hangul start");

    ATT_ASSERT(mjb_codepoint_is_hangul_jamo(0x1100), true, "Hangul Jamo - Choseong Kiyeok");
    ATT_ASSERT(mjb_codepoint_is_hangul_jamo(0x11AB), true, "Hangul Jamo - Jongseong Nieun");
    ATT_ASSERT(mjb_codepoint_is_hangul_jamo(0x1161), true, "Hangul Jamo - Jungseong A");
    ATT_ASSERT(mjb_codepoint_is_hangul_jamo(0x0000), false, "Not a Hangul Jamo");

    ATT_ASSERT(mjb_codepoint_is_hangul_leading_jamo(0x1100), true, "Hangul L - Choseong Kiyeok");
    ATT_ASSERT(mjb_codepoint_is_hangul_leading_jamo(0x1112), true, "Hangul L - Choseong Hieuh");
    ATT_ASSERT(mjb_codepoint_is_hangul_leading_jamo(0x1161), false, "Not a Hangul L");

    ATT_ASSERT(mjb_codepoint_is_hangul_vowel_jamo(0x1161), true, "Hangul V - Jungseong A");
    ATT_ASSERT(mjb_codepoint_is_hangul_vowel_jamo(0x1175), true, "Hangul V - Jungseong I");
    ATT_ASSERT(mjb_codepoint_is_hangul_vowel_jamo(0x1100), false, "Not a Hangul V");

    ATT_ASSERT(mjb_codepoint_is_hangul_trailing_jamo(0x11A8), true, "Hangul T - Jongseong Kiyeok");
    ATT_ASSERT(mjb_codepoint_is_hangul_trailing_jamo(0x11C2), true, "Hangul T - Jongseong Hieuh");
    ATT_ASSERT(mjb_codepoint_is_hangul_trailing_jamo(0x1100), false, "Not a Hangul T");
    ATT_ASSERT(mjb_codepoint_is_hangul_trailing_jamo(0x11A7), false,
        "Not a Hangul T - Jungseong O-YAE");

    mjb_codepoint codepoints[3];
    ATT_ASSERT_STATUS(mjb_hangul_syllable_decompose(0xAC01, NULL), MJB_STATUS_INVALID_ARGUMENT,
        "Hangul syllable decomposition rejects NULL buffer");
    ATT_ASSERT_STATUS(mjb_hangul_syllable_decompose(0xAC01, codepoints), MJB_STATUS_OK,
        "Hangul syllable decomposition");
    MJB_TEST_COVERAGE(mjb_hangul_syllable_decompose);
    ATT_ASSERT(codepoints[0], (mjb_codepoint)0x1100, "Hangul L - Choseong Kiyeok");
    ATT_ASSERT(codepoints[1], (mjb_codepoint)0x1161, "Hangul V - Jungseong A");
    ATT_ASSERT(codepoints[2], (mjb_codepoint)0x11A8, "Hangul T - Jongseong Kiyeok");

    MJB_TEST_COVERAGE(mjb_hangul_syllable_compose);
    mjb_codepoint syllable = 0;
    ATT_ASSERT_STATUS(mjb_hangul_syllable_compose(0x1100, 0x1161, 0, NULL),
        MJB_STATUS_INVALID_ARGUMENT, "Hangul syllable composition rejects NULL output");
    ATT_ASSERT_STATUS(mjb_hangul_syllable_compose(0x1100, 0x1161, 0, &syllable), MJB_STATUS_OK,
        "Hangul syllable composition LV");
    ATT_ASSERT(syllable, (mjb_codepoint)0xAC00, "Hangul LV composes to GA");
    ATT_ASSERT_STATUS(mjb_hangul_syllable_compose(0x1100, 0x1161, 0x11A8, &syllable),
        MJB_STATUS_OK, "Hangul syllable composition LVT");
    ATT_ASSERT(syllable, (mjb_codepoint)0xAC01, "Hangul LVT composes to GAG");
    ATT_ASSERT_STATUS(mjb_hangul_syllable_compose(0x1112, 0x1175, 0x11C2, &syllable),
        MJB_STATUS_OK, "Hangul syllable composition of the last jamo");
    ATT_ASSERT(syllable, (mjb_codepoint)0xD7A3, "Hangul last jamo compose to the last syllable");

    syllable = 0;
    ATT_ASSERT_STATUS(mjb_hangul_syllable_compose(0x1161, 0x1161, 0, &syllable),
        MJB_STATUS_NOT_FOUND, "Hangul syllable composition rejects a non-leading jamo");
    ATT_ASSERT(syllable, MJB_CODEPOINT_NOT_VALID, "Hangul failed composition clears output");
    ATT_ASSERT_STATUS(mjb_hangul_syllable_compose(0x1100, 0x1100, 0, &syllable),
        MJB_STATUS_NOT_FOUND, "Hangul syllable composition rejects a non-vowel jamo");
    ATT_ASSERT_STATUS(mjb_hangul_syllable_compose(0x1100, 0x1161, 0x11A7, &syllable),
        MJB_STATUS_NOT_FOUND, "Hangul syllable composition rejects the trailing base");
    ATT_ASSERT_STATUS(mjb_hangul_syllable_compose(0x1100, 0x1161, 0x11C3, &syllable),
        MJB_STATUS_NOT_FOUND, "Hangul syllable composition rejects an out-of-range trailing jamo");

    // Every syllable decomposes and composes back to itself
    bool round_trip = true;

    for(mjb_codepoint s = MJB_CP_HANGUL_S_BASE; s < MJB_CP_HANGUL_S_BASE + MJB_CP_HANGUL_S_COUNT;
        ++s) {
        if(mjb_hangul_syllable_decompose(s, codepoints) != MJB_STATUS_OK ||
            mjb_hangul_syllable_compose(codepoints[0], codepoints[1], codepoints[2], &syllable) !=
                MJB_STATUS_OK ||
            syllable != s) {
            round_trip = false;
            break;
        }
    }

    ATT_ASSERT(round_trip, true, "Hangul syllables round-trip through decompose and compose");
}
