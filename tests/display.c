/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"

void *test_display(void *arg) {
    size_t sw = 0;

    ATT_ASSERT(mjb_display_width("", 0, MJB_ENCODING_UTF_8, &sw), true, "Empty string")
    ATT_ASSERT(sw, 0, "Empty string")

    ATT_ASSERT(mjb_display_width(" ", 1, MJB_ENCODING_UTF_8, &sw), true, "Space")
    ATT_ASSERT(sw, 1, "Space")

    // Test combining marks (should be zero width)
    ATT_ASSERT(mjb_display_width("e\xCC\x81", 3, MJB_ENCODING_UTF_8, &sw), true, "e + combining acute")
    ATT_ASSERT(sw, 1, "e + combining acute (é)")

    ATT_ASSERT(mjb_display_width("\xCC\x81", 2, MJB_ENCODING_UTF_8, &sw), true, "combining acute alone")
    ATT_ASSERT(sw, 0, "combining acute alone")

    // Test zero-width format characters
    ATT_ASSERT(mjb_display_width("\xE2\x80\x8B", 3, MJB_ENCODING_UTF_8, &sw), true, "zero-width space")
    ATT_ASSERT(sw, 0, "zero-width space (U+200B)")

    ATT_ASSERT(mjb_display_width("\xE2\x80\x8C", 3, MJB_ENCODING_UTF_8, &sw), true, "zero-width non-joiner")
    ATT_ASSERT(sw, 0, "zero-width non-joiner (U+200C)")

    ATT_ASSERT(mjb_display_width("\xE2\x80\x8D", 3, MJB_ENCODING_UTF_8, &sw), true, "zero-width joiner")
    ATT_ASSERT(sw, 0, "zero-width joiner (U+200D)")

    // Test control characters
    ATT_ASSERT(mjb_display_width("\x00", 1, MJB_ENCODING_UTF_8, &sw), true, "null character")
    ATT_ASSERT(sw, 0, "null character")

    ATT_ASSERT(mjb_display_width("\t", 1, MJB_ENCODING_UTF_8, &sw), true, "tab character")
    ATT_ASSERT(sw, 0, "tab character")

    ATT_ASSERT(mjb_display_width("\n", 1, MJB_ENCODING_UTF_8, &sw), true, "newline character")
    ATT_ASSERT(sw, 0, "newline character")

    // Test wide characters (CJK)
    ATT_ASSERT(mjb_display_width("\xe4\xb8\xad", 3, MJB_ENCODING_UTF_8, &sw), true, "CJK ideograph")
    ATT_ASSERT(sw, 2, "CJK ideograph (中)")

    ATT_ASSERT(mjb_display_width("\xe4\xb8\xad\xe6\x96\x87", 6, MJB_ENCODING_UTF_8, &sw), true, "two CJK ideographs")
    ATT_ASSERT(sw, 4, "two CJK ideographs (中文)")

    // Test full-width characters
    ATT_ASSERT(mjb_display_width("\xef\xbc\xa1", 3, MJB_ENCODING_UTF_8, &sw), true, "full-width A")
    ATT_ASSERT(sw, 2, "full-width A (Ａ)")

    // Test mixed strings
    ATT_ASSERT(mjb_display_width("Hello", 5, MJB_ENCODING_UTF_8, &sw), true, "ASCII string")
    ATT_ASSERT(sw, 5, "ASCII string (Hello)")

    ATT_ASSERT(mjb_display_width("Hello\xe4\xb8\xad", 8, MJB_ENCODING_UTF_8, &sw), true, "ASCII + CJK")
    ATT_ASSERT(sw, 7, "ASCII + CJK (Hello中)")

    ATT_ASSERT(mjb_display_width("e\xcc\x81\xe4\xb8\xad", 6, MJB_ENCODING_UTF_8, &sw), true, "combining + CJK")
    ATT_ASSERT(sw, 3, "combining + CJK (é中)")

    // Test ambiguous width characters (now should be width 1 not 2)
    ATT_ASSERT(mjb_display_width("\xc2\xa1", 2, MJB_ENCODING_UTF_8, &sw), true, "inverted exclamation")
    ATT_ASSERT(sw, 1, "inverted exclamation (¡) - ambiguous treated as narrow")

    // Test complex emojis
    // Simple emoji: 👍 (U+1F44D THUMBS UP SIGN) - EAW=Wide
    ATT_ASSERT(mjb_display_width("\xf0\x9f\x91\x8d", 4, MJB_ENCODING_UTF_8, &sw), true, "simple emoji")
    ATT_ASSERT(sw, 2, "simple emoji (👍)")

    // Emoji with variation selector: ❤️ (U+2764 HEAVY BLACK HEART + U+FE0F VARIATION SELECTOR-16)
    ATT_ASSERT(mjb_display_width("\xe2\x9d\xa4\xef\xb8\x8f", 6, MJB_ENCODING_UTF_8, &sw), true, "emoji with variation selector")
    ATT_ASSERT(sw, 1, "emoji with variation selector (❤️) - base char is neutral width")

    // Emoji with skin tone modifier: 👋🏽 (U+1F44B WAVING HAND + U+1F3FD EMOJI MODIFIER FITZPATRICK TYPE-4)
    ATT_ASSERT(mjb_display_width("\xf0\x9f\x91\x8b\xf0\x9f\x8f\xbd", 8, MJB_ENCODING_UTF_8, &sw), true, "emoji with skin tone")
    ATT_ASSERT(sw, 4, "emoji with skin tone (👋🏽) - counts base + modifier separately")

    // ZWJ sequence: 👨‍👩‍👧 (U+1F468 MAN + U+200D ZWJ + U+1F469 WOMAN + U+200D ZWJ + U+1F467 GIRL)
    // Man (2) + ZWJ (0) + Woman (2) + ZWJ (0) + Girl (2) = 6
    ATT_ASSERT(mjb_display_width("\xf0\x9f\x91\xa8\xe2\x80\x8d\xf0\x9f\x91\xa9\xe2\x80\x8d\xf0\x9f\x91\xa7", 18, MJB_ENCODING_UTF_8, &sw), true, "family emoji ZWJ sequence")
    ATT_ASSERT(sw, 6, "family emoji ZWJ sequence (👨‍👩‍👧) - counts each emoji separately")

    return NULL;
}
