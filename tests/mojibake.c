/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"

static void test_set_failing_allocator(size_t fail_after) {
    mjb_test_allocator_fail_after(fail_after);
}

int test_mojibake(void *arg) {
    (void)arg;

    MJB_TEST_COVERAGE(mjb_set_allocator);
    ATT_ASSERT_STATUS(mjb_test_allocator_invalid_status(), MJB_STATUS_INVALID_ARGUMENT,
        "Reject incomplete allocator")
    ATT_ASSERT_STATUS(mjb_test_allocator_install_status(), MJB_STATUS_OK,
        "Install context-aware allocator")
    ATT_ASSERT_STATUS(mjb_set_allocator(NULL), MJB_STATUS_INVALID_ARGUMENT,
        "Reject repeated allocator configuration")

    mjb_result result = { NULL, 0, false };
    ATT_ASSERT_STATUS(mjb_result_free(NULL), MJB_STATUS_INVALID_ARGUMENT, "Free NULL result")

    mjb_test_allocator_reset();
    ATT_ASSERT_STATUS(mjb_convert_encoding("A", 1, MJB_ENC_UTF_8, MJB_ENC_UTF_16LE, &result),
        MJB_STATUS_OK, "Allocate result through configured allocator")
    ATT_ASSERT((int)(mjb_test_allocator_call_count() > 0), true,
        "Configured allocator receives allocation context")
    size_t calls_before_free = mjb_test_allocator_call_count();
    ATT_ASSERT_STATUS(mjb_result_free(&result), MJB_STATUS_OK, "Free result with output")
    ATT_ASSERT((int)(mjb_test_allocator_call_count() > calls_before_free), true,
        "Configured allocator frees result output")
    ATT_ASSERT(result.output == NULL, true, "Result output NULL after free")
    ATT_ASSERT(result.output_size, 0, "Result output size 0 after free")

    test_set_failing_allocator(0);

    mjb_locale_id locale;
    ATT_ASSERT_STATUS(mjb_locale_parse("en", 2, MJB_ENC_UTF_8, &locale),
        MJB_STATUS_NO_MEMORY, "Locale parsing handles allocation failure")

    char case_into_output[1];
    size_t case_into_size = sizeof(case_into_output);

    ATT_ASSERT_STATUS(mjb_map_case_into("a", 1, MJB_ENC_UTF_8, MJB_CASE_UPPER, MJB_ENC_UTF_8,
                          case_into_output, &case_into_size),
        MJB_STATUS_OK, "Caller-buffer case mapping does not allocate")
    ATT_ASSERT(case_into_size, (size_t)1, "Caller-buffer case mapping output size")
    ATT_ASSERT((unsigned int)case_into_output[0], (unsigned int)'A',
        "Caller-buffer case mapping output")

    char filter_into_output[3];
    size_t filter_into_size = sizeof(filter_into_output);

    ATT_ASSERT_STATUS(mjb_filter_into("a", 1, MJB_ENC_UTF_8, MJB_FILTER_NONE, MJB_ENC_UTF_8,
                          filter_into_output, &filter_into_size),
        MJB_STATUS_OK, "Caller-buffer filtering does not allocate")
    ATT_ASSERT(filter_into_size, (size_t)1, "Caller-buffer filtering output size")
    ATT_ASSERT((unsigned int)filter_into_output[0], (unsigned int)'a',
        "Caller-buffer filtering output")

    char normalize_into_output[3];
    size_t normalize_into_size = sizeof(normalize_into_output);

    ATT_ASSERT_STATUS(mjb_normalize_into("\xC3\xA9", 2, MJB_ENC_UTF_8,
                          MJB_NORMALIZATION_NFD, MJB_ENC_UTF_8, normalize_into_output,
                          &normalize_into_size),
        MJB_STATUS_OK, "Caller-buffer decomposition does not allocate")
    ATT_ASSERT(normalize_into_size, (size_t)3, "Caller-buffer decomposition output size")
    ATT_ASSERT((int)memcmp(normalize_into_output, "e\xCC\x81", 3), 0,
        "Caller-buffer decomposition output")

    char normalize_utf16_output[2];
    normalize_into_size = sizeof(normalize_utf16_output);
    ATT_ASSERT_STATUS(mjb_normalize_into("A", 1, MJB_ENC_UTF_8, MJB_NORMALIZATION_NFC,
                          MJB_ENC_UTF_16LE, normalize_utf16_output, &normalize_into_size),
        MJB_STATUS_OK, "Caller-buffer normalized encoding conversion does not allocate")
    ATT_ASSERT(normalize_into_size, (size_t)2,
        "Caller-buffer normalized encoding conversion output size")

    normalize_into_size = 0;
    ATT_ASSERT_STATUS(mjb_normalize_into("e\xCC\x81", 3, MJB_ENC_UTF_8,
                          MJB_NORMALIZATION_NFC, MJB_ENC_UTF_8, NULL, &normalize_into_size),
        MJB_STATUS_NO_MEMORY, "Caller-buffer composition handles temporary allocation failure")
    ATT_ASSERT(normalize_into_size, (size_t)0,
        "Caller-buffer composition clears size after allocation failure")

    filter_into_size = sizeof(filter_into_output);
    ATT_ASSERT_STATUS(mjb_filter_into("e\xCC\x81", 3, MJB_ENC_UTF_8, MJB_FILTER_NORMALIZE,
                          MJB_ENC_UTF_8, filter_into_output, &filter_into_size),
        MJB_STATUS_NO_MEMORY, "Caller-buffer normalization handles temporary allocation failure")

    size_t nfkc_casefold_into_size = 0;
    ATT_ASSERT_STATUS(mjb_nfkc_casefold_into("a", 1, MJB_ENC_UTF_8, MJB_ENC_UTF_8, NULL,
                          &nfkc_casefold_into_size),
        MJB_STATUS_NO_MEMORY, "Caller-buffer NFKC casefold handles temporary allocation failure")
    ATT_ASSERT(nfkc_casefold_into_size, (size_t)0,
        "Caller-buffer NFKC casefold clears size after allocation failure")

#if MJB_FEATURE_COLLATION
    size_t collation_key_into_size = 0;
    ATT_ASSERT_STATUS(mjb_collation_key_into("a", 1, MJB_ENC_UTF_8,
                          MJB_COLLATION_NON_IGNORABLE, MJB_COLLATION_TERTIARY, NULL,
                          &collation_key_into_size),
        MJB_STATUS_NO_MEMORY, "Caller-buffer collation key handles temporary allocation failure")
    ATT_ASSERT(collation_key_into_size, (size_t)0,
        "Caller-buffer collation key clears size after allocation failure")
#endif

#if MJB_FEATURE_SECURITY
    size_t skeleton_into_size = 0;
    ATT_ASSERT_STATUS(mjb_confusable_skeleton_into("a", 1, MJB_ENC_UTF_8, MJB_ENC_UTF_8, NULL,
                          &skeleton_into_size),
        MJB_STATUS_NO_MEMORY, "Caller-buffer skeleton handles temporary allocation failure")
    ATT_ASSERT(skeleton_into_size, (size_t)0,
        "Caller-buffer skeleton clears size after allocation failure")
#endif

    ATT_ASSERT_STATUS(mjb_convert_encoding("a", 1, MJB_ENC_UTF_8, MJB_ENC_UTF_16LE, &result),
        MJB_STATUS_NO_MEMORY, "Encoding conversion handles allocation failure")
    ATT_ASSERT_STATUS(mjb_filter("a", 1, MJB_ENC_UTF_8, MJB_FILTER_NONE, MJB_ENC_UTF_8,
                          &result),
        MJB_STATUS_NO_MEMORY, "Filter handles allocation failure")
    ATT_ASSERT_STATUS(mjb_normalize("e\xCC\x81", 3, MJB_ENC_UTF_8, MJB_NORMALIZATION_NFC,
                          MJB_ENC_UTF_8, &result),
        MJB_STATUS_NO_MEMORY, "Normalization handles allocation failure")
    ATT_ASSERT_STATUS(mjb_map_case("a", 1, MJB_ENC_UTF_8, MJB_CASE_UPPER, MJB_ENC_UTF_8, &result),
        MJB_STATUS_NO_MEMORY, "Case conversion handles allocation failure")
#if MJB_FEATURE_COLLATION
    ATT_ASSERT_STATUS(mjb_collation_key("a", 1, MJB_ENC_UTF_8, MJB_COLLATION_NON_IGNORABLE,
                          MJB_COLLATION_TERTIARY, &result),
        MJB_STATUS_NO_MEMORY, "Collation key handles allocation failure")
    int order;
    ATT_ASSERT_STATUS(mjb_collation_compare("a", 1, MJB_ENC_UTF_8, "b", 1, MJB_ENC_UTF_8,
                          MJB_COLLATION_NON_IGNORABLE, MJB_COLLATION_TERTIARY, &order),
        MJB_STATUS_NO_MEMORY, "Collation comparison handles allocation failure")
#endif
#if MJB_FEATURE_SECURITY
    bool confusable;
    ATT_ASSERT_STATUS(mjb_confusable_match("a", 1, MJB_ENC_UTF_8, "b", 1, MJB_ENC_UTF_8,
                          &confusable),
        MJB_STATUS_NO_MEMORY, "Confusable comparison handles allocation failure")
#endif

    mjb_test_allocator_reset();

    test_set_failing_allocator(1);
    ATT_ASSERT_STATUS(mjb_convert_encoding("ab", 2, MJB_ENC_UTF_8, MJB_ENC_UTF_16LE,
                          &result),
        MJB_STATUS_NO_MEMORY, "Encoding conversion handles reallocation failure")

    mjb_test_allocator_reset();

    ATT_ASSERT(mjb_status_message(MJB_STATUS_OK), "The operation completed successfully",
        "Status message returns OK")
    ATT_ASSERT(mjb_status_message((mjb_status)100), "The status code is unknown",
        "Status message returns unknown error for invalid status")

    return 0;
}
