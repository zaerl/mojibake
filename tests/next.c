/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"

// The callbacks count their calls through `context`, so no global state is needed.
static bool next_character(const mjb_character *character, mjb_character_position position,
    void *context) {
    (void)character;
    (void)position;
    ++*(size_t *)context;

    return true;
}

static bool stop_next_character(const mjb_character *character, mjb_character_position position,
    void *context) {
    (void)character;
    (void)position;
    ++*(size_t *)context;

    return false;
}

ATT_TEST(next) {
    size_t mjb_test_count = 0;

    ATT_ASSERT_STATUS(mjb_for_each_codepoint(NULL, MJB_MALFORMED_STOP, NULL, NULL, NULL),
        MJB_STATUS_INVALID_ARGUMENT, "For each codepoint rejects NULL text");

    mjb_text text = mjb_text_utf8(NULL, 1);
    ATT_ASSERT_STATUS(mjb_for_each_codepoint(&text, MJB_MALFORMED_STOP,
                          next_character, &mjb_test_count, NULL),
        MJB_STATUS_INVALID_ARGUMENT, "NULL next buffer");
    text = mjb_text_utf8("A", 1);
    ATT_ASSERT_STATUS(mjb_for_each_codepoint(&text, MJB_MALFORMED_STOP, NULL, NULL,
                          NULL),
        MJB_STATUS_INVALID_ARGUMENT, "NULL next callback");
    text = mjb_text_utf8("Hèllò", 7);
    ATT_ASSERT_STATUS(mjb_for_each_codepoint(&text, MJB_MALFORMED_STOP,
                          next_character, &mjb_test_count, NULL),
        MJB_STATUS_OK, "Next character");
    ATT_ASSERT(mjb_test_count, 5, "mjb_for_each_codepoint");

    mjb_test_count = 0;
    text = mjb_text_utf8("A", 1);
    ATT_ASSERT_STATUS(mjb_for_each_codepoint(&text, MJB_MALFORMED_STOP,
                          stop_next_character, &mjb_test_count, NULL),
        MJB_STATUS_CALLBACK_STOPPED, "Next character callback stopped");
    ATT_ASSERT(mjb_test_count, 1, "mjb_for_each_codepoint stopped after callback");

    mjb_test_count = 0;
    text = mjb_text_utf8(NULL, 0);
    ATT_ASSERT_STATUS(mjb_for_each_codepoint(&text, MJB_MALFORMED_STOP,
                          next_character, &mjb_test_count, NULL),
        MJB_STATUS_OK, "Empty codepoint iteration");
    ATT_ASSERT(mjb_test_count, (size_t)0, "Empty codepoint iteration has no callbacks");

    const char malformed[] = { 'A', '\x80', 'B' };
    mjb_diagnostic diagnostic;

    text = mjb_text_utf8(malformed, sizeof(malformed));
    ATT_ASSERT_STATUS(mjb_for_each_codepoint(&text,
                          MJB_MALFORMED_STOP, next_character, &mjb_test_count, &diagnostic),
        MJB_STATUS_MALFORMED_INPUT, "Codepoint iteration stop policy");
    ATT_ASSERT(mjb_test_count, (size_t)0, "Stop policy validates before invoking callbacks");
    ATT_ASSERT((unsigned int)diagnostic.error,
        (unsigned int)MJB_TEXT_ERROR_UNEXPECTED_CONTINUATION,
        "Codepoint iteration stop diagnostic");

    text = mjb_text_utf8(malformed, sizeof(malformed));
    ATT_ASSERT_STATUS(mjb_for_each_codepoint(&text,
                          MJB_MALFORMED_REPLACE, next_character, &mjb_test_count, &diagnostic),
        MJB_STATUS_OK, "Codepoint iteration replace policy");
    ATT_ASSERT(mjb_test_count, (size_t)3, "Codepoint iteration includes replacement");

    mjb_test_count = 0;
    text = mjb_text_utf8(malformed, sizeof(malformed));
    ATT_ASSERT_STATUS(mjb_for_each_codepoint(&text,
                          MJB_MALFORMED_SKIP, next_character, &mjb_test_count, &diagnostic),
        MJB_STATUS_OK, "Codepoint iteration skip policy");
    ATT_ASSERT(mjb_test_count, (size_t)2, "Codepoint iteration skips malformed input");
}
