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

#include "break.h"
#include "buffer.h"
#include "locales.h"
#include "segmentation.h"
#include "sqlite3/sqlite3.h"

#include "mojibake.h"

#define MJB_USED __attribute__((used))

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
    mjb_locale locale;
    sqlite3_stmt *stmt_get_codepoint;
    sqlite3_stmt *stmt_get_block;
    sqlite3_stmt *stmt_is_combining;
    sqlite3_stmt *stmt_decompose;
    sqlite3_stmt *stmt_compatibility_decompose;
    sqlite3_stmt *stmt_compose;
    sqlite3_stmt *stmt_buffer_character;
    sqlite3_stmt *stmt_case;
    sqlite3_stmt *stmt_special_casing;
    sqlite3_stmt *stmt_line_breaking_class;
} mojibake;

// Internal functions
MJB_NONNULL(1, 2, 4, 5) char *mjb_string_output(char *ret, char *input, size_t input_size,
    size_t *output_index, size_t *output_size);
MJB_NONNULL(2, 3) char *mjb_string_output_codepoint(mjb_codepoint codepoint, char *ret, size_t
    *output_index, size_t *output_size);
MJB_NONNULL(2) bool mjb_codepoint_line_breaking_class(mjb_codepoint codepoint,
    mjb_line_breaking_class *line_breaking_class);

#endif // MJB_MOJIBAKE_INTERNAL_H
