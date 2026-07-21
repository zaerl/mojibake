/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdlib.h>

#include "test.h"

static unsigned int test_counter = 0;
static size_t fail_alloc_count = 0;
static size_t fail_alloc_after = 0;

void *test_malloc(size_t size) {
    ++test_counter;

    return malloc(size);
}

void *test_realloc(void *ptr, size_t new_size) {
    ++test_counter;

    return realloc(ptr, new_size);
}

void test_free(void *ptr) {
    ++test_counter;

    free(ptr);
}

void *test_fail_malloc(size_t size) {
    if(fail_alloc_count++ >= fail_alloc_after) {
        return NULL;
    }

    return malloc(size);
}

void *test_fail_realloc(void *ptr, size_t new_size) {
    if(fail_alloc_count++ >= fail_alloc_after) {
        return NULL;
    }

    return realloc(ptr, new_size);
}

static void test_set_failing_allocator(size_t fail_after) {
    mjb_reset();
    fail_alloc_count = 0;
    fail_alloc_after = fail_after;
    ATT_ASSERT_STATUS(mjb_set_memory_functions(test_fail_malloc, test_fail_realloc, test_free),
        MJB_STATUS_OK, "Set failing allocator")
}

int test_mojibake(void *arg) {
    test_counter = 0;
    mjb_result result;
    ATT_ASSERT_STATUS(mjb_result_free(NULL), MJB_STATUS_INVALID_ARGUMENT, "Free NULL result")

    result.output = (char *)malloc(1);
    result.output_size = 1;
    result.transformed = true;

    ATT_ASSERT_STATUS(mjb_result_free(&result), MJB_STATUS_OK, "Free result with output")
    ATT_ASSERT(result.output == NULL, true, "Result output NULL after free")
    ATT_ASSERT(result.output_size, 0, "Result output size 0 after free")

    ATT_ASSERT_STATUS(mjb_set_locale(MJB_LOCALE_IT), MJB_STATUS_OK, "Set locale before reset")
    ATT_ASSERT((mjb_reset(), true), true, "Reset before memory functions")
    ATT_ASSERT((unsigned int)mjb_get_locale(), (unsigned int)MJB_LOCALE_EN,
        "Reset restores default locale")
    void *implicit_buffer = NULL;
    ATT_ASSERT((implicit_buffer = mjb_alloc(1)) != NULL, true, "Default alloc before set")
    ATT_ASSERT((mjb_free(implicit_buffer), true), true, "Default free before set")
    ATT_ASSERT((mjb_reset(), true), true, "Reset implicit default memory functions")
    ATT_ASSERT((mjb_reset(), true), true, "Reset idempotent")

    ATT_ASSERT_STATUS(mjb_set_memory_functions(NULL, NULL, NULL), MJB_STATUS_OK,
        "Void memory functions")

    void *default_buffer = NULL;
    ATT_ASSERT((default_buffer = mjb_alloc(1)) != NULL, true, "Default alloc")
    ATT_ASSERT((default_buffer = mjb_realloc(default_buffer, 2)) != NULL, true, "Default realloc")
    ATT_ASSERT((mjb_free(default_buffer), true), true, "Default free")
    ATT_ASSERT((mjb_reset(), true), true, "Reset void memory functions")

    test_counter = 0;
    ATT_ASSERT_STATUS(mjb_set_memory_functions(test_malloc, test_realloc, test_free), MJB_STATUS_OK,
        "Set custom memory functions")
    void *buffer = NULL;
    ATT_ASSERT((buffer = mjb_alloc(1)) != NULL, true, "Custom alloc")
    ATT_ASSERT(test_counter, 1, "Custom alloc function")
    ATT_ASSERT((buffer = mjb_realloc(buffer, 2)) != NULL, true, "Custom realloc")
    ATT_ASSERT(test_counter, 2, "Custom realloc function")
    ATT_ASSERT((mjb_free(buffer), test_counter), 3, "Custom free function")
    ATT_ASSERT((mjb_reset(), true), true, "Reset custom memory functions")

    test_set_failing_allocator(0);

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

    size_t collation_key_into_size = 0;
    ATT_ASSERT_STATUS(mjb_collation_key_into("a", 1, MJB_ENC_UTF_8,
                          MJB_COLLATION_NON_IGNORABLE, NULL, &collation_key_into_size),
        MJB_STATUS_NO_MEMORY, "Caller-buffer collation key handles temporary allocation failure")
    ATT_ASSERT(collation_key_into_size, (size_t)0,
        "Caller-buffer collation key clears size after allocation failure")

    size_t skeleton_into_size = 0;
    ATT_ASSERT_STATUS(mjb_confusable_skeleton_into("a", 1, MJB_ENC_UTF_8, MJB_ENC_UTF_8, NULL,
                          &skeleton_into_size),
        MJB_STATUS_NO_MEMORY, "Caller-buffer skeleton handles temporary allocation failure")
    ATT_ASSERT(skeleton_into_size, (size_t)0,
        "Caller-buffer skeleton clears size after allocation failure")

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
    ATT_ASSERT_STATUS(mjb_collation_key("a", 1, MJB_ENC_UTF_8, MJB_COLLATION_NON_IGNORABLE,
                          &result),
        MJB_STATUS_NO_MEMORY, "Collation key handles allocation failure")
    int order;
    ATT_ASSERT_STATUS(mjb_collation_compare("a", 1, MJB_ENC_UTF_8, "b", 1, MJB_ENC_UTF_8,
                          MJB_COLLATION_NON_IGNORABLE, &order),
        MJB_STATUS_NO_MEMORY, "Collation comparison handles allocation failure")
    bool confusable;
    ATT_ASSERT_STATUS(mjb_are_confusable("a", 1, MJB_ENC_UTF_8, "b", 1, MJB_ENC_UTF_8,
                          &confusable),
        MJB_STATUS_NO_MEMORY, "Confusable comparison handles allocation failure")

    ATT_ASSERT((mjb_reset(), true), true, "Reset failing allocator")

    test_set_failing_allocator(1);
    ATT_ASSERT_STATUS(mjb_convert_encoding("ab", 2, MJB_ENC_UTF_8, MJB_ENC_UTF_16LE,
                          &result),
        MJB_STATUS_NO_MEMORY, "Encoding conversion handles reallocation failure")

    ATT_ASSERT((mjb_reset(), true), true, "Reset realloc failing allocator")

    return 0;
}
