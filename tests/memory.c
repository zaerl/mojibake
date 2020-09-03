/**
 * The mojibake library
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

    return free(ptr);
}

MJB_EXPORT void mjb_memory_test() {
    bool result = false;
    mojibake *mjb;

    result = mjb_initialize_v2(MJB_DB_PATH, &mjb, NULL, NULL, NULL);
    mjb_assert("Void memory functions", result);

    mjb_close(mjb);

    result = mjb_initialize_v2(MJB_DB_PATH, &mjb, test_malloc, test_realloc, test_free);

    mjb_alloc(mjb, 1);
    mjb_realloc(mjb, NULL, 1);
    mjb_free(mjb, NULL);

    mjb_assert("Custom memory functions", test_counter == 4);

    mjb_close(mjb);
}
