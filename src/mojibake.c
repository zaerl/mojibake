/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdlib.h>
#include <string.h>

#include "mojibake-internal.h"

// Default allocators are valid before any explicit memory-function override.
mojibake mjb_global = { false, malloc, realloc, free, MJB_LOCALE_EN };

// Set the library memory functions.
MJB_EXPORT mjb_status mjb_set_memory_functions(mjb_alloc_fn alloc_fn, mjb_realloc_fn realloc_fn,
    mjb_free_fn free_fn) {
    if(mjb_global.memory_functions_locked) {
        return MJB_STATUS_OK;
    }

    if(alloc_fn == NULL) {
        alloc_fn = malloc;
    }

    if(realloc_fn == NULL) {
        realloc_fn = realloc;
    }

    if(free_fn == NULL) {
        free_fn = free;
    }

    mjb_global.memory_alloc = alloc_fn;
    mjb_global.memory_realloc = realloc_fn;
    mjb_global.memory_free = free_fn;
    mjb_global.memory_functions_locked = true;

    return MJB_STATUS_OK;
}

MJB_EXPORT void mjb_shutdown(void) {
    mjb_global.memory_functions_locked = false;
    mjb_global.memory_free = free;
    mjb_global.memory_realloc = realloc;
    mjb_global.memory_alloc = malloc;
    mjb_global.locale = MJB_LOCALE_EN;
}

// Allocate and zero memory
MJB_EXPORT void *mjb_alloc(size_t size) {
    mjb_global.memory_functions_locked = true;
    void *allocated = mjb_global.memory_alloc(size);

    if(allocated) {
        memset(allocated, 0, size);
    }

    return allocated;
}

// Reallocate memory
MJB_EXPORT void *mjb_realloc(void *ptr, size_t new_size) {
    mjb_global.memory_functions_locked = true;

    return mjb_global.memory_realloc(ptr, new_size);
}

// Free memory
MJB_EXPORT void mjb_free(void *ptr) {
    mjb_global.memory_functions_locked = true;
    mjb_global.memory_free(ptr);
}
