/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <string.h>

#include "mojibake-internal.h"
#include "unicode.h"
#include "utf.h"

extern mojibake mjb_global;

// Unicode collation algorithm
// See: https://www.unicode.org/reports/tr10/
MJB_PURE int mjb_string_compare(const char *s1, size_t s1_length, const char *s2, size_t s2_length,
    mjb_encoding encoding) {
    if(!mjb_initialize()) {
        return -1;
    }

    if(s1_length == 0 || s2_length == 0) {
        return 0;
    }

    if(encoding == MJB_ENCODING_ASCII) {
        return strncmp(s1, s2, s1_length < s2_length ? s1_length : s2_length);
    }

    mjb_result result_1;
    mjb_result result_2;

    bool normalized = mjb_normalize(s1, s1_length, encoding, MJB_NORMALIZATION_NFD, &result_1);

    if(!normalized) {
        return -1;
    }

    normalized = mjb_normalize(s2, s2_length, encoding, MJB_NORMALIZATION_NFD, &result_2);

    if(!normalized) {
        if(result_1.output != NULL && result_1.output != s1) {
            mjb_free(result_1.output);
        }

        return -1;
    }

    uint8_t state_1 = MJB_UTF_ACCEPT;
    uint8_t state_2 = MJB_UTF_ACCEPT;
    bool in_error_1 = false;
    bool in_error_2 = false;
    mjb_codepoint codepoint_1;
    mjb_codepoint codepoint_2;
    size_t i = 0;
    size_t j = 0;
    int ret = 0;

    for(i = 0; i < s1_length;) {
        // Find next codepoint.
        mjb_decode_result decode_status_1 = mjb_next_codepoint(s1, s1_length, &state_1, &i,
            encoding, &codepoint_1, &in_error_1);

        if(decode_status_1 == MJB_DECODE_END) {
            // TODO: not break here, continue to check the second string
            break;
        }

        if(decode_status_1 == MJB_DECODE_INCOMPLETE) {
            continue;
        }

        // decode_status_1 is MJB_DECODE_OK or MJB_DECODE_ERROR
        for(; j < s2_length;) {
            mjb_decode_result decode_status_2 = mjb_next_codepoint(s2, s2_length, &state_2, &j,
                encoding, &codepoint_2, &in_error_2);

            if(decode_status_2 == MJB_DECODE_END) {
                // TODO: check if the first string is in MJB_DECODE_END state
                break;
            }

            if(decode_status_2 == MJB_DECODE_INCOMPLETE) {
                continue;
            }

            // decode_status_2 is MJB_DECODE_OK or MJB_DECODE_ERROR
            if(codepoint_1 < codepoint_2) {
                return -1;
            } else if(codepoint_1 > codepoint_2) {
                return 1;
            }

            break;
        }
    }

    if(result_1.output != NULL && result_1.output != s1) {
        mjb_free(result_1.output);
    }

    if(result_2.output != NULL && result_2.output != s2) {
        mjb_free(result_2.output);
    }

    return ret;
}
