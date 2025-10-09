/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"

void *test_emoji(void *arg) {
    mjb_emoji_properties emoji;

    ATT_ASSERT(mjb_codepoint_emoji(MJB_CODEPOINT_MAX + 1, &emoji), false, "Invalid codepoint")

    // CURRENT_ASSERT mjb_codepoint_emoji
    ATT_ASSERT(mjb_codepoint_emoji(0x0, &emoji), false, "NULL")

    ATT_ASSERT(mjb_codepoint_emoji(0x23, &emoji), true, "U+23: #")
    ATT_ASSERT(emoji.codepoint, 0x23, "U+23: codepoint")
    ATT_ASSERT(emoji.emoji, true, "U+23: emoji")
    ATT_ASSERT(emoji.presentation, false, "U+23: presentation")
    ATT_ASSERT(emoji.modifier, false, "U+23: modifier")
    ATT_ASSERT(emoji.modifier_base, false, "U+23: modifier base")
    ATT_ASSERT(emoji.component, true, "U+23: component")
    ATT_ASSERT(emoji.extended_pictographic, false, "U+23: extended pictographic")

    ATT_ASSERT(mjb_codepoint_emoji(0x1F600, &emoji), true, "U+1F600: 😀")
    ATT_ASSERT(emoji.codepoint, 0x1F600, "U+1F600: codepoint")
    ATT_ASSERT(emoji.emoji, true, "U+1F600: emoji")
    ATT_ASSERT(emoji.presentation, true, "U+1F600: presentation")
    ATT_ASSERT(emoji.modifier, false, "U+1F600: modifier")
    ATT_ASSERT(emoji.modifier_base, false, "U+1F600: modifier base")
    ATT_ASSERT(emoji.component, false, "U+1F600: component")
    ATT_ASSERT(emoji.extended_pictographic, true, "U+1F600: extended pictographic")

    return NULL;
}
