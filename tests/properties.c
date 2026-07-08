/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"

int test_properties(void *arg) {
    uint8_t value = 0xFF;

    // U+0041 'A'
    ATT_ASSERT_STATUS(mjb_codepoint_property_value(0x41, MJB_PR_CASED, NULL), MJB_STATUS_OK,
        "U+0041 is cased")
    ATT_ASSERT_STATUS(mjb_codepoint_property_value(0x41, MJB_PR_ALPHABETIC, NULL),
        MJB_STATUS_OK, "U+0041 is alphabetic")
    ATT_ASSERT_STATUS(mjb_codepoint_property_value(0x41, MJB_PR_UPPERCASE, NULL),
        MJB_STATUS_OK, "U+0041 is uppercase")
    ATT_ASSERT_STATUS(mjb_codepoint_property_value(0x41, MJB_PR_CHANGES_WHEN_LOWERCASED, NULL),
        MJB_STATUS_OK, "U+0041 changes when lowercased exists")
    ATT_ASSERT_STATUS(mjb_codepoint_property_value(0x41, MJB_PR_CHANGES_WHEN_CASEFOLDED, NULL),
        MJB_STATUS_OK, "U+0041 changes when casefolded exists")
    ATT_ASSERT_STATUS(mjb_codepoint_property_value(0x41, MJB_PR_SCRIPT, &value),
        MJB_STATUS_OK, "U+0041 script value exists")
    ATT_ASSERT(value, MJB_SC_LATN, "U+0041 script value is Latin")

    value = 0xFF;
    ATT_ASSERT_STATUS(mjb_codepoint_property_value(0x41, MJB_PR_WORD_BREAK, &value),
        MJB_STATUS_OK, "U+0041 word break value exists")
    ATT_ASSERT(value, MJB_WBP_A_LETTER, "U+0041 word break value is A_Letter")

    value = 0xFF;
    ATT_ASSERT_STATUS(mjb_codepoint_property_value(0x41, MJB_PR_LINE_BREAK, &value),
        MJB_STATUS_OK, "U+0041 line break value exists")
    ATT_ASSERT(value, MJB_LBP_AL, "U+0041 line break value is Alphabetic")

    value = 0xFF;
    ATT_ASSERT_STATUS(mjb_codepoint_property_value(0x41, MJB_PR_SENTENCE_BREAK, &value),
        MJB_STATUS_OK, "U+0041 sentence break value exists")
    ATT_ASSERT(value, MJB_SBP_UPPER, "U+0041 sentence break value is Upper")

    value = 0xFF;
    ATT_ASSERT_STATUS(mjb_codepoint_property_value(0x41, MJB_PR_LOWERCASE, &value),
        MJB_STATUS_NOT_FOUND, "U+0041 lowercase value does not exist")
    ATT_ASSERT(value, 0xFF, "U+0041 lowercase missing value is not written")

    value = 0xFF;
    ATT_ASSERT_STATUS(mjb_codepoint_property_value(0x20, MJB_PR_EAST_ASIAN_WIDTH, &value),
        MJB_STATUS_OK, "Space East Asian Width exists")
    ATT_ASSERT(value, MJB_EAW_NARROW, "Space east asian width is Narrow")

    value = 0xFF;
    ATT_ASSERT_STATUS(mjb_codepoint_property_value(0x20, MJB_PR_CASED, NULL),
        MJB_STATUS_NOT_FOUND, "Space cased does not exist")

    // mjb_codepoint_script
    ATT_ASSERT((int)mjb_codepoint_script(MJB_CODEPOINT_MAX + 1), MJB_SC_ZZZZ, "Invalid codepoint script is Unknown")
    ATT_ASSERT((int)mjb_codepoint_script(0x41), MJB_SC_LATN, "U+0041 'A' is Latin")
    ATT_ASSERT((int)mjb_codepoint_script(0x0391), MJB_SC_GREK, "U+0391 'A' is Greek")
    ATT_ASSERT((int)mjb_codepoint_script(0x0410), MJB_SC_CYRL, "U+0410 'A' is Cyrillic")
    ATT_ASSERT((int)mjb_codepoint_script(0x05D0), MJB_SC_HEBR, "U+05D0 Alef is Hebrew")
    ATT_ASSERT((int)mjb_codepoint_script(0x0600), MJB_SC_ARAB, "U+0600 is Arabic")
    ATT_ASSERT((int)mjb_codepoint_script(0x4E00), MJB_SC_HANI, "U+4E00 CJK is Han")
    ATT_ASSERT((int)mjb_codepoint_script(0xAC00), MJB_SC_HANG, "U+AC00 is Hangul")
    ATT_ASSERT((int)mjb_codepoint_script(0x0030), MJB_SC_ZYYY, "U+0030 '0' is Common")

    ATT_ASSERT(mjb_property_name(MJB_PR_CASED), "Cased", "Property name for MJB_PR_CASED is 'Cased'")
    ATT_ASSERT(mjb_property_name((mjb_property)MJB_PR_COUNT), "Unknown", "Property name with invalid number is 'Unknown'")

    return 0;
}
