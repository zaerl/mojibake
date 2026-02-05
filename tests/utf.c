/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"
#include "../src/utf.h"

void *test_utf(void *arg) {
    // Test scanning single ASCII character "A"
    const char *buffer = "A";
    size_t size = 1;
    uint8_t state = MJB_UTF_ACCEPT;
    size_t index = 0;
    bool in_error = false;
    mjb_codepoint codepoint = 0;
    mjb_decode_result result;

    result = mjb_next_codepoint(buffer, size, &state, &index, MJB_ENCODING_UTF_8,
        &codepoint, &in_error);

    ATT_ASSERT((int)result, (int)MJB_DECODE_OK, "MJB_DECODE_OK");
    ATT_ASSERT(codepoint, 0x41, "0x41 (A)");
    ATT_ASSERT((int)state, (int)MJB_UTF_ACCEPT, "MJB_UTF_ACCEPT");
    ATT_ASSERT(index, 1, "Index 1");
    ATT_ASSERT(in_error, false, "Should not be in error state");

    result = mjb_next_codepoint(buffer, size, &state, &index, MJB_ENCODING_UTF_8,
        &codepoint, &in_error);

    ATT_ASSERT((int)result, (int)MJB_DECODE_END, "MJB_DECODE_END");
    ATT_ASSERT((int)state, (int)MJB_UTF_ACCEPT, "MJB_UTF_ACCEPT");
    ATT_ASSERT(index, 1, "Index 1");

    const char *buffer_utf16be = "\x00\x41";
    size_t size_utf16be = 2;
    state = MJB_UTF_ACCEPT;
    index = 0;
    in_error = false;
    codepoint = 0;

    result = mjb_next_codepoint(buffer_utf16be, size_utf16be, &state, &index, MJB_ENCODING_UTF_16_BE,
        &codepoint, &in_error);

    ATT_ASSERT((int)result, (int)MJB_DECODE_OK, "UTF-16BE: MJB_DECODE_OK");
    ATT_ASSERT(codepoint, 0x41, "UTF-16BE: 0x41 (A)");
    ATT_ASSERT((int)state, (int)MJB_UTF_ACCEPT, "UTF-16BE: MJB_UTF_ACCEPT");
    ATT_ASSERT(index, 2, "UTF-16BE: Index 2");
    ATT_ASSERT(in_error, false, "UTF-16BE: not error state");

    result = mjb_next_codepoint(buffer_utf16be, size_utf16be, &state, &index, MJB_ENCODING_UTF_16_BE,
        &codepoint, &in_error);

    ATT_ASSERT((int)result, (int)MJB_DECODE_END, "UTF-16BE: MJB_DECODE_END");
    ATT_ASSERT((int)state, (int)MJB_UTF_ACCEPT, "UTF-16BE: MJB_UTF_ACCEPT");
    ATT_ASSERT(index, 2, "UTF-16BE: index 2");

    const char *buffer_utf16le = "\x41\x00";
    size_t size_utf16le = 2;
    state = MJB_UTF_ACCEPT;
    index = 0;
    in_error = false;
    codepoint = 0;

    result = mjb_next_codepoint(buffer_utf16le, size_utf16le, &state, &index, MJB_ENCODING_UTF_16_LE,
        &codepoint, &in_error);

    ATT_ASSERT((int)result, (int)MJB_DECODE_OK, "UTF-16LE: MJB_DECODE_OK");
    ATT_ASSERT(codepoint, 0x41, "UTF-16LE: 0x41 (A)");
    ATT_ASSERT((int)state, (int)MJB_UTF_ACCEPT, "UTF-16LE: MJB_UTF_ACCEPT");
    ATT_ASSERT(index, 2, "UTF-16LE: Index 2");
    ATT_ASSERT(in_error, false, "UTF-16LE: not error state");

    // Second call: Should reach end of string
    result = mjb_next_codepoint(buffer_utf16le, size_utf16le, &state, &index, MJB_ENCODING_UTF_16_LE,
        &codepoint, &in_error);

    ATT_ASSERT((int)result, (int)MJB_DECODE_END, "UTF-16LE: MJB_DECODE_END");
    ATT_ASSERT((int)state, (int)MJB_UTF_ACCEPT, "UTF-16LE: MJB_UTF_ACCEPT");
    ATT_ASSERT(index, 2, "UTF-16LE: index 2");

    return NULL;
}
