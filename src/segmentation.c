/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "mojibake-internal.h"
#include "utf.h"

extern mojibake mjb_global;

// Word and Grapheme Cluster Breaking
// See: https://unicode.org/reports/tr29/
MJB_EXPORT bool mjb_segmentation(const char *buffer, size_t length, mjb_encoding encoding) {
    uint8_t state = MJB_UTF_ACCEPT;
    mjb_codepoint codepoint;

    for(size_t i = 0; i < length; ++i) {
        if(!mjb_decode_step(buffer, length, &state, &i, encoding, &codepoint)) {
            break;
        }

        /*
          A boundary is:
          1. A literal character
          2. A range of literal characters
          3. All characters satisfying a given condition, using properties defined in the UCD
          4. Boolean combinations of the above
          5. Two special identifiers, sot and eot, start of text and end of text
        */
        if(state == MJB_UTF_ACCEPT) {

        }
    }

    return true;
}
