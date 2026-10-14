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
    mjb_next_line_state state;
    state.index = 0;
    size_t index = 0;
    size_t successful_count = 0;

    mjb_text text = mjb_text_utf8(buffer, byte_length);
    while((bt = mjb_next_line_break(&text, &state)) != MJB_BT_NOT_SET) {
        snprintf(test_name, 256, "#%u index %zu", current_line, index);

        if(bt == MJB_BT_MANDATORY) {
            bt = MJB_BT_ALLOWED;
        }

        if((uint8_t)bt == (uint8_t)expected_types[index++]) {
            ++successful_count;
        } else {
            break;
        }
    }

    MJB_TEST_COVERAGE(mjb_next_line_break);
    ATT_ASSERT(index, successful_count, test_name);
}

ATT_TEST(break_line) {
    {
        mjb_next_line_state state;
        state.index = 0;
        ATT_ASSERT((int)mjb_next_line_break(NULL, &state), (int)MJB_BT_NOT_SET,
            "Line break rejects NULL text");
    }

    mjb_next_line_state state;
    state.index = 0;

    mjb_text text = mjb_text_utf8(NULL, 1);
    ATT_ASSERT((uint8_t)mjb_next_line_break(&text, &state), (uint8_t)MJB_BT_NOT_SET,
        "Line break rejects NULL buffer");
    text = mjb_text_utf8("A", 1);
    ATT_ASSERT((uint8_t)mjb_next_line_break(&text, NULL), (uint8_t)MJB_BT_NOT_SET,
        "Line break rejects NULL state");

    // Unicode 18 LB12a disallows BA × GL; U+2012 changed from HH to BA.
    const char ba_gl[] = "\xE2\x80\x92\xC2\xA0"; // FIGURE DASH, NO-BREAK SPACE
    state.index = 0;
    text = mjb_text_utf8(ba_gl, sizeof(ba_gl) - 1);
    ATT_ASSERT((uint8_t)mjb_next_line_break(&text, &state),
        (uint8_t)MJB_BT_NO_BREAK, "Unicode 18 LB12a BA x GL");
    text = mjb_text_utf8(ba_gl, sizeof(ba_gl) - 1);
    ATT_ASSERT((uint8_t)mjb_next_line_break(&text, &state),
        (uint8_t)MJB_BT_MANDATORY, "Unicode 18 LB12a end");

    // U+00AD changed from BA to HH and remains an LB12a exception before GL.
    const char hh_gl[] = "\xC2\xAD\xC2\xA0"; // SOFT HYPHEN, NO-BREAK SPACE
    state.index = 0;
    text = mjb_text_utf8(hh_gl, sizeof(hh_gl) - 1);
    ATT_ASSERT((uint8_t)mjb_next_line_break(&text, &state),
        (uint8_t)MJB_BT_ALLOWED, "Unicode 18 LB12a HH before GL");

    read_test_file("./unicode-data/UCD/auxiliary/LineBreakTest.txt", &break_line_callback);
}
