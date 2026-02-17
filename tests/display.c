/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"

void *test_display(void *arg) {
    size_t sw = 0;

    ATT_ASSERT(mjb_display_width("", 0, MJB_ENCODING_UTF_8, MJB_WIDTH_CONTEXT_AUTO, &sw), true, "Empty string")
    ATT_ASSERT(sw, 0, "Empty string")

    ATT_ASSERT(mjb_display_width(" ", 1, MJB_ENCODING_UTF_8, MJB_WIDTH_CONTEXT_AUTO, &sw), true, "Space")
    ATT_ASSERT(sw, 1, "Space")

    // Test combining marks (should be zero width)
    ATT_ASSERT(mjb_display_width("e\xCC\x81", 3, MJB_ENCODING_UTF_8, MJB_WIDTH_CONTEXT_AUTO, &sw), true, "e + combining acute")
    ATT_ASSERT(sw, 1, "e + combining acute (é)")

    ATT_ASSERT(mjb_display_width("\xCC\x81", 2, MJB_ENCODING_UTF_8, MJB_WIDTH_CONTEXT_AUTO, &sw), true, "Combining acute alone")
    ATT_ASSERT(sw, 0, "Combining acute alone")

    // Test zero-width format characters
    ATT_ASSERT(mjb_display_width("\xE2\x80\x8B", 3, MJB_ENCODING_UTF_8, MJB_WIDTH_CONTEXT_AUTO, &sw), true, "Zero-width space")
    ATT_ASSERT(sw, 0, "Zero-width space (U+200B)")

    ATT_ASSERT(mjb_display_width("\xE2\x80\x8C", 3, MJB_ENCODING_UTF_8, MJB_WIDTH_CONTEXT_AUTO, &sw), true, "Zero-width non-joiner")
    ATT_ASSERT(sw, 0, "Zero-width non-joiner (U+200C)")

    ATT_ASSERT(mjb_display_width("\xE2\x80\x8D", 3, MJB_ENCODING_UTF_8, MJB_WIDTH_CONTEXT_AUTO, &sw), true, "Zero-width joiner")
    ATT_ASSERT(sw, 0, "Zero-width joiner (U+200D)")

    ATT_ASSERT(mjb_display_width("\t", 1, MJB_ENCODING_UTF_8, MJB_WIDTH_CONTEXT_AUTO, &sw), true, "Tab character")
    ATT_ASSERT(sw, 0, "Tab character")

    ATT_ASSERT(mjb_display_width("\n", 1, MJB_ENCODING_UTF_8, MJB_WIDTH_CONTEXT_AUTO, &sw), true, "Newline character")
    ATT_ASSERT(sw, 0, "Newline character")

    // Test wide characters (CJK)
    ATT_ASSERT(mjb_display_width("\xe4\xb8\xad", 3, MJB_ENCODING_UTF_8, MJB_WIDTH_CONTEXT_AUTO, &sw), true, "CJK ideograph")
    ATT_ASSERT(sw, 2, "CJK ideograph (中)")

    ATT_ASSERT(mjb_display_width("\xe4\xb8\xad\xe6\x96\x87", 6, MJB_ENCODING_UTF_8, MJB_WIDTH_CONTEXT_AUTO, &sw), true, "Two CJK ideographs")
    ATT_ASSERT(sw, 4, "Two CJK ideographs (中文)")

    // Test full-width characters
    ATT_ASSERT(mjb_display_width("\xef\xbc\xa1", 3, MJB_ENCODING_UTF_8, MJB_WIDTH_CONTEXT_AUTO, &sw), true, "Full-width A")
    ATT_ASSERT(sw, 2, "Full-width A (Ａ)")

    // Test mixed strings
    ATT_ASSERT(mjb_display_width("Hello", 5, MJB_ENCODING_UTF_8, MJB_WIDTH_CONTEXT_AUTO, &sw), true, "ASCII string")
    ATT_ASSERT(sw, 5, "ASCII string (Hello)")

    ATT_ASSERT(mjb_display_width("Hello\xe4\xb8\xad", 8, MJB_ENCODING_UTF_8, MJB_WIDTH_CONTEXT_AUTO, &sw), true, "ASCII + CJK")
    ATT_ASSERT(sw, 7, "ASCII + CJK (Hello中)")

    ATT_ASSERT(mjb_display_width("e\xcc\x81\xe4\xb8\xad", 6, MJB_ENCODING_UTF_8, MJB_WIDTH_CONTEXT_AUTO, &sw), true, "Combining + CJK")
    ATT_ASSERT(sw, 3, "Combining + CJK (é中)")

    // Test complex emojis
    // Simple emoji: 👍 (U+1F44D THUMBS UP SIGN) - EAW=Wide
    ATT_ASSERT(mjb_display_width("\xf0\x9f\x91\x8d", 4, MJB_ENCODING_UTF_8, MJB_WIDTH_CONTEXT_AUTO, &sw), true, "Simple emoji")
    ATT_ASSERT(sw, 2, "Simple emoji (👍)")

    // Emoji with variation selector: ❤️ (U+2764 HEAVY BLACK HEART + U+FE0F VARIATION SELECTOR-16)
    ATT_ASSERT(mjb_display_width("\xe2\x9d\xa4\xef\xb8\x8f", 6, MJB_ENCODING_UTF_8, MJB_WIDTH_CONTEXT_AUTO, &sw), true, "Emoji with variation selector")
    ATT_ASSERT(sw, 1, "Emoji with variation selector (❤️) - base char is neutral width")

    // Emoji with skin tone modifier: 👋🏽 (U+1F44B WAVING HAND + U+1F3FD EMOJI MODIFIER FITZPATRICK TYPE-4)
    ATT_ASSERT(mjb_display_width("\xf0\x9f\x91\x8b\xf0\x9f\x8f\xbd", 8, MJB_ENCODING_UTF_8, MJB_WIDTH_CONTEXT_AUTO, &sw), true, "Emoji with skin tone")
    ATT_ASSERT(sw, 4, "Emoji with skin tone (👋🏽) - counts base + modifier separately")

    // ZWJ sequence: 👨‍👩‍👧 (U+1F468 MAN + U+200D ZWJ + U+1F469 WOMAN + U+200D ZWJ + U+1F467 GIRL)
    ATT_ASSERT(mjb_display_width("\xf0\x9f\x91\xa8\xe2\x80\x8d\xf0\x9f\x91\xa9\xe2\x80\x8d\xf0\x9f\x91\xa7", 18, MJB_ENCODING_UTF_8, MJB_WIDTH_CONTEXT_AUTO, &sw), true, "Family emoji ZWJ sequence")
    ATT_ASSERT(sw, 6, "Family emoji ZWJ sequence (👨‍👩‍👧) - counts each emoji separately")

    // Auto context with Western text → narrow (1)
    ATT_ASSERT(mjb_display_width("Hello \xc2\xa1Hola!", 13, MJB_ENCODING_UTF_8, MJB_WIDTH_CONTEXT_AUTO, &sw), true, "Ambiguous in Western text (AUTO)")
    ATT_ASSERT(sw, 12, "Ambiguous in Western text (AUTO) - treats as narrow")

    // AUTO context with CJK text → wide (2)
    ATT_ASSERT(mjb_display_width("\xe4\xb8\xad\xe6\x96\x87\xc2\xa1\xe4\xb8\xad", 11, MJB_ENCODING_UTF_8, MJB_WIDTH_CONTEXT_AUTO, &sw), true, "ambiguous in CJK text (AUTO)")
    ATT_ASSERT(sw, 8, "Ambiguous in CJK text (AUTO) - treats as wide: 中(2)+文(2)+¡(2)+中(2)")

    // Explicit WESTERN context → narrow (1)
    ATT_ASSERT(mjb_display_width("\xc2\xa1", 2, MJB_ENCODING_UTF_8, MJB_WIDTH_CONTEXT_WESTERN, &sw), true, "Ambiguous (WESTERN)")
    ATT_ASSERT(sw, 1, "Ambiguous (WESTERN) - narrow")

    // Explicit EAST_ASIAN context → wide (2)
    ATT_ASSERT(mjb_display_width("\xc2\xa1", 2, MJB_ENCODING_UTF_8, MJB_WIDTH_CONTEXT_EAST_ASIAN, &sw), true, "Ambiguous (EAST_ASIAN)")
    ATT_ASSERT(sw, 2, "Ambiguous (EAST_ASIAN) - wide")

    return NULL;
}
