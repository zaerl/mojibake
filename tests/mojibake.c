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
    mjb_shutdown();
    fail_alloc_count = 0;
    fail_alloc_after = fail_after;
    ATT_ASSERT(mjb_initialize_v2(test_fail_malloc, test_fail_realloc, test_free), true,
        "Initialize failing allocator")
}

void *test_mojibake(void *arg) {
    test_counter = 0;

    ATT_ASSERT((mjb_shutdown(), true), true, "Shutdown before initialization")
    ATT_ASSERT(mjb_initialize(), true, "Default initialize")
    ATT_ASSERT(mjb_initialize(), true, "Default initialize idempotent")
    ATT_ASSERT((mjb_shutdown(), true), true, "Shutdown default initialize")
    ATT_ASSERT((mjb_shutdown(), true), true, "Shutdown idempotent")

    ATT_ASSERT(mjb_initialize_v2(NULL, NULL, NULL), true, "Void memory functions")

    void *default_buffer = NULL;
    ATT_ASSERT((default_buffer = mjb_alloc(1)) != NULL, true, "Default alloc")
    ATT_ASSERT((default_buffer = mjb_realloc(default_buffer, 2)) != NULL, true,
        "Default realloc")
    ATT_ASSERT((mjb_free(default_buffer), true), true, "Default free")
    ATT_ASSERT((mjb_shutdown(), true), true, "Shutdown void memory functions")

    test_counter = 0;
    ATT_ASSERT(mjb_initialize_v2(test_malloc, test_realloc, test_free), true, "Valid initialize")
    void *buffer = NULL;
    ATT_ASSERT((buffer = mjb_alloc(1)) != NULL, true, "Custom alloc")
    ATT_ASSERT(test_counter, 1, "Custom alloc function")
    ATT_ASSERT((buffer = mjb_realloc(buffer, 2)) != NULL, true, "Custom realloc")
    ATT_ASSERT(test_counter, 2, "Custom realloc function")
    ATT_ASSERT((mjb_free(buffer), test_counter), 3, "Custom free function")
    ATT_ASSERT((mjb_shutdown(), true), true, "Shutdown custom memory functions")

    test_set_failing_allocator(0);

    mjb_result result;
    ATT_ASSERT_STATUS(mjb_string_convert_encoding("a", 1, MJB_ENCODING_UTF_8,
        MJB_ENCODING_UTF_16_LE, &result), MJB_STATUS_NO_MEMORY,
        "Encoding conversion handles allocation failure")
    ATT_ASSERT_STATUS(mjb_string_filter("a", 1, MJB_ENCODING_UTF_8, MJB_ENCODING_UTF_8,
        MJB_FILTER_NONE, &result), MJB_STATUS_NO_MEMORY, "Filter handles allocation failure")
    ATT_ASSERT_STATUS(mjb_normalize("e\xCC\x81", 3, MJB_ENCODING_UTF_8, MJB_NORMALIZATION_NFC,
        MJB_ENCODING_UTF_8, &result), MJB_STATUS_NO_MEMORY,
        "Normalization handles allocation failure")
    ATT_ASSERT(mjb_case("a", 1, MJB_CASE_UPPER, MJB_ENCODING_UTF_8), (char*)NULL,
        "Case conversion handles allocation failure")
    ATT_ASSERT(mjb_collation_key("a", 1, MJB_ENCODING_UTF_8, MJB_COLLATION_NON_IGNORABLE,
        &result), false, "Collation key handles allocation failure")

    ATT_ASSERT((mjb_shutdown(), true), true, "Shutdown failing allocator")

    return NULL;
}
