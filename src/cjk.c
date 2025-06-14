/**
 * The mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "mojibake.h"

// Return if the codepoint is a CJK ideograph
MJB_EXPORT bool mjb_codepoint_is_cjk_ideograph(mjb_codepoint codepoint) {
    return (codepoint >= MJB_CODEPOINT_CJK_IDEOGRAPH_START && codepoint <= MJB_CODEPOINT_CJK_IDEOGRAPH_END) ||
        (codepoint >= MJB_CODEPOINT_CJK_EXTENSION_A_START && codepoint <= MJB_CODEPOINT_CJK_EXTENSION_A_END) ||
        (codepoint >= MJB_CODEPOINT_CJK_EXTENSION_B_START && codepoint <= MJB_CODEPOINT_CJK_EXTENSION_B_END) ||
        (codepoint >= MJB_CODEPOINT_CJK_EXTENSION_C_START && codepoint <= MJB_CODEPOINT_CJK_EXTENSION_C_END) ||
        (codepoint >= MJB_CODEPOINT_CJK_EXTENSION_D_START && codepoint <= MJB_CODEPOINT_CJK_EXTENSION_D_END) ||
        (codepoint >= MJB_CODEPOINT_CJK_EXTENSION_E_START && codepoint <= MJB_CODEPOINT_CJK_EXTENSION_E_END) ||
        (codepoint >= MJB_CODEPOINT_CJK_EXTENSION_F_START && codepoint <= MJB_CODEPOINT_CJK_EXTENSION_F_END) ||
        (codepoint >= MJB_CODEPOINT_CJK_EXTENSION_I_START && codepoint <= MJB_CODEPOINT_CJK_EXTENSION_I_END) ||
        (codepoint >= MJB_CODEPOINT_TANGUT_IDEOGRAPH_START && codepoint <= MJB_CODEPOINT_TANGUT_IDEOGRAPH_END);
}
