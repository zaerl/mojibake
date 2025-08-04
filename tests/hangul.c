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

    // Test empty input
    size_t result_len;
    mjb_codepoint *result = mjb_hangul_syllable_composition(NULL, 0, &result_len);
    ATT_ASSERT(result, NULL, "Empty input should return NULL");
    ATT_ASSERT(result_len, 0, "Empty input should have length 0");

    // Test single character (no composition)
    mjb_codepoint single_char[] = {0x1100}; // HANGUL CHOSEONG KIYEOK
    result = mjb_hangul_syllable_composition(single_char, 1, &result_len);
    ATT_ASSERT(result_len, 1, "Single character should have length 1");
    ATT_ASSERT(result[0], 0x1100, "Single character should remain unchanged");
    mjb_free(result);

    // Test LV composition (L + V)
    mjb_codepoint lv_input[] = {0x1100, 0x1161}; // KIYEOK + A
    result = mjb_hangul_syllable_composition(lv_input, 2, &result_len);
    ATT_ASSERT(result_len, 1, "LV composition should produce 1 syllable");
    ATT_ASSERT(result[0], 0xAC00, "LV composition should produce GA (가)");
    mjb_free(result);

    // Test LVT composition (L + V + T)
    mjb_codepoint lvt_input[] = {0x1100, 0x1161, 0x11A8}; // KIYEOK + A + KIYEOK
    result = mjb_hangul_syllable_composition(lvt_input, 3, &result_len);
    ATT_ASSERT(result_len, 1, "LVT composition should produce 1 syllable");
    ATT_ASSERT(result[0], 0xAC01, "LVT composition should produce GAG (각)");
    mjb_free(result);

    // Test multiple syllables
    mjb_codepoint multi_input[] = {0x1100, 0x1161, 0x11A8, 0x1102, 0x1161}; // GAG + NIEUN + A
    result = mjb_hangul_syllable_composition(multi_input, 5, &result_len);
    ATT_ASSERT(result_len, 2, "Multiple syllables should produce 2 syllables");
    ATT_ASSERT(result[0], 0xAC01, "First syllable should be GAG (각)");
    ATT_ASSERT(result[1], 0xB098, "Second syllable should be NA (나)");
    mjb_free(result);

    // Test non-composable sequences
    mjb_codepoint non_composable[] = {0x1100, 0x1101, 0x1102}; // KIYEOK + SSANGKIYEOK + NIEUN (all L)
    result = mjb_hangul_syllable_composition(non_composable, 3, &result_len);
    ATT_ASSERT(result_len, 3, "Non-composable should remain separate");
    ATT_ASSERT(result[0], 0x1100, "First character should remain unchanged");
    ATT_ASSERT(result[1], 0x1101, "Second character should remain unchanged");
    ATT_ASSERT(result[2], 0x1102, "Third character should remain unchanged");
    mjb_free(result);

    // Test complex sequence
    mjb_codepoint complex_input[] = {
        0x1100, 0x1161, 0x11A8,  // GAG (각)
        0x1102, 0x1161, 0x11A8,  // NAG (낙)
        0x1103, 0x1161, 0x11A8   // DAG (닥)
    };
    result = mjb_hangul_syllable_composition(complex_input, 9, &result_len);
    ATT_ASSERT(result_len, 3, "Complex sequence should produce 3 syllables");
    ATT_ASSERT(result[0], 0xAC01, "First syllable should be GAG (각)");
    ATT_ASSERT(result[1], 0xB099, "Second syllable should be NAG (낙)");
    ATT_ASSERT(result[2], 0xB2E5, "Third syllable should be DAG (닥)");
    mjb_free(result);

    // Test with invalid trailing consonant
    mjb_codepoint invalid_t[] = {0x1100, 0x1161, 0x11C3}; // KIYEOK + A + invalid T (out of range)
    result = mjb_hangul_syllable_composition(invalid_t, 3, &result_len);
    ATT_ASSERT(result_len, 2, "Invalid T should not compose, LV should compose");
    ATT_ASSERT(result[0], 0xAC00, "LV should compose to GA");
    ATT_ASSERT(result[1], 0x11C3, "Invalid T should remain unchanged");
    mjb_free(result);

    // Test boundary conditions
    mjb_codepoint boundary_input[] = {
        0x1112, 0x1175, 0x11C2,  // Last valid L + V + T combination
        0x1100, 0x1161            // First valid L + V combination
    };
    result = mjb_hangul_syllable_composition(boundary_input, 5, &result_len);
    ATT_ASSERT(result_len, 2, "Boundary test should produce 2 syllables");
    ATT_ASSERT(result[0], 0xD7A3, "First syllable should be last valid Hangul syllable");
    ATT_ASSERT(result[1], 0xAC00, "Second syllable should be first valid Hangul syllable");
    mjb_free(result);

    return NULL;
}
