#include "db.h"

/* Set memory allocation functions */
MJB_EXPORT bool mjb_allocation(mjb_alloc_fn alloc_fn, mjb_realloc_fn realloc_fn, mjb_free_fn free_fn) {
    /* The library is alread initialized or functions are void */
    if(mjb.ok || !alloc_fn || !realloc_fn || !free_fn) {
        return false;
    }

    mjb.memory_alloc = alloc_fn;
    mjb.memory_realloc = realloc_fn;
    mjb.memory_free = free_fn;

    return true;
}

/* Allocate memory */
MJB_EXPORT void *mjb_alloc(size_t size) {
    return mjb.memory_alloc(size);
}

/* Reallocate memory */
MJB_EXPORT void *mjb_realloc(void *ptr, size_t new_size) {
    return mjb.memory_realloc(ptr, new_size);
}

/* Free memory */
MJB_EXPORT void mjb_free(void *ptr) {
    mjb.memory_free(ptr);
}
