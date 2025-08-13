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

 void *test_next(void *arg) {
    ATT_ASSERT(mjb_next_character("Hèllò", 7, MJB_ENCODING_UTF_8, next_character), true, "Next character")
    ATT_ASSERT(mjb_test_count, 5, "mjb_next_character")

    return NULL;
 }
