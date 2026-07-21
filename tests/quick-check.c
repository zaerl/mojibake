/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>

#include "test.h"

static void assert_quick_check(const char *buffer, size_t byte_length, mjb_encoding encoding,
    mjb_normalization form, mjb_status expected_status, mjb_quick_check_result expected_result,
    const char *message) {
    mjb_quick_check_result result = MJB_QC_MAYBE;
    mjb_status status = mjb_normalization_quick_check(buffer, byte_length, encoding, form, &result);
    MJB_TEST_COVERAGE(mjb_normalization_quick_check);

    ATT_ASSERT_STATUS(status, expected_status, message)

    if(status == MJB_STATUS_OK) {
        ATT_ASSERT((unsigned int)result, (unsigned int)expected_result, message)
    } else {
        ATT_ASSERT((unsigned int)result, (unsigned int)MJB_QC_NO,
            "Quick check clears output before failure")
    }
}

int test_quick_check(void *arg) {
    mjb_encoding enc = MJB_ENC_UTF_8;

    ATT_ASSERT_STATUS(mjb_normalization_quick_check("a", 1, enc, MJB_NORMALIZATION_NFC, NULL),
        MJB_STATUS_INVALID_ARGUMENT, "NULL quick-check output")
    assert_quick_check(NULL, 1, enc, MJB_NORMALIZATION_NFC, MJB_STATUS_INVALID_ARGUMENT, MJB_QC_NO,
        "NULL quick-check input");
    assert_quick_check("a", 1, enc, (mjb_normalization)99, MJB_STATUS_INVALID_FORM, MJB_QC_NO,
        "Invalid normalization form");
    assert_quick_check("a", 1, MJB_ENC_UNKNOWN, MJB_NORMALIZATION_NFC, MJB_STATUS_INVALID_ENCODING,
        MJB_QC_NO, "Invalid quick-check encoding");
    assert_quick_check("a", 1, MJB_ENC_UTF_16, MJB_NORMALIZATION_NFC, MJB_STATUS_INVALID_ENCODING,
        MJB_QC_NO, "Generic UTF-16 requires byte order");

    const char malformed_utf8[] = { (char)0xE2, (char)0x82 };
    assert_quick_check(malformed_utf8, sizeof(malformed_utf8), enc, MJB_NORMALIZATION_NFC,
        MJB_STATUS_MALFORMED_INPUT, MJB_QC_NO, "Malformed UTF-8 quick check");

    assert_quick_check("", 0, enc, MJB_NORMALIZATION_NFC, MJB_STATUS_OK, MJB_QC_YES,
        "Empty string is NFC normalized");
    assert_quick_check("", 0, enc, MJB_NORMALIZATION_NFD, MJB_STATUS_OK, MJB_QC_YES,
        "Empty string is NFD normalized");
    assert_quick_check("", 0, enc, MJB_NORMALIZATION_NFKC, MJB_STATUS_OK, MJB_QC_YES,
        "Empty string is NFKC normalized");
    assert_quick_check("", 0, enc, MJB_NORMALIZATION_NFKD, MJB_STATUS_OK, MJB_QC_YES,
        "Empty string is NFKD normalized");

    assert_quick_check("abc", 3, enc, MJB_NORMALIZATION_NFC, MJB_STATUS_OK, MJB_QC_YES,
        "ASCII string is NFC normalized");
    assert_quick_check("def", 3, enc, MJB_NORMALIZATION_NFD, MJB_STATUS_OK, MJB_QC_YES,
        "ASCII string is NFD normalized");
    assert_quick_check("ghi", 3, enc, MJB_NORMALIZATION_NFKC, MJB_STATUS_OK, MJB_QC_YES,
        "ASCII string is NFKC normalized");
    assert_quick_check("jkl", 3, enc, MJB_NORMALIZATION_NFKD, MJB_STATUS_OK, MJB_QC_YES,
        "ASCII string is NFKD normalized");
    assert_quick_check("áéíóú", 10, enc, MJB_NORMALIZATION_NFC, MJB_STATUS_OK, MJB_QC_YES,
        "Latin-1 string is NFC normalized");
    assert_quick_check("\xC3\xA9", 2, enc, MJB_NORMALIZATION_NFD, MJB_STATUS_OK, MJB_QC_NO,
        "Composed e acute is not NFD");
    assert_quick_check("e\xCC\x81", 3, enc, MJB_NORMALIZATION_NFC, MJB_STATUS_OK, MJB_QC_MAYBE,
        "Decomposed e acute NFC quick check is maybe");

    return 0;
}
