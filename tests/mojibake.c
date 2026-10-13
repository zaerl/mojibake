/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"

static void test_text(void) {
    mjb_text text = mjb_text_utf8("Hello", 5);

    ATT_ASSERT(text.buffer != NULL, true, "Text buffer is set");
    ATT_ASSERT(text.byte_length, (size_t)5, "Text byte length is correct");
    ATT_ASSERT((unsigned int)text.encoding, (unsigned int)MJB_ENC_UTF_8, "Text encoding is UTF-8");

    text = mjb_text_utf16le("A\0", 2);
    ATT_ASSERT((unsigned int)text.encoding, (unsigned int)MJB_ENC_UTF_16LE,
        "Text encoding is UTF-16LE");
    text = mjb_text_utf16be("\0A", 2);
    ATT_ASSERT((unsigned int)text.encoding, (unsigned int)MJB_ENC_UTF_16BE,
        "Text encoding is UTF-16BE");
    text = mjb_text_utf32le("A\0\0\0", 4);
    ATT_ASSERT((unsigned int)text.encoding, (unsigned int)MJB_ENC_UTF_32LE,
        "Text encoding is UTF-32LE");
    text = mjb_text_utf32be("\0\0\0A", 4);
    ATT_ASSERT((unsigned int)text.encoding, (unsigned int)MJB_ENC_UTF_32BE,
        "Text encoding is UTF-32BE");
    ATT_ASSERT(text.byte_length, (size_t)4, "Text keeps the byte length");
}

ATT_TEST(mojibake) {
    test_text();

    MJB_TEST_COVERAGE(mjb_set_allocator);
    ATT_ASSERT_STATUS(mjb_test_allocator_invalid_status(), MJB_STATUS_INVALID_ARGUMENT,
        "Reject incomplete allocator");
    ATT_ASSERT_STATUS(mjb_test_allocator_install_status(), MJB_STATUS_OK,
        "Install context-aware allocator");
    ATT_ASSERT_STATUS(mjb_set_allocator(NULL), MJB_STATUS_INVALID_ARGUMENT,
        "Reject repeated allocator configuration");

    mjb_result result = { NULL, 0, false };
    ATT_ASSERT_STATUS(mjb_result_free(NULL), MJB_STATUS_INVALID_ARGUMENT, "Free NULL result");

    mjb_test_allocator_reset();
    ATT_ASSERT_STATUS(mjb_convert_encoding("A", 1, MJB_ENC_UTF_8, MJB_MALFORMED_STOP,
                          MJB_ENC_UTF_16LE, &result, NULL),
        MJB_STATUS_OK, "Allocate result through configured allocator");
    ATT_ASSERT((int)(mjb_test_allocator_call_count() > 0), true,
        "Configured allocator receives allocation context");
    size_t calls_before_free = mjb_test_allocator_call_count();
    ATT_ASSERT_STATUS(mjb_result_free(&result), MJB_STATUS_OK, "Free result with output");
    ATT_ASSERT((int)(mjb_test_allocator_call_count() > calls_before_free), true,
        "Configured allocator frees result output");
    ATT_ASSERT(result.output == NULL, true, "Result output NULL after free");
    ATT_ASSERT(result.output_size, 0, "Result output size 0 after free");

    // An untransformed result borrows the caller's input and must not be released.
    char borrowed[] = "A";
    mjb_result untransformed = { borrowed, 1, false };
    mjb_test_allocator_reset();
    ATT_ASSERT_STATUS(mjb_result_free(&untransformed), MJB_STATUS_OK,
        "Free untransformed result");
    size_t untransformed_calls = mjb_test_allocator_call_count();
    ATT_ASSERT(untransformed_calls, (size_t)0, "Untransformed result is not freed");
    ATT_ASSERT(untransformed.output == borrowed, true, "Untransformed result keeps its input");
    ATT_ASSERT(untransformed.output_size, (size_t)1, "Untransformed result keeps its size");
    ATT_ASSERT(untransformed.transformed, false, "Untransformed result stays untransformed");

    mjb_test_allocator_reset();

    ATT_ASSERT(mjb_status_message(MJB_STATUS_OK), "The operation completed successfully",
        "Status message returns OK");
    ATT_ASSERT(mjb_status_message(MJB_STATUS_END_OF_INPUT), "The end of the input was reached",
        "Status message returns end of input");
    ATT_ASSERT(mjb_status_message((mjb_status)100), "The status code is unknown",
        "Status message returns unknown error for invalid status");
}
