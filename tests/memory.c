#include <stdlib.h>

#include "test.h"

static unsigned int test_counter = 0;

void *test_malloc(size_t size) {
    ++test_counter;

    return NULL;
}

void *test_realloc(void *ptr, size_t new_size) {
    ++test_counter;

    return NULL;
}

void test_free(void *ptr) {
    ++test_counter;
}

MJB_EXPORT void mjb_memory_test() {
    bool result = false;

    result = mjb_allocation(malloc, realloc, free);
    mjb_assert("Set memory before initialization", result);

    result = mjb_allocation(NULL, NULL, NULL);
    mjb_assert("Void memory functions", !result);

    result = mjb_allocation(test_malloc, test_realloc, test_free);
    mjb_alloc(1);
    mjb_realloc(NULL, 1);
    mjb_free(NULL);
    mjb_assert("Custom memory functions", test_counter == 3);

    result = mjb_initialize(MJB_DB_PATH);

    if(result) {
        result = mjb_allocation(malloc, realloc, free);
    }

    mjb_assert("Set memory after initialization", !result);
}
