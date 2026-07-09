/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"
#include "../src/utf.h"

int test_utf(void *arg) {
    // Test scanning single ASCII character "A"
    const char *buffer = "A";
    size_t size = 1;
    uint8_t state = MJB_UTF_ACCEPT;
    size_t index = 0;
    bool in_error = false;
    mjb_codepoint codepoint = 0;
    mjb_decode_result result;

    result = mjb_next_codepoint(buffer, size, &state, &index, MJB_ENC_UTF_8,
        &codepoint, &in_error);

    ATT_ASSERT((int)result, (int)MJB_DECODE_OK, "MJB_DECODE_OK")
    ATT_ASSERT(codepoint, 0x41, "0x41 (A)")
    ATT_ASSERT((int)state, (int)MJB_UTF_ACCEPT, "MJB_UTF_ACCEPT")
    ATT_ASSERT(index, 1, "Index 1")
    ATT_ASSERT(in_error, false, "Should not be in error state")

    result = mjb_next_codepoint(buffer, size, &state, &index, MJB_ENC_UTF_8,
        &codepoint, &in_error);

    ATT_ASSERT((int)result, (int)MJB_DECODE_END, "MJB_DECODE_END")
    ATT_ASSERT((int)state, (int)MJB_UTF_ACCEPT, "MJB_UTF_ACCEPT")
    ATT_ASSERT(index, 1, "Index 1")

    const char *buffer_utf16be = "\x00\x41";
    size_t size_utf16be = 2;

#define RESET_STATE() \
    state = MJB_UTF_ACCEPT; \
    index = 0; \
    in_error = false; \
    codepoint = 0;

    RESET_STATE()

    result = mjb_next_codepoint(buffer_utf16be, size_utf16be, &state, &index, MJB_ENC_UTF_16BE,
        &codepoint, &in_error);

    ATT_ASSERT((int)result, (int)MJB_DECODE_OK, "UTF-16BE: MJB_DECODE_OK")
    ATT_ASSERT(codepoint, 0x41, "UTF-16BE: 0x41 (A)")
    ATT_ASSERT((int)state, (int)MJB_UTF_ACCEPT, "UTF-16BE: MJB_UTF_ACCEPT")
    ATT_ASSERT(index, 2, "UTF-16BE: Index 2")
    ATT_ASSERT(in_error, false, "UTF-16BE: not error state")

    result = mjb_next_codepoint(buffer_utf16be, size_utf16be, &state, &index, MJB_ENC_UTF_16BE,
        &codepoint, &in_error);

    ATT_ASSERT((int)result, (int)MJB_DECODE_END, "UTF-16BE: MJB_DECODE_END")
    ATT_ASSERT((int)state, (int)MJB_UTF_ACCEPT, "UTF-16BE: MJB_UTF_ACCEPT")
    ATT_ASSERT(index, 2, "UTF-16BE: index 2")

    const char *buffer_utf16le = "\x41\x00";
    size_t size_utf16le = 2;

    RESET_STATE()

    result = mjb_next_codepoint(buffer_utf16le, size_utf16le, &state, &index, MJB_ENC_UTF_16LE,
        &codepoint, &in_error);

    ATT_ASSERT((int)result, (int)MJB_DECODE_OK, "UTF-16LE: MJB_DECODE_OK")
    ATT_ASSERT(codepoint, 0x41, "UTF-16LE: 0x41 (A)")
    ATT_ASSERT((int)state, (int)MJB_UTF_ACCEPT, "UTF-16LE: MJB_UTF_ACCEPT")
    ATT_ASSERT(index, 2, "UTF-16LE: Index 2")
    ATT_ASSERT(in_error, false, "UTF-16LE: not error state")

    // Second call: Should reach end of string
    result = mjb_next_codepoint(buffer_utf16le, size_utf16le, &state, &index, MJB_ENC_UTF_16LE,
        &codepoint, &in_error);

    ATT_ASSERT((int)result, (int)MJB_DECODE_END, "UTF-16LE: MJB_DECODE_END")
    ATT_ASSERT((int)state, (int)MJB_UTF_ACCEPT, "UTF-16LE: MJB_UTF_ACCEPT")
    ATT_ASSERT(index, 2, "UTF-16LE: index 2")

    const char buffer_utf16be_bom[] = { '\xFE', '\xFF', '\x00', 'A' };

    RESET_STATE()

    result = mjb_next_codepoint(buffer_utf16be_bom, sizeof(buffer_utf16be_bom), &state, &index,
        MJB_ENC_UTF_16, &codepoint, &in_error);

    ATT_ASSERT((int)result, (int)MJB_DECODE_OK, "UTF-16 generic BOM: MJB_DECODE_OK")
    ATT_ASSERT(codepoint, 0x41, "UTF-16 generic BOM: U+0041")
    ATT_ASSERT(index, (size_t)4, "UTF-16 generic BOM: consumed signature")
    ATT_ASSERT(in_error, false, "UTF-16 generic BOM: not error state")

    // Test that inner BOMs are preserved as U+FEFF codepoints, not consumed as signatures.
    const char buffer_utf16be_inner_bom[] = {
        '\xFE', '\xFF',
        '\x00', 'A',
        '\xFE', '\xFF',
        '\x00', 'B',
    };

    RESET_STATE()

    result = mjb_next_codepoint(buffer_utf16be_inner_bom, sizeof(buffer_utf16be_inner_bom),
        &state, &index, MJB_ENC_UTF_16, &codepoint, &in_error);

    ATT_ASSERT((int)result, (int)MJB_DECODE_OK, "UTF-16 generic inner BOM: first codepoint")
    ATT_ASSERT(codepoint, 0x41, "UTF-16 generic inner BOM: U+0041")
    ATT_ASSERT(index, (size_t)4, "UTF-16 generic inner BOM: consumed initial signature only")

    result = mjb_next_codepoint(buffer_utf16be_inner_bom, sizeof(buffer_utf16be_inner_bom),
        &state, &index, MJB_ENC_UTF_16, &codepoint, &in_error);

    ATT_ASSERT((int)result, (int)MJB_DECODE_OK, "UTF-16 generic inner BOM: second codepoint")
    ATT_ASSERT(codepoint, 0xFEFF, "UTF-16 generic inner BOM: preserves inner U+FEFF")
    ATT_ASSERT(index, (size_t)6, "UTF-16 generic inner BOM: index 6")

    result = mjb_next_codepoint(buffer_utf16be_inner_bom, sizeof(buffer_utf16be_inner_bom),
        &state, &index, MJB_ENC_UTF_16, &codepoint, &in_error);

    ATT_ASSERT((int)result, (int)MJB_DECODE_OK, "UTF-16 generic inner BOM: third codepoint")
    ATT_ASSERT(codepoint, 0x42, "UTF-16 generic inner BOM: U+0042")
    ATT_ASSERT(index, sizeof(buffer_utf16be_inner_bom), "UTF-16 generic inner BOM: end index")

    RESET_STATE()

    result = mjb_next_codepoint(buffer_utf16be_bom, sizeof(buffer_utf16be_bom), &state, &index,
        MJB_ENC_UTF_16BE, &codepoint, &in_error);

    ATT_ASSERT((int)result, (int)MJB_DECODE_OK, "UTF-16BE BOM: MJB_DECODE_OK")
    ATT_ASSERT(codepoint, 0xFEFF, "UTF-16BE BOM: preserves U+FEFF")
    ATT_ASSERT(index, (size_t)2, "UTF-16BE BOM: index 2")
    ATT_ASSERT(in_error, false, "UTF-16BE BOM: not error state")

    const char buffer_utf16_no_bom[] = { '\x00', 'A' };

    RESET_STATE()

    result = mjb_next_codepoint(buffer_utf16_no_bom, sizeof(buffer_utf16_no_bom), &state, &index,
        MJB_ENC_UTF_16, &codepoint, &in_error);

    ATT_ASSERT((int)result, (int)MJB_DECODE_ERROR, "UTF-16 generic no BOM: decode error")
    ATT_ASSERT(codepoint, MJB_CODEPOINT_REPLACEMENT, "UTF-16 generic no BOM: replacement")
    ATT_ASSERT(index, sizeof(buffer_utf16_no_bom), "UTF-16 generic no BOM: consumed input")
    ATT_ASSERT(in_error, true, "UTF-16 generic no BOM: error state")

    const char buffer_utf32le_bom[] = {
        '\xFF', '\xFE', '\x00', '\x00',
        'A', '\x00', '\x00', '\x00',
    };

    RESET_STATE()

    result = mjb_next_codepoint(buffer_utf32le_bom, sizeof(buffer_utf32le_bom), &state, &index,
        MJB_ENC_UTF_32, &codepoint, &in_error);

    ATT_ASSERT((int)result, (int)MJB_DECODE_OK, "UTF-32 generic BOM: MJB_DECODE_OK")
    ATT_ASSERT(codepoint, 0x41, "UTF-32 generic BOM: U+0041")
    ATT_ASSERT(index, sizeof(buffer_utf32le_bom), "UTF-32 generic BOM: consumed signature")
    ATT_ASSERT(in_error, false, "UTF-32 generic BOM: not error state")

    const char *buffer_utf16be_emoji = "\xD8\x3D\xDE\x42";

    RESET_STATE()

    result = mjb_next_codepoint(buffer_utf16be_emoji, 4, &state, &index,
        MJB_ENC_UTF_16BE, &codepoint, &in_error);

    ATT_ASSERT((int)result, (int)MJB_DECODE_INCOMPLETE,
        "UTF-16BE surrogate: high surrogate incomplete")
    ATT_ASSERT((int)state, (int)MJB_UTF_PENDING_SURROGATE,
        "UTF-16BE surrogate: pending state")
    ATT_ASSERT(index, 2, "UTF-16BE surrogate: index 2")

    result = mjb_next_codepoint(buffer_utf16be_emoji, 4, &state, &index,
        MJB_ENC_UTF_16BE, &codepoint, &in_error);

    ATT_ASSERT((int)result, (int)MJB_DECODE_OK, "UTF-16BE surrogate: MJB_DECODE_OK")
    ATT_ASSERT(codepoint, 0x1F642, "UTF-16BE surrogate: U+1F642")
    ATT_ASSERT((int)state, (int)MJB_UTF_ACCEPT, "UTF-16BE surrogate: MJB_UTF_ACCEPT")
    ATT_ASSERT(index, 4, "UTF-16BE surrogate: index 4")
    ATT_ASSERT(in_error, false, "UTF-16BE surrogate: not error state")

    const char *buffer_utf16le_emoji = "\x3D\xD8\x42\xDE";

    RESET_STATE()

    result = mjb_next_codepoint(buffer_utf16le_emoji, 4, &state, &index,
        MJB_ENC_UTF_16LE, &codepoint, &in_error);

    ATT_ASSERT((int)result, (int)MJB_DECODE_INCOMPLETE,
        "UTF-16LE surrogate: high surrogate incomplete")
    ATT_ASSERT((int)state, (int)MJB_UTF_PENDING_SURROGATE,
        "UTF-16LE surrogate: pending state")
    ATT_ASSERT(index, 2, "UTF-16LE surrogate: index 2")

    result = mjb_next_codepoint(buffer_utf16le_emoji, 4, &state, &index,
        MJB_ENC_UTF_16LE, &codepoint, &in_error);

    ATT_ASSERT((int)result, (int)MJB_DECODE_OK, "UTF-16LE surrogate: MJB_DECODE_OK")
    ATT_ASSERT(codepoint, 0x1F642, "UTF-16LE surrogate: U+1F642")
    ATT_ASSERT((int)state, (int)MJB_UTF_ACCEPT, "UTF-16LE surrogate: MJB_UTF_ACCEPT")
    ATT_ASSERT(index, 4, "UTF-16LE surrogate: index 4")
    ATT_ASSERT(in_error, false, "UTF-16LE surrogate: not error state")

    // Truncated trailing units must terminate decoding (one replacement, then end), not loop.
    ATT_ASSERT(mjb_string_length("A\0B", 3, MJB_ENC_UTF_16BE), 2,
        "UTF-16BE: truncated trailing unit ends decoding")
    ATT_ASSERT(mjb_string_length("A", 1, MJB_ENC_UTF_16LE), 1,
        "UTF-16LE: lone trailing byte decodes as replacement")
    ATT_ASSERT(mjb_string_length("\0\0\0A!", 5, MJB_ENC_UTF_32BE), 2,
        "UTF-32BE: truncated trailing unit ends decoding")
    ATT_ASSERT(mjb_string_length("A!\0", 3, MJB_ENC_UTF_32LE), 1,
        "UTF-32LE: lone truncated unit decodes as replacement")

#undef RESET_STATE

    return 0;
}
