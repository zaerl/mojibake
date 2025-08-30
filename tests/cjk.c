/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"

void *test_cjk(void *arg) {
    ATT_ASSERT(mjb_codepoint_is_cjk_ideograph(MJB_CJK_IDEOGRAPH_START), true, "CJK start");

    return NULL;
}
