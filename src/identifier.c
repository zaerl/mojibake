/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "mojibake-internal.h"
#include "utf.h"

// Return true if the string is a valid Unicode identifier (UAX#31).
MJB_EXPORT bool mjb_string_is_identifier(const char *buffer, size_t size, mjb_encoding encoding,
    mjb_identifier_profile profile) {
    if(size == 0) {
        return false;
    }

    if(!mjb_initialize()) {
        return false;
    }

    mjb_property start_prop = (profile == MJB_IDENTIFIER_NFKC) ? MJB_PR_XID_START : MJB_PR_ID_START;
    mjb_property cont_prop = (profile == MJB_IDENTIFIER_NFKC) ? MJB_PR_XID_CONTINUE : MJB_PR_ID_CONTINUE;

    uint8_t state = MJB_UTF_ACCEPT;
    mjb_codepoint codepoint;
    bool in_error = false;
    bool first = true;

    for(size_t i = 0; i < size;) {
        mjb_decode_result dr = mjb_next_codepoint(buffer, size, &state, &i, encoding, &codepoint,
            &in_error);

        if(dr == MJB_DECODE_END) {
            break;
        }

        if(dr == MJB_DECODE_INCOMPLETE) {
            continue;
        }

        if(dr == MJB_DECODE_ERROR) {
            return false;
        }

        if(first) {
            if(!mjb_codepoint_has_property(codepoint, start_prop, NULL)) {
                return false;
            }

            first = false;
        } else {
            if(!mjb_codepoint_has_property(codepoint, cont_prop, NULL)) {
                return false;
            }
        }
    }

    return !first;
}
