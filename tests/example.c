/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"

int test_example(void *arg) {
    char test_buffer[128];

    // This function is automatically generated. Do not edit.
{
    // Example for mjb_codepoint_character
    MJB_TEST_COVERAGE(mjb_codepoint_character); // Added by the script
    mjb_character character;

    if(mjb_codepoint_character(0x022A, &character) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_codepoint_character test failed") // Added by the script
        return 1;
    }

    // U+022A lowercase: U+022B
    // printf("U+%04X lowercase: U+%04X", character.codepoint, character.lowercase);
    snprintf(test_buffer, sizeof(test_buffer), "U+%04X lowercase: U+%04X", character.codepoint, character.lowercase); // Added by the script
    ATT_ASSERT(test_buffer, "U+022A lowercase: U+022B", "mjb_codepoint_character test failed") // Added by the script
}

{
    // Example for mjb_normalize
    MJB_TEST_COVERAGE(mjb_normalize); // Added by the script
    const char *input = "Cafe\xCC\x81"; // "Cafe" + U+0301 COMBINING ACUTE ACCENT
    mjb_result result;

    if(mjb_normalize(input, strlen(input), MJB_NORMALIZATION_NFC, MJB_ENC_UTF_8, MJB_ENC_UTF_8,
        &result) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_normalize test failed") // Added by the script
        return 1;
    }

    // NFC: Café
    // printf("NFC: %.*s", (int)result.output_size, result.output);
    snprintf(test_buffer, sizeof(test_buffer), "NFC: %.*s", (int)result.output_size, result.output); // Added by the script
    ATT_ASSERT(test_buffer, "NFC: Café", "mjb_normalize test failed") // Added by the script

    if(result.transformed) {
        mjb_free(result.output);
    }
}

{
    // Example for mjb_string_filter
    MJB_TEST_COVERAGE(mjb_string_filter); // Added by the script
    const char *mixed_whitespace = "Hello\t\t\n\nworld";
    mjb_result result;

    if(mjb_string_filter(mixed_whitespace, strlen(mixed_whitespace), MJB_ENC_UTF_8, MJB_ENC_UTF_8,
        MJB_FILTER_COLLAPSE_SPACES, &result) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_string_filter test failed") // Added by the script
        return 1;
    }

    // Filtered: Hello world
    // printf("Filtered: %.*s", (int)result.output_size, result.output);
    snprintf(test_buffer, sizeof(test_buffer), "Filtered: %.*s", (int)result.output_size, result.output); // Added by the script
    ATT_ASSERT(test_buffer, "Filtered: Hello world", "mjb_string_filter test failed") // Added by the script

    if(result.transformed) {
        mjb_free(result.output);
    }

    const char *controls = "\x1\x2\t\n\v\f\r\x1f";

    if(mjb_string_filter(controls, strlen(controls), MJB_ENC_UTF_8, MJB_ENC_UTF_8,
        MJB_FILTER_CONTROLS, &result) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_string_filter test failed") // Added by the script
        return 1;
    }

    // Filtered: \t\n\v\f\r
    // printf("Filtered: %.*s", (int)result.output_size, result.output);
    snprintf(test_buffer, sizeof(test_buffer), "Filtered: %.*s", (int)result.output_size, result.output); // Added by the script
    ATT_ASSERT(test_buffer, "Filtered: \t\n\v\f\r", "mjb_string_filter test failed") // Added by the script

    if(result.transformed) {
        mjb_free(result.output);
    }
}

{
    // Example for mjb_nfkc_casefold
    MJB_TEST_COVERAGE(mjb_nfkc_casefold); // Added by the script
    const char *input = "Stra\xC3\x9F" "e\xC2\xAD";
    mjb_result result;

    if(mjb_nfkc_casefold(input, strlen(input), MJB_ENC_UTF_8, MJB_ENC_UTF_8,
        &result) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_nfkc_casefold test failed") // Added by the script
        return 1;
    }

    // strasse
    // printf("%.*s", (int)result.output_size, result.output);
    snprintf(test_buffer, sizeof(test_buffer), "%.*s", (int)result.output_size, result.output); // Added by the script
    ATT_ASSERT(test_buffer, "strasse", "mjb_nfkc_casefold test failed") // Added by the script
    mjb_result_free(&result);
}

{
    // Example for mjb_codepoint_numeric_value
    MJB_TEST_COVERAGE(mjb_codepoint_numeric_value); // Added by the script
    mjb_numeric_value num;

    if(mjb_codepoint_numeric_value(0x0031, &num) != MJB_STATUS_OK) { // U+0031 = 1
        ATT_ASSERT(0, 1, "mjb_codepoint_numeric_value test failed") // Added by the script
        return 1;
    }

    // decimal=1, digit=1, numeric=1
    // printf("decimal=%d, digit=%d, numeric=%s", num.decimal, num.digit, num.numeric);
    snprintf(test_buffer, sizeof(test_buffer), "decimal=%d, digit=%d, numeric=%s", num.decimal, num.digit, num.numeric); // Added by the script
    ATT_ASSERT(test_buffer, "decimal=1, digit=1, numeric=1", "mjb_codepoint_numeric_value test failed") // Added by the script

    if(mjb_codepoint_numeric_value(0x00BD, &num) != MJB_STATUS_OK) { // U+00BD = '½'
        ATT_ASSERT(0, 1, "mjb_codepoint_numeric_value test failed") // Added by the script
        return 1;
    }

    // decimal=-1, digit=-1, numeric=1/2
    // printf("decimal=%d, digit=%d, numeric=%s", num.decimal, num.digit, num.numeric);
    snprintf(test_buffer, sizeof(test_buffer), "decimal=%d, digit=%d, numeric=%s", num.decimal, num.digit, num.numeric); // Added by the script
    ATT_ASSERT(test_buffer, "decimal=-1, digit=-1, numeric=1/2", "mjb_codepoint_numeric_value test failed") // Added by the script
}

{
    // Example for mjb_case
    MJB_TEST_COVERAGE(mjb_case); // Added by the script
    const char *input = "Stra\xC3\x9F""e"; // "Straße"
    mjb_result result;

    if(mjb_case(input, strlen(input), MJB_CASE_UPPER, MJB_ENC_UTF_8, MJB_ENC_UTF_8,
        &result) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_case test failed") // Added by the script
        return 1;
    }

    // Upper: STRASSE
    // printf("Upper: %.*s", (int)result.output_size, result.output);
    snprintf(test_buffer, sizeof(test_buffer), "Upper: %.*s", (int)result.output_size, result.output); // Added by the script
    ATT_ASSERT(test_buffer, "Upper: STRASSE", "mjb_case test failed") // Added by the script

    if(result.transformed) {
        mjb_free(result.output);
    }
}

{
    // Example for mjb_codepoint_to_lowercase
    MJB_TEST_COVERAGE(mjb_codepoint_to_lowercase); // Added by the script
    mjb_codepoint codepoint;

    codepoint = mjb_codepoint_to_lowercase(0x0041); // U+0041 = 'A'

    // A > a
    // printf("%c > %c", 'A', codepoint);
    snprintf(test_buffer, sizeof(test_buffer), "%c > %c", 'A', codepoint); // Added by the script
    ATT_ASSERT(test_buffer, "A > a", "mjb_codepoint_to_lowercase test failed") // Added by the script

    codepoint = mjb_codepoint_to_lowercase(0x03A3); // U+03A3 = 'Σ'

    // U+03A3 > U+03C3, Σ > σ
    // printf("U+%04X > U+%04X, %s > %s",  0x03A3, codepoint, "Σ", "σ");
    snprintf(test_buffer, sizeof(test_buffer), "U+%04X > U+%04X, %s > %s",  0x03A3, codepoint, "Σ", "σ"); // Added by the script
    ATT_ASSERT(test_buffer, "U+03A3 > U+03C3, Σ > σ", "mjb_codepoint_to_lowercase test failed") // Added by the script
}
    return 0;
}
