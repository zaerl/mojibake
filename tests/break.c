/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"

void *test_break(void *arg) {
    ATT_ASSERT(mjb_break("", 0, MJB_ENCODING_UTF_8), true, "Break with empty buffer")

    return NULL;
}
