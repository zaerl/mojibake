/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "mojibake-internal.h"

static bool mjb_utf8_is_continuation(uint8_t byte) {
    return (byte & 0xC0) == 0x80;
}

static size_t mjb_utf8_complete_prefix_length(const char *buffer, size_t byte_length) {
    if(buffer == NULL || byte_length == 0) {
        return 0;
    }

    size_t sequence_start = byte_length;

    while(sequence_start > 0 && mjb_utf8_is_continuation((uint8_t)buffer[sequence_start - 1])) {
        --sequence_start;
    }

    if(sequence_start == 0) {
        // A continuation byte without a leading byte is malformed interior data, not a suffix
        // created by truncating an otherwise well-formed result.
        return byte_length;
    }

    size_t leading_index = sequence_start - 1;
    uint8_t leading = (uint8_t)buffer[leading_index];
    size_t expected_length = 0;

    if(leading >= 0xC2 && leading <= 0xDF) {
        expected_length = 2;
    } else if(leading >= 0xE0 && leading <= 0xEF) {
        expected_length = 3;
    } else if(leading >= 0xF0 && leading <= 0xF4) {
        expected_length = 4;
    } else {
        // ASCII ends at a codepoint boundary. Other values are malformed input and are preserved.
        return byte_length;
    }

    size_t available_length = byte_length - leading_index;

    return available_length < expected_length ? leading_index : byte_length;
}

MJB_EXPORT int mjb_utf8_vsnprintf(char *buffer, size_t buffer_size, const char *format,
    va_list args) {
    int required = vsnprintf(buffer, buffer_size, format, args);

    if(required >= 0 && buffer_size > 0 && (size_t)required >= buffer_size) {
        size_t stored = buffer_size - 1;
        size_t complete = mjb_utf8_complete_prefix_length(buffer, stored);
        buffer[complete] = '\0';
    }

    return required;
}

MJB_EXPORT int mjb_utf8_snprintf(char *buffer, size_t buffer_size, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int required = mjb_utf8_vsnprintf(buffer, buffer_size, format, args);
    va_end(args);

    return required;
}

static int mjb_utf8_grapheme_format_error(char *buffer, size_t buffer_size, int error) {
    if(buffer != NULL && buffer_size > 0) {
        buffer[0] = '\0';
    }

    return error;
}

MJB_EXPORT int mjb_utf8_grapheme_vsnprintf(char *buffer, size_t buffer_size, const char *format,
    va_list args) {
    va_list truncated_args;
    va_copy(truncated_args, args);
    int required = mjb_utf8_vsnprintf(buffer, buffer_size, format, truncated_args);
    va_end(truncated_args);

    if(required < 0 || buffer_size == 0 || (size_t)required < buffer_size || buffer_size == 1) {
        return required;
    }

    size_t complete_size = (size_t)required + 1;
    char *complete = (char *)mjb_alloc(complete_size);

    if(complete == NULL) {
        errno = ENOMEM;

        return mjb_utf8_grapheme_format_error(buffer, buffer_size, -1);
    }

    va_list complete_args;
    va_copy(complete_args, args);
    int complete_required = mjb_utf8_vsnprintf(complete, complete_size, format, complete_args);
    va_end(complete_args);

    if(complete_required != required) {
        mjb_free(complete);

        if(complete_required >= 0) {
            errno = EINVAL;
            complete_required = -1;
        }

        return mjb_utf8_grapheme_format_error(buffer, buffer_size, complete_required);
    }

    size_t prefix_length = mjb_grapheme_prefix_bytes(complete, (size_t)required, MJB_ENC_UTF_8,
        buffer_size - 1);
    memcpy(buffer, complete, prefix_length);
    buffer[prefix_length] = '\0';
    mjb_free(complete);

    return required;
}

MJB_EXPORT int mjb_utf8_grapheme_snprintf(char *buffer, size_t buffer_size, const char *format,
    ...) {
    va_list args;
    va_start(args, format);
    int required = mjb_utf8_grapheme_vsnprintf(buffer, buffer_size, format, args);
    va_end(args);

    return required;
}
