/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdlib.h>
#include <string.h>

#include "mojibake-internal.h"

static void *mjb_default_alloc(void *context, size_t size) {
    (void)context;

    return malloc(size);
}

static void *mjb_default_realloc(void *context, void *ptr, size_t new_size) {
    (void)context;

    return realloc(ptr, new_size);
}

static void mjb_default_free(void *context, void *ptr) {
    (void)context;

    free(ptr);
}

// The default allocator is usable without explicit configuration. Custom configuration is copied
// once before any other library call and remains immutable for the process lifetime.
mojibake mjb_global = { { NULL, mjb_default_alloc, mjb_default_realloc, mjb_default_free }, false,
    MJB_LOCALE_EN };

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

// Reallocate memory
MJB_LOCAL void *mjb_realloc(void *ptr, size_t new_size) {
    return mjb_global.allocator.realloc(mjb_global.allocator.context, ptr, new_size);
}

// Free memory
MJB_LOCAL void mjb_free(void *ptr) {
    mjb_global.allocator.free(mjb_global.allocator.context, ptr);
}

// Free a mjb_result.
MJB_EXPORT mjb_status mjb_result_free(mjb_result *result) {
    if(result == NULL) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    if(result->transformed && result->output != NULL) {
        mjb_free(result->output);
        result->output = NULL;
        result->output_size = 0;
    }

    result->transformed = false;

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
        "The requested feature was disabled when the library was built"
    };

    if((unsigned int)status < (sizeof(messages) / sizeof(messages[0]))) {
        message = messages[status];
    }

    return message;
}
