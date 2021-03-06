/**
 * The mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#ifndef MJB_DB_H
#define MJB_DB_H

#include "mojibake.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mojibake {
    bool ok;
    mjb_alloc_fn memory_alloc;
    mjb_realloc_fn memory_realloc;
    mjb_free_fn memory_free;
};

#ifdef __cplusplus
}
#endif

#endif /* MJB_DB_H */
