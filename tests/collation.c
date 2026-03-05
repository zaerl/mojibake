/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"

void *test_collation(void *arg) {
    mjb_encoding enc = MJB_ENCODING_UTF_8;

    ATT_ASSERT(mjb_string_compare("hello", 5, enc, "hello", 5, enc), 0, "UTF-8 compare: hello == hello")

    return NULL;
}
