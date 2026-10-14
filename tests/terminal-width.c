/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <string.h>

#include "test.h"

ATT_TEST(terminal_width) {
    {
        size_t width = 9;
        ATT_ASSERT_STATUS(
            mjb_terminal_width(NULL, MJB_MALFORMED_STOP, MJB_TERMINAL_WIDTH_NARROW, &width, NULL),
            MJB_STATUS_INVALID_ARGUMENT, "Terminal width rejects NULL text");
        ATT_ASSERT(width, (size_t)0, "Terminal width clears width after NULL text");
    }

    size_t sw = 0;

    mjb_text text = mjb_text_utf8("A", 1);
    ATT_ASSERT_STATUS(mjb_terminal_width(&text, MJB_MALFORMED_STOP, MJB_TERMINAL_WIDTH_NARROW, NULL, NULL),
        MJB_STATUS_INVALID_ARGUMENT, "NULL width pointer");
    text = mjb_text_utf8(NULL, 1);
    ATT_ASSERT_STATUS(mjb_terminal_width(&text, MJB_MALFORMED_STOP, MJB_TERMINAL_WIDTH_NARROW, &sw, NULL),
        MJB_STATUS_INVALID_ARGUMENT, "NULL terminal buffer");
    text = mjb_text_utf8("A", 1);
    ATT_ASSERT_STATUS(mjb_terminal_width(&text, MJB_MALFORMED_STOP,
        (mjb_terminal_width_profile)2, &sw, NULL), MJB_STATUS_INVALID_ARGUMENT,
        "Invalid terminal-width profile");
    text = mjb_text_make("A", 1, MJB_ENC_UNKNOWN);
    ATT_ASSERT_STATUS(mjb_terminal_width(&text, MJB_MALFORMED_STOP, MJB_TERMINAL_WIDTH_NARROW, &sw, NULL),
        MJB_STATUS_INVALID_ENCODING, "Invalid input encoding");

    text = mjb_text_utf8("", 0);
    ATT_ASSERT_STATUS(mjb_terminal_width(&text, MJB_MALFORMED_STOP, MJB_TERMINAL_WIDTH_NARROW, &sw, NULL),
        MJB_STATUS_OK, "Empty string");
    ATT_ASSERT(sw, 0, "Empty string");

    text = mjb_text_utf8(" ", 1);
    ATT_ASSERT_STATUS(mjb_terminal_width(&text, MJB_MALFORMED_STOP, MJB_TERMINAL_WIDTH_NARROW, &sw, NULL),
        MJB_STATUS_OK, "Space");
    ATT_ASSERT(sw, 1, "Space");

    // Test combining marks (should be zero width)
    text = mjb_text_utf8("e\xCC\x81", 3);
    ATT_ASSERT_STATUS(mjb_terminal_width(&text, MJB_MALFORMED_STOP,
        MJB_TERMINAL_WIDTH_NARROW, &sw, NULL),
        MJB_STATUS_OK, "e + combining acute");
    ATT_ASSERT(sw, 1, "e + combining acute (é)");

    text = mjb_text_utf8("\xCC\x81", 2);
    ATT_ASSERT_STATUS(mjb_terminal_width(&text, MJB_MALFORMED_STOP,
        MJB_TERMINAL_WIDTH_NARROW, &sw, NULL),
        MJB_STATUS_OK, "Combining acute alone");
    ATT_ASSERT(sw, 0, "Combining acute alone");

    // Test zero-width format characters
    text = mjb_text_utf8("\xE2\x80\x8B", 3);
    ATT_ASSERT_STATUS(mjb_terminal_width(&text, MJB_MALFORMED_STOP,
        MJB_TERMINAL_WIDTH_NARROW, &sw, NULL), MJB_STATUS_OK, "Zero-width space");
    ATT_ASSERT(sw, 0, "Zero-width space (U+200B)");

    text = mjb_text_utf8("\xE2\x80\x8C", 3);
    ATT_ASSERT_STATUS(mjb_terminal_width(&text, MJB_MALFORMED_STOP,
        MJB_TERMINAL_WIDTH_NARROW, &sw, NULL), MJB_STATUS_OK, "Zero-width non-joiner");
    ATT_ASSERT(sw, 0, "Zero-width non-joiner (U+200C)");

    text = mjb_text_utf8("\xE2\x80\x8D", 3);
    ATT_ASSERT_STATUS(mjb_terminal_width(&text, MJB_MALFORMED_STOP,
        MJB_TERMINAL_WIDTH_NARROW, &sw, NULL), MJB_STATUS_OK, "Zero-width joiner");
    ATT_ASSERT(sw, 0, "Zero-width joiner (U+200D)");

    text = mjb_text_utf8("\t", 1);
    ATT_ASSERT_STATUS(mjb_terminal_width(&text, MJB_MALFORMED_STOP, MJB_TERMINAL_WIDTH_NARROW, &sw, NULL),
        MJB_STATUS_UNSUPPORTED, "Tab depends on terminal state");
    text = mjb_text_utf8("\n", 1);
    ATT_ASSERT_STATUS(mjb_terminal_width(&text, MJB_MALFORMED_STOP, MJB_TERMINAL_WIDTH_NARROW, &sw, NULL),
        MJB_STATUS_UNSUPPORTED, "Newline is not single-line text");
    text = mjb_text_utf8("\x1B[31m", 5);
    ATT_ASSERT_STATUS(mjb_terminal_width(&text, MJB_MALFORMED_STOP,
        MJB_TERMINAL_WIDTH_NARROW, &sw, NULL), MJB_STATUS_UNSUPPORTED,
        "ANSI escape sequence is not printable text");
    text = mjb_text_utf8("\xE2\x80\xA8", 3);
    ATT_ASSERT_STATUS(mjb_terminal_width(&text, MJB_MALFORMED_STOP,
        MJB_TERMINAL_WIDTH_NARROW, &sw, NULL), MJB_STATUS_UNSUPPORTED,
        "Unicode line separator is not single-line text");
    ATT_ASSERT(sw, 0, "Failed measurement clears the output width");

    // Test wide characters (CJK)
    text = mjb_text_utf8("\xe4\xb8\xad", 3);
    ATT_ASSERT_STATUS(mjb_terminal_width(&text, MJB_MALFORMED_STOP,
        MJB_TERMINAL_WIDTH_NARROW, &sw, NULL), MJB_STATUS_OK, "CJK ideograph");
    ATT_ASSERT(sw, 2, "CJK ideograph (中)");

    text = mjb_text_utf8("\xe4\xb8\xad\xe6\x96\x87", 6);
    ATT_ASSERT_STATUS(mjb_terminal_width(&text, MJB_MALFORMED_STOP,
        MJB_TERMINAL_WIDTH_NARROW, &sw, NULL), MJB_STATUS_OK, "Two CJK ideographs");
    ATT_ASSERT(sw, 4, "Two CJK ideographs (中文)");

    // Test full-width characters
    text = mjb_text_utf8("\xef\xbc\xa1", 3);
    ATT_ASSERT_STATUS(mjb_terminal_width(&text, MJB_MALFORMED_STOP,
        MJB_TERMINAL_WIDTH_NARROW, &sw, NULL), MJB_STATUS_OK, "Full-width A");
    ATT_ASSERT(sw, 2, "Full-width A (Ａ)");

    // Test mixed strings
    text = mjb_text_utf8("Hello", 5);
    ATT_ASSERT_STATUS(mjb_terminal_width(&text, MJB_MALFORMED_STOP,
        MJB_TERMINAL_WIDTH_NARROW, &sw, NULL),
        MJB_STATUS_OK, "ASCII string");
    ATT_ASSERT(sw, 5, "ASCII string (Hello)");
    text = mjb_text_utf8("Hello", MJB_NUL_TERMINATED);
    ATT_ASSERT_STATUS(mjb_terminal_width(&text, MJB_MALFORMED_STOP,
        MJB_TERMINAL_WIDTH_NARROW, &sw, NULL), MJB_STATUS_OK, "NUL-terminated UTF-8");
    ATT_ASSERT(sw, 5, "NUL-terminated UTF-8 string");

    const char utf16le[] = { 0x41, 0x00, 0x4C, 0x75 };
    text = mjb_text_make(utf16le, sizeof(utf16le), MJB_ENC_UTF_16LE);
    ATT_ASSERT_STATUS(mjb_terminal_width(&text, MJB_MALFORMED_STOP,
        MJB_TERMINAL_WIDTH_NARROW, &sw, NULL), MJB_STATUS_OK, "UTF-16LE input");
    ATT_ASSERT(sw, 3, "UTF-16LE A + CJK");

    const char utf32be[] = { 0x00, 0x00, 0x00, 0x41, 0x00, 0x00, 0x75, 0x4C };
    text = mjb_text_make(utf32be, sizeof(utf32be), MJB_ENC_UTF_32BE);
    ATT_ASSERT_STATUS(mjb_terminal_width(&text, MJB_MALFORMED_STOP,
        MJB_TERMINAL_WIDTH_NARROW, &sw, NULL), MJB_STATUS_OK, "UTF-32BE input");
    ATT_ASSERT(sw, 3, "UTF-32BE A + CJK");

    text = mjb_text_utf8("Hello\xe4\xb8\xad", 8);
    ATT_ASSERT_STATUS(mjb_terminal_width(&text, MJB_MALFORMED_STOP,
                          MJB_TERMINAL_WIDTH_NARROW, &sw, NULL),
        MJB_STATUS_OK, "ASCII + CJK");
    ATT_ASSERT(sw, 7, "ASCII + CJK (Hello中)");

    text = mjb_text_utf8("e\xcc\x81\xe4\xb8\xad", 6);
    ATT_ASSERT_STATUS(mjb_terminal_width(&text, MJB_MALFORMED_STOP,
        MJB_TERMINAL_WIDTH_NARROW, &sw, NULL), MJB_STATUS_OK, "Combining + CJK");
    ATT_ASSERT(sw, 3, "Combining + CJK (é中)");

    // Test complex emojis
    // Simple emoji: 👍 (U+1F44D THUMBS UP SIGN) - EAW=Wide
    text = mjb_text_utf8("\xf0\x9f\x91\x8d", 4);
    ATT_ASSERT_STATUS(mjb_terminal_width(&text, MJB_MALFORMED_STOP,
        MJB_TERMINAL_WIDTH_NARROW, &sw, NULL), MJB_STATUS_OK, "Simple emoji");
    ATT_ASSERT(sw, 2, "Simple emoji (👍)");

    // Emoji with variation selector: ❤️ (U+2764 HEAVY BLACK HEART + U+FE0F VARIATION SELECTOR-16)
    text = mjb_text_utf8("\xe2\x9d\xa4\xef\xb8\x8f", 6);
    ATT_ASSERT_STATUS(mjb_terminal_width(&text, MJB_MALFORMED_STOP,
        MJB_TERMINAL_WIDTH_NARROW, &sw, NULL), MJB_STATUS_OK, "Emoji with variation selector");
    ATT_ASSERT(sw, 2, "Emoji presentation sequence (❤️)");

    text = mjb_text_utf8("\xe2\x9d\xa4\xef\xb8\x8e", 6);
    ATT_ASSERT_STATUS(mjb_terminal_width(&text, MJB_MALFORMED_STOP,
        MJB_TERMINAL_WIDTH_NARROW, &sw, NULL), MJB_STATUS_OK, "Text variation selector");
    ATT_ASSERT(sw, 1, "Text presentation sequence (❤︎) retains base East Asian Width");

    // Emoji with skin tone modifier: 👋🏽 (U+1F44B WAVING HAND + U+1F3FD EMOJI MODIFIER FITZPATRICK
    // TYPE-4)
    text = mjb_text_utf8("\xf0\x9f\x91\x8b\xf0\x9f\x8f\xbd", 8);
    ATT_ASSERT_STATUS(mjb_terminal_width(&text, MJB_MALFORMED_STOP,
        MJB_TERMINAL_WIDTH_NARROW, &sw, NULL), MJB_STATUS_OK, "Emoji with skin tone");
    ATT_ASSERT(sw, 2, "Emoji with skin tone (👋🏽) is one terminal glyph");

    // ZWJ sequence: 👨‍👩‍👧 (U+1F468 MAN + U+200D ZWJ + U+1F469 WOMAN + U+200D ZWJ +
    // U+1F467 GIRL)
    text = mjb_text_utf8("\xf0\x9f\x91\xa8\xe2\x80\x8d\xf0\x9f\x91\xa9\xe2\x80\x8d"
                         "\xf0\x9f\x91\xa7",
        18);
    ATT_ASSERT_STATUS(mjb_terminal_width(&text, MJB_MALFORMED_STOP, MJB_TERMINAL_WIDTH_NARROW, &sw, NULL), MJB_STATUS_OK,
        "Family emoji ZWJ sequence");
    ATT_ASSERT(sw, 2, "Family emoji ZWJ sequence (👨‍👩‍👧)");

    const char *kiss = "\xf0\x9f\x91\xa8\xf0\x9f\x8f\xbb\xe2\x80\x8d\xe2\x9d\xa4\xef\xb8\x8f"
                       "\xe2\x80\x8d\xf0\x9f\x92\x8b\xe2\x80\x8d\xf0\x9f\x91\xa8\xf0\x9f\x8f\xbb";
    text = mjb_text_utf8(kiss, strlen(kiss));
    ATT_ASSERT_STATUS(mjb_terminal_width(&text, MJB_MALFORMED_STOP,
        MJB_TERMINAL_WIDTH_NARROW, &sw, NULL), MJB_STATUS_OK, "Ten-scalar kiss emoji");
    ATT_ASSERT(sw, 2, "Ten-scalar kiss emoji occupies two terminal cells");

    text = mjb_text_utf8("\xc2\xa1", 2);
    ATT_ASSERT_STATUS(mjb_terminal_width(&text, MJB_MALFORMED_STOP,
        MJB_TERMINAL_WIDTH_NARROW, &sw, NULL), MJB_STATUS_OK, "Ambiguous (narrow profile)");
    ATT_ASSERT(sw, 1, "Ambiguous character is narrow");
    text = mjb_text_utf8("\xc2\xa1", 2);
    ATT_ASSERT_STATUS(mjb_terminal_width(&text, MJB_MALFORMED_STOP,
        MJB_TERMINAL_WIDTH_EAST_ASIAN, &sw, NULL), MJB_STATUS_OK, "Ambiguous (East Asian profile)");
    ATT_ASSERT(sw, 2, "Ambiguous character is wide");

    text = mjb_text_utf8("\xEA\xB0\x80", 3);
    ATT_ASSERT_STATUS(mjb_terminal_width(&text, MJB_MALFORMED_STOP,
        MJB_TERMINAL_WIDTH_NARROW, &sw, NULL), MJB_STATUS_OK, "NFC Hangul syllable");
    ATT_ASSERT(sw, 2, "NFC Hangul syllable");
    text = mjb_text_utf8("\xE1\x84\x80\xE1\x85\xA1", 6);
    ATT_ASSERT_STATUS(mjb_terminal_width(&text, MJB_MALFORMED_STOP,
        MJB_TERMINAL_WIDTH_NARROW, &sw, NULL), MJB_STATUS_OK, "NFD Hangul syllable");
    ATT_ASSERT(sw, 2, "Canonically equivalent Hangul has equal terminal width");

    const unsigned char malformed[] = { 0xC3, 0x28 };
    mjb_diagnostic diagnostic;
    text = mjb_text_utf8((const char *)malformed, sizeof(malformed));
    ATT_ASSERT_STATUS(mjb_terminal_width(&text,
                          MJB_MALFORMED_STOP, MJB_TERMINAL_WIDTH_NARROW, &sw, &diagnostic),
        MJB_STATUS_MALFORMED_INPUT, "Malformed UTF-8");
    ATT_ASSERT(diagnostic.byte_offset, (size_t)0, "Terminal width malformed offset");

    const unsigned char recoverable_width[] = { 'A', 0x80, 0xE4, 0xB8, 0xAD };
    text = mjb_text_utf8((const char *)recoverable_width, sizeof(recoverable_width));
    ATT_ASSERT_STATUS(mjb_terminal_width(&text, MJB_MALFORMED_REPLACE, MJB_TERMINAL_WIDTH_NARROW, &sw,
                          &diagnostic),
        MJB_STATUS_OK, "Terminal width replaces malformed input");
    ATT_ASSERT(sw, (size_t)4, "Terminal width counts replacement glyph");
    ATT_ASSERT(diagnostic.byte_offset, (size_t)1,
        "Terminal width replacement retains malformed offset");

    text = mjb_text_utf8((const char *)recoverable_width, sizeof(recoverable_width));
    ATT_ASSERT_STATUS(mjb_terminal_width(&text, MJB_MALFORMED_SKIP, MJB_TERMINAL_WIDTH_NARROW, &sw,
                          &diagnostic),
        MJB_STATUS_OK, "Terminal width skips malformed input");
    ATT_ASSERT(sw, (size_t)3, "Terminal width omits skipped malformed input");
}
