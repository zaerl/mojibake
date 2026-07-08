/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdlib.h>
#include <string.h>

#include "mojibake-internal.h"

#if defined(_WIN32) || defined(_WIN64)
static void *mjb_default_alloc(size_t size) {
    return malloc(size);
}

static void *mjb_default_realloc(void *ptr, size_t new_size) {
    return realloc(ptr, new_size);
}

static void mjb_default_free(void *ptr) {
    free(ptr);
}

// warning C4232: nonstandard extension used: 'memory_alloc': address of dllimport 'malloc' is not
// static, identity not guaranteed
#define MJB_DEFAULT_ALLOC mjb_default_alloc
#define MJB_DEFAULT_REALLOC mjb_default_realloc
#define MJB_DEFAULT_FREE mjb_default_free
#else
#define MJB_DEFAULT_ALLOC malloc
#define MJB_DEFAULT_REALLOC realloc
#define MJB_DEFAULT_FREE free
#endif

// Default allocators are valid before any explicit memory-function override.
mojibake mjb_global = { false, MJB_DEFAULT_ALLOC, MJB_DEFAULT_REALLOC, MJB_DEFAULT_FREE,
    MJB_LOCALE_EN };

// Set the library memory functions.
MJB_EXPORT mjb_status mjb_set_memory_functions(mjb_alloc_fn alloc_fn, mjb_realloc_fn realloc_fn,
    mjb_free_fn free_fn) {
    if(mjb_global.memory_functions_locked) {
        return MJB_STATUS_OK;
    }

    if(alloc_fn == NULL) {
        alloc_fn = MJB_DEFAULT_ALLOC;
    }

    if(realloc_fn == NULL) {
        realloc_fn = MJB_DEFAULT_REALLOC;
    }

    if(free_fn == NULL) {
        free_fn = MJB_DEFAULT_FREE;
    }

    mjb_global.memory_alloc = alloc_fn;
    mjb_global.memory_realloc = realloc_fn;
    mjb_global.memory_free = free_fn;
    mjb_global.memory_functions_locked = true;

    return MJB_STATUS_OK;
}

MJB_EXPORT void mjb_shutdown(void) {
    mjb_global.memory_functions_locked = false;
    mjb_global.memory_free = MJB_DEFAULT_FREE;
    mjb_global.memory_realloc = MJB_DEFAULT_REALLOC;
    mjb_global.memory_alloc = MJB_DEFAULT_ALLOC;
    mjb_global.locale = MJB_LOCALE_EN;
}

// Allocate memory
MJB_EXPORT void *mjb_alloc(size_t size) {
    mjb_global.memory_functions_locked = true;
    void *allocated = mjb_global.memory_alloc(size);

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
