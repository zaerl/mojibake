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
    mjb_shutdown();
    ATT_ASSERT(mjb_initialize_v2(NULL, NULL, NULL), false, "Void memory functions");

    mjb_shutdown();
    mjb_initialize_v2(test_malloc, test_realloc, test_free);
    mjb_alloc(1);
    mjb_realloc(NULL, 1);
    mjb_free(NULL);

    // CURRENT_ASSERT mjb_alloc
    ATT_ASSERT(test_counter, 3, "Custom memory functions");

    return NULL;
}
