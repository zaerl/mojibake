/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <errno.h>
#include <stdarg.h>
#include <string.h>

#include "test.h"

static int test_utf8_vsnprintf(char *buffer, size_t buffer_size, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int required = mjb_utf8_vsnprintf(buffer, buffer_size, format, args);
    va_end(args);

    return required;
}

static int test_utf8_grapheme_vsnprintf(char *buffer, size_t buffer_size, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int required = mjb_utf8_grapheme_vsnprintf(buffer, buffer_size, format, args);
    va_end(args);

    return required;
}

static void *test_format_no_memory(size_t size) {
    (void)size;

    return NULL;
}

int test_format(void *arg) {
    (void)arg;

    char buffer[32];
    MJB_TEST_COVERAGE(mjb_utf8_snprintf);
    int required = mjb_utf8_snprintf(buffer, sizeof(buffer), "value %d", 42);

    ATT_ASSERT(required, 8, "UTF-8 snprintf returns the written length")
    ATT_ASSERT(buffer, "value 42", "UTF-8 snprintf formats an untruncated result")

    required = mjb_utf8_snprintf(NULL, 0, "%s", "\xC3\xA9\xC3\xA9");
    ATT_ASSERT(required, 4, "UTF-8 snprintf supports a size query")

    char one_byte[2] = { '#', '#' };
    required = mjb_utf8_snprintf(one_byte, 1, "%s", "\xC3\xA9");
    ATT_ASSERT(required, 2, "UTF-8 snprintf reports a two-byte result")
    ATT_ASSERT((unsigned int)(unsigned char)one_byte[0], 0u,
        "UTF-8 snprintf terminates a one-byte buffer")
    ATT_ASSERT((unsigned int)(unsigned char)one_byte[1], (unsigned int)'#',
        "UTF-8 snprintf stays within a one-byte buffer")

    char two_byte[4] = { '#', '#', '#', '#' };
    required = mjb_utf8_snprintf(two_byte, sizeof(two_byte), "%s", "\xC3\xA9\xC3\xA9");
    ATT_ASSERT(required, 4, "UTF-8 snprintf preserves the required two-byte sequence length")
    ATT_ASSERT(two_byte, "\xC3\xA9", "UTF-8 snprintf removes a partial two-byte suffix")

    char three_byte[3] = { '#', '#', '#' };
    required = mjb_utf8_snprintf(three_byte, sizeof(three_byte), "%s", "\xE2\x82\xAC");
    ATT_ASSERT(required, 3, "UTF-8 snprintf preserves the required three-byte sequence length")
    ATT_ASSERT(three_byte, "", "UTF-8 snprintf removes a partial three-byte suffix")

    char four_byte[4] = { '#', '#', '#', '#' };
    required = mjb_utf8_snprintf(four_byte, sizeof(four_byte), "%s", "\xF0\x9F\x98\x80");
    ATT_ASSERT(required, 4, "UTF-8 snprintf preserves the required four-byte sequence length")
    ATT_ASSERT(four_byte, "", "UTF-8 snprintf removes a partial four-byte suffix")

    char prefixed[5] = { '#', '#', '#', '#', '#' };
    required = mjb_utf8_snprintf(prefixed, sizeof(prefixed), "A%sB", "\xF0\x9F\x98\x80");
    ATT_ASSERT(required, 6, "UTF-8 snprintf reports the full prefixed result length")
    ATT_ASSERT(prefixed, "A", "UTF-8 snprintf retains the prefix before a partial codepoint")

    char boundary[3] = { '#', '#', '#' };
    required = mjb_utf8_snprintf(boundary, sizeof(boundary), "%sZ", "\xC3\xA9");
    ATT_ASSERT(required, 3, "UTF-8 snprintf reports truncation at a complete boundary")
    ATT_ASSERT(boundary, "\xC3\xA9", "UTF-8 snprintf retains a complete trailing codepoint")

    char ascii[4] = { '#', '#', '#', '#' };
    required = mjb_utf8_snprintf(ascii, sizeof(ascii), "%s", "abcd");
    ATT_ASSERT(required, 4, "UTF-8 snprintf preserves the standard ASCII return value")
    ATT_ASSERT(ascii, "abc", "UTF-8 snprintf preserves standard ASCII truncation")

    char embedded_null[3] = { '#', '#', '#' };
    required = mjb_utf8_snprintf(embedded_null, sizeof(embedded_null), "%c%s", 0, "\xC3\xA9");
    ATT_ASSERT(required, 3, "UTF-8 snprintf counts an embedded NULL")
    ATT_ASSERT((unsigned int)(unsigned char)embedded_null[1], 0u,
        "UTF-8 snprintf clips after an embedded NULL")

    char variadic[4] = { '#', '#', '#', '#' };
    MJB_TEST_COVERAGE(mjb_utf8_vsnprintf);
    required = test_utf8_vsnprintf(variadic, sizeof(variadic), "%s", "\xC3\xA9\xC3\xA9");
    ATT_ASSERT(required, 4, "UTF-8 vsnprintf preserves the required result length")
    ATT_ASSERT(variadic, "\xC3\xA9", "UTF-8 vsnprintf removes a partial suffix")

    MJB_TEST_COVERAGE(mjb_utf8_grapheme_snprintf);
    const char combining_sequence[] = "Ae\xCC\x81"
                                      "B";
    required = mjb_utf8_grapheme_snprintf(NULL, 0, "%s", combining_sequence);
    ATT_ASSERT(required, 5, "Grapheme snprintf supports a size query")

    char grapheme_one_byte[2] = { '#', '#' };
    required = mjb_utf8_grapheme_snprintf(grapheme_one_byte, 1, "%s", combining_sequence);
    ATT_ASSERT(required, 5, "Grapheme snprintf reports a result for a one-byte buffer")
    ATT_ASSERT((unsigned int)(unsigned char)grapheme_one_byte[0], 0u,
        "Grapheme snprintf terminates a one-byte buffer")
    ATT_ASSERT((unsigned int)(unsigned char)grapheme_one_byte[1], (unsigned int)'#',
        "Grapheme snprintf stays within a one-byte buffer")

    const char first_combining_sequence[] = "e\xCC\x81"
                                            "B";
    char first_combining[2] = { '#', '#' };
    required = mjb_utf8_grapheme_snprintf(first_combining, sizeof(first_combining), "%s",
        first_combining_sequence);
    ATT_ASSERT(required, 4, "Grapheme snprintf reports a partial first grapheme")
    ATT_ASSERT(first_combining, "", "Grapheme snprintf removes a partial first grapheme")

    char combining[4] = { '#', '#', '#', '#' };
    required = mjb_utf8_grapheme_snprintf(combining, sizeof(combining), "%s", combining_sequence);
    ATT_ASSERT(required, 5, "Grapheme snprintf reports the complete combining-sequence length")
    ATT_ASSERT(combining, "A", "Grapheme snprintf removes a partial combining sequence")

    char combining_boundary[5] = { '#', '#', '#', '#', '#' };
    required = mjb_utf8_grapheme_snprintf(combining_boundary, sizeof(combining_boundary), "%s",
        combining_sequence);
    ATT_ASSERT(required, 5, "Grapheme snprintf reports truncation at a grapheme boundary")
    ATT_ASSERT(combining_boundary, "Ae\xCC\x81",
        "Grapheme snprintf retains a complete combining sequence")

    const char flag_sequence[] = "A\xF0\x9F\x87\xAE\xF0\x9F\x87\xB9"
                                 "B";
    char flag[6] = { '#', '#', '#', '#', '#', '#' };
    required = mjb_utf8_grapheme_snprintf(flag, sizeof(flag), "%s", flag_sequence);
    ATT_ASSERT(required, 10, "Grapheme snprintf reports the complete flag-sequence length")
    ATT_ASSERT(flag, "A", "Grapheme snprintf removes a partial flag sequence")

    const char modifier_sequence[] = "A\xF0\x9F\x91\x8D\xF0\x9F\x8F\xBD"
                                     "B";
    char modifier[6] = { '#', '#', '#', '#', '#', '#' };
    required = mjb_utf8_grapheme_snprintf(modifier, sizeof(modifier), "%s", modifier_sequence);
    ATT_ASSERT(required, 10, "Grapheme snprintf reports the complete modifier-sequence length")
    ATT_ASSERT(modifier, "A", "Grapheme snprintf removes a partial emoji modifier sequence")

    const char zwj_sequence[] = "A\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x92\xBB"
                                "B";
    char zwj[9] = { '#', '#', '#', '#', '#', '#', '#', '#', '#' };
    required = mjb_utf8_grapheme_snprintf(zwj, sizeof(zwj), "%s", zwj_sequence);
    ATT_ASSERT(required, 13, "Grapheme snprintf reports the complete ZWJ-sequence length")
    ATT_ASSERT(zwj, "A", "Grapheme snprintf removes a partial emoji ZWJ sequence")

    char grapheme_ascii[4] = { '#', '#', '#', '#' };
    required = mjb_utf8_grapheme_snprintf(grapheme_ascii, sizeof(grapheme_ascii), "%s", "abcd");
    ATT_ASSERT(required, 4, "Grapheme snprintf preserves the standard ASCII return value")
    ATT_ASSERT(grapheme_ascii, "abc", "Grapheme snprintf preserves ASCII truncation")

    char grapheme_variadic[4] = { '#', '#', '#', '#' };
    MJB_TEST_COVERAGE(mjb_utf8_grapheme_vsnprintf);
    required = test_utf8_grapheme_vsnprintf(grapheme_variadic, sizeof(grapheme_variadic), "%s",
        combining_sequence);
    ATT_ASSERT(required, 5, "Grapheme vsnprintf reports the complete result length")
    ATT_ASSERT(grapheme_variadic, "A", "Grapheme vsnprintf removes a partial grapheme")

    mjb_reset();
    ATT_ASSERT_STATUS(mjb_set_memory_functions(test_format_no_memory, NULL, NULL), MJB_STATUS_OK,
        "Set allocation failure for grapheme snprintf")

    char no_allocation[8] = { '#', '#', '#', '#', '#', '#', '#', '#' };
    MJB_TEST_COVERAGE(mjb_utf8_grapheme_snprintf);
    required = mjb_utf8_grapheme_snprintf(no_allocation, sizeof(no_allocation), "%s", "ok");
    ATT_ASSERT(required, 2, "Untruncated grapheme snprintf does not require allocation")
    ATT_ASSERT(no_allocation, "ok", "Untruncated grapheme snprintf succeeds without allocation")

    errno = 0;
    char no_memory[4] = { '#', '#', '#', '#' };
    required = mjb_utf8_grapheme_snprintf(no_memory, sizeof(no_memory), "%s", combining_sequence);
    ATT_ASSERT(required, -1, "Truncated grapheme snprintf reports allocation failure")
    ATT_ASSERT(no_memory, "", "Grapheme snprintf clears the output after allocation failure")
    ATT_ASSERT(errno, ENOMEM, "Grapheme snprintf reports ENOMEM after allocation failure")
    mjb_reset();

    return 0;
}
