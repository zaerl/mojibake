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

// Internal functions
char *mjb_string_output(char *ret, char *input, size_t input_size, size_t *output_index,
    size_t *output_size);

char *mjb_string_output_codepoint(mjb_codepoint codepoint, char *ret, size_t *output_index,
    size_t *output_size, mjb_encoding encoding);

// A smaller version of mjb_character that only contains the information needed for the
// normalization process.
typedef struct mjb_n_character {
    mjb_codepoint codepoint;
    uint8_t combining;
    uint8_t decomposition;
    uint16_t quick_check;
} mjb_n_character;

/**
 * A smaller version of mjb_codepoint_character() that only returns the character information.
 * This is used to avoid the overhead of the full normalization process.
 */
bool mjb_n_codepoint_character(mjb_codepoint codepoint, mjb_n_character *character);

mjb_status mjb_codepoint_properties_lookup(mjb_codepoint codepoint, uint8_t *buffer);

uint8_t mjb_codepoint_properties_get(const uint8_t *properties, mjb_property property);

bool mjb_codepoint_has_binary_property(mjb_codepoint codepoint, mjb_property property);

#endif // MJB_MOJIBAKE_INTERNAL_H
