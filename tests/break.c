/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"
#include "../src/mojibake-internal.h"

void *test_break(void *arg) {
    mjb_line_breaking_class lbc;

    ATT_ASSERT(mjb_codepoint_line_breaking_class(MJB_CODEPOINT_MAX + 1, &lbc), false, "Invalid")

    // CURRENT_ASSERT mjb_codepoint_line_breaking_class
    ATT_ASSERT(mjb_codepoint_line_breaking_class(0x0, &lbc), true, "NULL")
    ATT_ASSERT(lbc, MJB_LBC_CM, "CM")

    return NULL;
}
