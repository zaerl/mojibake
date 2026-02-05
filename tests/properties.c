/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"

void *test_properties(void *arg) {
    char buffer[MJB_PR_BUFFER_SIZE] = { 0 };
    ATT_ASSERT(mjb_codepoint_properties(0x41, buffer), true, "LCLA pproperties");
    ATT_ASSERT(mjb_codepoint_property(buffer, MJB_PR_CASED), 1, "LCLA cased");
    ATT_ASSERT(mjb_codepoint_property(buffer, MJB_PR_ALPHABETIC), 1, "LCLA alphabetic");
    ATT_ASSERT(mjb_codepoint_property(buffer, MJB_PR_LOWERCASE), 0, "LCLA uppercase");
    ATT_ASSERT(mjb_codepoint_property(buffer, MJB_PR_UPPERCASE), 1, "LCLA uppercase");
    ATT_ASSERT(mjb_codepoint_property(buffer, MJB_PR_CHANGES_WHEN_LOWERCASED), 1, "LCLA changes when lowercased");
    ATT_ASSERT(mjb_codepoint_property(buffer, MJB_PR_CHANGES_WHEN_CASEFOLDED), 1, "LCLA changes when casefolded");

    return NULL;
}
