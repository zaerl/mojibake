/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "mojibake-internal.h"
#include "utf.h"

// Return true if the string is a valid Unicode identifier (UAX#31).
MJB_EXPORT bool mjb_is_identifier(const mjb_text *text, mjb_identifier_profile profile) {
    if(text == NULL) {
        return false;
    }

    const char *buffer = text->buffer;
    size_t byte_length = text->byte_length;
    mjb_encoding encoding = text->encoding;

    if(buffer == NULL || byte_length == 0) {
        return false;
    }

    if(mjb_resolve_input_byte_length(buffer, &byte_length, encoding) != MJB_STATUS_OK ||
        byte_length == 0) {
        return false;
    }

    mjb_property start_prop = (profile == MJB_IDENTIFIER_NFKC) ? MJB_PR_XID_START : MJB_PR_ID_START;
    // clang-format off
    mjb_property cont_prop = (profile == MJB_IDENTIFIER_NFKC) ? MJB_PR_XID_CONTINUE :
        MJB_PR_ID_CONTINUE;
    // clang-format on

    mjb_codepoint codepoint = 0;
    bool first = true;
    size_t i = 0;

    if(!mjb_decode_begin(buffer, byte_length, &encoding, &i)) {
        return false;
    }

    for(;;) {
        mjb_status dr = mjb_decode_next_raw(buffer, byte_length, encoding, &i, &codepoint, NULL);

        if(dr == MJB_STATUS_END_OF_INPUT) {
            break;
        }

        if(dr != MJB_STATUS_OK) {
            return false;
        }

        if(first) {
            if(!mjb_codepoint_has_binary_property(codepoint, start_prop)) {
                return false;
            }

            first = false;
        } else {
            if(!mjb_codepoint_has_binary_property(codepoint, cont_prop)) {
                return false;
            }
        }
    }

    return !first;
}
