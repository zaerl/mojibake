/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <string.h>

#include "mojibake-internal.h"
#include "utf.h"

extern mojibake mjb_global;

// Sentence boundaries breaking
// See: https://unicode.org/reports/tr29/
MJB_EXPORT mjb_break_type mjb_break_sentence(const char *buffer, size_t size, mjb_encoding encoding,
    mjb_next_sentence_state *state) {
    if(size == 0) {
        return MJB_BT_NOT_SET;
    }

    if(state->index == 0) {
        // Initialize the state.
        state->state = MJB_UTF_ACCEPT;
        state->previous = MJB_SBP_NOT_SET;
        state->current = MJB_SBP_NOT_SET;
        state->previous_codepoint = MJB_CODEPOINT_NOT_VALID;
        state->current_codepoint = MJB_CODEPOINT_NOT_VALID;
    }

    if(state->index == size) {
        // Reached end of string.
        ++state->index;

        // SB2 Any ÷ eot
        return MJB_BT_ALLOWED;
    } else if(state->index > size) {
        // Last step
        return MJB_BT_NOT_SET;
    }

    mjb_codepoint codepoint = 0;
    bool first_codepoint = state->index == 0;
    uint8_t cpb[MJB_PR_BUFFER_SIZE] = { 0 };

    for(; state->index < size;) {
        mjb_decode_result decode_status = mjb_next_codepoint(buffer, size, &state->state,
            &state->index, encoding, &codepoint, &state->in_error);

        if(decode_status == MJB_DECODE_END) {
            return MJB_BT_ALLOWED;
        }

        if(decode_status == MJB_DECODE_INCOMPLETE) {
            continue;
        }

        // Break at the start and end of text, unless the text is empty.
        // SB1 sot ÷ Any
        // Not needed

        memset(cpb, 0, MJB_PR_BUFFER_SIZE);
        mjb_codepoint_properties(codepoint, cpb);
        mjb_sbp wbp = (mjb_sbp)mjb_codepoint_property(cpb, MJB_PR_SENTENCE_BREAK);

        if(wbp == MJB_SBP_NOT_SET) {
            // # @missing: 0000..10FFFF; Other
            wbp = MJB_SBP_XX;
        }

        if(first_codepoint) {
            // First codepoint
            state->current = wbp;
            state->current_codepoint = codepoint;
            first_codepoint = false;

            continue;
        }

        // Swap previous and current codepoints
        state->previous = state->current;
        state->current = wbp;
        state->previous_codepoint = state->current_codepoint;
        state->current_codepoint = codepoint;

        // Do not break within CRLF
        // SB3 CR × LF
        if(state->previous == MJB_SBP_CR && state->current == MJB_SBP_LF) {
            return MJB_BT_NO_BREAK;
        }

        // Break after paragraph separators.
        // ParaSep ÷
        if(state->previous == MJB_SBP_SE || state->previous == MJB_SBP_CR && state->previous == MJB_SBP_LF) {
            return MJB_BT_ALLOWED;
        }

        // Ignore Format and Extend characters, except after sot, ParaSep, and within CRLF. This
        // also has the effect of: Any × (Format | Extend)
        // SB5 X (Extend | Format)* -> X

        // SB6 ATerm × Numeric
        if(state->previous == MJB_SBP_AT && state->current == MJB_SBP_NU) {
            return MJB_BT_NO_BREAK;
        }

        // SB7 (Upper | Lower) ATerm × Upper
        // TODO

        // SB8 ATerm Close* Sp* × ( ¬(OLetter | Upper | Lower | ParaSep | SATerm) )* Lower
        // TODO

        // SB8a SATerm Close* Sp* × (SContinue | SATerm)
        // TODO

        // Break after sentence terminators, but include closing punctuation, trailing spaces, and
        // any paragraph separator.
        // SB9 SATerm Close* × (Close | Sp | ParaSep)
        // TODO

        // SB10 SATerm Close* Sp* × (Sp | ParaSep)
        // TODO

        // SB11 SATerm Close* Sp* ParaSep? ÷
        // TODO

        // Otherwise, do not break.
        // SB998 Any × Any
        return MJB_BT_NO_BREAK;
    }

    ++state->index;

    return MJB_BT_NO_BREAK;
}
