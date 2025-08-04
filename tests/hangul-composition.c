/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

 #include "../src/mojibake.h"
 #include "test.h"

 void *test_hangul_composition(void *arg) {
     // CURRENT_ASSERT mjb_hangul_syllable_composition
     mjb_character empty_array[1] = {{0}}; // Empty array for testing
     size_t result_len = mjb_hangul_syllable_composition(empty_array, 0);
     ATT_ASSERT(result_len, 0, "Empty input should have length 0");

     // Test single character (not Hangul syllable)
     mjb_character not_hangul[2];
     not_hangul[0].codepoint = 0x41; // LATIN CAPITAL LETTER A
     not_hangul[1].codepoint = 0x42; // LATIN CAPITAL LETTER B
     result_len = mjb_hangul_syllable_composition(not_hangul, 2);
     ATT_ASSERT(result_len, 2, "ASCII characters should have length 2");
     ATT_ASSERT(not_hangul[0].codepoint, 0x41, "Single not hangul character should return the same codepoint");
     ATT_ASSERT(not_hangul[1].codepoint, 0x42, "Single not hangul character should return the same codepoint");

     // Test single character (no composition)
     mjb_character single_hangul_char[1];
     single_hangul_char[0].codepoint = 0x1100; // HANGUL CHOSEONG KIYEOK
     result_len = mjb_hangul_syllable_composition(single_hangul_char, 1);
     ATT_ASSERT(result_len, 1, "Single character should have length 1");
     ATT_ASSERT(single_hangul_char[0].codepoint, 0x1100, "Single character should remain unchanged");

     // Test LV composition (L + V)
     mjb_character lv_input[2];
     lv_input[0].codepoint = 0x1100; // KIYEOK
     lv_input[1].codepoint = 0x1161; // A
     result_len = mjb_hangul_syllable_composition(lv_input, 2);
     ATT_ASSERT(result_len, 1, "LV composition should produce 1 syllable");
     ATT_ASSERT(lv_input[0].codepoint, 0xAC00, "LV composition should produce GA (가)");
     ATT_ASSERT(lv_input[1].codepoint, MJB_CODEPOINT_NOT_VALID, "Second character should be marked as invalid");

     // Test LVT composition (L + V + T)
     mjb_character lvt_input[3];
     lvt_input[0].codepoint = 0x1100; // KIYEOK
     lvt_input[1].codepoint = 0x1161; // A
     lvt_input[2].codepoint = 0x11A8; // KIYEOK
     result_len = mjb_hangul_syllable_composition(lvt_input, 3);
     ATT_ASSERT(result_len, 1, "LVT composition should produce 1 syllable");
     ATT_ASSERT(lvt_input[0].codepoint, 0xAC01, "LVT composition should produce GAG (각)");
     ATT_ASSERT(lvt_input[1].codepoint, MJB_CODEPOINT_NOT_VALID, "Second character should be marked as invalid");
     ATT_ASSERT(lvt_input[2].codepoint, MJB_CODEPOINT_NOT_VALID, "Third character should be marked as invalid");

     // Test multiple syllables
     mjb_character multi_input[5];
     multi_input[0].codepoint = 0x1100; // KIYEOK
     multi_input[1].codepoint = 0x1161; // A
     multi_input[2].codepoint = 0x11A8; // KIYEOK
     multi_input[3].codepoint = 0x1102; // NIEUN
     multi_input[4].codepoint = 0x1161; // A
     result_len = mjb_hangul_syllable_composition(multi_input, 5);
     ATT_ASSERT(result_len, 2, "Multiple syllables should produce 2 syllables");
     ATT_ASSERT(multi_input[0].codepoint, 0xAC01, "First syllable should be GAG (각)");
     ATT_ASSERT(multi_input[1].codepoint, 0xB098, "Second syllable should be NA (나)");
     ATT_ASSERT(multi_input[2].codepoint, MJB_CODEPOINT_NOT_VALID, "Third character should be marked as invalid");
     ATT_ASSERT(multi_input[3].codepoint, MJB_CODEPOINT_NOT_VALID, "Fourth character should be marked as invalid");
     ATT_ASSERT(multi_input[4].codepoint, MJB_CODEPOINT_NOT_VALID, "Fifth character should be marked as invalid");

     // Test non-composable sequences
     mjb_character non_composable[3];
     non_composable[0].codepoint = 0x1100; // KIYEOK
     non_composable[1].codepoint = 0x1101; // SSANGKIYEOK
     non_composable[2].codepoint = 0x1102; // NIEUN (all L)
     result_len = mjb_hangul_syllable_composition(non_composable, 3);
     ATT_ASSERT(result_len, 3, "Non-composable should remain separate");
     ATT_ASSERT(non_composable[0].codepoint, 0x1100, "First character should remain unchanged");
     ATT_ASSERT(non_composable[1].codepoint, 0x1101, "Second character should remain unchanged");
     ATT_ASSERT(non_composable[2].codepoint, 0x1102, "Third character should remain unchanged");

     // Test complex sequence
     mjb_character complex_input[9];
     complex_input[0].codepoint = 0x1100; // KIYEOK
     complex_input[1].codepoint = 0x1161; // A
     complex_input[2].codepoint = 0x11A8; // KIYEOK
     complex_input[3].codepoint = 0x1102; // NIEUN
     complex_input[4].codepoint = 0x1161; // A
     complex_input[5].codepoint = 0x11A8; // KIYEOK
     complex_input[6].codepoint = 0x1103; // TIKEUT
     complex_input[7].codepoint = 0x1161; // A
     complex_input[8].codepoint = 0x11A8; // KIYEOK
     result_len = mjb_hangul_syllable_composition(complex_input, 9);
     ATT_ASSERT(result_len, 3, "Complex sequence should produce 3 syllables");
     ATT_ASSERT(complex_input[0].codepoint, 0xAC01, "First syllable should be GAG (각)");
     ATT_ASSERT(complex_input[1].codepoint, 0xB099, "Second syllable should be NAG (낙)");
     ATT_ASSERT(complex_input[2].codepoint, 0xB2E5, "Third syllable should be DAG (닥)");
     ATT_ASSERT(complex_input[3].codepoint, MJB_CODEPOINT_NOT_VALID, "Fourth character should be marked as invalid");
     ATT_ASSERT(complex_input[4].codepoint, MJB_CODEPOINT_NOT_VALID, "Fifth character should be marked as invalid");
     ATT_ASSERT(complex_input[5].codepoint, MJB_CODEPOINT_NOT_VALID, "Sixth character should be marked as invalid");
     ATT_ASSERT(complex_input[6].codepoint, MJB_CODEPOINT_NOT_VALID, "Seventh character should be marked as invalid");
     ATT_ASSERT(complex_input[7].codepoint, MJB_CODEPOINT_NOT_VALID, "Eighth character should be marked as invalid");
     ATT_ASSERT(complex_input[8].codepoint, MJB_CODEPOINT_NOT_VALID, "Ninth character should be marked as invalid");

     // Test with invalid trailing consonant
     mjb_character invalid_t[3];
     invalid_t[0].codepoint = 0x1100; // KIYEOK
     invalid_t[1].codepoint = 0x1161; // A
     invalid_t[2].codepoint = 0x11C3; // invalid T (out of range)
     result_len = mjb_hangul_syllable_composition(invalid_t, 3);
     ATT_ASSERT(result_len, 2, "Invalid T should not compose, LV should compose");
     ATT_ASSERT(invalid_t[0].codepoint, 0xAC00, "LV should compose to GA");
     ATT_ASSERT(invalid_t[1].codepoint, 0x11C3, "Invalid T should remain unchanged");
     ATT_ASSERT(invalid_t[2].codepoint, MJB_CODEPOINT_NOT_VALID, "Third character should be marked as invalid");

     // Test boundary conditions
     mjb_character boundary_input[5];
     boundary_input[0].codepoint = 0x1112; // Last valid L
     boundary_input[1].codepoint = 0x1175; // Last valid V
     boundary_input[2].codepoint = 0x11C2; // Last valid T
     boundary_input[3].codepoint = 0x1100; // First valid L
     boundary_input[4].codepoint = 0x1161; // First valid V
     result_len = mjb_hangul_syllable_composition(boundary_input, 5);
     ATT_ASSERT(result_len, 2, "Boundary test should produce 2 syllables");
     ATT_ASSERT(boundary_input[0].codepoint, 0xD7A3, "First syllable should be last valid Hangul syllable");
     ATT_ASSERT(boundary_input[1].codepoint, 0xAC00, "Second syllable should be first valid Hangul syllable");
     ATT_ASSERT(boundary_input[2].codepoint, MJB_CODEPOINT_NOT_VALID, "Third character should be marked as invalid");
     ATT_ASSERT(boundary_input[3].codepoint, MJB_CODEPOINT_NOT_VALID, "Fourth character should be marked as invalid");
     ATT_ASSERT(boundary_input[4].codepoint, MJB_CODEPOINT_NOT_VALID, "Fifth character should be marked as invalid");

     return NULL;
 }
