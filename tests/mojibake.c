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

    char storage[4] = { 'a', 'b', 'c', 'd' };
    mjb_buffer buffer = mjb_buffer_make(storage, 3, MJB_ENC_UTF_16BE);
    text = mjb_buffer_text(&buffer);
    ATT_ASSERT(text.buffer == storage, true, "Buffer text points at the output storage");
    ATT_ASSERT(text.byte_length, (size_t)3, "Buffer text uses the buffer size");
    ATT_ASSERT((unsigned int)text.encoding, (unsigned int)MJB_ENC_UTF_16BE,
        "Buffer text uses the buffer encoding");
}

static void test_result_chaining(void) {
    mjb_text text = mjb_text_utf8("Ab", 2);
    mjb_result result;

    // An already normalized input is returned as-is, without allocating
    ATT_ASSERT_STATUS(mjb_normalize(&text, MJB_MALFORMED_STOP, MJB_NORMALIZATION_NFC,
                          MJB_ENC_UTF_8, &result, NULL),
        MJB_STATUS_OK, "Normalize returns normalized input as-is");
    ATT_ASSERT(result.owned, false, "Normalized input is not owned");
    ATT_ASSERT(result.output == text.buffer, true, "Normalized input points into the input");

    // Chain a result into another call: NFD of U+00E9 is U+0065 U+0301 in UTF-32BE.
    text = mjb_text_utf8("\xC3\xA9", 2);
    ATT_ASSERT_STATUS(mjb_normalize(&text, MJB_MALFORMED_STOP, MJB_NORMALIZATION_NFD,
                          MJB_ENC_UTF_32BE, &result, NULL),
        MJB_STATUS_OK, "Normalize a result to chain");
    ATT_ASSERT(result.owned, true, "A converted result is owned");
    mjb_text chained = mjb_text_make(result.output, result.output_size, MJB_ENC_UTF_32BE);
    size_t chained_count = 0;
    ATT_ASSERT_STATUS(mjb_codepoint_count(&chained, MJB_MALFORMED_STOP, &chained_count, NULL),
        MJB_STATUS_OK, "Count the chained result");
    ATT_ASSERT(chained_count, (size_t)2, "Chained result has two codepoints");
    ATT_ASSERT_STATUS(mjb_result_free(&result), MJB_STATUS_OK, "Free the chained result");
}

ATT_TEST(mojibake) {
    test_text();
    test_result_chaining();

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
    mjb_text text = mjb_text_utf8("A", 1);
    ATT_ASSERT_STATUS(mjb_convert_encoding(&text, MJB_MALFORMED_STOP,
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

    // A result that does not own its output borrows the caller's input and must not be released.
    char borrowed[] = "A";
    mjb_result borrowed_result = { borrowed, 1, false };
    mjb_test_allocator_reset();
    ATT_ASSERT_STATUS(mjb_result_free(&borrowed_result), MJB_STATUS_OK,
        "Free a result that does not own its output");
    size_t borrowed_calls = mjb_test_allocator_call_count();
    ATT_ASSERT(borrowed_calls, (size_t)0, "A borrowed output is not freed");
    ATT_ASSERT(borrowed_result.output == borrowed, true, "A borrowed result keeps its input");
    ATT_ASSERT(borrowed_result.output_size, (size_t)1, "A borrowed result keeps its size");
    ATT_ASSERT(borrowed_result.owned, false, "A borrowed result stays unowned");

    mjb_test_allocator_reset();

    ATT_ASSERT(mjb_status_message(MJB_STATUS_OK), "The operation completed successfully",
        "Status message returns OK");
    ATT_ASSERT(mjb_status_message(MJB_STATUS_END_OF_INPUT), "The end of the input was reached",
        "Status message returns end of input");
    ATT_ASSERT(mjb_status_message((mjb_status)100), "The status code is unknown",
        "Status message returns unknown error for invalid status");
}
