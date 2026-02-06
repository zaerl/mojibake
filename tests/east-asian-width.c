/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"

void *test_east_asian_width(void *arg) {
    mjb_east_asian_width width = MJB_EAW_NEUTRAL;

    ATT_ASSERT(mjb_codepoint_east_asian_width(MJB_CODEPOINT_MAX + 1, &width), false, "Invalid")

    ATT_ASSERT(mjb_codepoint_east_asian_width(0x00A1, &width), true, "First A")
    ATT_ASSERT((int)width, MJB_EAW_AMBIGUOUS, "INVERTED EXCLAMATION MARK")

    ATT_ASSERT(mjb_codepoint_east_asian_width(0x3000, &width), true, "First F")
    ATT_ASSERT((int)width, MJB_EAW_FULL_WIDTH, "IDEOGRAPHIC SPACE")

    ATT_ASSERT(mjb_codepoint_east_asian_width(0x20A9, &width), true, "First H")
    ATT_ASSERT((int)width, MJB_EAW_HALF_WIDTH, "WON SIGN")

    ATT_ASSERT(mjb_codepoint_east_asian_width(0x0, &width), true, "First N")
    ATT_ASSERT((int)width, MJB_EAW_NEUTRAL, "NULL")

    ATT_ASSERT(mjb_codepoint_east_asian_width(0x20, &width), true, "First Na")
    ATT_ASSERT((int)width, MJB_EAW_NARROW, "Space")

    ATT_ASSERT(mjb_codepoint_east_asian_width(0x1100, &width), true, "First W")
    ATT_ASSERT((int)width, MJB_EAW_WIDE, "HANGUL CHOSEONG KIYEOK")

    ATT_ASSERT(mjb_codepoint_east_asian_width(0x3400, &width), true, "CJK Extension A")
    ATT_ASSERT((int)width, MJB_EAW_WIDE, "CJK Extension A")

    ATT_ASSERT(mjb_codepoint_east_asian_width(0x4E00, &width), true, "CJK Unified Ideograph")
    ATT_ASSERT((int)width, MJB_EAW_WIDE, "CJK Unified Ideograph")

    ATT_ASSERT(mjb_codepoint_east_asian_width(0xF900, &width), true, "CJK Compatibility Ideograph")
    ATT_ASSERT((int)width, MJB_EAW_WIDE, "CJK Compatibility Ideograph")

    ATT_ASSERT(mjb_codepoint_east_asian_width(0x20000, &width), true, "Plane 2")
    ATT_ASSERT((int)width, MJB_EAW_WIDE, "Plane 2")

    ATT_ASSERT(mjb_codepoint_east_asian_width(0x30000, &width), true, "Plane 3")
    ATT_ASSERT((int)width, MJB_EAW_WIDE, "Plane 3")

    ATT_ASSERT(mjb_codepoint_east_asian_width(0xE0100, &width), true, "VARIATION SELECTOR-17")
    ATT_ASSERT((int)width, MJB_EAW_AMBIGUOUS, "VARIATION SELECTOR-17")

    ATT_ASSERT(mjb_codepoint_east_asian_width(0xF0000, &width), true, "<private-use-F0000>")
    ATT_ASSERT((int)width, MJB_EAW_AMBIGUOUS, "<private-use-F0000>")

    ATT_ASSERT(mjb_codepoint_east_asian_width(0x100000, &width), true, "<private-use-100000>")
    ATT_ASSERT((int)width, MJB_EAW_AMBIGUOUS, "<private-use-100000>")

    ATT_ASSERT(mjb_codepoint_east_asian_width(0x1FBFA + 1, &width), true, "Unassigned")
    ATT_ASSERT((int)width, MJB_EAW_NEUTRAL, "ALARM BELL SYMBOL + 1")

    return NULL;
}
