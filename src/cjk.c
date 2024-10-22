/**
 * The mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "mojibake.h"

// Return if the codepoint is a CJK ideograph
MJB_EXPORT bool mjb_codepoint_is_cjk_ideograph(mjb_codepoint codepoint) {
    return codepoint >= MJB_CODEPOINT_CJK_IDEOGRAPH_START && codepoint <= MJB_CODEPOINT_CJK_IDEOGRAPH_END;
}
