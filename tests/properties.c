/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"

void *test_properties(void *arg) {
    ATT_ASSERT(mjb_codepoint_has_property(MJB_CODEPOINT_MAX + 1, MJB_PROPERTY_ALPHABETIC), false, "Not valid codepoint");
    ATT_ASSERT(mjb_codepoint_has_property(0x41, MJB_PROPERTY_CASED), true, "LATIN CAPITAL LETTER A");
    ATT_ASSERT(mjb_codepoint_has_property(0x41, MJB_PROPERTY_UPPERCASE), true, "LATIN CAPITAL LETTER A");

    return NULL;
}
