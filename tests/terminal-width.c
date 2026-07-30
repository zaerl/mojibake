/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <string.h>

#include "test.h"

int test_terminal_width(void *arg) {
    size_t sw = 0;

    ATT_ASSERT_STATUS(mjb_terminal_width("A", 1, MJB_ENC_UTF_8, MJB_TERMINAL_WIDTH_NARROW, NULL),
        MJB_STATUS_INVALID_ARGUMENT, "NULL width pointer")
    ATT_ASSERT_STATUS(mjb_terminal_width(NULL, 1, MJB_ENC_UTF_8, MJB_TERMINAL_WIDTH_NARROW, &sw),
        MJB_STATUS_INVALID_ARGUMENT, "NULL terminal buffer")
    ATT_ASSERT_STATUS(mjb_terminal_width("A", 1, MJB_ENC_UTF_8,
        (mjb_terminal_width_profile)2, &sw), MJB_STATUS_INVALID_ARGUMENT,
        "Invalid terminal-width profile")
    ATT_ASSERT_STATUS(mjb_terminal_width("A", 1, MJB_ENC_UNKNOWN, MJB_TERMINAL_WIDTH_NARROW, &sw),
        MJB_STATUS_INVALID_ENCODING, "Invalid input encoding")

    ATT_ASSERT_STATUS(mjb_terminal_width("", 0, MJB_ENC_UTF_8, MJB_TERMINAL_WIDTH_NARROW, &sw),
        MJB_STATUS_OK, "Empty string")
    ATT_ASSERT(sw, 0, "Empty string")

    ATT_ASSERT_STATUS(mjb_terminal_width(" ", 1, MJB_ENC_UTF_8, MJB_TERMINAL_WIDTH_NARROW, &sw),
        MJB_STATUS_OK, "Space")
    ATT_ASSERT(sw, 1, "Space")

    // Test combining marks (should be zero width)
    ATT_ASSERT_STATUS(mjb_terminal_width("e\xCC\x81", 3, MJB_ENC_UTF_8,
        MJB_TERMINAL_WIDTH_NARROW, &sw),
        MJB_STATUS_OK, "e + combining acute")
    ATT_ASSERT(sw, 1, "e + combining acute (é)")

    ATT_ASSERT_STATUS(mjb_terminal_width("\xCC\x81", 2, MJB_ENC_UTF_8,
        MJB_TERMINAL_WIDTH_NARROW, &sw),
        MJB_STATUS_OK, "Combining acute alone")
    ATT_ASSERT(sw, 0, "Combining acute alone")

    // Test zero-width format characters
    ATT_ASSERT_STATUS(mjb_terminal_width("\xE2\x80\x8B", 3, MJB_ENC_UTF_8,
        MJB_TERMINAL_WIDTH_NARROW, &sw), MJB_STATUS_OK, "Zero-width space")
    ATT_ASSERT(sw, 0, "Zero-width space (U+200B)")

    ATT_ASSERT_STATUS(mjb_terminal_width("\xE2\x80\x8C", 3, MJB_ENC_UTF_8,
        MJB_TERMINAL_WIDTH_NARROW, &sw), MJB_STATUS_OK, "Zero-width non-joiner")
    ATT_ASSERT(sw, 0, "Zero-width non-joiner (U+200C)")

    ATT_ASSERT_STATUS(mjb_terminal_width("\xE2\x80\x8D", 3, MJB_ENC_UTF_8,
        MJB_TERMINAL_WIDTH_NARROW, &sw), MJB_STATUS_OK, "Zero-width joiner")
    ATT_ASSERT(sw, 0, "Zero-width joiner (U+200D)")

    ATT_ASSERT_STATUS(mjb_terminal_width("\t", 1, MJB_ENC_UTF_8, MJB_TERMINAL_WIDTH_NARROW, &sw),
        MJB_STATUS_UNSUPPORTED, "Tab depends on terminal state")
    ATT_ASSERT_STATUS(mjb_terminal_width("\n", 1, MJB_ENC_UTF_8, MJB_TERMINAL_WIDTH_NARROW, &sw),
        MJB_STATUS_UNSUPPORTED, "Newline is not single-line text")
    ATT_ASSERT_STATUS(mjb_terminal_width("\x1B[31m", 5, MJB_ENC_UTF_8,
        MJB_TERMINAL_WIDTH_NARROW, &sw), MJB_STATUS_UNSUPPORTED,
        "ANSI escape sequence is not printable text")
    ATT_ASSERT_STATUS(mjb_terminal_width("\xE2\x80\xA8", 3, MJB_ENC_UTF_8,
        MJB_TERMINAL_WIDTH_NARROW, &sw), MJB_STATUS_UNSUPPORTED,
        "Unicode line separator is not single-line text")
    ATT_ASSERT(sw, 0, "Failed measurement clears the output width")

    // Test wide characters (CJK)
    ATT_ASSERT_STATUS(mjb_terminal_width("\xe4\xb8\xad", 3, MJB_ENC_UTF_8,
        MJB_TERMINAL_WIDTH_NARROW, &sw), MJB_STATUS_OK, "CJK ideograph")
    ATT_ASSERT(sw, 2, "CJK ideograph (中)")

    ATT_ASSERT_STATUS(mjb_terminal_width("\xe4\xb8\xad\xe6\x96\x87", 6, MJB_ENC_UTF_8,
        MJB_TERMINAL_WIDTH_NARROW, &sw), MJB_STATUS_OK, "Two CJK ideographs")
    ATT_ASSERT(sw, 4, "Two CJK ideographs (中文)")

    // Test full-width characters
    ATT_ASSERT_STATUS(mjb_terminal_width("\xef\xbc\xa1", 3, MJB_ENC_UTF_8,
        MJB_TERMINAL_WIDTH_NARROW, &sw), MJB_STATUS_OK, "Full-width A")
    ATT_ASSERT(sw, 2, "Full-width A (Ａ)")

    // Test mixed strings
    ATT_ASSERT_STATUS(mjb_terminal_width("Hello", 5, MJB_ENC_UTF_8,
        MJB_TERMINAL_WIDTH_NARROW, &sw),
        MJB_STATUS_OK, "ASCII string")
    ATT_ASSERT(sw, 5, "ASCII string (Hello)")
    ATT_ASSERT_STATUS(mjb_terminal_width("Hello", MJB_NUL_TERMINATED, MJB_ENC_UTF_8,
        MJB_TERMINAL_WIDTH_NARROW, &sw), MJB_STATUS_OK, "NUL-terminated UTF-8")
    ATT_ASSERT(sw, 5, "NUL-terminated UTF-8 string")

    const char utf16le[] = { 0x41, 0x00, 0x4C, 0x75 };
    ATT_ASSERT_STATUS(mjb_terminal_width(utf16le, sizeof(utf16le), MJB_ENC_UTF_16LE,
        MJB_TERMINAL_WIDTH_NARROW, &sw), MJB_STATUS_OK, "UTF-16LE input")
    ATT_ASSERT(sw, 3, "UTF-16LE A + CJK")

    const char utf32be[] = { 0x00, 0x00, 0x00, 0x41, 0x00, 0x00, 0x75, 0x4C };
    ATT_ASSERT_STATUS(mjb_terminal_width(utf32be, sizeof(utf32be), MJB_ENC_UTF_32BE,
        MJB_TERMINAL_WIDTH_NARROW, &sw), MJB_STATUS_OK, "UTF-32BE input")
    ATT_ASSERT(sw, 3, "UTF-32BE A + CJK")

    ATT_ASSERT_STATUS(mjb_terminal_width("Hello\xe4\xb8\xad", 8, MJB_ENC_UTF_8,
                          MJB_TERMINAL_WIDTH_NARROW, &sw),
        MJB_STATUS_OK, "ASCII + CJK")
    ATT_ASSERT(sw, 7, "ASCII + CJK (Hello中)")

    ATT_ASSERT_STATUS(mjb_terminal_width("e\xcc\x81\xe4\xb8\xad", 6, MJB_ENC_UTF_8,
        MJB_TERMINAL_WIDTH_NARROW, &sw), MJB_STATUS_OK, "Combining + CJK")
    ATT_ASSERT(sw, 3, "Combining + CJK (é中)")

    // Test complex emojis
    // Simple emoji: 👍 (U+1F44D THUMBS UP SIGN) - EAW=Wide
    ATT_ASSERT_STATUS(mjb_terminal_width("\xf0\x9f\x91\x8d", 4, MJB_ENC_UTF_8,
        MJB_TERMINAL_WIDTH_NARROW, &sw), MJB_STATUS_OK, "Simple emoji")
    ATT_ASSERT(sw, 2, "Simple emoji (👍)")

    // Emoji with variation selector: ❤️ (U+2764 HEAVY BLACK HEART + U+FE0F VARIATION SELECTOR-16)
    ATT_ASSERT_STATUS(mjb_terminal_width("\xe2\x9d\xa4\xef\xb8\x8f", 6, MJB_ENC_UTF_8,
        MJB_TERMINAL_WIDTH_NARROW, &sw), MJB_STATUS_OK, "Emoji with variation selector")
    ATT_ASSERT(sw, 2, "Emoji presentation sequence (❤️)")

    ATT_ASSERT_STATUS(mjb_terminal_width("\xe2\x9d\xa4\xef\xb8\x8e", 6, MJB_ENC_UTF_8,
        MJB_TERMINAL_WIDTH_NARROW, &sw), MJB_STATUS_OK, "Text variation selector")
    ATT_ASSERT(sw, 1, "Text presentation sequence (❤︎) retains base East Asian Width")

    // Emoji with skin tone modifier: 👋🏽 (U+1F44B WAVING HAND + U+1F3FD EMOJI MODIFIER FITZPATRICK
    // TYPE-4)
    ATT_ASSERT_STATUS(mjb_terminal_width("\xf0\x9f\x91\x8b\xf0\x9f\x8f\xbd", 8, MJB_ENC_UTF_8,
        MJB_TERMINAL_WIDTH_NARROW, &sw), MJB_STATUS_OK, "Emoji with skin tone")
    ATT_ASSERT(sw, 2, "Emoji with skin tone (👋🏽) is one terminal glyph")

    // ZWJ sequence: 👨‍👩‍👧 (U+1F468 MAN + U+200D ZWJ + U+1F469 WOMAN + U+200D ZWJ +
    // U+1F467 GIRL)
    ATT_ASSERT_STATUS(mjb_terminal_width("\xf0\x9f\x91\xa8\xe2\x80\x8d\xf0\x9f\x91\xa9\xe2\x80\x8d"
        "\xf0\x9f\x91\xa7", 18, MJB_ENC_UTF_8, MJB_TERMINAL_WIDTH_NARROW, &sw), MJB_STATUS_OK,
        "Family emoji ZWJ sequence")
    ATT_ASSERT(sw, 2, "Family emoji ZWJ sequence (👨‍👩‍👧)")

    const char *kiss = "\xf0\x9f\x91\xa8\xf0\x9f\x8f\xbb\xe2\x80\x8d\xe2\x9d\xa4\xef\xb8\x8f"
                       "\xe2\x80\x8d\xf0\x9f\x92\x8b\xe2\x80\x8d\xf0\x9f\x91\xa8\xf0\x9f\x8f\xbb";
    ATT_ASSERT_STATUS(mjb_terminal_width(kiss, strlen(kiss), MJB_ENC_UTF_8,
        MJB_TERMINAL_WIDTH_NARROW, &sw), MJB_STATUS_OK, "Ten-scalar kiss emoji")
    ATT_ASSERT(sw, 2, "Ten-scalar kiss emoji occupies two terminal cells")

    ATT_ASSERT_STATUS(mjb_terminal_width("\xc2\xa1", 2, MJB_ENC_UTF_8,
        MJB_TERMINAL_WIDTH_NARROW, &sw), MJB_STATUS_OK, "Ambiguous (narrow profile)")
    ATT_ASSERT(sw, 1, "Ambiguous character is narrow")
    ATT_ASSERT_STATUS(mjb_terminal_width("\xc2\xa1", 2, MJB_ENC_UTF_8,
        MJB_TERMINAL_WIDTH_EAST_ASIAN, &sw), MJB_STATUS_OK, "Ambiguous (East Asian profile)")
    ATT_ASSERT(sw, 2, "Ambiguous character is wide")

    ATT_ASSERT_STATUS(mjb_terminal_width("\xEA\xB0\x80", 3, MJB_ENC_UTF_8,
        MJB_TERMINAL_WIDTH_NARROW, &sw), MJB_STATUS_OK, "NFC Hangul syllable")
    ATT_ASSERT(sw, 2, "NFC Hangul syllable")
    ATT_ASSERT_STATUS(mjb_terminal_width("\xE1\x84\x80\xE1\x85\xA1", 6, MJB_ENC_UTF_8,
        MJB_TERMINAL_WIDTH_NARROW, &sw), MJB_STATUS_OK, "NFD Hangul syllable")
    ATT_ASSERT(sw, 2, "Canonically equivalent Hangul has equal terminal width")

    const char malformed[] = { (char)0xC3, (char)0x28 };
    ATT_ASSERT_STATUS(mjb_terminal_width(malformed, sizeof(malformed), MJB_ENC_UTF_8,
        MJB_TERMINAL_WIDTH_NARROW, &sw), MJB_STATUS_MALFORMED_INPUT, "Malformed UTF-8")

    return 0;
}
