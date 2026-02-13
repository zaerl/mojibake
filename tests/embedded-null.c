/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>

#include "test.h"

/**
 * Test mjb_strnlen behavior with embedded NULL codepoints (U+0000).
 */
void *test_embedded_null(void *arg) {
    // UTF-8: NULL codepoint is single byte 0x00
    // String: "A\0B\0C" (3 visible chars + 2 embedded NULLs = 5 codepoints)
    const char utf8_with_nulls[] = { 'A', '\0', 'B', '\0', 'C' };
#ifdef MJB_DANGEROUSLY_ALLOW_EMBEDDED_NULLS
    ATT_ASSERT(mjb_strnlen(utf8_with_nulls, 5, MJB_ENCODING_UTF_8), 5,
        "UTF-8: A\\0B\\0C = 5 codepoints (NULL as data)")
#else
    ATT_ASSERT(mjb_strnlen(utf8_with_nulls, 5, MJB_ENCODING_UTF_8), 1,
        "UTF-8: A\\0B\\0C = 1 codepoint (stops at NULL)")
#endif

    // String: "Hello\0World" (5 + 1 + 5 = 11 codepoints)
    const char utf8_hello_null_world[] = { 'H', 'e', 'l', 'l', 'o', '\0', 'W', 'o', 'r', 'l', 'd' };
#ifdef MJB_DANGEROUSLY_ALLOW_EMBEDDED_NULLS
    ATT_ASSERT(mjb_strnlen(utf8_hello_null_world, 11, MJB_ENCODING_UTF_8), 11,
        "UTF-8: Hello\\0World = 11 codepoints (with macro)")
#else
    ATT_ASSERT(mjb_strnlen(utf8_hello_null_world, 11, MJB_ENCODING_UTF_8), 5,
        "UTF-8: Hello\\0World = 5 codepoints (stops at NULL)")
#endif

    // String with only NULLs: "\0\0\0"
    const char utf8_only_nulls[] = { '\0', '\0', '\0' };
#ifdef MJB_DANGEROUSLY_ALLOW_EMBEDDED_NULLS
    ATT_ASSERT(mjb_strnlen(utf8_only_nulls, 3, MJB_ENCODING_UTF_8), 3,
        "UTF-8: \\0\\0\\0 = 3 codepoints (with macro)")
#else
    ATT_ASSERT(mjb_strnlen(utf8_only_nulls, 3, MJB_ENCODING_UTF_8), 0,
        "UTF-8: \\0\\0\\0 = 0 codepoints (starts with NULL)")
#endif

    // String starting with NULL: "\0ABC"
    const char utf8_null_prefix[] = { '\0', 'A', 'B', 'C' };
#ifdef MJB_DANGEROUSLY_ALLOW_EMBEDDED_NULLS
    ATT_ASSERT(mjb_strnlen(utf8_null_prefix, 4, MJB_ENCODING_UTF_8), 4,
        "UTF-8: \\0ABC = 4 codepoints (with macro)")
#else
    ATT_ASSERT(mjb_strnlen(utf8_null_prefix, 4, MJB_ENCODING_UTF_8), 0,
        "UTF-8: \\0ABC = 0 codepoints (starts with NULL)")
#endif

    // String with multibyte chars and embedded NULLs: "é\0ツ\0"
    // 'é' is 0xC3 0xA9 (2 bytes), NULL is 0x00, 'ツ' is 0xE3 0x83 0x84 (3 bytes)
    const char utf8_multibyte_nulls[] = {
        '\xC3', '\xA9',         // é
        '\0',                   // NULL
        '\xE3', '\x83', '\x84', // ツ
        '\0'                    // NULL
    };
#ifdef MJB_DANGEROUSLY_ALLOW_EMBEDDED_NULLS
    ATT_ASSERT(mjb_strnlen(utf8_multibyte_nulls, 7, MJB_ENCODING_UTF_8), 4,
        "UTF-8: é\\0ツ\\0 = 4 codepoints (with macro)")
#else
    ATT_ASSERT(mjb_strnlen(utf8_multibyte_nulls, 7, MJB_ENCODING_UTF_8), 1,
        "UTF-8: é\\0ツ\\0 = 1 codepoint (stops after é)")
#endif

    // UTF-16LE: NULL codepoint is 0x00 0x00
    // String: "A\0B" in UTF-16LE = 'A' 0x00, NULL 0x00 0x00, 'B' 0x00
    const char utf16le_with_null[] = {
        'A', '\0',  // U+0041 'A'
        '\0', '\0', // U+0000 NULL
        'B', '\0'   // U+0042 'B'
    };
#ifdef MJB_DANGEROUSLY_ALLOW_EMBEDDED_NULLS
    ATT_ASSERT(mjb_strnlen(utf16le_with_null, 6, MJB_ENCODING_UTF_16_LE), 3,
        "UTF-16LE: A\\0B = 3 codepoints (with macro)")
#else
    ATT_ASSERT(mjb_strnlen(utf16le_with_null, 6, MJB_ENCODING_UTF_16_LE), 1,
        "UTF-16LE: A\\0B = 1 codepoint (stops at NULL)")
#endif

    // String: "Hello\0!" in UTF-16LE
    const char utf16le_hello_null_exclaim[] = {
        'H', '\0', 'e', '\0', 'l', '\0', 'l', '\0', 'o', '\0', // Hello
        '\0', '\0', // NULL
        '!', '\0'   // !
    };
#ifdef MJB_DANGEROUSLY_ALLOW_EMBEDDED_NULLS
    ATT_ASSERT(mjb_strnlen(utf16le_hello_null_exclaim, 14, MJB_ENCODING_UTF_16_LE), 7,
        "UTF-16LE: Hello\\0! = 7 codepoints (with macro)")
#else
    ATT_ASSERT(mjb_strnlen(utf16le_hello_null_exclaim, 14, MJB_ENCODING_UTF_16_LE), 5,
        "UTF-16LE: Hello\\0! = 5 codepoints (stops at NULL)")
#endif

    // Multiple consecutive NULLs in UTF-16LE
    const char utf16le_multiple_nulls[] = {
        'X', '\0',  // U+0058 'X'
        '\0', '\0', // U+0000 NULL
        '\0', '\0', // U+0000 NULL
        'Y', '\0'   // U+0059 'Y'
    };
#ifdef MJB_DANGEROUSLY_ALLOW_EMBEDDED_NULLS
    ATT_ASSERT(mjb_strnlen(utf16le_multiple_nulls, 8, MJB_ENCODING_UTF_16_LE), 4,
        "UTF-16LE: X\\0\\0Y = 4 codepoints (with macro)")
#else
    ATT_ASSERT(mjb_strnlen(utf16le_multiple_nulls, 8, MJB_ENCODING_UTF_16_LE), 1,
        "UTF-16LE: X\\0\\0Y = 1 codepoint (stops at first NULL)")
#endif

    // UTF-16BE: NULL codepoint is 0x00 0x00
    // String: "A\0B" in UTF-16BE
    const char utf16be_with_null[] = {
        '\0', 'A',  // U+0041 'A'
        '\0', '\0', // U+0000 NULL
        '\0', 'B'   // U+0042 'B'
    };
#ifdef MJB_DANGEROUSLY_ALLOW_EMBEDDED_NULLS
    ATT_ASSERT(mjb_strnlen(utf16be_with_null, 6, MJB_ENCODING_UTF_16_BE), 3,
        "UTF-16BE: A\\0B = 3 codepoints (with macro)")
#else
    ATT_ASSERT(mjb_strnlen(utf16be_with_null, 6, MJB_ENCODING_UTF_16_BE), 1,
        "UTF-16BE: A\\0B = 1 codepoint (stops at NULL)")
#endif

    // String: "Test\0Data" in UTF-16BE
    const char utf16be_test_null_data[] = {
        '\0', 'T', '\0', 'e', '\0', 's', '\0', 't', // Test
        '\0', '\0', // NULL
        '\0', 'D', '\0', 'a', '\0', 't', '\0', 'a' // Data
    };
#ifdef MJB_DANGEROUSLY_ALLOW_EMBEDDED_NULLS
    ATT_ASSERT(mjb_strnlen(utf16be_test_null_data, 18, MJB_ENCODING_UTF_16_BE), 9,
        "UTF-16BE: Test\\0Data = 9 codepoints (with macro)")
#else
    ATT_ASSERT(mjb_strnlen(utf16be_test_null_data, 18, MJB_ENCODING_UTF_16_BE), 4,
        "UTF-16BE: Test\\0Data = 4 codepoints (stops at NULL)")
#endif

    // UTF-32LE: NULL codepoint is 0x00 0x00 0x00 0x00
    // String: "A\0B" in UTF-32LE
    const char utf32le_with_null[] = {
        'A', '\0', '\0', '\0',  // U+00000041 'A'
        '\0', '\0', '\0', '\0', // U+00000000 NULL
        'B', '\0', '\0', '\0'   // U+00000042 'B'
    };
#ifdef MJB_DANGEROUSLY_ALLOW_EMBEDDED_NULLS
    ATT_ASSERT(mjb_strnlen(utf32le_with_null, 12, MJB_ENCODING_UTF_32_LE), 3,
        "UTF-32LE: A\\0B = 3 codepoints (with macro)")
#else
    ATT_ASSERT(mjb_strnlen(utf32le_with_null, 12, MJB_ENCODING_UTF_32_LE), 1,
        "UTF-32LE: A\\0B = 1 codepoint (stops at NULL)")
#endif

    // String: "1\0\02\0\03" in UTF-32LE (multiple NULLs)
    const char utf32le_digits_nulls[] = {
        '1', '\0', '\0', '\0',  // U+00000031 '1'
        '\0', '\0', '\0', '\0', // U+00000000 NULL
        '\0', '\0', '\0', '\0', // U+00000000 NULL
        '2', '\0', '\0', '\0',  // U+00000032 '2'
        '\0', '\0', '\0', '\0', // U+00000000 NULL
        '\0', '\0', '\0', '\0', // U+00000000 NULL
        '3', '\0', '\0', '\0'   // U+00000033 '3'
    };
#ifdef MJB_DANGEROUSLY_ALLOW_EMBEDDED_NULLS
    ATT_ASSERT(mjb_strnlen(utf32le_digits_nulls, 28, MJB_ENCODING_UTF_32_LE), 7,
        "UTF-32LE: 1\\0\\02\\0\\03 = 7 codepoints (with macro)")
#else
    ATT_ASSERT(mjb_strnlen(utf32le_digits_nulls, 28, MJB_ENCODING_UTF_32_LE), 1,
        "UTF-32LE: 1\\0\\02\\0\\03 = 1 codepoint (stops at first NULL)")
#endif

    // UTF-32BE: NULL codepoint is 0x00 0x00 0x00 0x00
    // String: "A\0B" in UTF-32BE
    const char utf32be_with_null[] = {
        '\0', '\0', '\0', 'A',  // U+00000041 'A'
        '\0', '\0', '\0', '\0', // U+00000000 NULL
        '\0', '\0', '\0', 'B'   // U+00000042 'B'
    };
#ifdef MJB_DANGEROUSLY_ALLOW_EMBEDDED_NULLS
    ATT_ASSERT(mjb_strnlen(utf32be_with_null, 12, MJB_ENCODING_UTF_32_BE), 3,
        "UTF-32BE: A\\0B = 3 codepoints (with macro)")
#else
    ATT_ASSERT(mjb_strnlen(utf32be_with_null, 12, MJB_ENCODING_UTF_32_BE), 1,
        "UTF-32BE: A\\0B = 1 codepoint (stops at NULL)")
#endif

    // String with emoji and NULL: "🤝\0🌍" (handshake, NULL, world) in UTF-32BE
    const char utf32be_emoji_null[] = {
        '\0', '\x01', '\xF9', '\x1D', // U+0001F91D 🤝 handshake
        '\0', '\0', '\0', '\0',       // U+00000000 NULL
        '\0', '\x01', '\xF3', '\x0D'  // U+0001F30D 🌍 world globe
    };
#ifdef MJB_DANGEROUSLY_ALLOW_EMBEDDED_NULLS
    ATT_ASSERT(mjb_strnlen(utf32be_emoji_null, 12, MJB_ENCODING_UTF_32_BE), 3,
        "UTF-32BE: 🤝\\0🌍 = 3 codepoints (with macro)")
#else
    ATT_ASSERT(mjb_strnlen(utf32be_emoji_null, 12, MJB_ENCODING_UTF_32_BE), 1,
        "UTF-32BE: 🤝\\0🌍 = 1 codepoint (stops at NULL)")
#endif

    // Edge case: Buffer size exactly at NULL codepoint boundary
    const char utf8_boundary[] = { 'A', '\0', 'B' };
#ifdef MJB_DANGEROUSLY_ALLOW_EMBEDDED_NULLS
    ATT_ASSERT(mjb_strnlen(utf8_boundary, 2, MJB_ENCODING_UTF_8), 2,
        "UTF-8: A\\0 = 2 codepoints (with macro)")
#else
    ATT_ASSERT(mjb_strnlen(utf8_boundary, 2, MJB_ENCODING_UTF_8), 1,
        "UTF-8: A\\0 = 1 codepoint (stops at NULL)")
#endif

    // Edge case: Empty string followed by data
    const char utf8_empty_then_data[] = { '\0', 'A', 'B', 'C' };
#ifdef MJB_DANGEROUSLY_ALLOW_EMBEDDED_NULLS
    ATT_ASSERT(mjb_strnlen(utf8_empty_then_data, 4, MJB_ENCODING_UTF_8), 4,
        "UTF-8: \\0ABC = 4 codepoints (with macro)")
#else
    ATT_ASSERT(mjb_strnlen(utf8_empty_then_data, 4, MJB_ENCODING_UTF_8), 0,
        "UTF-8: \\0ABC = 0 codepoints (starts with NULL)")
#endif

    return NULL;
}
