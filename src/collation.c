/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "mojibake-internal.h"
#include "unicode.h"
#include "utf.h"

extern mojibake mjb_global;

// Unicode collation algorithm
// See: https://www.unicode.org/reports/tr10/
MJB_PURE int mjb_string_compare(const char *s1, size_t s1_length, mjb_encoding s1_encoding,
    const char *s2, size_t s2_length, mjb_encoding s2_encoding) {
    /*uint8_t state_1 = MJB_UTF_ACCEPT;
    uint8_t state_2 = MJB_UTF_ACCEPT;
    bool in_error_1 = false;
    bool in_error_2 = false;
    mjb_codepoint codepoint_1;
    mjb_codepoint codepoint_2;
    size_t i = 0;
    size_t j = 0;

    for(i = 0; i < s1_length;) {
        // Find next codepoint.
        mjb_decode_result decode_status_1 = mjb_next_codepoint(s1, s1_length, &state_1, &i,
            s1_encoding, &codepoint_1, &in_error_1);

        if(decode_status_1 == MJB_DECODE_END) {
            break;
        }

        if(decode_status_1 == MJB_DECODE_INCOMPLETE) {
            continue;
        }

        // decode_status_1 is MJB_DECODE_OK or MJB_DECODE_ERROR
        for(; j < s2_length; ) {
            mjb_decode_result decode_status_2 = mjb_next_codepoint(s2, s2_length, &state_2, &j,
                s2_encoding, &codepoint_2, &in_error_2);

            if(decode_status_2 == MJB_DECODE_END) {
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

    if(j < s2_length) {
        return -1;
    } else if(i < s1_length) {
        return 1;
    }*/

    return 0;
}
