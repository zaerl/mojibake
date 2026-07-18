/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>

#include "../src/mojibake-internal.h"
#include "test.h"

void break_line_callback(const char *buffer, size_t byte_length, unsigned int current_line,
    mjb_break_type *expected_types) {
    char test_name[256];
    mjb_break_type bt = MJB_BT_NOT_SET;
    mjb_next_line_state state;
    state.index = 0;
    size_t index = 0;
    size_t successful_count = 0;

    while((bt = mjb_break_line(buffer, byte_length, MJB_ENC_UTF_8, &state)) != MJB_BT_NOT_SET) {
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

    MJB_TEST_COVERAGE(mjb_break_line);
    ATT_ASSERT(index, successful_count, test_name)
}

int test_break_line(void *arg) {
    mjb_next_line_state state;
    state.index = 0;

    ATT_ASSERT((uint8_t)mjb_break_line(NULL, 1, MJB_ENC_UTF_8, &state), (uint8_t)MJB_BT_NOT_SET,
        "Line break rejects NULL buffer")
    ATT_ASSERT((uint8_t)mjb_break_line("A", 1, MJB_ENC_UTF_8, NULL), (uint8_t)MJB_BT_NOT_SET,
        "Line break rejects NULL state")

#if !defined(MJB_DANGEROUSLY_ALLOW_EMBEDDED_NULLS) || !MJB_DANGEROUSLY_ALLOW_EMBEDDED_NULLS
    const char utf16le_null[] = { '\0', '\0', 'A', '\0' };

    state.index = 0;
    ATT_ASSERT((uint8_t)mjb_break_line(utf16le_null, sizeof(utf16le_null), MJB_ENC_UTF_16LE,
                   &state),
        (uint8_t)MJB_BT_ALLOWED, "Line break stops at UTF-16LE NULL")
    ATT_ASSERT((uint8_t)mjb_break_line(utf16le_null, sizeof(utf16le_null), MJB_ENC_UTF_16LE,
                   &state),
        (uint8_t)MJB_BT_NOT_SET, "Line break finishes after UTF-16LE NULL")
#endif

    // Unicode 18 LB12a disallows BA × GL; U+2012 changed from HH to BA.
    const char ba_gl[] = "\xE2\x80\x92\xC2\xA0"; // FIGURE DASH, NO-BREAK SPACE
    state.index = 0;
    ATT_ASSERT((uint8_t)mjb_break_line(ba_gl, sizeof(ba_gl) - 1, MJB_ENC_UTF_8, &state),
        (uint8_t)MJB_BT_NO_BREAK, "Unicode 18 LB12a BA x GL")
    ATT_ASSERT((uint8_t)mjb_break_line(ba_gl, sizeof(ba_gl) - 1, MJB_ENC_UTF_8, &state),
        (uint8_t)MJB_BT_MANDATORY, "Unicode 18 LB12a end")

    // U+00AD changed from BA to HH and remains an LB12a exception before GL.
    const char hh_gl[] = "\xC2\xAD\xC2\xA0"; // SOFT HYPHEN, NO-BREAK SPACE
    state.index = 0;
    ATT_ASSERT((uint8_t)mjb_break_line(hh_gl, sizeof(hh_gl) - 1, MJB_ENC_UTF_8, &state),
        (uint8_t)MJB_BT_ALLOWED, "Unicode 18 LB12a HH before GL")

    read_test_file("./utils/generate/unicode-data/UCD/auxiliary/LineBreakTest.txt",
        &break_line_callback);

    return 0;
}
