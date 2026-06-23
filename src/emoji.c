/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "mojibake-internal.h"
#include "unicode-tables.h"

MJB_EXPORT bool mjb_codepoint_emoji(mjb_codepoint codepoint, mjb_emoji_properties *emoji) {
    if(emoji == NULL || !mjb_codepoint_is_valid(codepoint)) {
        return false;
    }

    return mjb_unicode_emoji_lookup(codepoint, emoji);
}
