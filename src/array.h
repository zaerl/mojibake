/**
 * The mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#ifndef MJB_ARRAY_H
#define MJB_ARRAY_H

#include "mojibake.h"
#include "db.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mjb_array {
    char *buffer;
    size_t element_size;
    size_t used;
    size_t size;
} mjb_array;

MJB_EXPORT bool mjb_array_init(mojibake *mjb, mjb_array *array, size_t element_size, size_t count);

MJB_EXPORT bool mjb_array_push(mojibake *mjb, mjb_array *array, char *element);

MJB_EXPORT char *mjb_array_at(mojibake *mjb, mjb_array *array, size_t index);

MJB_EXPORT bool mjb_array_set(mojibake *mjb, mjb_array *array, char *element, size_t index);

MJB_EXPORT bool mjb_array_free(mojibake *mjb, mjb_array *array);

#ifdef __cplusplus
}
#endif

#endif /* MJB_ARRAY_H */
