/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"
#include "../src/east-asian-width.h"

void *test_properties(void *arg) {
    char buffer[MJB_PR_BUFFER_SIZE] = { 0 };
    ATT_ASSERT(mjb_codepoint_properties(0x41, buffer), true, "LCLA pproperties");
    ATT_ASSERT(mjb_codepoint_property(buffer, MJB_PR_CASED), 1, "LCLA cased");
    ATT_ASSERT(mjb_codepoint_property(buffer, MJB_PR_ALPHABETIC), 1, "LCLA alphabetic");
    ATT_ASSERT(mjb_codepoint_property(buffer, MJB_PR_LOWERCASE), 0, "LCLA uppercase");
    ATT_ASSERT(mjb_codepoint_property(buffer, MJB_PR_UPPERCASE), 1, "LCLA uppercase");
    ATT_ASSERT(mjb_codepoint_property(buffer, MJB_PR_CHANGES_WHEN_LOWERCASED), 1, "LCLA changes when lowercased");
    ATT_ASSERT(mjb_codepoint_property(buffer, MJB_PR_CHANGES_WHEN_CASEFOLDED), 1, "LCLA changes when casefolded");

    char value;
    ATT_ASSERT(mjb_codepoint_has_property(0x41, MJB_PR_CASED, NULL), true, "LCLA cased exists");
    ATT_ASSERT(mjb_codepoint_has_property(0x20, MJB_PR_EAST_ASIAN_WIDTH, &value), true, "Space East Asian Width exists");
    ATT_ASSERT(value, MJB_EAW_NARROW, "Space east asian width is Narrow");

    return NULL;
}
