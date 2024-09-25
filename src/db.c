/**
 * The mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mojibake.h"

struct mojibake {
    bool ok;
    mjb_alloc_fn memory_alloc;
    mjb_realloc_fn memory_realloc;
    mjb_free_fn memory_free;
};

static mojibake mjb_global = { false, NULL, NULL, NULL };

// Initialize the library
MJB_EXPORT bool mjb_initialize(void) {
    if(mjb_global.ok) {
        return true;
    }

    return mjb_initialize_v2(malloc, realloc, free);
}

// Initialize the library with custom values
MJB_EXPORT bool mjb_initialize_v2(mjb_alloc_fn alloc_fn, mjb_realloc_fn realloc_fn, mjb_free_fn free_fn) {
    if(mjb_global.ok) {
        return true;
    }

    if(alloc_fn == NULL || realloc_fn == NULL || free_fn == NULL) {
        return false;
    }

    mjb_global.memory_alloc = alloc_fn;
    mjb_global.memory_realloc = realloc_fn;
    mjb_global.memory_free = free_fn;

    mjb_global.ok = true;

    return true;
}

MJB_EXPORT void mjb_shutdown(void) {
    if(!mjb_global.ok) {
        return;
    }

    mjb_global.ok = false;
    mjb_global.memory_free = NULL;
    mjb_global.memory_realloc = NULL;
    mjb_global.memory_alloc = NULL;
}

/* Allocate and zero memory*/
MJB_EXPORT void *mjb_alloc(size_t size) {
    mjb_initialize();
    void *allocated = mjb_global.memory_alloc(size);

    if(allocated) {
        memset(allocated, 0, size);
    }

    return allocated;
}

/* Reallocate memory */
MJB_EXPORT void *mjb_realloc(void *ptr, size_t new_size) {
    mjb_initialize();

    return mjb_global.memory_realloc(ptr, new_size);
}

/* Free memory */
MJB_EXPORT void mjb_free(void *ptr) {
    mjb_initialize();
    mjb_global.memory_free(ptr);
}
