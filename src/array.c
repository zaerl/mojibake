/**
 * The mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "array.h"
#include <string.h>

MJB_EXPORT bool mjb_array_init(mojibake *mjb, mjb_array *array, size_t element_size, size_t count) {
    if(array == NULL || element_size == 0 || !mjb_ready(mjb)) {
        return false;
    }

    char *buffer = mjb_alloc(mjb, count * element_size);

    if(!buffer) {
        return false;
    }

    array->buffer = buffer;
    array->element_size = element_size;
    array->used = 0;
    array->size = count;

    return true;
}

MJB_EXPORT bool mjb_array_push(mojibake *mjb, mjb_array *array, char *element) {
    if(array == NULL || !mjb_ready(mjb)) {
        return false;
    }

    if(array->used == array->size) {
        size_t new_size = array->size * 2;
        array->buffer = mjb_realloc(mjb, array->buffer, new_size * array->element_size);

        if(!array->buffer) {
            return false;
        }

        /* Zero new memory */
        // memset(array->buffer + array->size * array->element_size, 0, array->size * array->element_size);
        array->size = new_size;
    }

    memcpy(array->buffer + (array->used * array->element_size), element, array->element_size);
    ++array->used;

    return true;
}

MJB_EXPORT char *mjb_array_at(mojibake *mjb, mjb_array *array, size_t index) {
    if(array == NULL || !mjb_ready(mjb) || array->size == 0 || (index * array->element_size) >= array->size) {
        return NULL;
    }

    return array->buffer + (array->used * array->element_size);
}

MJB_EXPORT bool mjb_array_set(mojibake *mjb, mjb_array *array, char *element, size_t index) {
    if(array == NULL || !mjb_ready(mjb) || (index * array->element_size) >= array->used) {
        return false;
    }

    memcpy(array->buffer + (index * array->element_size), element, array->element_size);

    return true;
}

MJB_EXPORT bool mjb_array_free(mojibake *mjb, mjb_array *array) {
    if(array == NULL || !mjb_ready(mjb)) {
        return false;
    }

    mjb_free(mjb, array->buffer);

    array->buffer = NULL;
    array->used = 0;
    array->size = 0;

    return true;
}
