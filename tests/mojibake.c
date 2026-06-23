/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdlib.h>

#include "test.h"

static unsigned int test_counter = 0;

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

    return NULL;
}
