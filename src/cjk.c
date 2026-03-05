/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "mojibake.h"

// Return if the codepoint is a CJK ideograph
MJB_EXPORT bool mjb_codepoint_is_cjk_ideograph(mjb_codepoint cp) {
    return (cp >= MJB_CJK_IDEOGRAPH_START && cp <= MJB_CJK_IDEOGRAPH_END) ||
        mjb_codepoint_is_cjk_ext(cp);
}

MJB_EXPORT bool mjb_codepoint_is_cjk_ext(mjb_codepoint cp) {
    return (cp >= MJB_CJK_EXTENSION_A_START  && cp <= MJB_CJK_EXTENSION_A_END) ||
        (cp >= MJB_CJK_EXTENSION_B_START && cp <= MJB_CJK_EXTENSION_B_END) ||
        (cp >= MJB_CJK_EXTENSION_C_START && cp <= MJB_CJK_EXTENSION_C_END) ||
        (cp >= MJB_CJK_EXTENSION_D_START && cp <= MJB_CJK_EXTENSION_D_END) ||
        (cp >= MJB_CJK_EXTENSION_E_START && cp <= MJB_CJK_EXTENSION_E_END) ||
        (cp >= MJB_CJK_EXTENSION_F_START && cp <= MJB_CJK_EXTENSION_F_END) ||
        (cp >= MJB_CJK_EXTENSION_G_START && cp <= MJB_CJK_EXTENSION_G_END) ||
        (cp >= MJB_CJK_EXTENSION_H_START && cp <= MJB_CJK_EXTENSION_H_END) ||
        (cp >= MJB_CJK_EXTENSION_I_START && cp <= MJB_CJK_EXTENSION_I_END) ||
        (cp >= MJB_CJK_EXTENSION_J_START && cp <= MJB_CJK_EXTENSION_J_END);
}
