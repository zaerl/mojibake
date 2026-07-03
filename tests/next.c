/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"

static size_t mjb_test_count = 0;

static bool next_character(mjb_character *character, mjb_next_character_type type) {
    ++mjb_test_count;

    return true;
}

static bool stop_next_character(mjb_character *character, mjb_next_character_type type) {
    ++mjb_test_count;

    return false;
}

void *test_next(void *arg) {
    ATT_ASSERT_STATUS(mjb_next_character(NULL, 1, MJB_ENCODING_UTF_8, next_character),
        MJB_STATUS_INVALID_ARGUMENT,
        "NULL next buffer")
    ATT_ASSERT_STATUS(mjb_next_character("A", 1, MJB_ENCODING_UTF_8, NULL),
        MJB_STATUS_INVALID_ARGUMENT,
        "NULL next callback")
    ATT_ASSERT_STATUS(mjb_next_character("Hèllò", 7, MJB_ENCODING_UTF_8, next_character),
        MJB_STATUS_OK, "Next character")
    ATT_ASSERT(mjb_test_count, 5, "mjb_next_character")

    mjb_test_count = 0;
    ATT_ASSERT_STATUS(mjb_next_character("A", 1, MJB_ENCODING_UTF_8, stop_next_character),
        MJB_STATUS_CALLBACK_STOPPED, "Next character callback stopped")
    ATT_ASSERT(mjb_test_count, 1, "mjb_next_character stopped after callback")

    return NULL;
}
