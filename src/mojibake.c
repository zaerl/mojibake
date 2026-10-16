/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdlib.h>
#include <string.h>

#include "mojibake-internal.h"

#ifdef __EMSCRIPTEN__
// src/api/index.ts writes mjb_text into WASM memory as three consecutive 32-bit fields
MJB_STATIC_ASSERT(sizeof(mjb_text) == 12, "mjb_text must be 12 bytes on wasm32");
MJB_STATIC_ASSERT(offsetof(mjb_text, byte_length) == 4, "mjb_text byte_length must be at 4");
MJB_STATIC_ASSERT(offsetof(mjb_text, encoding) == 8, "mjb_text encoding must be at 8");
// and reads mjb_result back as output, output_size, capacity, and the owned flag
MJB_STATIC_ASSERT(sizeof(mjb_result) == 16, "mjb_result must be 16 bytes on wasm32");
MJB_STATIC_ASSERT(offsetof(mjb_result, capacity) == 8, "mjb_result capacity must be at 8");
MJB_STATIC_ASSERT(offsetof(mjb_result, owned) == 12, "mjb_result owned must be at 12");
// and reads the 32-bit state.position of every break state at byte 8 of a 256-byte state
MJB_STATIC_ASSERT(offsetof(mjb_grapheme_break_state, position) == 8,
    "mjb_grapheme_break_state position must be at 8");
MJB_STATIC_ASSERT(offsetof(mjb_line_break_state, position) == 8,
    "mjb_line_break_state position must be at 8");
MJB_STATIC_ASSERT(offsetof(mjb_word_break_state, position) == 8,
    "mjb_word_break_state position must be at 8");
MJB_STATIC_ASSERT(offsetof(mjb_sentence_break_state, position) == 8,
    "mjb_sentence_break_state position must be at 8");
MJB_STATIC_ASSERT(sizeof(mjb_grapheme_break_state) <= 256 && sizeof(mjb_line_break_state) <= 256 &&
        sizeof(mjb_word_break_state) <= 256 && sizeof(mjb_sentence_break_state) <= 256,
    "break states must fit in 256 bytes");
#endif

// These wrappers adapt the context-aware callback signatures. On MSVC they also avoid taking the
// address of imported CRT allocation functions, which triggers warning C4232.
static void *mjb_default_alloc(void *context, size_t size) {
    (void)context;

    return malloc(size);
}

// The C library tracks block sizes itself, so the default allocator ignores the passed sizes.
static void *mjb_default_realloc(void *context, void *ptr, size_t old_size, size_t new_size) {
    (void)context;
    (void)old_size;

    return realloc(ptr, new_size);
}

static void mjb_default_free(void *context, void *ptr, size_t size) {
    (void)context;
    (void)size;

    free(ptr);
}

// The default allocator is usable without explicit configuration. Custom configuration is copied
// once before any other library call and remains immutable for the process lifetime.
mojibake mjb_global = { { NULL, mjb_default_alloc, mjb_default_realloc, mjb_default_free }, false };

// Set the process-global allocator once, before any other library call.
MJB_EXPORT mjb_status mjb_set_allocator(const mjb_allocator *allocator) {
    if(mjb_global.allocator_configured) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    if(allocator != NULL) {
        if(allocator->alloc == NULL || allocator->realloc == NULL || allocator->free == NULL) {
            return MJB_STATUS_INVALID_ARGUMENT;
        }

        mjb_global.allocator = *allocator;
    }

    mjb_global.allocator_configured = true;

    return MJB_STATUS_OK;
}

// Allocate memory
MJB_LOCAL void *mjb_alloc(size_t size) {
    return mjb_global.allocator.alloc(mjb_global.allocator.context, size);
}

// Reallocate memory. `old_size` is the size of the block at `ptr`, or 0 when `ptr` is NULL.
MJB_LOCAL void *mjb_realloc(void *ptr, size_t old_size, size_t new_size) {
    return mjb_global.allocator.realloc(mjb_global.allocator.context, ptr,
        ptr == NULL ? 0 : old_size, new_size);
}

// Free memory. `size` is the size of the block at `ptr`. Freeing NULL does nothing.
MJB_LOCAL void mjb_free(void *ptr, size_t size) {
    if(ptr != NULL) {
        mjb_global.allocator.free(mjb_global.allocator.context, ptr, size);
    }
}

// Make `output` an owned result. `capacity` is the size of the allocation behind `output`.
MJB_LOCAL void mjb_result_own(mjb_result *result, char *output, size_t output_size,
    size_t capacity) {
    result->output = output;
    result->output_size = output_size;
    result->capacity = capacity;
    result->owned = true;
}

// Make `output` a result that borrows the caller's input.
MJB_LOCAL void mjb_result_borrow(mjb_result *result, const char *output, size_t output_size) {
    result->output = (char *)output;
    result->output_size = output_size;
    result->capacity = 0;
    result->owned = false;
}

// Free a mjb_result.
MJB_EXPORT mjb_status mjb_result_free(mjb_result *result) {
    if(result == NULL) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    if(result->owned && result->output != NULL) {
        mjb_free(result->output, result->capacity);
        result->output = NULL;
        result->output_size = 0;
    }

    result->capacity = 0;
    result->owned = false;

    return MJB_STATUS_OK;
}

MJB_EXPORT MJB_CONST const char *mjb_status_message(mjb_status status) {
    const char *message = "The status code is unknown";

    static const char *const messages[] = { // MJB_STATUS_OK
        "The operation completed successfully",
        // MJB_STATUS_INVALID_ARGUMENT
        "One or more arguments are invalid or inconsistent with the requested operation",
        // MJB_STATUS_INVALID_ENCODING
        "The encoding is invalid or missing required byte-order information",
        // MJB_STATUS_INVALID_CODEPOINT,
        "The codepoint is not a valid Unicode scalar value",
        // MJB_STATUS_INVALID_FORM
        "The normalization form is invalid",
        // MJB_STATUS_UNSUPPORTED
        "The requested operation, conversion, or value is not supported",
        // MJB_STATUS_NO_MEMORY
        "Memory allocation failed",
        // MJB_STATUS_OVERFLOW
        "The required size exceeds the supported range",
        // MJB_STATUS_MALFORMED_INPUT
        "The input contains a malformed code-unit sequence",
        // MJB_STATUS_OUTPUT_TOO_SMALL
        "The output buffer is too small for the complete result",
        // MJB_STATUS_CALLBACK_STOPPED
        "The callback requested that iteration stop",
        // MJB_STATUS_NOT_FOUND,
        "No Unicode data was found for the requested value",
        // MJB_STATUS_FEATURE_NOT_ENABLED
        "The requested feature was disabled when the library was built",
        // MJB_STATUS_END_OF_INPUT
        "The end of the input was reached"
    };

    if((unsigned int)status < (sizeof(messages) / sizeof(messages[0]))) {
        message = messages[status];
    }

    return message;
}
