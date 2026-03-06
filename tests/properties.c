/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"

void *test_properties(void *arg) {
    uint8_t buffer[MJB_PR_BUFFER_SIZE] = { 0 };
    ATT_ASSERT(mjb_codepoint_properties(0x41, buffer), true, "LCLA pproperties");
    ATT_ASSERT(mjb_codepoint_property(buffer, MJB_PR_CASED), 1, "LCLA cased");
    ATT_ASSERT(mjb_codepoint_property(buffer, MJB_PR_ALPHABETIC), 1, "LCLA alphabetic");
    ATT_ASSERT(mjb_codepoint_property(buffer, MJB_PR_LOWERCASE), 0, "LCLA uppercase");
    ATT_ASSERT(mjb_codepoint_property(buffer, MJB_PR_UPPERCASE), 1, "LCLA uppercase");
    ATT_ASSERT(mjb_codepoint_property(buffer, MJB_PR_CHANGES_WHEN_LOWERCASED), 1, "LCLA changes when lowercased");
    ATT_ASSERT(mjb_codepoint_property(buffer, MJB_PR_CHANGES_WHEN_CASEFOLDED), 1, "LCLA changes when casefolded");

    uint8_t value;
    ATT_ASSERT(mjb_codepoint_has_property(0x41, MJB_PR_CASED, NULL), true, "LCLA cased exists");
    ATT_ASSERT(mjb_codepoint_has_property(0x20, MJB_PR_EAST_ASIAN_WIDTH, &value), true, "Space East Asian Width exists");
    ATT_ASSERT(value, MJB_EAW_NARROW, "Space east asian width is Narrow");

    // mjb_codepoint_script
    ATT_ASSERT((int)mjb_codepoint_script(MJB_CODEPOINT_MAX + 1), MJB_SC_ZZZZ, "Invalid codepoint script is Unknown");
    ATT_ASSERT((int)mjb_codepoint_script(0x41), MJB_SC_LATN, "U+0041 'A' is Latin");
    ATT_ASSERT((int)mjb_codepoint_script(0x0391), MJB_SC_GREK, "U+0391 'A' is Greek");
    ATT_ASSERT((int)mjb_codepoint_script(0x0410), MJB_SC_CYRL, "U+0410 'A' is Cyrillic");
    ATT_ASSERT((int)mjb_codepoint_script(0x05D0), MJB_SC_HEBR, "U+05D0 Alef is Hebrew");
    ATT_ASSERT((int)mjb_codepoint_script(0x0600), MJB_SC_ARAB, "U+0600 is Arabic");
    ATT_ASSERT((int)mjb_codepoint_script(0x4E00), MJB_SC_HANI, "U+4E00 CJK is Han");
    ATT_ASSERT((int)mjb_codepoint_script(0xAC00), MJB_SC_HANG, "U+AC00 is Hangul");
    ATT_ASSERT((int)mjb_codepoint_script(0x0030), MJB_SC_ZYYY, "U+0030 '0' is Common");

    return NULL;
}
