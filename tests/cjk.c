/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"

int test_cjk(void *arg) {
    ATT_ASSERT(mjb_codepoint_is_cjk_ideograph(MJB_CJK_IDEOGRAPH_START), true, "CJK start")
    ATT_ASSERT(mjb_codepoint_is_cjk_ideograph(MJB_CJK_IDEOGRAPH_END), true, "CJK end")
    ATT_ASSERT(mjb_codepoint_is_cjk_ideograph(MJB_CJK_IDEOGRAPH_START - 1), false, "Before CJK")
    ATT_ASSERT(mjb_codepoint_is_cjk_ideograph(MJB_CJK_IDEOGRAPH_END + 1), false, "After CJK")
    ATT_ASSERT(mjb_codepoint_is_cjk_ideograph(MJB_CJK_EXTENSION_A_START), true,
        "CJK ext A is ideograph")
    ATT_ASSERT(mjb_codepoint_is_cjk_ideograph(MJB_CJK_EXTENSION_J_END), true,
        "CJK ext J is ideograph")
    ATT_ASSERT(mjb_codepoint_is_cjk_ideograph(MJB_CJK_COMPATIBILITY_IDEOGRAPH_START), false,
        "CJK compatibility is not unified")

    ATT_ASSERT(mjb_codepoint_is_cjk_extension_ideograph(MJB_CJK_EXTENSION_A_START), true,
        "CJK ext A start")
    ATT_ASSERT(mjb_codepoint_is_cjk_extension_ideograph(MJB_CJK_EXTENSION_B_START), true,
        "CJK ext B start")
    ATT_ASSERT(mjb_codepoint_is_cjk_extension_ideograph(MJB_CJK_EXTENSION_C_START), true,
        "CJK ext C start")
    ATT_ASSERT(mjb_codepoint_is_cjk_extension_ideograph(MJB_CJK_EXTENSION_D_START), true,
        "CJK ext D start")
    ATT_ASSERT(mjb_codepoint_is_cjk_extension_ideograph(MJB_CJK_EXTENSION_E_START), true,
        "CJK ext E start")
    ATT_ASSERT(mjb_codepoint_is_cjk_extension_ideograph(MJB_CJK_EXTENSION_F_START), true,
        "CJK ext F start")
    ATT_ASSERT(mjb_codepoint_is_cjk_extension_ideograph(MJB_CJK_EXTENSION_G_START), true,
        "CJK ext G start")
    ATT_ASSERT(mjb_codepoint_is_cjk_extension_ideograph(MJB_CJK_EXTENSION_H_START), true,
        "CJK ext H start")
    ATT_ASSERT(mjb_codepoint_is_cjk_extension_ideograph(MJB_CJK_EXTENSION_I_START), true,
        "CJK ext I start")
    ATT_ASSERT(mjb_codepoint_is_cjk_extension_ideograph(MJB_CJK_EXTENSION_J_START), true,
        "CJK ext J start")
    ATT_ASSERT(mjb_codepoint_is_cjk_extension_ideograph(MJB_CJK_EXTENSION_J_END), true,
        "CJK ext J end")
    ATT_ASSERT(mjb_codepoint_is_cjk_extension_ideograph(0x2B81E), true, "Unicode 18 CJK ext D end")
    ATT_ASSERT(mjb_codepoint_is_cjk_extension_ideograph(MJB_CJK_EXTENSION_A_START - 1), false,
        "Before CJK ext A")
    ATT_ASSERT(mjb_codepoint_is_cjk_extension_ideograph(MJB_CJK_EXTENSION_D_END + 1), false,
        "CJK ext D-E gap")
    ATT_ASSERT(mjb_codepoint_is_cjk_extension_ideograph(MJB_CJK_IDEOGRAPH_START), false,
        "Main CJK is not ext")
    ATT_ASSERT(mjb_codepoint_is_cjk_extension_ideograph(MJB_CJK_COMPATIBILITY_IDEOGRAPH_START),
        false, "CJK compatibility is not ext")

    return 0;
}
