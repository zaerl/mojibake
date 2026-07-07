/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"

/**
 * East Asian Width (EAW)
 * https://www.unicode.org/reports/tr11/
 */
int test_east_asian_width(void *arg) {
    mjb_east_asian_width width = MJB_EAW_NEUTRAL;

    ATT_ASSERT_STATUS(mjb_codepoint_east_asian_width(MJB_CODEPOINT_MAX + 1, &width),
        MJB_STATUS_INVALID_ARGUMENT, "Invalid")
    ATT_ASSERT_STATUS(mjb_codepoint_east_asian_width(0x00A1, NULL),
        MJB_STATUS_INVALID_ARGUMENT, "NULL width pointer")

    ATT_ASSERT_STATUS(mjb_codepoint_east_asian_width(0x00A1, &width), MJB_STATUS_OK, "First A")
    ATT_ASSERT((int)width, MJB_EAW_AMBIGUOUS, "INVERTED EXCLAMATION MARK")

    ATT_ASSERT_STATUS(mjb_codepoint_east_asian_width(0x3000, &width), MJB_STATUS_OK, "First F")
    ATT_ASSERT((int)width, MJB_EAW_FULL_WIDTH, "IDEOGRAPHIC SPACE")

    ATT_ASSERT_STATUS(mjb_codepoint_east_asian_width(0x20A9, &width), MJB_STATUS_OK, "First H")
    ATT_ASSERT((int)width, MJB_EAW_HALF_WIDTH, "WON SIGN")

    ATT_ASSERT_STATUS(mjb_codepoint_east_asian_width(0x0, &width), MJB_STATUS_OK, "First N")
    ATT_ASSERT((int)width, MJB_EAW_NEUTRAL, "NULL")

    ATT_ASSERT_STATUS(mjb_codepoint_east_asian_width(0x20, &width), MJB_STATUS_OK, "First Na")
    ATT_ASSERT((int)width, MJB_EAW_NARROW, "Space")

    ATT_ASSERT_STATUS(mjb_codepoint_east_asian_width(0x1100, &width), MJB_STATUS_OK, "First W")
    ATT_ASSERT((int)width, MJB_EAW_WIDE, "HANGUL CHOSEONG KIYEOK")

    ATT_ASSERT_STATUS(mjb_codepoint_east_asian_width(0x3400, &width), MJB_STATUS_OK,
        "CJK Extension A")
    ATT_ASSERT((int)width, MJB_EAW_WIDE, "CJK Extension A")

    ATT_ASSERT_STATUS(mjb_codepoint_east_asian_width(0x4E00, &width), MJB_STATUS_OK,
        "CJK Unified Ideograph")
    ATT_ASSERT((int)width, MJB_EAW_WIDE, "CJK Unified Ideograph")

    ATT_ASSERT_STATUS(mjb_codepoint_east_asian_width(0xF900, &width), MJB_STATUS_OK,
        "CJK Compatibility Ideograph")
    ATT_ASSERT((int)width, MJB_EAW_WIDE, "CJK Compatibility Ideograph")

    ATT_ASSERT_STATUS(mjb_codepoint_east_asian_width(0x20000, &width), MJB_STATUS_OK, "Plane 2")
    ATT_ASSERT((int)width, MJB_EAW_WIDE, "Plane 2")

    ATT_ASSERT_STATUS(mjb_codepoint_east_asian_width(0x30000, &width), MJB_STATUS_OK, "Plane 3")
    ATT_ASSERT((int)width, MJB_EAW_WIDE, "Plane 3")

    ATT_ASSERT_STATUS(mjb_codepoint_east_asian_width(0xE0100, &width), MJB_STATUS_OK,
        "VARIATION SELECTOR-17")
    ATT_ASSERT((int)width, MJB_EAW_AMBIGUOUS, "VARIATION SELECTOR-17")

    ATT_ASSERT_STATUS(mjb_codepoint_east_asian_width(0xF0000, &width), MJB_STATUS_OK,
        "<private-use-F0000>")
    ATT_ASSERT((int)width, MJB_EAW_AMBIGUOUS, "<private-use-F0000>")

    ATT_ASSERT_STATUS(mjb_codepoint_east_asian_width(0x100000, &width), MJB_STATUS_OK,
        "<private-use-100000>")
    ATT_ASSERT((int)width, MJB_EAW_AMBIGUOUS, "<private-use-100000>")

    ATT_ASSERT_STATUS(mjb_codepoint_east_asian_width(0x1FBFA + 1, &width), MJB_STATUS_OK,
        "Unassigned")
    ATT_ASSERT((int)width, MJB_EAW_NEUTRAL, "ALARM BELL SYMBOL + 1")

    return 0;
}
