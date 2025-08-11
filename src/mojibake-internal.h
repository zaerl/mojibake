/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#pragma once

#ifndef MJB_MOJIBAKE_INTERNAL_H
#define MJB_MOJIBAKE_INTERNAL_H

#include <stdbool.h>
#include <stddef.h>
#include "sqlite3/sqlite3.h"

#include "mojibake.h"

/**
 * Internal mojibake structure
 * This contains the internal state of the library
 */
typedef struct mojibake {
    bool ok;
    mjb_alloc_fn memory_alloc;
    mjb_realloc_fn memory_realloc;
    mjb_free_fn memory_free;
    sqlite3 *db;
    sqlite3_stmt *stmt_get_codepoint;
    sqlite3_stmt *stmt_get_block;
    sqlite3_stmt *stmt_is_combining;
    sqlite3_stmt *stmt_decompose;
    sqlite3_stmt *stmt_compatibility_decompose;
    sqlite3_stmt *stmt_compose;
    sqlite3_stmt *stmt_buffer_character;
    sqlite3_stmt *stmt_case;
    sqlite3_stmt *stmt_special_casing;
} mojibake;

// Internal functions
MJB_NONNULL(1, 2, 4, 5) char *mjb_string_output(char *ret, char *input, size_t input_size, size_t *output_index, size_t *output_size);

#endif // MJB_MOJIBAKE_INTERNAL_H
