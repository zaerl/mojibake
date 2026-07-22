/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>

#include "test.h"

/**
 * Test mjb_count_codepoints behavior with embedded NULL codepoints (U+0000).
 */
int test_embedded_null(void *arg) {
    // Explicit lengths always include U+0000 codepoints.
    const char utf8_with_nulls[] = { 'A', '\0', 'B', '\0', 'C' };
    ATT_ASSERT(mjb_count_codepoints(utf8_with_nulls, 5, MJB_ENC_UTF_8), 5,
        "UTF-8: A\\0B\\0C = 5 codepoints")
    ATT_ASSERT(mjb_count_codepoints(utf8_with_nulls, MJB_NUL_TERMINATED, MJB_ENC_UTF_8), 1,
        "UTF-8: NUL-terminated A\\0B\\0C = 1 codepoint")

    const char utf8_hello_null_world[] = { 'H', 'e', 'l', 'l', 'o', '\0', 'W', 'o', 'r', 'l', 'd' };
    ATT_ASSERT(mjb_count_codepoints(utf8_hello_null_world, 11, MJB_ENC_UTF_8), 11,
        "UTF-8: Hello\\0World = 11 codepoints")

    const char utf8_only_nulls[] = { '\0', '\0', '\0' };
    ATT_ASSERT(mjb_count_codepoints(utf8_only_nulls, 3, MJB_ENC_UTF_8), 3,
        "UTF-8: \\0\\0\\0 = 3 codepoints")
    ATT_ASSERT(mjb_count_codepoints(utf8_only_nulls, MJB_NUL_TERMINATED, MJB_ENC_UTF_8), 0,
        "UTF-8: NUL-terminated \\0\\0\\0 is empty")

    const char utf8_null_prefix[] = { '\0', 'A', 'B', 'C' };
    ATT_ASSERT(mjb_count_codepoints(utf8_null_prefix, 4, MJB_ENC_UTF_8), 4,
        "UTF-8: \\0ABC = 4 codepoints")

    const char utf8_multibyte_nulls[] = {
        '\xC3', '\xA9',         // é
        '\0',                   // U+0000
        '\xE3', '\x83', '\x84', // ツ
        '\0'                    // U+0000
    };
    ATT_ASSERT(mjb_count_codepoints(utf8_multibyte_nulls, 7, MJB_ENC_UTF_8), 4,
        "UTF-8: é\\0ツ\\0 = 4 codepoints")

    const char utf16le_with_null[] = {
        'A', '\0',  // U+0041 'A'
        '\0', '\0', // U+0000
        'B', '\0'   // U+0042 'B'
    };
    ATT_ASSERT(mjb_count_codepoints(utf16le_with_null, 6, MJB_ENC_UTF_16LE), 3,
        "UTF-16LE: A\\0B = 3 codepoints")
    ATT_ASSERT(mjb_count_codepoints(utf16le_with_null, MJB_NUL_TERMINATED, MJB_ENC_UTF_16LE), 1,
        "UTF-16LE: NUL-terminated A\\0B = 1 codepoint")

    const char utf16le_hello_null_exclaim[] = {
        'H', '\0', 'e', '\0', 'l', '\0', 'l', '\0', 'o', '\0', // Hello
        '\0', '\0',                                            // U+0000
        '!', '\0'                                              // !
    };
    ATT_ASSERT(mjb_count_codepoints(utf16le_hello_null_exclaim, 14, MJB_ENC_UTF_16LE), 7,
        "UTF-16LE: Hello\\0! = 7 codepoints")

    const char utf16le_multiple_nulls[] = {
        'X', '\0',  // U+0058 'X'
        '\0', '\0', // U+0000
        '\0', '\0', // U+0000
        'Y', '\0'   // U+0059 'Y'
    };
    ATT_ASSERT(mjb_count_codepoints(utf16le_multiple_nulls, 8, MJB_ENC_UTF_16LE), 4,
        "UTF-16LE: X\\0\\0Y = 4 codepoints")

    const char utf16be_with_null[] = {
        '\0', 'A',  // U+0041 'A'
        '\0', '\0', // U+0000
        '\0', 'B'   // U+0042 'B'
    };
    ATT_ASSERT(mjb_count_codepoints(utf16be_with_null, 6, MJB_ENC_UTF_16BE), 3,
        "UTF-16BE: A\\0B = 3 codepoints")
    ATT_ASSERT(mjb_count_codepoints(utf16be_with_null, MJB_NUL_TERMINATED, MJB_ENC_UTF_16BE), 1,
        "UTF-16BE: NUL-terminated A\\0B = 1 codepoint")

    const char utf16_with_bom[] = { '\xFF', '\xFE', 'A', '\0', '\0', '\0' };
    ATT_ASSERT(mjb_count_codepoints(utf16_with_bom, MJB_NUL_TERMINATED, MJB_ENC_UTF_16), 1,
        "Generic UTF-16: NUL termination is resolved before BOM decoding")

    const char utf16be_test_null_data[] = {
        '\0', 'T', '\0', 'e', '\0', 's', '\0', 't', // Test
        '\0', '\0',                                 // U+0000
        '\0', 'D', '\0', 'a', '\0', 't', '\0', 'a'  // Data
    };
    ATT_ASSERT(mjb_count_codepoints(utf16be_test_null_data, 18, MJB_ENC_UTF_16BE), 9,
        "UTF-16BE: Test\\0Data = 9 codepoints")

    const char utf32le_with_null[] = {
        'A', '\0', '\0', '\0',  // U+00000041 'A'
        '\0', '\0', '\0', '\0', // U+00000000
        'B', '\0', '\0', '\0'   // U+00000042 'B'
    };
    ATT_ASSERT(mjb_count_codepoints(utf32le_with_null, 12, MJB_ENC_UTF_32LE), 3,
        "UTF-32LE: A\\0B = 3 codepoints")
    ATT_ASSERT(mjb_count_codepoints(utf32le_with_null, MJB_NUL_TERMINATED, MJB_ENC_UTF_32LE), 1,
        "UTF-32LE: NUL-terminated A\\0B = 1 codepoint")

    const char utf32le_digits_nulls[] = {
        '1', '\0', '\0', '\0',  // U+00000031 '1'
        '\0', '\0', '\0', '\0', // U+00000000
        '\0', '\0', '\0', '\0', // U+00000000
        '2', '\0', '\0', '\0',  // U+00000032 '2'
        '\0', '\0', '\0', '\0', // U+00000000
        '\0', '\0', '\0', '\0', // U+00000000
        '3', '\0', '\0', '\0'   // U+00000033 '3'
    };
    ATT_ASSERT(mjb_count_codepoints(utf32le_digits_nulls, 28, MJB_ENC_UTF_32LE), 7,
        "UTF-32LE: 1\\0\\02\\0\\03 = 7 codepoints")

    const char utf32be_with_null[] = {
        '\0', '\0', '\0', 'A',  // U+00000041 'A'
        '\0', '\0', '\0', '\0', // U+00000000
        '\0', '\0', '\0', 'B'   // U+00000042 'B'
    };
    ATT_ASSERT(mjb_count_codepoints(utf32be_with_null, 12, MJB_ENC_UTF_32BE), 3,
        "UTF-32BE: A\\0B = 3 codepoints")
    ATT_ASSERT(mjb_count_codepoints(utf32be_with_null, MJB_NUL_TERMINATED, MJB_ENC_UTF_32BE), 1,
        "UTF-32BE: NUL-terminated A\\0B = 1 codepoint")

    const char utf32_with_bom[] = { '\xFF', '\xFE', '\0', '\0', 'A', '\0', '\0', '\0', '\0', '\0',
        '\0', '\0' };
    ATT_ASSERT(mjb_count_codepoints(utf32_with_bom, MJB_NUL_TERMINATED, MJB_ENC_UTF_32), 1,
        "Generic UTF-32: NUL termination is resolved before BOM decoding")

    const char utf32be_emoji_null[] = {
        '\0', '\x01', '\xF9', '\x1D', // U+0001F91D 🤝 handshake
        '\0', '\0', '\0', '\0',       // U+00000000
        '\0', '\x01', '\xF3', '\x0D'  // U+0001F30D 🌍 world globe
    };
    ATT_ASSERT(mjb_count_codepoints(utf32be_emoji_null, 12, MJB_ENC_UTF_32BE), 3,
        "UTF-32BE: 🤝\\0🌍 = 3 codepoints")

    const char utf8_boundary[] = { 'A', '\0', 'B' };
    ATT_ASSERT(mjb_count_codepoints(utf8_boundary, 2, MJB_ENC_UTF_8), 2,
        "UTF-8: A\\0 = 2 codepoints")

    const char utf8_empty_then_data[] = { '\0', 'A', 'B', 'C' };
    ATT_ASSERT(mjb_count_codepoints(utf8_empty_then_data, 4, MJB_ENC_UTF_8), 4,
        "UTF-8: \\0ABC = 4 codepoints")

    // The normalization fast path must report the resolved payload length, not SIZE_MAX.
    const char normalized[] = { 'A', '\0', 'B', '\0' };
    mjb_result result;
    ATT_ASSERT_STATUS(mjb_normalize(normalized, sizeof(normalized) - 1, MJB_ENC_UTF_8,
                          MJB_NORMALIZATION_NFC, MJB_ENC_UTF_8, &result),
        MJB_STATUS_OK, "Normalization with explicit embedded U+0000")
    ATT_ASSERT(result.output_size, sizeof(normalized) - 1,
        "Explicit normalization preserves bytes after U+0000")
    ATT_ASSERT_STATUS(mjb_result_free(&result), MJB_STATUS_OK,
        "Free explicit normalization result")

    ATT_ASSERT_STATUS(mjb_normalize(normalized, MJB_NUL_TERMINATED, MJB_ENC_UTF_8,
                          MJB_NORMALIZATION_NFC, MJB_ENC_UTF_8, &result),
        MJB_STATUS_OK, "Normalization with MJB_NUL_TERMINATED")
    ATT_ASSERT(result.output_size, (size_t)1,
        "NUL-terminated normalization excludes the terminator and suffix")
    ATT_ASSERT_STATUS(mjb_result_free(&result), MJB_STATUS_OK,
        "Free NUL-terminated normalization result")

    ATT_ASSERT((unsigned int)mjb_detect_encoding("A", MJB_NUL_TERMINATED),
        (unsigned int)MJB_ENC_UNKNOWN, "Encoding detection requires an explicit byte length")

    mjb_next_state state;
    state.index = 0;
    ATT_ASSERT((uint8_t)mjb_next_grapheme_break("A", MJB_NUL_TERMINATED, MJB_ENC_UTF_8, &state),
        (uint8_t)MJB_BT_NOT_SET, "Stateful segmentation requires an explicit byte length")

    return 0;
}
