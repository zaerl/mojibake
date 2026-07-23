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

#define MJB_UTF_ACCEPT 0
#define MJB_UTF_PENDING_SURROGATE 0xD
#define MJB_UTF_TERMINATED 0xE
#define MJB_UTF_REJECT 0xF

/**
 * Internal mojibake structure
 * This contains the internal state of the library
 */
typedef struct mojibake {
    bool memory_functions_locked;
    mjb_alloc_fn memory_alloc;
    mjb_realloc_fn memory_realloc;
    mjb_free_fn memory_free;
    mjb_locale locale;
} mojibake;

// Shared output sink for transformations that can allocate, measure, or write into a fixed
// caller-provided buffer.
typedef enum mjb_output_mode {
    MJB_OUTPUT_DYNAMIC,
    MJB_OUTPUT_MEASURE,
    MJB_OUTPUT_FIXED
} mjb_output_mode;

typedef struct mjb_output {
    char *buffer;
    size_t size;
    size_t capacity;
    mjb_output_mode mode;
} mjb_output;

// A smaller version of mjb_character that only contains the information needed for the
// normalization process.
typedef struct mjb_n_character {
    mjb_codepoint codepoint;
    uint8_t combining;
    uint8_t decomposition;
    uint16_t quick_check;
} mjb_n_character;

typedef mjb_status (*mjb_output_writer)(mjb_output *output, const void *context);

// Internal functions
char *mjb_string_output(char *ret, char *input, size_t input_size, size_t *output_index,
    size_t *output_size);

char *mjb_string_output_codepoint(mjb_codepoint codepoint, char *ret, size_t *output_index,
    size_t *output_size, mjb_encoding encoding);

void mjb_output_init_dynamic(mjb_output *output, char *buffer, size_t capacity);

mjb_status mjb_output_write(mjb_output *output, const void *buffer, size_t byte_length);

mjb_status mjb_output_codepoint(mjb_output *output, mjb_codepoint codepoint, mjb_encoding encoding);

mjb_status mjb_output_into(void *output, size_t *output_size, mjb_output_writer writer,
    const void *context);

mjb_status mjb_output_copy_into(const void *buffer, size_t byte_length, void *output,
    size_t *output_size);

mjb_status mjb_casefold_default(const char *buffer, size_t byte_length, mjb_encoding encoding,
    mjb_encoding output_encoding, mjb_result *result);

size_t mjb_grapheme_prefix_bytes(const char *buffer, size_t byte_length, mjb_encoding encoding,
    size_t max_bytes);

bool mjb_n_codepoint_character(mjb_codepoint codepoint, mjb_n_character *character);

mjb_status mjb_codepoint_properties_lookup(mjb_codepoint codepoint, uint8_t *buffer);

uint8_t mjb_codepoint_properties_get(const uint8_t *properties, mjb_property property);

bool mjb_codepoint_has_binary_property(mjb_codepoint codepoint, mjb_property property);

#endif // MJB_MOJIBAKE_INTERNAL_H
