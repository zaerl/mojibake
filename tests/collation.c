/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"

void *test_collation(void *arg) {
    ATT_ASSERT(mjb_string_compare("", 0, "", 0, MJB_ENCODING_UTF_8), 0, "Collation: '' == ''")
    ATT_ASSERT(mjb_string_compare("hello", 5, "hello", 5, MJB_ENCODING_UTF_8), 0, "Collation: hello == hello")

    return NULL;
}
