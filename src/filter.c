/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "mojibake-internal.h"
#include "breaking.h"
#include "utf.h"

extern mojibake mjb_global;

MJB_EXPORT bool mjb_string_filter(const char *buffer, size_t size, mjb_encoding encoding,
    mjb_encoding output_encoding, mjb_filter filters, mjb_result *result) {
    if(!mjb_initialize()) {
        return false;
    }

    if(filters == MJB_FILTER_NONE || size == 0) {
        result->output = (char*)buffer;
        result->output_size = size;
        result->transformed = false;

        return true;
    }

    bool is_normalized = false;

    if(filters & MJB_FILTER_NORMALIZE) {
        if(!mjb_normalize(buffer, size, encoding, MJB_NORMALIZATION_NFC, result)) {
            return false;
        }

        is_normalized = result->transformed;
    }

    // Only normalize, no filtering.
    if(filters == MJB_FILTER_NORMALIZE) {
        return true;
    }

    if(is_normalized) {
        buffer = result->output;
        size = result->output_size;
    }

    uint8_t state = MJB_UTF_ACCEPT;
    mjb_codepoint codepoint = 0;
    mjb_character character;

    char *output = (char*)mjb_alloc(size);
    size_t output_size = size;
    size_t output_index = 0;
    result->transformed = true;

    for(size_t i = 0; i < size; ++i) {
        if(!mjb_decode_step(buffer, size, &state, &i, encoding, &codepoint)) {
            break;
        }

        bool replaced = false;

        if(state == MJB_UTF_REJECT) {
            codepoint = MJB_CODEPOINT_REPLACEMENT;
            replaced = true;
        }

        if(state == MJB_UTF_ACCEPT || replaced) {
            // Get current character.
            if(!mjb_codepoint_character(codepoint, &character)) {
                continue;
            }

            if(filters & MJB_FILTER_SPACES) {
                if(character.category == MJB_CATEGORY_ZS) {
                    // Transform all space characters to ASCII space.
                    codepoint = 0x20;
                }
            }

            char *new_output = mjb_string_output_codepoint(codepoint, output, &output_index,
                &output_size, output_encoding);

            if(new_output != NULL) {
                output = new_output;
            } else {
                // TODO: check if this is the correct behavior
                return false;
            }
        }
    }

    if(is_normalized) {
        mjb_free(result->output);
    }

    result->output = output;
    result->output_size = output_index;

    return true;
}
