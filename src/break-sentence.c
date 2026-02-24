/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <string.h>

#include "mojibake-internal.h"
#include "utf.h"

extern mojibake mjb_global;

// Check if an SBP value blocks SB8 look-ahead.
// The blocked set is: OLetter | Upper | ParaSep | SATerm
// (Lower is NOT blocked here — it's the target; handled separately.)
static inline bool mjb_sbp_blocks_sb8(mjb_sbp sbp) {
    return sbp == MJB_SBP_OLETTER || sbp == MJB_SBP_UPPER || sbp == MJB_SBP_SEP ||
        sbp == MJB_SBP_CR || sbp == MJB_SBP_LF || sbp == MJB_SBP_LOWER || sbp == MJB_SBP_STERM ||
        sbp == MJB_SBP_ATERM;
}

// Peek ahead from peek_index to check if Lower is reachable through
// (¬(OLetter | Upper | Lower | ParaSep | SATerm))* — the SB8 look-ahead.
// Extend and Format are transparent (SB5). Returns true if Lower is found.
static inline bool mjb_peek_lower_sentence(const char *buffer, size_t size, size_t peek_index,
    mjb_encoding encoding) {
    uint8_t peek_state = MJB_UTF_ACCEPT;
    mjb_codepoint peek_cp = 0;
    bool peek_error = false;

    for(; peek_index < size;) {
        mjb_decode_result dr = mjb_next_codepoint(buffer, size, &peek_state, &peek_index,
            encoding, &peek_cp, &peek_error);

        if(dr == MJB_DECODE_END) {
            return false;
        }

        if(dr == MJB_DECODE_OK) {
            uint8_t cpb[MJB_PR_BUFFER_SIZE] = {0};
            mjb_codepoint_properties(peek_cp, cpb);
            mjb_sbp sbp = (mjb_sbp)mjb_codepoint_property(cpb, MJB_PR_SENTENCE_BREAK);

            if(sbp == MJB_SBP_NOT_SET) {
                sbp = MJB_SBP_OTHER;
            }

            // SB5: Extend and Format are transparent
            if(sbp == MJB_SBP_EXTEND || sbp == MJB_SBP_FORMAT) {
                continue;
            }

            // Found Lower — SB8 applies
            if(sbp == MJB_SBP_LOWER) {
                return true;
            }

            // Blocked by OLetter, Upper, ParaSep, or SATerm
            if(mjb_sbp_blocks_sb8(sbp)) {
                return false;
            }

            // Continue scanning through: Close, Sp, Numeric, SContinue, Other, etc.
        }
    }

    return false;
}

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
        state->prev_prev = MJB_SBP_NOT_SET;
        state->previous_codepoint = MJB_CODEPOINT_NOT_VALID;
        state->current_codepoint = MJB_CODEPOINT_NOT_VALID;
        state->in_error = false;
        state->sb5_merged = false;
        state->in_sat = false;
        state->sat_has_sp = false;
        state->sat_is_aterm = false;
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
            wbp = MJB_SBP_OTHER;
        }

        if(first_codepoint) {
            // First codepoint: store and initialize SAT context if needed.
            state->current = wbp;
            state->current_codepoint = codepoint;

            if(wbp == MJB_SBP_STERM || wbp == MJB_SBP_ATERM) {
                state->in_sat = true;
                state->sat_has_sp = false;
                state->sat_is_aterm = (wbp == MJB_SBP_ATERM);
            }

            first_codepoint = false;

            continue;
        }

        // Save the SAT context as of state->current (the previous character).
        bool prev_in_sat = state->in_sat;
        bool prev_sat_has_sp = state->sat_has_sp;
        bool prev_sat_is_aterm = state->sat_is_aterm;

        // Update prev_prev, but skip the update when SB5 just merged a character.
        // This ensures SB7's 3-char lookback sees the correct base character.
        if(!state->sb5_merged) {
            state->prev_prev = state->previous;
        }

        state->sb5_merged = false;

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
        // SB4 ParaSep ÷
        if(state->previous == MJB_SBP_SEP ||
            state->previous == MJB_SBP_CR ||
            state->previous == MJB_SBP_LF) {
            state->in_sat = false;
            state->sat_has_sp = false;
            state->sat_is_aterm = false;

            return MJB_BT_ALLOWED;
        }

        // Ignore Format and Extend characters, except after sot, ParaSep, and within CRLF.
        // SB5 X (Extend | Format)* -> X
        if((state->current == MJB_SBP_EXTEND || state->current == MJB_SBP_FORMAT) &&
           state->previous != MJB_SBP_NOT_SET &&
           state->previous != MJB_SBP_CR &&
           state->previous != MJB_SBP_LF &&
           state->previous != MJB_SBP_SEP) {
            // Absorb: remap to the base class so subsequent calls see X as previous.
            state->current = state->previous;
            state->current_codepoint = state->previous_codepoint;
            state->sb5_merged = true;

            // SAT context is unchanged (Extend/Format are transparent).
            return MJB_BT_NO_BREAK;
        }

        // Update SAT context for the next call based on incoming wbp.
        // Must happen after SB5 (which returns early, keeping context unchanged).
        // Updating here (before rule checks) ensures the context is correct even
        // when rules return early.
        if(wbp == MJB_SBP_STERM || wbp == MJB_SBP_ATERM) {
            state->in_sat = true;
            state->sat_has_sp = false;
            state->sat_is_aterm = (wbp == MJB_SBP_ATERM);
        } else if(prev_in_sat && wbp == MJB_SBP_CLOSE && !prev_sat_has_sp) {
            // Close in SATerm Close* (before any Sp)
            state->in_sat = true;
            state->sat_has_sp = false;
            state->sat_is_aterm = prev_sat_is_aterm;
        } else if(prev_in_sat && wbp == MJB_SBP_SP) {
            // Sp in SATerm Close* Sp*
            state->in_sat = true;
            state->sat_has_sp = true;
            state->sat_is_aterm = prev_sat_is_aterm;
        } else {
            state->in_sat = false;
            state->sat_has_sp = false;
            state->sat_is_aterm = false;
        }

        // SB6 ATerm × Numeric
        if(state->previous == MJB_SBP_ATERM && state->current == MJB_SBP_NUMERIC) {
            return MJB_BT_NO_BREAK;
        }

        // SB7 (Upper | Lower) ATerm × Upper
        if((state->prev_prev == MJB_SBP_UPPER || state->prev_prev == MJB_SBP_LOWER) &&
           state->previous == MJB_SBP_ATERM &&
           state->current == MJB_SBP_UPPER) {
            return MJB_BT_NO_BREAK;
        }

        // SB8 ATerm Close* Sp* × (¬(OLetter | Upper | Lower | ParaSep | SATerm))* Lower
        if(prev_in_sat && prev_sat_is_aterm) {
            if(state->current == MJB_SBP_LOWER) {
                // Direct match: ATerm (Close*) (Sp*) × Lower
                return MJB_BT_NO_BREAK;
            }

            // Current is in ¬(OLetter|Upper|Lower|ParaSep|SATerm): look ahead for Lower
            if(!mjb_sbp_blocks_sb8(state->current) && state->current != MJB_SBP_LOWER) {
                if(mjb_peek_lower_sentence(buffer, size, state->index, encoding)) {
                    return MJB_BT_NO_BREAK;
                }
            }
        }

        // SB8a SATerm Close* Sp* × (SContinue | SATerm)
        if(prev_in_sat &&
           (state->current == MJB_SBP_SCONTINUE ||
            state->current == MJB_SBP_STERM ||
            state->current == MJB_SBP_ATERM)) {
            return MJB_BT_NO_BREAK;
        }

        // SB9 SATerm Close* × (Close | Sp | ParaSep)
        if(prev_in_sat && !prev_sat_has_sp &&
           (state->current == MJB_SBP_CLOSE ||
            state->current == MJB_SBP_SP ||
            state->current == MJB_SBP_SEP ||
            state->current == MJB_SBP_CR ||
            state->current == MJB_SBP_LF)) {
            return MJB_BT_NO_BREAK;
        }

        // SB10 SATerm Close* Sp* × (Sp | ParaSep)
        if(prev_in_sat &&
           (state->current == MJB_SBP_SP ||
            state->current == MJB_SBP_SEP ||
            state->current == MJB_SBP_CR ||
            state->current == MJB_SBP_LF)) {
            return MJB_BT_NO_BREAK;
        }

        // SB11 SATerm Close* Sp* ParaSep? ÷
        if(prev_in_sat) {
            return MJB_BT_ALLOWED;
        }

        // Otherwise, do not break.
        // SB998 Any × Any
        return MJB_BT_NO_BREAK;
    }

    ++state->index;

    return MJB_BT_NO_BREAK;
}
