/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"

int test_properties(void *arg) {
    bool binary = false;
    int32_t enumerated = -1;

    MJB_TEST_COVERAGE(mjb_codepoint_property_binary);
    ATT_ASSERT_STATUS(mjb_codepoint_property_binary(0x41, MJB_PR_ALPHABETIC, &binary),
        MJB_STATUS_OK, "Typed binary property present")
    ATT_ASSERT(binary, true, "U+0041 Alphabetic is true")
    ATT_ASSERT_STATUS(mjb_codepoint_property_binary(0x20, MJB_PR_ALPHABETIC, &binary),
        MJB_STATUS_OK, "Typed binary property absent")
    ATT_ASSERT(binary, false, "U+0020 Alphabetic is false")
    ATT_ASSERT_STATUS(mjb_codepoint_property_binary(0x41, MJB_PR_SCRIPT, &binary),
        MJB_STATUS_INVALID_ARGUMENT, "Binary getter rejects enumerated property")
    ATT_ASSERT_STATUS(mjb_codepoint_property_binary(0x41, MJB_PR_ALPHABETIC, NULL),
        MJB_STATUS_INVALID_ARGUMENT, "Binary getter rejects NULL output")

    MJB_TEST_COVERAGE(mjb_codepoint_property_int);
    ATT_ASSERT_STATUS(mjb_codepoint_property_int(0x41, MJB_PR_SCRIPT, &enumerated),
        MJB_STATUS_OK, "Typed enumerated property present")
    ATT_ASSERT(enumerated, MJB_SC_LATN, "U+0041 Script is Latin")
    ATT_ASSERT_STATUS(mjb_codepoint_property_int(0x41, MJB_PR_ALPHABETIC, &enumerated),
        MJB_STATUS_INVALID_ARGUMENT, "Enumerated getter rejects binary property")
    ATT_ASSERT_STATUS(mjb_codepoint_property_int(0x41, MJB_PR_SCRIPT, NULL),
        MJB_STATUS_INVALID_ARGUMENT, "Enumerated getter rejects NULL output")
    ATT_ASSERT_STATUS(mjb_codepoint_property_int(MJB_CODEPOINT_MAX + 1, MJB_PR_SCRIPT,
        &enumerated), MJB_STATUS_INVALID_ARGUMENT, "Typed getter rejects invalid codepoint")

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
