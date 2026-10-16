/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>

#include "../src/mojibake-internal.h"
#include "test.h"

static void break_line_callback(const char *buffer, size_t byte_length, unsigned int current_line,
    mjb_break_type *expected_types) {
    char test_name[256];
    mjb_break_type bt = MJB_BT_NOT_SET;
    mjb_line_break_state state;
    state.index = 0;
    size_t index = 0;
    size_t successful_count = 0;
    size_t starts[1025];
    size_t start_count = mjb_test_codepoint_starts(buffer, byte_length, starts, 1025);
    size_t position_mismatches = 0;

    mjb_text text = mjb_text_utf8(buffer, byte_length);
    while((bt = mjb_line_break_next(&text, &state)) != MJB_BT_NOT_SET) {
        snprintf(test_name, 256, "#%u index %zu", current_line, index);

        if(bt == MJB_BT_MANDATORY) {
            bt = MJB_BT_ALLOWED;
        }

        // There is no event for the start of the text: event i is the boundary after codepoint
        // i, at the start of codepoint i + 1, and the last one is the end of the text.
        if(index < start_count && state.position != starts[index + 1]) {
            ++position_mismatches;
        }

        if((uint8_t)bt == (uint8_t)expected_types[index++]) {
            ++successful_count;
        } else {
            break;
        }
    }

    MJB_TEST_COVERAGE(mjb_line_break_next);
    ATT_ASSERT(index, successful_count, test_name);
    ATT_ASSERT(position_mismatches, (size_t)0, "Break positions are codepoint starts");
}

ATT_TEST(break_line) {
    {
        mjb_line_break_state state;
        state.index = 0;
        ATT_ASSERT((int)mjb_line_break_next(NULL, &state), (int)MJB_BT_NOT_SET,
            "Line break rejects NULL text");
    }

    mjb_line_break_state state;
    state.index = 0;

    mjb_text text = mjb_text_utf8(NULL, 1);
    ATT_ASSERT((uint8_t)mjb_line_break_next(&text, &state), (uint8_t)MJB_BT_NOT_SET,
        "Line break rejects NULL buffer");
    text = mjb_text_utf8("A", 1);
    ATT_ASSERT((uint8_t)mjb_line_break_next(&text, NULL), (uint8_t)MJB_BT_NOT_SET,
        "Line break rejects NULL state");

    // Unicode 18 LB12a disallows BA × GL; U+2012 changed from HH to BA.
    const char ba_gl[] = "\xE2\x80\x92\xC2\xA0"; // FIGURE DASH, NO-BREAK SPACE
    state.index = 0;
    text = mjb_text_utf8(ba_gl, sizeof(ba_gl) - 1);
    ATT_ASSERT((uint8_t)mjb_line_break_next(&text, &state),
        (uint8_t)MJB_BT_NO_BREAK, "Unicode 18 LB12a BA x GL");
    text = mjb_text_utf8(ba_gl, sizeof(ba_gl) - 1);
    ATT_ASSERT((uint8_t)mjb_line_break_next(&text, &state),
        (uint8_t)MJB_BT_MANDATORY, "Unicode 18 LB12a end");

    // U+00AD changed from BA to HH and remains an LB12a exception before GL.
    const char hh_gl[] = "\xC2\xAD\xC2\xA0"; // SOFT HYPHEN, NO-BREAK SPACE
    state.index = 0;
    text = mjb_text_utf8(hh_gl, sizeof(hh_gl) - 1);
    ATT_ASSERT((uint8_t)mjb_line_break_next(&text, &state),
        (uint8_t)MJB_BT_ALLOWED, "Unicode 18 LB12a HH before GL");

    read_test_file("./unicode-data/UCD/auxiliary/LineBreakTest.txt", &break_line_callback);
}
