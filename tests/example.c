/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"

// clang-format off
int test_example(void *arg) {
    char test_buffer[256];

    // This function is automatically generated. Do not edit.
{
    // Example for mjb_codepoint_info
    MJB_TEST_COVERAGE(mjb_codepoint_info); // Added by the script
    mjb_character character;

    if(mjb_codepoint_info(0x022A, &character) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_codepoint_info test failed") // Added by the script
        return 1;
    }

    // U+022A lowercase: U+022B
    // printf("U+%04X lowercase: U+%04X", character.codepoint, character.lowercase);
    snprintf(test_buffer, sizeof(test_buffer), "U+%04X lowercase: U+%04X", character.codepoint, character.lowercase); // Added by the script
    ATT_ASSERT(test_buffer, "U+022A lowercase: U+022B", "mjb_codepoint_info test failed") // Added by the script
}

{
    // Example for mjb_normalize
    MJB_TEST_COVERAGE(mjb_normalize); // Added by the script
    const char *input = "Cafe\xCC\x81"; // "Cafe" + U+0301 COMBINING ACUTE ACCENT
    mjb_result result;

    if(mjb_normalize(input, strlen(input), MJB_ENC_UTF_8, MJB_NORMALIZATION_NFC, MJB_ENC_UTF_8,
        &result) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_normalize test failed") // Added by the script
        return 1;
    }

    // NFC: Café
    // printf("NFC: %.*s", (int)result.output_size, result.output);
    snprintf(test_buffer, sizeof(test_buffer), "NFC: %.*s", (int)result.output_size, result.output); // Added by the script
    ATT_ASSERT(test_buffer, "NFC: Café", "mjb_normalize test failed") // Added by the script

    mjb_result_free(&result);
}

{
    // Example for mjb_normalize_into
    MJB_TEST_COVERAGE(mjb_normalize_into); // Added by the script
    const char *input = "Cafe\xCC\x81"; // "Cafe" + U+0301 COMBINING ACUTE ACCENT
    size_t output_size = 0;

    if(mjb_normalize_into(input, strlen(input), MJB_ENC_UTF_8, MJB_NORMALIZATION_NFC,
        MJB_ENC_UTF_8, NULL, &output_size) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_normalize_into test failed") // Added by the script
        return 1;
    }

    char output[5];

    if(output_size > sizeof(output) || mjb_normalize_into(input, strlen(input), MJB_ENC_UTF_8,
        MJB_NORMALIZATION_NFC, MJB_ENC_UTF_8, output, &output_size) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_normalize_into test failed") // Added by the script
        return 1;
    }

    // NFC payload (no terminator): Café
    // printf("NFC payload (no terminator): %.*s", (int)output_size, output);
    snprintf(test_buffer, sizeof(test_buffer), "NFC payload (no terminator): %.*s", (int)output_size, output); // Added by the script
    ATT_ASSERT(test_buffer, "NFC payload (no terminator): Café", "mjb_normalize_into test failed") // Added by the script
}

{
    // Example for mjb_filter
    MJB_TEST_COVERAGE(mjb_filter); // Added by the script
    const char *mixed_whitespace = "Hello\t\t\n\nworld";
    mjb_result result;

    if(mjb_filter(mixed_whitespace, strlen(mixed_whitespace), MJB_ENC_UTF_8,
        MJB_FILTER_COLLAPSE_SPACES, MJB_ENC_UTF_8, &result) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_filter test failed") // Added by the script
        return 1;
    }

    // Filtered: Hello world
    // printf("Filtered: %.*s", (int)result.output_size, result.output);
    snprintf(test_buffer, sizeof(test_buffer), "Filtered: %.*s", (int)result.output_size, result.output); // Added by the script
    ATT_ASSERT(test_buffer, "Filtered: Hello world", "mjb_filter test failed") // Added by the script

    mjb_result_free(&result);

    const char *controls = "\x1\x2\t\n\v\f\r\x1f";

    if(mjb_filter(controls, strlen(controls), MJB_ENC_UTF_8, MJB_FILTER_CONTROLS,
        MJB_ENC_UTF_8, &result) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_filter test failed") // Added by the script
        return 1;
    }

    // Filtered: \t\n\v\f\r
    // printf("Filtered: %.*s", (int)result.output_size, result.output);
    snprintf(test_buffer, sizeof(test_buffer), "Filtered: %.*s", (int)result.output_size, result.output); // Added by the script
    ATT_ASSERT(test_buffer, "Filtered: \t\n\v\f\r", "mjb_filter test failed") // Added by the script

    mjb_result_free(&result);
}

{
    // Example for mjb_filter_into
    MJB_TEST_COVERAGE(mjb_filter_into); // Added by the script
    const char *input = "Hello\t\t\nworld";
    size_t output_size = 0;

    if(mjb_filter_into(input, strlen(input), MJB_ENC_UTF_8, MJB_FILTER_COLLAPSE_SPACES,
        MJB_ENC_UTF_8, NULL, &output_size) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_filter_into test failed") // Added by the script
        return 1;
    }

    char output[11];

    if(output_size > sizeof(output) || mjb_filter_into(input, strlen(input), MJB_ENC_UTF_8,
        MJB_FILTER_COLLAPSE_SPACES, MJB_ENC_UTF_8, output, &output_size) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_filter_into test failed") // Added by the script
        return 1;
    }

    // Filtered payload (no terminator): Hello world
    // printf("Filtered payload (no terminator): %.*s", (int)output_size, output);
    snprintf(test_buffer, sizeof(test_buffer), "Filtered payload (no terminator): %.*s", (int)output_size, output); // Added by the script
    ATT_ASSERT(test_buffer, "Filtered payload (no terminator): Hello world", "mjb_filter_into test failed") // Added by the script
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
    // Example for mjb_nfkc_casefold_into
    MJB_TEST_COVERAGE(mjb_nfkc_casefold_into); // Added by the script
    const char *input = "Stra\xC3\x9F" "e\xC2\xAD";
    size_t output_size = 0;

    if(mjb_nfkc_casefold_into(input, strlen(input), MJB_ENC_UTF_8, MJB_ENC_UTF_8,
        NULL, &output_size) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_nfkc_casefold_into test failed") // Added by the script
        return 1;
    }

    char output[7];

    if(output_size > sizeof(output) || mjb_nfkc_casefold_into(input, strlen(input), MJB_ENC_UTF_8,
        MJB_ENC_UTF_8, output, &output_size) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_nfkc_casefold_into test failed") // Added by the script
        return 1;
    }

    // NFKC casefold payload (no terminator): strasse
    // printf("NFKC casefold payload (no terminator): %.*s", (int)output_size, output);
    snprintf(test_buffer, sizeof(test_buffer), "NFKC casefold payload (no terminator): %.*s", (int)output_size, output); // Added by the script
    ATT_ASSERT(test_buffer, "NFKC casefold payload (no terminator): strasse", "mjb_nfkc_casefold_into test failed") // Added by the script
}

{
    // Example for mjb_normalization_quick_check
    MJB_TEST_COVERAGE(mjb_normalization_quick_check); // Added by the script
    const char *input = "caf\xC3\xA9";
    mjb_quick_check_result check;

    if(mjb_normalization_quick_check(input, strlen(input), MJB_ENC_UTF_8,
        MJB_NORMALIZATION_NFC, &check) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_normalization_quick_check test failed") // Added by the script
        return 1;
    }

    // NFC normalized: yes
    // printf("NFC normalized: %s", check == MJB_QC_YES ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "NFC normalized: %s", check == MJB_QC_YES ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "NFC normalized: yes", "mjb_normalization_quick_check test failed") // Added by the script
}

{
    // Example for mjb_detect_encoding
    MJB_TEST_COVERAGE(mjb_detect_encoding); // Added by the script
    const char utf16le[] = "\xFF\xFEH\0i\0";
    mjb_encoding detected = mjb_detect_encoding(utf16le, sizeof(utf16le) - 1);
    bool is_utf16le = detected == (MJB_ENC_UTF_16 | MJB_ENC_UTF_16LE);

    // UTF-16LE detected: yes
    // printf("UTF-16LE detected: %s", is_utf16le ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "UTF-16LE detected: %s", is_utf16le ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "UTF-16LE detected: yes", "mjb_detect_encoding test failed") // Added by the script
}

{
    // Example for mjb_is_ascii
    MJB_TEST_COVERAGE(mjb_is_ascii); // Added by the script
    const char *input = "Plain ASCII";

    // ASCII: yes
    // printf("ASCII: %s", mjb_is_ascii(input, strlen(input)) ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "ASCII: %s", mjb_is_ascii(input, strlen(input)) ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "ASCII: yes", "mjb_is_ascii test failed") // Added by the script
}

{
    // Example for mjb_is_utf8
    MJB_TEST_COVERAGE(mjb_is_utf8); // Added by the script
    const char *input = "caf\xC3\xA9";

    // Valid UTF-8: yes
    // printf("Valid UTF-8: %s", mjb_is_utf8(input, strlen(input)) ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "Valid UTF-8: %s", mjb_is_utf8(input, strlen(input)) ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "Valid UTF-8: yes", "mjb_is_utf8 test failed") // Added by the script
}

{
    // Example for mjb_is_utf16
    MJB_TEST_COVERAGE(mjb_is_utf16); // Added by the script
    const char utf16be[] = "\xFE\xFF\0H\0i"; // BOM + "Hi" in UTF-16BE

    // UTF-16: yes
    // printf("UTF-16: %s", mjb_is_utf16(utf16be, sizeof(utf16be) - 1) ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "UTF-16: %s", mjb_is_utf16(utf16be, sizeof(utf16be) - 1) ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "UTF-16: yes", "mjb_is_utf16 test failed") // Added by the script
}

{
    // Example for mjb_count_codepoints
    MJB_TEST_COVERAGE(mjb_count_codepoints); // Added by the script
    // The "Héllö" string is five Unicode characters, but has different byte lengths in different encodings.

    const char *utf8 = "H\xC3\xA9ll\xC3\xB6"; // 7 bytes
    const char utf16le[] = "H\0\xE9\0l\0l\0\xF6\0"; // 10 bytes
    const char utf16be[] = "\0H\0\xE9\0l\0l\0\xF6"; // 10 bytes
    const char utf32le[] = "H\0\0\0\xE9\0\0\0l\0\0\0l\0\0\0\xF6\0\0\0"; // 20 bytes
    const char utf32be[] = "\0\0\0H\0\0\0\xE9\0\0\0l\0\0\0l\0\0\0\xF6"; // 20 bytes

    // 5 UTF-8 characters
    // printf("%zu UTF-8 characters", mjb_count_codepoints(utf8, 7, MJB_ENC_UTF_8));
    snprintf(test_buffer, sizeof(test_buffer), "%zu UTF-8 characters", mjb_count_codepoints(utf8, 7, MJB_ENC_UTF_8)); // Added by the script
    ATT_ASSERT(test_buffer, "5 UTF-8 characters", "mjb_count_codepoints test failed") // Added by the script
    // 5 UTF-16LE characters
    // printf("%zu UTF-16LE characters", mjb_count_codepoints(utf16le, 10, MJB_ENC_UTF_16LE));
    snprintf(test_buffer, sizeof(test_buffer), "%zu UTF-16LE characters", mjb_count_codepoints(utf16le, 10, MJB_ENC_UTF_16LE)); // Added by the script
    ATT_ASSERT(test_buffer, "5 UTF-16LE characters", "mjb_count_codepoints test failed") // Added by the script
    // 5 UTF-16BE characters
    // printf("%zu UTF-16BE characters", mjb_count_codepoints(utf16be, 10, MJB_ENC_UTF_16BE));
    snprintf(test_buffer, sizeof(test_buffer), "%zu UTF-16BE characters", mjb_count_codepoints(utf16be, 10, MJB_ENC_UTF_16BE)); // Added by the script
    ATT_ASSERT(test_buffer, "5 UTF-16BE characters", "mjb_count_codepoints test failed") // Added by the script
    // 5 UTF-32LE characters
    // printf("%zu UTF-32LE characters", mjb_count_codepoints(utf32le, 20, MJB_ENC_UTF_32LE));
    snprintf(test_buffer, sizeof(test_buffer), "%zu UTF-32LE characters", mjb_count_codepoints(utf32le, 20, MJB_ENC_UTF_32LE)); // Added by the script
    ATT_ASSERT(test_buffer, "5 UTF-32LE characters", "mjb_count_codepoints test failed") // Added by the script
    // 5 UTF-32BE characters
    // printf("%zu UTF-32BE characters", mjb_count_codepoints(utf32be, 20, MJB_ENC_UTF_32BE));
    snprintf(test_buffer, sizeof(test_buffer), "%zu UTF-32BE characters", mjb_count_codepoints(utf32be, 20, MJB_ENC_UTF_32BE)); // Added by the script
    ATT_ASSERT(test_buffer, "5 UTF-32BE characters", "mjb_count_codepoints test failed") // Added by the script
}

{
    // Example for mjb_for_each_character
    MJB_TEST_COVERAGE(mjb_for_each_character); // Added by the script
    mjb_status status = mjb_for_each_character("ABC", 3, MJB_ENC_UTF_8, NULL);

    // A callback is required: yes
    bool callback_required = status == MJB_STATUS_INVALID_ARGUMENT;

    // A callback is required: yes
    // printf("A callback is required: %s", callback_required ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "A callback is required: %s", callback_required ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "A callback is required: yes", "mjb_for_each_character test failed") // Added by the script
}

{
    // Example for mjb_codepoint_property_binary
    MJB_TEST_COVERAGE(mjb_codepoint_property_binary); // Added by the script
    bool is_alphabetic;

    if(mjb_codepoint_property_binary('A', MJB_PR_ALPHABETIC,
        &is_alphabetic) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_codepoint_property_binary test failed") // Added by the script
        return 1;
    }

    // U+0041 is alphabetic: yes
    // printf("U+0041 is alphabetic: %s", is_alphabetic ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "U+0041 is alphabetic: %s", is_alphabetic ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "U+0041 is alphabetic: yes", "mjb_codepoint_property_binary test failed") // Added by the script
}

{
    // Example for mjb_codepoint_property_int
    MJB_TEST_COVERAGE(mjb_codepoint_property_int); // Added by the script
    int32_t script;

    if(mjb_codepoint_property_int('A', MJB_PR_SCRIPT, &script) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_codepoint_property_int test failed") // Added by the script
        return 1;
    }

    // U+0041 uses the Latin script: yes
    // printf("U+0041 uses the Latin script: %s", script == MJB_SC_LATN ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "U+0041 uses the Latin script: %s", script == MJB_SC_LATN ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "U+0041 uses the Latin script: yes", "mjb_codepoint_property_int test failed") // Added by the script
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
    // Example for mjb_codepoint_block
    MJB_TEST_COVERAGE(mjb_codepoint_block); // Added by the script
    mjb_block_info block;

    if(mjb_codepoint_block('A', &block) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_codepoint_block test failed") // Added by the script
        return 1;
    }

    // Block: Basic Latin
    // printf("Block: %s", block.name);
    snprintf(test_buffer, sizeof(test_buffer), "Block: %s", block.name); // Added by the script
    ATT_ASSERT(test_buffer, "Block: Basic Latin", "mjb_codepoint_block test failed") // Added by the script
}

{
    // Example for mjb_codepoint_script
    MJB_TEST_COVERAGE(mjb_codepoint_script); // Added by the script
    mjb_script script = mjb_codepoint_script(0x03A9); // Greek capital omega

    // Greek script: yes
    // printf("Greek script: %s", script == MJB_SC_GREK ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "Greek script: %s", script == MJB_SC_GREK ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "Greek script: yes", "mjb_codepoint_script test failed") // Added by the script
}

{
    // Example for mjb_codepoint_script_extensions
    MJB_TEST_COVERAGE(mjb_codepoint_script_extensions); // Added by the script
    size_t count = 0;

    if(mjb_codepoint_script_extensions(0x30FC, NULL, &count) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_codepoint_script_extensions test failed") // Added by the script
        return 1;
    }

    mjb_script scripts[3];

    if(count > 3 || mjb_codepoint_script_extensions(0x30FC, scripts,
        &count) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_codepoint_script_extensions test failed") // Added by the script
        return 1;
    }

    // U+30FC has 2 Script_Extensions
    // printf("U+30FC has %zu Script_Extensions", count);
    snprintf(test_buffer, sizeof(test_buffer), "U+30FC has %zu Script_Extensions", count); // Added by the script
    ATT_ASSERT(test_buffer, "U+30FC has 2 Script_Extensions", "mjb_codepoint_script_extensions test failed") // Added by the script
}

{
    // Example for mjb_codepoint_encode
    MJB_TEST_COVERAGE(mjb_codepoint_encode); // Added by the script
    char encoded[4];
    unsigned int size = mjb_codepoint_encode(0x20AC, encoded, sizeof(encoded), MJB_ENC_UTF_8);

    // € sign uses 3 UTF-8 bytes
    // printf("%.*s sign uses %u UTF-8 bytes", (int)size, encoded, size);
    snprintf(test_buffer, sizeof(test_buffer), "%.*s sign uses %u UTF-8 bytes", (int)size, encoded, size); // Added by the script
    ATT_ASSERT(test_buffer, "€ sign uses 3 UTF-8 bytes", "mjb_codepoint_encode test failed") // Added by the script
}

{
    // Example for mjb_convert_encoding
    MJB_TEST_COVERAGE(mjb_convert_encoding); // Added by the script
    const char *input = "caf\xC3\xA9";
    mjb_result result;

    if(mjb_convert_encoding(input, strlen(input), MJB_ENC_UTF_8,
        MJB_ENC_UTF_16LE, &result) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_convert_encoding test failed") // Added by the script
        return 1;
    }

    // UTF-16LE bytes: 8
    // printf("UTF-16LE bytes: %zu", result.output_size);
    snprintf(test_buffer, sizeof(test_buffer), "UTF-16LE bytes: %zu", result.output_size); // Added by the script
    ATT_ASSERT(test_buffer, "UTF-16LE bytes: 8", "mjb_convert_encoding test failed") // Added by the script
    mjb_result_free(&result);
}

{
    // Example for mjb_convert_encoding_into
    MJB_TEST_COVERAGE(mjb_convert_encoding_into); // Added by the script
    const char *input = "caf\xC3\xA9";
    size_t output_size = 0;

    if(mjb_convert_encoding_into(input, strlen(input), MJB_ENC_UTF_8,
        MJB_ENC_UTF_16LE, NULL, &output_size) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_convert_encoding_into test failed") // Added by the script
        return 1;
    }

    unsigned char output[8];

    if(output_size > sizeof(output) || mjb_convert_encoding_into(input, strlen(input),
        MJB_ENC_UTF_8, MJB_ENC_UTF_16LE, output, &output_size) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_convert_encoding_into test failed") // Added by the script
        return 1;
    }

    // UTF-16LE payload bytes (no terminator): 8
    // printf("UTF-16LE payload bytes (no terminator): %zu", output_size);
    snprintf(test_buffer, sizeof(test_buffer), "UTF-16LE payload bytes (no terminator): %zu", output_size); // Added by the script
    ATT_ASSERT(test_buffer, "UTF-16LE payload bytes (no terminator): 8", "mjb_convert_encoding_into test failed") // Added by the script
}

{
    // Example for mjb_collation_compare
    MJB_TEST_COVERAGE(mjb_collation_compare); // Added by the script
    int order;

    if(mjb_collation_compare("apple", 5, MJB_ENC_UTF_8,
        "banana", 6, MJB_ENC_UTF_8, MJB_COLLATION_NON_IGNORABLE, &order) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_collation_compare test failed") // Added by the script
        return 1;
    }

    // apple sorts before banana: yes
    // printf("apple sorts before banana: %s", order < 0 ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "apple sorts before banana: %s", order < 0 ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "apple sorts before banana: yes", "mjb_collation_compare test failed") // Added by the script
}

{
    // Example for mjb_collation_key
    MJB_TEST_COVERAGE(mjb_collation_key); // Added by the script
    mjb_result key;

    if(mjb_collation_key("r\xC3\xA9sum\xC3\xA9", 8, MJB_ENC_UTF_8,
        MJB_COLLATION_NON_IGNORABLE, &key) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_collation_key test failed") // Added by the script
        return 1;
    }

    // Sort key is non-empty: yes
    // printf("Sort key is non-empty: %s", key.output_size > 0 ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "Sort key is non-empty: %s", key.output_size > 0 ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "Sort key is non-empty: yes", "mjb_collation_key test failed") // Added by the script
    mjb_result_free(&key);
}

{
    // Example for mjb_collation_key_into
    MJB_TEST_COVERAGE(mjb_collation_key_into); // Added by the script
    const char *input = "r\xC3\xA9sum\xC3\xA9";
    size_t output_size = 0;

    if(mjb_collation_key_into(input, 8, MJB_ENC_UTF_8, MJB_COLLATION_NON_IGNORABLE,
        NULL, &output_size) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_collation_key_into test failed") // Added by the script
        return 1;
    }

    unsigned char output[64];

    if(output_size > sizeof(output) || mjb_collation_key_into(input, 8, MJB_ENC_UTF_8,
        MJB_COLLATION_NON_IGNORABLE, output, &output_size) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_collation_key_into test failed") // Added by the script
        return 1;
    }

    // Sort key is non-empty: yes
    // printf("Sort key is non-empty: %s", output_size > 0 ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "Sort key is non-empty: %s", output_size > 0 ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "Sort key is non-empty: yes", "mjb_collation_key_into test failed") // Added by the script
}

{
    // Example for mjb_map_case
    MJB_TEST_COVERAGE(mjb_map_case); // Added by the script
    const char *input = "Stra\xC3\x9F""e"; // "Straße"
    mjb_result result;

    if(mjb_map_case(input, strlen(input), MJB_ENC_UTF_8, MJB_CASE_UPPER, MJB_ENC_UTF_8,
        &result) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_map_case test failed") // Added by the script
        return 1;
    }

    // Upper: STRASSE
    // printf("Upper: %.*s", (int)result.output_size, result.output);
    snprintf(test_buffer, sizeof(test_buffer), "Upper: %.*s", (int)result.output_size, result.output); // Added by the script
    ATT_ASSERT(test_buffer, "Upper: STRASSE", "mjb_map_case test failed") // Added by the script

    mjb_result_free(&result);
}

{
    // Example for mjb_map_case_into
    MJB_TEST_COVERAGE(mjb_map_case_into); // Added by the script
    const char *input = "Stra\xC3\x9F""e"; // "Straße"
    size_t output_size = 0;

    if(mjb_map_case_into(input, strlen(input), MJB_ENC_UTF_8, MJB_CASE_UPPER, MJB_ENC_UTF_8,
        NULL, &output_size) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_map_case_into test failed") // Added by the script
        return 1;
    }

    char output[7];

    if(output_size > sizeof(output) || mjb_map_case_into(input, strlen(input), MJB_ENC_UTF_8,
        MJB_CASE_UPPER, MJB_ENC_UTF_8, output, &output_size) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_map_case_into test failed") // Added by the script
        return 1;
    }

    // Upper payload (no terminator): STRASSE
    // printf("Upper payload (no terminator): %.*s", (int)output_size, output);
    snprintf(test_buffer, sizeof(test_buffer), "Upper payload (no terminator): %.*s", (int)output_size, output); // Added by the script
    ATT_ASSERT(test_buffer, "Upper payload (no terminator): STRASSE", "mjb_map_case_into test failed") // Added by the script
}

{
    // Example for mjb_codepoint_is_valid
    MJB_TEST_COVERAGE(mjb_codepoint_is_valid); // Added by the script
    // U+10FFFD valid: yes
    // printf("U+10FFFD valid: %s", mjb_codepoint_is_valid(0x10FFFD) ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "U+10FFFD valid: %s", mjb_codepoint_is_valid(0x10FFFD) ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "U+10FFFD valid: yes", "mjb_codepoint_is_valid test failed") // Added by the script
}

{
    // Example for mjb_codepoint_is_graphic
    MJB_TEST_COVERAGE(mjb_codepoint_is_graphic); // Added by the script
    // Letter A is graphic: yes
    // printf("Letter A is graphic: %s", mjb_codepoint_is_graphic('A') ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "Letter A is graphic: %s", mjb_codepoint_is_graphic('A') ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "Letter A is graphic: yes", "mjb_codepoint_is_graphic test failed") // Added by the script
}

{
    // Example for mjb_codepoint_is_combining
    MJB_TEST_COVERAGE(mjb_codepoint_is_combining); // Added by the script
    // U+0301 is combining: yes
    // printf("U+0301 is combining: %s", mjb_codepoint_is_combining(0x0301) ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "U+0301 is combining: %s", mjb_codepoint_is_combining(0x0301) ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "U+0301 is combining: yes", "mjb_codepoint_is_combining test failed") // Added by the script
}

{
    // Example for mjb_codepoint_is_hangul_leading_jamo
    MJB_TEST_COVERAGE(mjb_codepoint_is_hangul_leading_jamo); // Added by the script
    // U+1100 is a leading Jamo: yes
    // printf("U+1100 is a leading Jamo: %s", mjb_codepoint_is_hangul_leading_jamo(0x1100) ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "U+1100 is a leading Jamo: %s", mjb_codepoint_is_hangul_leading_jamo(0x1100) ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "U+1100 is a leading Jamo: yes", "mjb_codepoint_is_hangul_leading_jamo test failed") // Added by the script
}

{
    // Example for mjb_codepoint_is_hangul_vowel_jamo
    MJB_TEST_COVERAGE(mjb_codepoint_is_hangul_vowel_jamo); // Added by the script
    // U+1161 is a vowel Jamo: yes
    // printf("U+1161 is a vowel Jamo: %s", mjb_codepoint_is_hangul_vowel_jamo(0x1161) ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "U+1161 is a vowel Jamo: %s", mjb_codepoint_is_hangul_vowel_jamo(0x1161) ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "U+1161 is a vowel Jamo: yes", "mjb_codepoint_is_hangul_vowel_jamo test failed") // Added by the script
}

{
    // Example for mjb_codepoint_is_hangul_trailing_jamo
    MJB_TEST_COVERAGE(mjb_codepoint_is_hangul_trailing_jamo); // Added by the script
    // U+11A8 is a trailing Jamo: yes
    // printf("U+11A8 is a trailing Jamo: %s", mjb_codepoint_is_hangul_trailing_jamo(0x11A8) ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "U+11A8 is a trailing Jamo: %s", mjb_codepoint_is_hangul_trailing_jamo(0x11A8) ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "U+11A8 is a trailing Jamo: yes", "mjb_codepoint_is_hangul_trailing_jamo test failed") // Added by the script
}

{
    // Example for mjb_codepoint_is_hangul_jamo
    MJB_TEST_COVERAGE(mjb_codepoint_is_hangul_jamo); // Added by the script
    // U+1100 is Hangul Jamo: yes
    // printf("U+1100 is Hangul Jamo: %s", mjb_codepoint_is_hangul_jamo(0x1100) ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "U+1100 is Hangul Jamo: %s", mjb_codepoint_is_hangul_jamo(0x1100) ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "U+1100 is Hangul Jamo: yes", "mjb_codepoint_is_hangul_jamo test failed") // Added by the script
}

{
    // Example for mjb_codepoint_is_hangul_syllable
    MJB_TEST_COVERAGE(mjb_codepoint_is_hangul_syllable); // Added by the script
    // U+AC00 is a Hangul syllable: yes
    // printf("U+AC00 is a Hangul syllable: %s", mjb_codepoint_is_hangul_syllable(0xAC00) ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "U+AC00 is a Hangul syllable: %s", mjb_codepoint_is_hangul_syllable(0xAC00) ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "U+AC00 is a Hangul syllable: yes", "mjb_codepoint_is_hangul_syllable test failed") // Added by the script
}

{
    // Example for mjb_codepoint_is_cjk_ideograph
    MJB_TEST_COVERAGE(mjb_codepoint_is_cjk_ideograph); // Added by the script
    // U+4E00 is a CJK ideograph: yes
    // printf("U+4E00 is a CJK ideograph: %s", mjb_codepoint_is_cjk_ideograph(0x4E00) ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "U+4E00 is a CJK ideograph: %s", mjb_codepoint_is_cjk_ideograph(0x4E00) ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "U+4E00 is a CJK ideograph: yes", "mjb_codepoint_is_cjk_ideograph test failed") // Added by the script
}

{
    // Example for mjb_codepoint_is_cjk_extension_ideograph
    MJB_TEST_COVERAGE(mjb_codepoint_is_cjk_extension_ideograph); // Added by the script
    // U+20000 is a CJK extension ideograph: yes
    // printf("U+20000 is a CJK extension ideograph: %s", mjb_codepoint_is_cjk_extension_ideograph(0x20000) ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "U+20000 is a CJK extension ideograph: %s", mjb_codepoint_is_cjk_extension_ideograph(0x20000) ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "U+20000 is a CJK extension ideograph: yes", "mjb_codepoint_is_cjk_extension_ideograph test failed") // Added by the script
}

{
    // Example for mjb_category_is_graphic
    MJB_TEST_COVERAGE(mjb_category_is_graphic); // Added by the script
    // Uppercase letters are graphic: yes
    bool graphic = mjb_category_is_graphic(MJB_CATEGORY_LU);

    // Uppercase letters are graphic: yes
    // printf("Uppercase letters are graphic: %s", graphic ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "Uppercase letters are graphic: %s", graphic ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "Uppercase letters are graphic: yes", "mjb_category_is_graphic test failed") // Added by the script
}

{
    // Example for mjb_category_is_combining
    MJB_TEST_COVERAGE(mjb_category_is_combining); // Added by the script
    // Nonspacing marks are combining: yes
    bool combining = mjb_category_is_combining(MJB_CATEGORY_MN);

    // Nonspacing marks are combining: yes
    // printf("Nonspacing marks are combining: %s", combining ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "Nonspacing marks are combining: %s", combining ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "Nonspacing marks are combining: yes", "mjb_category_is_combining test failed") // Added by the script
}

{
    // Example for mjb_next_line_break
    MJB_TEST_COVERAGE(mjb_next_line_break); // Added by the script
    mjb_next_line_state state;
    state.index = 0;
    mjb_break_type type = mjb_next_line_break("Hello world", 11, MJB_ENC_UTF_8, &state);

    // First line-break result is set: yes
    // printf("First line-break result is set: %s", type != MJB_BT_NOT_SET ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "First line-break result is set: %s", type != MJB_BT_NOT_SET ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "First line-break result is set: yes", "mjb_next_line_break test failed") // Added by the script
}

{
    // Example for mjb_next_word_break
    MJB_TEST_COVERAGE(mjb_next_word_break); // Added by the script
    mjb_next_word_state state;
    state.index = 0;
    size_t boundaries = 0;

    while(mjb_next_word_break("Hello world", 11, MJB_ENC_UTF_8, &state) != MJB_BT_NOT_SET) {
        ++boundaries;
    }

    // Word-break positions: 11
    // printf("Word-break positions: %zu", boundaries);
    snprintf(test_buffer, sizeof(test_buffer), "Word-break positions: %zu", boundaries); // Added by the script
    ATT_ASSERT(test_buffer, "Word-break positions: 11", "mjb_next_word_break test failed") // Added by the script
}

{
    // Example for mjb_next_sentence_break
    MJB_TEST_COVERAGE(mjb_next_sentence_break); // Added by the script
    mjb_next_sentence_state state;
    state.index = 0;
    size_t boundaries = 0;
    const char *input = "Hello. Goodbye.";

    while(mjb_next_sentence_break(input, strlen(input), MJB_ENC_UTF_8, &state) != MJB_BT_NOT_SET) {
        ++boundaries;
    }

    // Sentence-break positions: 15
    // printf("Sentence-break positions: %zu", boundaries);
    snprintf(test_buffer, sizeof(test_buffer), "Sentence-break positions: %zu", boundaries); // Added by the script
    ATT_ASSERT(test_buffer, "Sentence-break positions: 15", "mjb_next_sentence_break test failed") // Added by the script
}

{
    // Example for mjb_next_grapheme_break
    MJB_TEST_COVERAGE(mjb_next_grapheme_break); // Added by the script
    const char *input = "e\xCC\x81"; // e + combining acute accent
    mjb_next_state state;
    state.index = 0;
    size_t codepoints = 0;

    while(mjb_next_grapheme_break(input, strlen(input), MJB_ENC_UTF_8,
        &state) != MJB_BT_NOT_SET) {
        ++codepoints;
    }

    // Codepoints examined: 2
    // printf("Codepoints examined: %zu", codepoints);
    snprintf(test_buffer, sizeof(test_buffer), "Codepoints examined: %zu", codepoints); // Added by the script
    ATT_ASSERT(test_buffer, "Codepoints examined: 2", "mjb_next_grapheme_break test failed") // Added by the script
}

{
    // Example for mjb_truncate_grapheme
    MJB_TEST_COVERAGE(mjb_truncate_grapheme); // Added by the script
    const char *input = "A\xF0\x9F\x87\xAE\xF0\x9F\x87\xB9Z"; // A🇮🇹Z
    size_t bytes = mjb_truncate_grapheme(input, strlen(input), MJB_ENC_UTF_8, 2);

    // First two graphemes use 9 bytes
    // printf("First two graphemes use %zu bytes", bytes);
    snprintf(test_buffer, sizeof(test_buffer), "First two graphemes use %zu bytes", bytes); // Added by the script
    ATT_ASSERT(test_buffer, "First two graphemes use 9 bytes", "mjb_truncate_grapheme test failed") // Added by the script
}

{
    // Example for mjb_truncate_grapheme_width
    MJB_TEST_COVERAGE(mjb_truncate_grapheme_width); // Added by the script
    const char *input = "A\xE7\x95\x8C"; // A界
    size_t bytes = mjb_truncate_grapheme_width(input, strlen(input), MJB_ENC_UTF_8,
        MJB_WIDTH_CONTEXT_WESTERN, 2);

    // Two columns include 1 byte
    // printf("Two columns include %zu byte", bytes);
    snprintf(test_buffer, sizeof(test_buffer), "Two columns include %zu byte", bytes); // Added by the script
    ATT_ASSERT(test_buffer, "Two columns include 1 byte", "mjb_truncate_grapheme_width test failed") // Added by the script
}

{
    // Example for mjb_truncate_word
    MJB_TEST_COVERAGE(mjb_truncate_word); // Added by the script
    const char *input = "Hello world";
    size_t bytes = mjb_truncate_word(input, strlen(input), MJB_ENC_UTF_8, 1);

    // First word segment uses 5 bytes
    // printf("First word segment uses %zu bytes", bytes);
    snprintf(test_buffer, sizeof(test_buffer), "First word segment uses %zu bytes", bytes); // Added by the script
    ATT_ASSERT(test_buffer, "First word segment uses 5 bytes", "mjb_truncate_word test failed") // Added by the script
}

{
    // Example for mjb_truncate_word_width
    MJB_TEST_COVERAGE(mjb_truncate_word_width); // Added by the script
    const char *input = "Hello world";
    size_t bytes = mjb_truncate_word_width(input, strlen(input), MJB_ENC_UTF_8,
        MJB_WIDTH_CONTEXT_WESTERN, 6);

    // Six columns include 6 bytes
    // printf("Six columns include %zu bytes", bytes);
    snprintf(test_buffer, sizeof(test_buffer), "Six columns include %zu bytes", bytes); // Added by the script
    ATT_ASSERT(test_buffer, "Six columns include 6 bytes", "mjb_truncate_word_width test failed") // Added by the script
}

{
    // Example for mjb_bidi_resolve
    MJB_TEST_COVERAGE(mjb_bidi_resolve); // Added by the script
    const char *input = "abc \xD7\x90\xD7\x91\xD7\x92"; // abc אבג
    mjb_bidi_paragraph paragraph;

    if(mjb_bidi_resolve(input, strlen(input), MJB_ENC_UTF_8, MJB_DIRECTION_AUTO,
        &paragraph) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_bidi_resolve test failed") // Added by the script
        return 1;
    }

    // Paragraph codepoints: 7
    // printf("Paragraph codepoints: %zu", paragraph.count);
    snprintf(test_buffer, sizeof(test_buffer), "Paragraph codepoints: %zu", paragraph.count); // Added by the script
    ATT_ASSERT(test_buffer, "Paragraph codepoints: 7", "mjb_bidi_resolve test failed") // Added by the script
    mjb_bidi_paragraph_free(&paragraph);
}

{
    // Example for mjb_bidi_reorder_line
    MJB_TEST_COVERAGE(mjb_bidi_reorder_line); // Added by the script
    const char *input = "\xD7\x90\xD7\x91\xD7\x92"; // אבג
    mjb_bidi_paragraph paragraph;
    size_t visual_order[3];

    if(mjb_bidi_resolve(input, strlen(input), MJB_ENC_UTF_8, MJB_DIRECTION_AUTO,
        &paragraph) != MJB_STATUS_OK ||
        mjb_bidi_reorder_line(&paragraph, 0, paragraph.count,
            visual_order) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_bidi_reorder_line test failed") // Added by the script
        return 1;
    }

    // First visual index: 2
    // printf("First visual index: %zu", visual_order[0]);
    snprintf(test_buffer, sizeof(test_buffer), "First visual index: %zu", visual_order[0]); // Added by the script
    ATT_ASSERT(test_buffer, "First visual index: 2", "mjb_bidi_reorder_line test failed") // Added by the script
    mjb_bidi_paragraph_free(&paragraph);
}

{
    // Example for mjb_bidi_line_runs
    MJB_TEST_COVERAGE(mjb_bidi_line_runs); // Added by the script
    mjb_bidi_paragraph paragraph;
    size_t visual_order[3];
    size_t run_count = 0;

    if(mjb_bidi_resolve("abc", 3, MJB_ENC_UTF_8, MJB_DIRECTION_LTR,
        &paragraph) != MJB_STATUS_OK ||
        mjb_bidi_reorder_line(&paragraph, 0, 3, visual_order) != MJB_STATUS_OK ||
        mjb_bidi_line_runs(&paragraph, visual_order, 3, NULL,
            &run_count) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_bidi_line_runs test failed") // Added by the script
        return 1;
    }

    // Visual runs: 1
    // printf("Visual runs: %zu", run_count);
    snprintf(test_buffer, sizeof(test_buffer), "Visual runs: %zu", run_count); // Added by the script
    ATT_ASSERT(test_buffer, "Visual runs: 1", "mjb_bidi_line_runs test failed") // Added by the script
    mjb_bidi_paragraph_free(&paragraph);
}

{
    // Example for mjb_bidi_paragraph_free
    MJB_TEST_COVERAGE(mjb_bidi_paragraph_free); // Added by the script
    mjb_bidi_paragraph paragraph;

    if(mjb_bidi_resolve("abc", 3, MJB_ENC_UTF_8, MJB_DIRECTION_LTR,
        &paragraph) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_bidi_paragraph_free test failed") // Added by the script
        return 1;
    }

    mjb_bidi_paragraph_free(&paragraph);

    // Paragraph released: yes
    // printf("Paragraph released: %s", paragraph.chars == NULL ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "Paragraph released: %s", paragraph.chars == NULL ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "Paragraph released: yes", "mjb_bidi_paragraph_free test failed") // Added by the script
}

{
    // Example for mjb_codepoint_is_id_start
    MJB_TEST_COVERAGE(mjb_codepoint_is_id_start); // Added by the script
    // Greek alpha starts an identifier: yes
    bool starts = mjb_codepoint_is_id_start(0x03B1);

    // Greek alpha starts an identifier: yes
    // printf("Greek alpha starts an identifier: %s", starts ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "Greek alpha starts an identifier: %s", starts ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "Greek alpha starts an identifier: yes", "mjb_codepoint_is_id_start test failed") // Added by the script
}

{
    // Example for mjb_codepoint_is_id_continue
    MJB_TEST_COVERAGE(mjb_codepoint_is_id_continue); // Added by the script
    // Digit 7 continues an identifier: yes
    bool continues = mjb_codepoint_is_id_continue('7');

    // Digit 7 continues an identifier: yes
    // printf("Digit 7 continues an identifier: %s", continues ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "Digit 7 continues an identifier: %s", continues ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "Digit 7 continues an identifier: yes", "mjb_codepoint_is_id_continue test failed") // Added by the script
}

{
    // Example for mjb_codepoint_is_xid_start
    MJB_TEST_COVERAGE(mjb_codepoint_is_xid_start); // Added by the script
    // Letter A is XID_Start: yes
    // printf("Letter A is XID_Start: %s", mjb_codepoint_is_xid_start('A') ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "Letter A is XID_Start: %s", mjb_codepoint_is_xid_start('A') ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "Letter A is XID_Start: yes", "mjb_codepoint_is_xid_start test failed") // Added by the script
}

{
    // Example for mjb_codepoint_is_xid_continue
    MJB_TEST_COVERAGE(mjb_codepoint_is_xid_continue); // Added by the script
    // Underscore is XID_Continue: yes
    bool continues = mjb_codepoint_is_xid_continue('_');

    // Underscore is XID_Continue: yes
    // printf("Underscore is XID_Continue: %s", continues ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "Underscore is XID_Continue: %s", continues ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "Underscore is XID_Continue: yes", "mjb_codepoint_is_xid_continue test failed") // Added by the script
}

{
    // Example for mjb_codepoint_is_pattern_syntax
    MJB_TEST_COVERAGE(mjb_codepoint_is_pattern_syntax); // Added by the script
    // Plus sign is Pattern_Syntax: yes
    bool syntax = mjb_codepoint_is_pattern_syntax('+');

    // Plus sign is Pattern_Syntax: yes
    // printf("Plus sign is Pattern_Syntax: %s", syntax ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "Plus sign is Pattern_Syntax: %s", syntax ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "Plus sign is Pattern_Syntax: yes", "mjb_codepoint_is_pattern_syntax test failed") // Added by the script
}

{
    // Example for mjb_codepoint_is_pattern_white_space
    MJB_TEST_COVERAGE(mjb_codepoint_is_pattern_white_space); // Added by the script
    // Space is Pattern_White_Space: yes
    bool whitespace = mjb_codepoint_is_pattern_white_space(' ');

    // Space is Pattern_White_Space: yes
    // printf("Space is Pattern_White_Space: %s", whitespace ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "Space is Pattern_White_Space: %s", whitespace ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "Space is Pattern_White_Space: yes", "mjb_codepoint_is_pattern_white_space test failed") // Added by the script
}

{
    // Example for mjb_is_identifier
    MJB_TEST_COVERAGE(mjb_is_identifier); // Added by the script
    const char *identifier = "delta_2";

    bool valid = mjb_is_identifier(identifier, strlen(identifier), MJB_ENC_UTF_8,
        MJB_IDENTIFIER_NFKC);

    // Valid identifier: yes
    // printf("Valid identifier: %s", valid ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "Valid identifier: %s", valid ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "Valid identifier: yes", "mjb_is_identifier test failed") // Added by the script
}

{
    // Example for mjb_property_name
    MJB_TEST_COVERAGE(mjb_property_name); // Added by the script
    const char *name = mjb_property_name(MJB_PR_ALPHABETIC);

    // Property: Alphabetic
    // printf("Property: %s", name);
    snprintf(test_buffer, sizeof(test_buffer), "Property: %s", name); // Added by the script
    ATT_ASSERT(test_buffer, "Property: Alphabetic", "mjb_property_name test failed") // Added by the script
}

{
    // Example for mjb_confusable_skeleton
    MJB_TEST_COVERAGE(mjb_confusable_skeleton); // Added by the script
    const char *input = "h\xD0\xB5llo"; // Cyrillic U+0435 in place of e
    mjb_result result;

    if(mjb_confusable_skeleton(input, strlen(input), MJB_ENC_UTF_8, MJB_ENC_UTF_8,
        &result) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_confusable_skeleton test failed") // Added by the script
        return 1;
    }

    // hello
    // printf("%.*s", (int)result.output_size, result.output);
    snprintf(test_buffer, sizeof(test_buffer), "%.*s", (int)result.output_size, result.output); // Added by the script
    ATT_ASSERT(test_buffer, "hello", "mjb_confusable_skeleton test failed") // Added by the script
    mjb_result_free(&result);
}

{
    // Example for mjb_confusable_skeleton_into
    MJB_TEST_COVERAGE(mjb_confusable_skeleton_into); // Added by the script
    const char *input = "h\xD0\xB5llo"; // Cyrillic U+0435 in place of e
    size_t output_size = 0;

    if(mjb_confusable_skeleton_into(input, strlen(input), MJB_ENC_UTF_8, MJB_ENC_UTF_8,
        NULL, &output_size) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_confusable_skeleton_into test failed") // Added by the script
        return 1;
    }

    char output[5];

    if(output_size > sizeof(output) || mjb_confusable_skeleton_into(input, strlen(input),
        MJB_ENC_UTF_8, MJB_ENC_UTF_8, output, &output_size) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_confusable_skeleton_into test failed") // Added by the script
        return 1;
    }

    // Skeleton payload (no terminator): hello
    // printf("Skeleton payload (no terminator): %.*s", (int)output_size, output);
    snprintf(test_buffer, sizeof(test_buffer), "Skeleton payload (no terminator): %.*s", (int)output_size, output); // Added by the script
    ATT_ASSERT(test_buffer, "Skeleton payload (no terminator): hello", "mjb_confusable_skeleton_into test failed") // Added by the script
}

{
    // Example for mjb_are_confusable
    MJB_TEST_COVERAGE(mjb_are_confusable); // Added by the script
    const char *latin = "hello";
    const char *mixed = "h\xD0\xB5llo"; // Cyrillic е
    bool confusable;

    if(mjb_are_confusable(latin, strlen(latin), MJB_ENC_UTF_8,
        mixed, strlen(mixed), MJB_ENC_UTF_8, &confusable) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_are_confusable test failed") // Added by the script
        return 1;
    }

    // Visually confusable: yes
    // printf("Visually confusable: %s", confusable ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "Visually confusable: %s", confusable ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "Visually confusable: yes", "mjb_are_confusable test failed") // Added by the script
}

{
    // Example for mjb_codepoint_emoji_properties
    MJB_TEST_COVERAGE(mjb_codepoint_emoji_properties); // Added by the script
    mjb_emoji_properties emoji;

    if(mjb_codepoint_emoji_properties(0x1F600, &emoji) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_codepoint_emoji_properties test failed") // Added by the script
        return 1;
    }

    // U+1F600 has Emoji_Presentation: yes
    // printf("U+1F600 has Emoji_Presentation: %s", emoji.presentation ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "U+1F600 has Emoji_Presentation: %s", emoji.presentation ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "U+1F600 has Emoji_Presentation: yes", "mjb_codepoint_emoji_properties test failed") // Added by the script
}

{
    // Example for mjb_codepoint_is_emoji
    MJB_TEST_COVERAGE(mjb_codepoint_is_emoji); // Added by the script
    // Number sign has the Emoji property: yes
    bool emoji = mjb_codepoint_is_emoji('#');

    // Number sign has the Emoji property: yes
    // printf("Number sign has the Emoji property: %s", emoji ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "Number sign has the Emoji property: %s", emoji ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "Number sign has the Emoji property: yes", "mjb_codepoint_is_emoji test failed") // Added by the script
}

{
    // Example for mjb_codepoint_is_emoji_presentation
    MJB_TEST_COVERAGE(mjb_codepoint_is_emoji_presentation); // Added by the script
    // Grinning face defaults to emoji presentation: yes
    bool presentation = mjb_codepoint_is_emoji_presentation(0x1F600);

    // Grinning face defaults to emoji presentation: yes
    // printf("Grinning face defaults to emoji presentation: %s", presentation ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "Grinning face defaults to emoji presentation: %s", presentation ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "Grinning face defaults to emoji presentation: yes", "mjb_codepoint_is_emoji_presentation test failed") // Added by the script
}

{
    // Example for mjb_codepoint_is_emoji_modifier
    MJB_TEST_COVERAGE(mjb_codepoint_is_emoji_modifier); // Added by the script
    // Medium skin tone is an emoji modifier: yes
    bool modifier = mjb_codepoint_is_emoji_modifier(0x1F3FD);

    // Medium skin tone is an emoji modifier: yes
    // printf("Medium skin tone is an emoji modifier: %s", modifier ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "Medium skin tone is an emoji modifier: %s", modifier ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "Medium skin tone is an emoji modifier: yes", "mjb_codepoint_is_emoji_modifier test failed") // Added by the script
}

{
    // Example for mjb_codepoint_is_emoji_modifier_base
    MJB_TEST_COVERAGE(mjb_codepoint_is_emoji_modifier_base); // Added by the script
    // Waving hand accepts an emoji modifier: yes
    bool modifier_base = mjb_codepoint_is_emoji_modifier_base(0x1F44B);

    // Waving hand accepts an emoji modifier: yes
    // printf("Waving hand accepts an emoji modifier: %s", modifier_base ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "Waving hand accepts an emoji modifier: %s", modifier_base ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "Waving hand accepts an emoji modifier: yes", "mjb_codepoint_is_emoji_modifier_base test failed") // Added by the script
}

{
    // Example for mjb_codepoint_is_emoji_component
    MJB_TEST_COVERAGE(mjb_codepoint_is_emoji_component); // Added by the script
    // Zero-width joiner is an emoji component: yes
    bool component = mjb_codepoint_is_emoji_component(0x200D);

    // Zero-width joiner is an emoji component: yes
    // printf("Zero-width joiner is an emoji component: %s", component ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "Zero-width joiner is an emoji component: %s", component ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "Zero-width joiner is an emoji component: yes", "mjb_codepoint_is_emoji_component test failed") // Added by the script
}

{
    // Example for mjb_codepoint_is_extended_pictographic
    MJB_TEST_COVERAGE(mjb_codepoint_is_extended_pictographic); // Added by the script
    // Red heart is Extended_Pictographic: yes
    bool pictographic = mjb_codepoint_is_extended_pictographic(0x2764);

    // Red heart is Extended_Pictographic: yes
    // printf("Red heart is Extended_Pictographic: %s", pictographic ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "Red heart is Extended_Pictographic: %s", pictographic ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "Red heart is Extended_Pictographic: yes", "mjb_codepoint_is_extended_pictographic test failed") // Added by the script
}

{
    // Example for mjb_codepoint_plane
    MJB_TEST_COVERAGE(mjb_codepoint_plane); // Added by the script
    mjb_plane plane = mjb_codepoint_plane(0x1F600);

    // U+1F600 is in the SMP: yes
    // printf("U+1F600 is in the SMP: %s", plane == MJB_PLANE_SMP ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "U+1F600 is in the SMP: %s", plane == MJB_PLANE_SMP ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "U+1F600 is in the SMP: yes", "mjb_codepoint_plane test failed") // Added by the script
}

{
    // Example for mjb_plane_is_valid
    MJB_TEST_COVERAGE(mjb_plane_is_valid); // Added by the script
    // Plane 16 is valid: yes
    // printf("Plane 16 is valid: %s", mjb_plane_is_valid(MJB_PLANE_PUA_B) ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "Plane 16 is valid: %s", mjb_plane_is_valid(MJB_PLANE_PUA_B) ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "Plane 16 is valid: yes", "mjb_plane_is_valid test failed") // Added by the script
}

{
    // Example for mjb_plane_name
    MJB_TEST_COVERAGE(mjb_plane_name); // Added by the script
    // Plane: Basic Multilingual Plane
    // printf("Plane: %s", mjb_plane_name(MJB_PLANE_BMP, false));
    snprintf(test_buffer, sizeof(test_buffer), "Plane: %s", mjb_plane_name(MJB_PLANE_BMP, false)); // Added by the script
    ATT_ASSERT(test_buffer, "Plane: Basic Multilingual Plane", "mjb_plane_name test failed") // Added by the script
}

{
    // Example for mjb_classify_emoji_sequence
    MJB_TEST_COVERAGE(mjb_classify_emoji_sequence); // Added by the script
    const char *flag = "\xF0\x9F\x87\xAE\xF0\x9F\x87\xB9"; // 🇮🇹
    mjb_emoji_sequence emoji;

    if(mjb_classify_emoji_sequence(flag, strlen(flag), MJB_ENC_UTF_8,
        &emoji) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_classify_emoji_sequence test failed") // Added by the script
        return 1;
    }

    // Sequence codepoints: 2
    // printf("Sequence codepoints: %zu", emoji.codepoint_count);
    snprintf(test_buffer, sizeof(test_buffer), "Sequence codepoints: %zu", emoji.codepoint_count); // Added by the script
    ATT_ASSERT(test_buffer, "Sequence codepoints: 2", "mjb_classify_emoji_sequence test failed") // Added by the script
}

{
    // Example for mjb_is_emoji_sequence
    MJB_TEST_COVERAGE(mjb_is_emoji_sequence); // Added by the script
    const char *keycap = "1\xEF\xB8\x8F\xE2\x83\xA3"; // 1️⃣

    bool listed = mjb_is_emoji_sequence(keycap, strlen(keycap), MJB_ENC_UTF_8);

    // Listed emoji sequence: yes
    // printf("Listed emoji sequence: %s", listed ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "Listed emoji sequence: %s", listed ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "Listed emoji sequence: yes", "mjb_is_emoji_sequence test failed") // Added by the script
}

{
    // Example for mjb_is_rgi_emoji
    MJB_TEST_COVERAGE(mjb_is_rgi_emoji); // Added by the script
    const char *flag = "\xF0\x9F\x87\xAE\xF0\x9F\x87\xB9"; // 🇮🇹

    bool rgi = mjb_is_rgi_emoji(flag, strlen(flag), MJB_ENC_UTF_8);

    // RGI emoji: yes
    // printf("RGI emoji: %s", rgi ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "RGI emoji: %s", rgi ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "RGI emoji: yes", "mjb_is_rgi_emoji test failed") // Added by the script
}

{
    // Example for mjb_hangul_syllable_name
    MJB_TEST_COVERAGE(mjb_hangul_syllable_name); // Added by the script
    char name[32];

    if(mjb_hangul_syllable_name(0xAC01, name, sizeof(name)) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_hangul_syllable_name test failed") // Added by the script
        return 1;
    }

    // Name: HANGUL SYLLABLE GAG
    // printf("Name: %s", name);
    snprintf(test_buffer, sizeof(test_buffer), "Name: %s", name); // Added by the script
    ATT_ASSERT(test_buffer, "Name: HANGUL SYLLABLE GAG", "mjb_hangul_syllable_name test failed") // Added by the script
}

{
    // Example for mjb_hangul_syllable_decomposition
    MJB_TEST_COVERAGE(mjb_hangul_syllable_decomposition); // Added by the script
    mjb_codepoint decomposition[3];

    if(mjb_hangul_syllable_decomposition(0xAC01,
        decomposition) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_hangul_syllable_decomposition test failed") // Added by the script
        return 1;
    }

    // Decomposition starts with: U+1100
    // printf("Decomposition starts with: U+%04X", decomposition[0]);
    snprintf(test_buffer, sizeof(test_buffer), "Decomposition starts with: U+%04X", decomposition[0]); // Added by the script
    ATT_ASSERT(test_buffer, "Decomposition starts with: U+1100", "mjb_hangul_syllable_decomposition test failed") // Added by the script
}

{
    // Example for mjb_hangul_syllable_composition
    MJB_TEST_COVERAGE(mjb_hangul_syllable_composition); // Added by the script
    mjb_buffer_character characters[] = {
        { 0x1100, 0 }, // choseong kiyeok
        { 0x1161, 0 }, // jungseong a
        { 0x11A8, 0 }  // jongseong kiyeok
    };
    size_t length = mjb_hangul_syllable_composition(characters, 3);

    // Composition: U+AC01
    // printf("Composition: U+%04X", length == 1 ? characters[0].codepoint : 0);
    snprintf(test_buffer, sizeof(test_buffer), "Composition: U+%04X", length == 1 ? characters[0].codepoint : 0); // Added by the script
    ATT_ASSERT(test_buffer, "Composition: U+AC01", "mjb_hangul_syllable_composition test failed") // Added by the script
}

{
    // Example for mjb_codepoint_east_asian_width
    MJB_TEST_COVERAGE(mjb_codepoint_east_asian_width); // Added by the script
    mjb_east_asian_width width;

    if(mjb_codepoint_east_asian_width(0x754C, &width) != MJB_STATUS_OK) { // 界
        ATT_ASSERT(0, 1, "mjb_codepoint_east_asian_width test failed") // Added by the script
        return 1;
    }

    // U+754C is wide: yes
    // printf("U+754C is wide: %s", width == MJB_EAW_WIDE ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "U+754C is wide: %s", width == MJB_EAW_WIDE ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "U+754C is wide: yes", "mjb_codepoint_east_asian_width test failed") // Added by the script
}

{
    // Example for mjb_display_width
    MJB_TEST_COVERAGE(mjb_display_width); // Added by the script
    const char *input = "A\xE7\x95\x8C"; // A界
    size_t width;

    if(mjb_display_width(input, strlen(input), MJB_ENC_UTF_8,
        MJB_WIDTH_CONTEXT_WESTERN, &width) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_display_width test failed") // Added by the script
        return 1;
    }

    // Display columns: 3
    // printf("Display columns: %zu", width);
    snprintf(test_buffer, sizeof(test_buffer), "Display columns: %zu", width); // Added by the script
    ATT_ASSERT(test_buffer, "Display columns: 3", "mjb_display_width test failed") // Added by the script
}

{
    // Example for mjb_locale_parse
    MJB_TEST_COVERAGE(mjb_locale_parse); // Added by the script
    mjb_locale_id locale;
    mjb_error error;

    if(mjb_locale_parse("sr-Latn-RS", 10, MJB_ENC_UTF_8, &locale,
        &error) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_locale_parse test failed") // Added by the script
        return 1;
    }

    // Locale: sr Latn RS
    // printf("Locale: %s %s %s", locale.language, locale.script, locale.region);
    snprintf(test_buffer, sizeof(test_buffer), "Locale: %s %s %s", locale.language, locale.script, locale.region); // Added by the script
    ATT_ASSERT(test_buffer, "Locale: sr Latn RS", "mjb_locale_parse test failed") // Added by the script
}

{
    // Example for mjb_set_locale
    MJB_TEST_COVERAGE(mjb_set_locale); // Added by the script
    if(mjb_set_locale(MJB_LOCALE_TR) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_set_locale test failed") // Added by the script
        return 1;
    }

    // Turkish locale selected: yes
    // printf("Turkish locale selected: yes");
    snprintf(test_buffer, sizeof(test_buffer), "Turkish locale selected: yes"); // Added by the script
    ATT_ASSERT(test_buffer, "Turkish locale selected: yes", "mjb_set_locale test failed") // Added by the script
    if(mjb_set_locale(MJB_LOCALE_EN) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_set_locale test failed") // Added by the script
        return 1;
    }
}

{
    // Example for mjb_get_locale
    MJB_TEST_COVERAGE(mjb_get_locale); // Added by the script
    mjb_locale locale = mjb_get_locale();

    // Current locale is English: yes
    // printf("Current locale is English: %s", locale == MJB_LOCALE_EN ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "Current locale is English: %s", locale == MJB_LOCALE_EN ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "Current locale is English: yes", "mjb_get_locale test failed") // Added by the script
}

{
    // Example for mjb_result_free
    MJB_TEST_COVERAGE(mjb_result_free); // Added by the script
    mjb_result result;

    if(mjb_convert_encoding("A", 1, MJB_ENC_UTF_8, MJB_ENC_UTF_16LE,
        &result) != MJB_STATUS_OK || mjb_result_free(&result) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_result_free test failed") // Added by the script
        return 1;
    }

    // Result released: yes
    // printf("Result released: %s", result.output == NULL ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "Result released: %s", result.output == NULL ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "Result released: yes", "mjb_result_free test failed") // Added by the script
}

{
    // Example for mjb_utf8_snprintf
    MJB_TEST_COVERAGE(mjb_utf8_snprintf); // Added by the script
    char buffer[4];
    int required = mjb_utf8_snprintf(buffer, sizeof(buffer), "%s",
        "\xC3\xA9\xC3\xA9"); // éé

    // 4: é
    // printf("%d: %s", required, buffer);
    snprintf(test_buffer, sizeof(test_buffer), "%d: %s", required, buffer); // Added by the script
    ATT_ASSERT(test_buffer, "4: é", "mjb_utf8_snprintf test failed") // Added by the script
}

{
    // Example for mjb_utf8_grapheme_snprintf
    MJB_TEST_COVERAGE(mjb_utf8_grapheme_snprintf); // Added by the script
    char buffer[4];
    int required = mjb_utf8_grapheme_snprintf(buffer, sizeof(buffer), "%s",
        "Ae\xCC\x81" "B"); // A, e + combining acute accent, B

    // 5:A
    // printf("%d:%s", required, buffer);
    snprintf(test_buffer, sizeof(test_buffer), "%d:%s", required, buffer); // Added by the script
    ATT_ASSERT(test_buffer, "5:A", "mjb_utf8_grapheme_snprintf test failed") // Added by the script
}

{
    // Example for mjb_version
    MJB_TEST_COVERAGE(mjb_version); // Added by the script
    const char *version = mjb_version();

    // Version is available: yes
    // printf("Version is available: %s", version[0] != '\0' ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "Version is available: %s", version[0] != '\0' ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "Version is available: yes", "mjb_version test failed") // Added by the script
}

{
    // Example for mjb_version_number
    MJB_TEST_COVERAGE(mjb_version_number); // Added by the script
    unsigned int version = mjb_version_number();

    // Version number is positive: yes
    // printf("Version number is positive: %s", version > 0 ? "yes" : "no");
    snprintf(test_buffer, sizeof(test_buffer), "Version number is positive: %s", version > 0 ? "yes" : "no"); // Added by the script
    ATT_ASSERT(test_buffer, "Version number is positive: yes", "mjb_version_number test failed") // Added by the script
}

{
    // Example for mjb_unicode_version
    MJB_TEST_COVERAGE(mjb_unicode_version); // Added by the script
    const char *version = mjb_unicode_version();

    // Unicode version: 18.0.0
    // printf("Unicode version: %s", version);
    snprintf(test_buffer, sizeof(test_buffer), "Unicode version: %s", version); // Added by the script
    ATT_ASSERT(test_buffer, "Unicode version: 18.0.0", "mjb_unicode_version test failed") // Added by the script
}

{
    // Example for mjb_set_memory_functions
    MJB_TEST_COVERAGE(mjb_set_memory_functions); // Added by the script
    mjb_reset(); // Ensure no allocator is currently locked in.

    if(mjb_set_memory_functions(malloc, realloc, free) != MJB_STATUS_OK) {
        ATT_ASSERT(0, 1, "mjb_set_memory_functions test failed") // Added by the script
        return 1;
    }

    // Standard allocator installed: yes
    // printf("Standard allocator installed: yes");
    snprintf(test_buffer, sizeof(test_buffer), "Standard allocator installed: yes"); // Added by the script
    ATT_ASSERT(test_buffer, "Standard allocator installed: yes", "mjb_set_memory_functions test failed") // Added by the script
    mjb_reset();
}

{
    // Example for mjb_reset
    MJB_TEST_COVERAGE(mjb_reset); // Added by the script
    mjb_reset();

    // Library state reset: yes
    // printf("Library state reset: yes");
    snprintf(test_buffer, sizeof(test_buffer), "Library state reset: yes"); // Added by the script
    ATT_ASSERT(test_buffer, "Library state reset: yes", "mjb_reset test failed") // Added by the script
}

{
    // Example for mjb_alloc
    MJB_TEST_COVERAGE(mjb_alloc); // Added by the script
    char *buffer = (char*)mjb_alloc(sizeof("allocated"));

    if(buffer == NULL) {
        ATT_ASSERT(0, 1, "mjb_alloc test failed") // Added by the script
        return 1;
    }

    memcpy(buffer, "allocated", sizeof("allocated"));

    // Buffer: allocated
    // printf("Buffer: %s", buffer);
    snprintf(test_buffer, sizeof(test_buffer), "Buffer: %s", buffer); // Added by the script
    ATT_ASSERT(test_buffer, "Buffer: allocated", "mjb_alloc test failed") // Added by the script
    mjb_free(buffer);
}

{
    // Example for mjb_realloc
    MJB_TEST_COVERAGE(mjb_realloc); // Added by the script
    char *buffer = (char*)mjb_alloc(8);

    if(buffer == NULL) {
        ATT_ASSERT(0, 1, "mjb_realloc test failed") // Added by the script
        return 1;
    }

    char *larger = (char*)mjb_realloc(buffer, 32);

    if(larger == NULL) {
        mjb_free(buffer);
        ATT_ASSERT(0, 1, "mjb_realloc test failed") // Added by the script
        return 1;
    }

    // Reallocation succeeded: yes
    // printf("Reallocation succeeded: yes");
    snprintf(test_buffer, sizeof(test_buffer), "Reallocation succeeded: yes"); // Added by the script
    ATT_ASSERT(test_buffer, "Reallocation succeeded: yes", "mjb_realloc test failed") // Added by the script
    mjb_free(larger);
}

{
    // Example for mjb_free
    MJB_TEST_COVERAGE(mjb_free); // Added by the script
    void *memory = mjb_alloc(16);

    if(memory == NULL) {
        ATT_ASSERT(0, 1, "mjb_free test failed") // Added by the script
        return 1;
    }

    mjb_free(memory);

    // Memory freed: yes
    // printf("Memory freed: yes");
    snprintf(test_buffer, sizeof(test_buffer), "Memory freed: yes"); // Added by the script
    ATT_ASSERT(test_buffer, "Memory freed: yes", "mjb_free test failed") // Added by the script
}
    return 0;
}
// clang-format on
