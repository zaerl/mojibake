/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdlib.h>
#include <string.h>

#include "mojibake-internal.h"

MJB_EXPORT mojibake mjb_global;

// Initialize the library
MJB_EXPORT bool mjb_initialize(void) {
    if(mjb_global.ok) {
        return true;
    }

    if(mjb_initialize_v2(malloc, realloc, free)) {
        return true;
    }

    return false;
}

// Initialize the library with custom values
MJB_EXPORT bool mjb_initialize_v2(mjb_alloc_fn alloc_fn, mjb_realloc_fn realloc_fn,
    mjb_free_fn free_fn) {
    if(mjb_global.ok) {
        return true;
    }

    MJB_LOG("Mojibake initialization");

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
    mjb_global.ok = true;
    mjb_global.locale = MJB_LOCALE_EN;

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

// Allocate and zero memory
MJB_EXPORT void *mjb_alloc(size_t size) {
    if(!mjb_initialize()) {
        return NULL;
    }

    void *allocated = mjb_global.memory_alloc(size);

    if(allocated) {
        memset(allocated, 0, size);
    }

    return allocated;
}

// Reallocate memory
MJB_EXPORT void *mjb_realloc(void *ptr, size_t new_size) {
    if(!mjb_initialize()) {
        return NULL;
    }

    return mjb_global.memory_realloc(ptr, new_size);
}

// Free memory
MJB_EXPORT void mjb_free(void *ptr) {
    if(!mjb_initialize()) {
        return;
    }

    mjb_global.memory_free(ptr);
}
