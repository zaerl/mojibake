/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"

static size_t mjb_test_count = 0;

static bool next_character(mjb_character *character, mjb_character_position type) {
    ++mjb_test_count;

    return true;
}

static bool stop_next_character(mjb_character *character, mjb_character_position type) {
    ++mjb_test_count;

    return false;
}

int test_next(void *arg) {
    ATT_ASSERT_STATUS(mjb_for_each_codepoint(NULL, 1, MJB_ENC_UTF_8, MJB_MALFORMED_STOP,
                          next_character, NULL),
        MJB_STATUS_INVALID_ARGUMENT, "NULL next buffer")
    ATT_ASSERT_STATUS(mjb_for_each_codepoint("A", 1, MJB_ENC_UTF_8, MJB_MALFORMED_STOP, NULL,
                          NULL),
        MJB_STATUS_INVALID_ARGUMENT, "NULL next callback")
    ATT_ASSERT_STATUS(mjb_for_each_codepoint("Hèllò", 7, MJB_ENC_UTF_8, MJB_MALFORMED_STOP,
                          next_character, NULL),
        MJB_STATUS_OK, "Next character")
    ATT_ASSERT(mjb_test_count, 5, "mjb_for_each_codepoint")

    mjb_test_count = 0;
    ATT_ASSERT_STATUS(mjb_for_each_codepoint("A", 1, MJB_ENC_UTF_8, MJB_MALFORMED_STOP,
                          stop_next_character, NULL),
        MJB_STATUS_CALLBACK_STOPPED, "Next character callback stopped")
    ATT_ASSERT(mjb_test_count, 1, "mjb_for_each_codepoint stopped after callback")

    mjb_test_count = 0;
    ATT_ASSERT_STATUS(mjb_for_each_codepoint(NULL, 0, MJB_ENC_UTF_8, MJB_MALFORMED_STOP,
                          next_character, NULL),
        MJB_STATUS_OK, "Empty codepoint iteration")
    ATT_ASSERT(mjb_test_count, (size_t)0, "Empty codepoint iteration has no callbacks")

    const char malformed[] = { 'A', '\x80', 'B' };
    mjb_diagnostic diagnostic;

    ATT_ASSERT_STATUS(mjb_for_each_codepoint(malformed, sizeof(malformed), MJB_ENC_UTF_8,
                          MJB_MALFORMED_STOP, next_character, &diagnostic),
        MJB_STATUS_MALFORMED_INPUT, "Codepoint iteration stop policy")
    ATT_ASSERT(mjb_test_count, (size_t)0, "Stop policy validates before invoking callbacks")
    ATT_ASSERT((unsigned int)diagnostic.error,
        (unsigned int)MJB_TEXT_ERROR_UNEXPECTED_CONTINUATION,
        "Codepoint iteration stop diagnostic")

    ATT_ASSERT_STATUS(mjb_for_each_codepoint(malformed, sizeof(malformed), MJB_ENC_UTF_8,
                          MJB_MALFORMED_REPLACE, next_character, &diagnostic),
        MJB_STATUS_OK, "Codepoint iteration replace policy")
    ATT_ASSERT(mjb_test_count, (size_t)3, "Codepoint iteration includes replacement")

    mjb_test_count = 0;
    ATT_ASSERT_STATUS(mjb_for_each_codepoint(malformed, sizeof(malformed), MJB_ENC_UTF_8,
                          MJB_MALFORMED_SKIP, next_character, &diagnostic),
        MJB_STATUS_OK, "Codepoint iteration skip policy")
    ATT_ASSERT(mjb_test_count, (size_t)2, "Codepoint iteration skips malformed input")

    return 0;
}
