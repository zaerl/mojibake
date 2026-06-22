/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#pragma once

#ifndef MJB_MOJIBAKE_INTERNAL_H
#define MJB_MOJIBAKE_INTERNAL_H

#ifdef __EMSCRIPTEN__
    #include <emscripten.h>
    #define MJB_LOG(msg) emscripten_log(EM_LOG_CONSOLE, msg)
    #define MJB_LOG_VA(msg, ...) emscripten_log(EM_LOG_CONSOLE, msg, __VA_ARGS__)
#else
    #define MJB_LOG(msg) ((void)0)
    #define MJB_LOG_VA(msg, ...) ((void)0)
#endif

#include <stdbool.h>
#include <stddef.h>

#include "buffer.h"
#include "locales.h"

#include "mojibake.h"

#if defined(_MSC_VER)
    #define MJB_USED
#else
    #define MJB_USED __attribute__((used))
#endif

#define	MJB_UTF_ACCEPT 0
#define	MJB_UTF_REJECT 0xF

/**
 * Internal mojibake structure
 * This contains the internal state of the library
 */
typedef struct mojibake {
    bool ok;
    mjb_alloc_fn memory_alloc;
    mjb_realloc_fn memory_realloc;
    mjb_free_fn memory_free;
    mjb_locale locale;
} mojibake;

// Internal functions
MJB_NONNULL(1, 2, 4, 5) char *mjb_string_output(char *ret, char *input, size_t input_size,
    size_t *output_index, size_t *output_size);
MJB_NONNULL(2, 3) char *mjb_string_output_codepoint(mjb_codepoint codepoint, char *ret, size_t
    *output_index, size_t *output_size, mjb_encoding encoding);

#endif // MJB_MOJIBAKE_INTERNAL_H
