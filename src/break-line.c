/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdlib.h>
#include <string.h>

#include "mojibake-internal.h"
#include "utf.h"

extern mojibake mjb_global;

static inline mjb_category mjb_lbp_category(mjb_codepoint cp) {
    sqlite3_reset(mjb_global.stmt_line_breaking);
    sqlite3_bind_int(mjb_global.stmt_line_breaking, 1, (int)cp);
    mjb_category cat = (mjb_category)0;

    if(sqlite3_step(mjb_global.stmt_line_breaking) == SQLITE_ROW) {
        cat = (mjb_category)sqlite3_column_int(mjb_global.stmt_line_breaking, 0);
    }

    sqlite3_reset(mjb_global.stmt_line_breaking);

    return cat;
}

static inline bool mjb_is_ea(mjb_east_asian_width ea) {
    return ea == MJB_EAW_FULL_WIDTH || ea == MJB_EAW_WIDE || ea == MJB_EAW_HALF_WIDTH;
}

// Peek at the next codepoint and return its LBP (resolved by LB1) and EA width.
// Returns MJB_LBP_NOT_SET for EOT.
static inline mjb_lbp mjb_peek_next(const char *buffer, size_t size, size_t peek_index,
    mjb_encoding encoding, mjb_east_asian_width *ea_out) {
    uint8_t peek_state = MJB_UTF_ACCEPT;
    mjb_codepoint peek_cp = 0;
    bool peek_error = false;
    size_t i = peek_index;

    for(; i < size;) {
        mjb_decode_result dr = mjb_next_codepoint(buffer, size, &peek_state, &peek_index,
            encoding, &peek_cp, &peek_error);

        if(dr == MJB_DECODE_END) {
            if(ea_out) {
                *ea_out = MJB_EAW_NOT_SET;
            }

            return MJB_LBP_NOT_SET;
        }

        if(dr == MJB_DECODE_OK) {
            uint8_t cpb[MJB_PR_BUFFER_SIZE] = {0};
            mjb_codepoint_properties(peek_cp, cpb);
            mjb_lbp lbp = (mjb_lbp)mjb_codepoint_property(cpb, MJB_PR_LINE_BREAK);

            if(lbp == MJB_LBP_NOT_SET) {
                lbp = MJB_LBP_XX;
            }

            // Apply LB1 resolution
            if(lbp == MJB_LBP_AI || lbp == MJB_LBP_SG || lbp == MJB_LBP_XX) {
                lbp = MJB_LBP_AL;
            } else if(lbp == MJB_LBP_CJ) {
                lbp = MJB_LBP_NS;
            } else if(lbp == MJB_LBP_SA) {
                mjb_category gc = mjb_lbp_category(peek_cp);
                lbp = (gc == MJB_CATEGORY_MN || gc == MJB_CATEGORY_MC) ? MJB_LBP_CM : MJB_LBP_AL;
            }

            if(ea_out) {
                *ea_out = (mjb_east_asian_width)cpb[MJB_PR_EAST_ASIAN_WIDTH];
            }

            return lbp;
        }
    }

    return MJB_LBP_NOT_SET;
}

// Return true if lbp is in the LB15b follower set
static inline bool mjb_is_lb15b_follower(mjb_lbp lbp) {
    return lbp == MJB_LBP_NOT_SET ||
        lbp == MJB_LBP_SP || lbp == MJB_LBP_GL || lbp == MJB_LBP_WJ ||
        lbp == MJB_LBP_CL || lbp == MJB_LBP_QU || lbp == MJB_LBP_CP ||
        lbp == MJB_LBP_EX || lbp == MJB_LBP_IS || lbp == MJB_LBP_SY ||
        lbp == MJB_LBP_BK || lbp == MJB_LBP_CR || lbp == MJB_LBP_LF ||
        lbp == MJB_LBP_NL || lbp == MJB_LBP_ZW;
}

// Line breaking algorithm
// see: https://www.unicode.org/reports/tr14
MJB_EXPORT mjb_break_type mjb_break_line(const char *buffer, size_t size, mjb_encoding encoding,
    mjb_next_line_state *state) {
    if(size == 0) {
        return MJB_BT_NOT_SET;
    }

    if(state->index == 0) {
        // Initialize the state.
        state->state = MJB_UTF_ACCEPT;
        state->previous = MJB_LBP_NOT_SET;
        state->current = MJB_LBP_NOT_SET;
        state->previous_codepoint = MJB_CODEPOINT_NOT_VALID;
        state->current_codepoint = MJB_CODEPOINT_NOT_VALID;
        state->in_error = false;
        state->ri_count = 0;
        state->zw_seen = false;
        state->prev_resolved = MJB_LBP_NOT_SET;
        state->prev_ea = MJB_EAW_NOT_SET;
        state->qu_prev_ea = MJB_EAW_NOT_SET;
        state->prev_prev_lbp = MJB_LBP_NOT_SET;
        state->prev_num_lbp = MJB_LBP_NOT_SET;
        state->prev_prev_codepoint = MJB_CODEPOINT_NOT_VALID;
        state->pi_qu_context = false;
        state->cm_merged = false;
    }

    if(state->index == size) {
        // Reached end of string.
        ++state->index;

        // Always break at the end of text.
        // LB3 ! eot
        return MJB_BT_MANDATORY;
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

        // LB2 Never break at the start of text.
        // sot ×
        // Not needed

        memset(cpb, 0, MJB_PR_BUFFER_SIZE);
        mjb_codepoint_properties(codepoint, cpb);
        mjb_lbp lbp = (mjb_lbp)mjb_codepoint_property(cpb, MJB_PR_LINE_BREAK);

        if(lbp == MJB_LBP_NOT_SET) {
            // # @missing: 0000..10FFFF; XX
            lbp = MJB_LBP_XX;
        }

        // LB1 Resolve ambiguous and context-dependent classes before applying pairwise rules.
        // AI, SG, XX -> AL
        // CJ -> NS
        // SA -> CM if the codepoint is a non-spacing or spacing-combining mark, otherwise AL
        if(lbp == MJB_LBP_AI || lbp == MJB_LBP_SG || lbp == MJB_LBP_XX) {
            lbp = MJB_LBP_AL;
        } else if(lbp == MJB_LBP_CJ) {
            lbp = MJB_LBP_NS;
        } else if(lbp == MJB_LBP_SA) {
            mjb_category gc = mjb_lbp_category(codepoint);
            lbp = (gc == MJB_CATEGORY_MN || gc == MJB_CATEGORY_MC) ? MJB_LBP_CM : MJB_LBP_AL;
        }

        mjb_east_asian_width ea = (mjb_east_asian_width)cpb[MJB_PR_EAST_ASIAN_WIDTH];

        if(first_codepoint) {
            // First codepoint
            state->current = lbp;
            state->current_codepoint = codepoint;
            state->prev_ea = ea;
            first_codepoint = false;

            continue;
        }

        // Save prev_prev_lbp BEFORE the swap (captures the char that was "previous" last time).
        // Exception: when LB9 merged a CM in the previous call, prev_prev_lbp was already
        // set to reflect the pre-base context (e.g. sot) — preserve it instead of overwriting.
        if(!state->cm_merged) {
            state->prev_prev_lbp = state->previous;
            state->prev_prev_codepoint = state->previous_codepoint;
        }

        state->cm_merged = false;

        // Swap previous and current codepoints
        state->previous = state->current;
        state->current = lbp;
        state->previous_codepoint = state->current_codepoint;
        state->current_codepoint = codepoint;

        // LB10 retroactive: if previous is CM that wasn't absorbed by LB9 in the prior call
        // (because the base was BK/CR/LF/NL and LB4/LB5 returned early before LB9 ran),
        // treat it as AL now so subsequent rules see AL as previous.
        // NOTE: ZWJ is intentionally excluded — ZWJ as previous is handled by LB8a (ZWJ ×)
        // which takes priority. Retroactively reclassifying ZWJ→AL would prevent LB8a
        // from firing (e.g. ZWJ as the first codepoint, or ZWJ after a non-excluded base).
        if(state->previous == MJB_LBP_CM) {
            state->previous = MJB_LBP_AL;
        }

        // LB30a early RI count reset: reset ri_count when previous is not RI.
        // This must happen before early returns (LB6, LB7, LB8) which prevent the
        // LB30a else-branch from firing. For example: RI RI ZW RI RI — the ZW→RI pair
        // triggers LB8 before LB30a, leaving ri_count non-zero and breaking the next RI×RI.
        if(state->previous != MJB_LBP_RI) {
            state->ri_count = 0;
        }

        // Early Pi-QU context detection for LB15a.
        // Must run before early returns (LB6, LB7) so that pi_qu_context is set
        // when we return NO_BREAK for SP following a Pi-QU, enabling the SP* × part
        // of LB15a to fire in the next iteration.
        if(state->previous == MJB_LBP_QU) {
            mjb_category pq_cat = mjb_lbp_category(state->previous_codepoint);

            if(pq_cat == MJB_CATEGORY_PI) {
                bool in_ctx = (
                    state->prev_prev_lbp == MJB_LBP_NOT_SET || // sot
                    state->prev_prev_lbp == MJB_LBP_BK  || state->prev_prev_lbp == MJB_LBP_CR ||
                    state->prev_prev_lbp == MJB_LBP_LF  || state->prev_prev_lbp == MJB_LBP_NL ||
                    state->prev_prev_lbp == MJB_LBP_OP  || state->prev_prev_lbp == MJB_LBP_QU ||
                    state->prev_prev_lbp == MJB_LBP_GL  || state->prev_prev_lbp == MJB_LBP_SP ||
                    state->prev_prev_lbp == MJB_LBP_ZW
                );

                // Also propagate through SP* chain
                if(!in_ctx) {
                    in_ctx = state->pi_qu_context && state->prev_prev_lbp == MJB_LBP_SP;
                }

                state->pi_qu_context = in_ctx;
            }
        }

        // Update EA width tracking: prev_ea = old current's EA, save new current's EA for next call
        mjb_east_asian_width prev_ea = state->prev_ea;
        state->prev_ea = ea;

        // Update prev_resolved for SP* rules (LB14, LB16, LB17).
        // Tracks the last non-SP LBP before the current position.
        if(state->previous != MJB_LBP_SP) {
            state->prev_resolved = state->previous;
        }

        // Snapshot prev_num_lbp for LB25 NU-sequence checks, then update.
        // Tracks the last non-IS/SY LBP (the base of a number sequence).
        mjb_lbp snap_num_lbp = state->prev_num_lbp;
        if(state->previous != MJB_LBP_IS && state->previous != MJB_LBP_SY) {
            state->prev_num_lbp = state->previous;
        }

        if(state->previous == MJB_LBP_ZW) {
            state->zw_seen = true;
        } else if(state->previous != MJB_LBP_SP) {
            state->zw_seen = false;
        }

        // LB4 Always break after hard line breaks.
        // BK !
        if(state->previous == MJB_LBP_BK) {
            state->pi_qu_context = false;

            return MJB_BT_MANDATORY;
        }

        // LB5 Treat CR followed by LF, as well as CR, LF, and NL as hard line breaks.

        // CR × LF
        if(state->previous == MJB_LBP_CR && state->current == MJB_LBP_LF) {
            return MJB_BT_NO_BREAK;
        }

        // CR !
        // LF !
        // NL !
        if(
            state->previous == MJB_LBP_CR ||
            state->previous == MJB_LBP_LF ||
            state->previous == MJB_LBP_NL
        ) {
            state->pi_qu_context = false;

            return MJB_BT_MANDATORY;
        }

        // Do not break before hard line breaks.
        // LB6 × ( BK | CR | LF | NL )
        if(
            state->current == MJB_LBP_BK ||
            state->current == MJB_LBP_CR ||
            state->current == MJB_LBP_LF ||
            state->current == MJB_LBP_NL
        ) {
            return MJB_BT_NO_BREAK;
        }

        // LB7 Do not break before spaces or zero width space.
        // × SP
        // × ZW
        if(state->current == MJB_LBP_SP || state->current == MJB_LBP_ZW) {
            return MJB_BT_NO_BREAK;
        }

        // LB8 Break before any character following a zero-width space, even if one or more spaces
        // intervene.
        // ZW SP* ÷
        if(state->zw_seen) {
            state->pi_qu_context = false;

            return MJB_BT_ALLOWED;
        }

        // LB8a Do not break after a zero width joiner.
        // ZWJ ×
        if(state->previous == MJB_LBP_ZWJ) {
            return MJB_BT_NO_BREAK;
        }

        // LB9 Do not break a combining character sequence; treat it as if it has the line breaking
        // class of the base character in all of the following rules. Treat ZWJ as if it were CM.
        // Treat X (CM | ZWJ)* as if it were X.
        // X is any class except BK, CR, LF, NL, SP, ZW.
        if(
            (state->current == MJB_LBP_CM || state->current == MJB_LBP_ZWJ) &&
            state->previous != MJB_LBP_BK &&
            state->previous != MJB_LBP_CR &&
            state->previous != MJB_LBP_LF &&
            state->previous != MJB_LBP_NL &&
            state->previous != MJB_LBP_SP &&
            state->previous != MJB_LBP_ZW
        ) {
            // Re-map to the base class so subsequent calls see X as previous, not CM/ZWJ.
            // Also remap current_codepoint so that state->previous_codepoint in the
            // next pair reflects the true base codepoint (needed for e.g. LB28a [◌]).
            state->current = state->previous;
            state->current_codepoint = state->previous_codepoint;
            // Signal that prev_prev_lbp should be preserved (not overwritten) in the next
            // call, so that rules like LB20a see the correct context before the base.
            state->cm_merged = true;

            return MJB_BT_NO_BREAK;
        }

        // LB10 Treat any remaining CM or ZWJ as AL (base was one of the excluded classes above).
        if(state->current == MJB_LBP_CM || state->current == MJB_LBP_ZWJ) {
            state->current = MJB_LBP_AL;
        }

        // LB11 Do not break before or after Word joiner and related characters.
        // × WJ
        // WJ ×
        if(state->previous == MJB_LBP_WJ || state->current == MJB_LBP_WJ) {
            return MJB_BT_NO_BREAK;
        }

        // LB12 Do not break after NBSP and related characters.
        // GL ×
        if(state->previous == MJB_LBP_GL) {
            return MJB_BT_NO_BREAK;
        }

        // 6.2 Tailorable Line Breaking Rules

        // LB12a Do not break before NBSP and related characters, except after spaces and hyphens.
        // [^SP BA HY HH] × GL
        if(
            state->current == MJB_LBP_GL &&
            state->previous != MJB_LBP_SP &&
            state->previous != MJB_LBP_BA &&
            state->previous != MJB_LBP_HY &&
            state->previous != MJB_LBP_HH
        ) {
            return MJB_BT_NO_BREAK;
        }

        // LB13 Do not break before ']' or '!' or '/', even after spaces.
        // × CL
        // × CP
        // × EX
        // × SY
        if(
            state->current == MJB_LBP_CL ||
            state->current == MJB_LBP_CP ||
            state->current == MJB_LBP_EX ||
            state->current == MJB_LBP_SY
        ) {
            return MJB_BT_NO_BREAK;
        }

        // LB14 Do not break after '[', even after spaces.
        // OP SP* ×
        if(state->prev_resolved == MJB_LBP_OP) {
            return MJB_BT_NO_BREAK;
        }

        // For LB15a, LB15b, and LB19: when current is QU we need its General_Category.
        // general_category not in property blob — use the lightweight stmt_line_breaking query.
        mjb_category qu_cur_cat = (mjb_category)0xFF; // sentinel: not yet loaded

        if(state->current == MJB_LBP_QU) {
            qu_cur_cat = mjb_lbp_category(codepoint);
        }

        // Save the Pi-QU context state BEFORE the LB15a update below.
        // The SP* × check must use the pre-update value so that:
        //   - A zone established by a PREVIOUS call fires NO_BREAK for SP* × X
        //   - A zone established THIS call (QU_Pi just arrived as current) defers its
        //     NO_BREAK effect to the NEXT call (LB18 still owns the SP → QU_Pi break).
        bool pi_qu_context_before = state->pi_qu_context;

        // LB15a Do not break after an unresolved initial punctuation that lies at the start of
        // the line, after a space, after opening punctuation, or after an unresolved quotation
        // mark, even after spaces.
        // (sot | BK | CR | LF | NL | OP | QU | GL | SP | ZW) [\p{Pi}&QU] SP* ×
        // We track the Pi-QU context: set pi_qu_context when a Pi-QU character appears in that
        // context, and apply it to subsequent positions.
        if(state->current == MJB_LBP_QU) {
            if(qu_cur_cat == MJB_CATEGORY_PI) {
                // Check if previous is in the LB15a context set
                bool in_ctx = (
                    state->previous == MJB_LBP_NOT_SET || // sot
                    state->previous == MJB_LBP_BK  || state->previous == MJB_LBP_CR ||
                    state->previous == MJB_LBP_LF  || state->previous == MJB_LBP_NL ||
                    state->previous == MJB_LBP_OP  || state->previous == MJB_LBP_QU ||
                    state->previous == MJB_LBP_GL  || state->previous == MJB_LBP_SP ||
                    state->previous == MJB_LBP_ZW
                );

                if(!in_ctx) {
                    // Also check via pi_qu_context (SP* continuation after Pi-QU)
                    in_ctx = pi_qu_context_before && state->previous == MJB_LBP_SP;
                }

                if(in_ctx) {
                    state->pi_qu_context = true;
                } else {
                    state->pi_qu_context = false;
                }
            } else {
                // Not Pi-QU: clear pi_qu_context unless SP continuation
                if(state->previous != MJB_LBP_SP) {
                    state->pi_qu_context = false;
                }
            }
        } else if(state->previous != MJB_LBP_SP && state->previous != MJB_LBP_QU) {
            // Non-SP, non-QU previous clears the Pi-QU zone
            state->pi_qu_context = false;
        }

        // LB15b Do not break before an unresolved final punctuation that lies at the end of
        // the line, before a space, before a prohibited break, or before an unresolved quotation
        // mark, even after spaces.
        // × [\p{Pf}&QU] ( SP | GL | WJ | CL | QU | CP | EX | IS | SY | BK | CR | LF | NL | ZW | eot)
        // NOTE: This rule must be checked BEFORE LB18 (SP ÷) because it overrides it.
        if(state->current == MJB_LBP_QU && qu_cur_cat == MJB_CATEGORY_PF) {
            mjb_lbp next_lbp = mjb_peek_next(buffer, size, state->index, encoding, NULL);

            if(mjb_is_lb15b_follower(next_lbp)) {
                return MJB_BT_NO_BREAK;
            }
        }

        // LB15c Break before a decimal mark that follows a space, for instance, in 'subtract .5'.
        // SP ÷ IS NU
        // (Requires look-ahead: next after IS must be NU. Implemented below before LB15d.)
        if(state->previous == MJB_LBP_SP && state->current == MJB_LBP_IS) {
            mjb_lbp next_lbp = mjb_peek_next(buffer, size, state->index, encoding, NULL);

            if(next_lbp == MJB_LBP_NU) {
                // SP ÷ IS NU: break between SP and IS
                return MJB_BT_ALLOWED;
            }
        }

        // LB15d Otherwise, do not break before ';', ',', or '.', even after spaces.
        // × IS
        if(state->current == MJB_LBP_IS) {
            return MJB_BT_NO_BREAK;
        }

        // LB16 Do not break between closing punctuation and a nonstarter (lb=NS), even with
        // intervening spaces.
        // (CL | CP) SP* × NS
        if(
            state->current == MJB_LBP_NS &&
            (state->prev_resolved == MJB_LBP_CL || state->prev_resolved == MJB_LBP_CP)
        ) {
            return MJB_BT_NO_BREAK;
        }

        // LB17 Do not break within '——', even with intervening spaces.
        // B2 SP* × B2
        if(state->current == MJB_LBP_B2 && state->prev_resolved == MJB_LBP_B2) {
            return MJB_BT_NO_BREAK;
        }

        // LB15a SP* × part: do not break after SP that follows a Pi-QU in context.
        // Use pi_qu_context_before (the value BEFORE this call's LB15a update) so that:
        //   - An already-established zone (from a prior call) fires NO_BREAK here.
        //   - A zone opened THIS call (SP → QU_Pi just arrived) defers to LB18.
        // This overrides LB18 (SP ÷).
        if(pi_qu_context_before && state->previous == MJB_LBP_SP) {
            return MJB_BT_NO_BREAK;
        }

        // LB18 Break after spaces.
        // SP ÷
        if(state->previous == MJB_LBP_SP) {
            state->pi_qu_context = false;

            return MJB_BT_ALLOWED;
        }

        // LB19 Do not break before non-initial unresolved quotation marks, such as ' " ' or ' " ',
        // nor after non-final unresolved quotation marks, such as ' " ' or ' " '.
        // × [ QU - \p{Pi} ]
        // [ QU - \p{Pf} ] ×
        if(state->current == MJB_LBP_QU) {
            // Save EA of char before QU for LB19a rule 4 (used when processing next pair)
            state->qu_prev_ea = prev_ea;

            // qu_cur_cat was already loaded above (before LB15a); reuse it here.
            // LB19 rule 1: × [QU - Pi]  (QU that is NOT Pi → no break before it)
            if(qu_cur_cat != MJB_CATEGORY_PI) {
                return MJB_BT_NO_BREAK;
            }

            // QU is Pi — check LB19a rules 1 and 2
            // LB19a rule 1: [^$EastAsian] × QU
            if(!mjb_is_ea(prev_ea)) {
                return MJB_BT_NO_BREAK;
            }

            // LB19a rule 2: × QU ( [^$EastAsian] | eot )
            // Look ahead: check EA of next character (or EOT)
            {
                mjb_east_asian_width next_ea = MJB_EAW_NOT_SET;
                mjb_lbp next_lbp = mjb_peek_next(buffer, size, state->index, encoding, &next_ea);

                if(next_lbp == MJB_LBP_NOT_SET || !mjb_is_ea(next_ea)) {
                    // EOT or next char is not East Asian → no break before this Pi-QU
                    return MJB_BT_NO_BREAK;
                }
            }

            // Both surrounding characters are East Asian → allow break (fall through to LB31)
        }

        if(state->previous == MJB_LBP_QU) {
            mjb_category prev_cat = mjb_lbp_category(state->previous_codepoint);

            // LB19 rule 2: [QU - Pf] ×  (QU that is NOT Pf → no break after it)
            if(prev_cat != MJB_CATEGORY_PF) {
                return MJB_BT_NO_BREAK;
            }

            // QU is Pf — check LB19a rules 3 and 4
            // LB19a rule 3: QU × [^$EastAsian]
            if(!mjb_is_ea(ea)) {
                return MJB_BT_NO_BREAK;
            }

            // LB19a rule 4: ( sot | [^$EastAsian] ) QU ×
            // qu_prev_ea holds EA of char before the QU (or NOT_SET = SOT)
            if(!mjb_is_ea(state->qu_prev_ea)) {
                return MJB_BT_NO_BREAK;
            }

            // Both surrounding chars are East Asian → allow break (fall through to LB31)
        }

        // LB20 Break before and after unresolved CB.
        // ÷ CB
        // CB ÷
        if(state->previous == MJB_LBP_CB || state->current == MJB_LBP_CB) {
            return MJB_BT_ALLOWED;
        }

        // LB20a Do not break after a word-initial hyphen.
        // ( sot | BK | CR | LF | NL | SP | ZW | CB | GL ) ( HY | HH ) × ( AL | HL )
        if(
            (state->previous == MJB_LBP_HY || state->previous == MJB_LBP_HH) &&
            (state->current == MJB_LBP_AL || state->current == MJB_LBP_HL) &&
            (
                state->prev_prev_lbp == MJB_LBP_NOT_SET || // sot
                state->prev_prev_lbp == MJB_LBP_BK  ||
                state->prev_prev_lbp == MJB_LBP_CR  ||
                state->prev_prev_lbp == MJB_LBP_LF  ||
                state->prev_prev_lbp == MJB_LBP_NL  ||
                state->prev_prev_lbp == MJB_LBP_SP  ||
                state->prev_prev_lbp == MJB_LBP_ZW  ||
                state->prev_prev_lbp == MJB_LBP_CB  ||
                state->prev_prev_lbp == MJB_LBP_GL
            )
        ) {
            return MJB_BT_NO_BREAK;
        }

        // LB21 Do not break before hyphen-minus, other hyphens, fixed-width spaces, small kana,
        // and other non-starters, or after acute accents.
        // × BA
        // × HH
        // × HY
        // × NS
        if(
            state->current == MJB_LBP_BA ||
            state->current == MJB_LBP_HH ||
            state->current == MJB_LBP_HY ||
            state->current == MJB_LBP_NS
        ) {
            return MJB_BT_NO_BREAK;
        }

        // BB ×
        if(state->previous == MJB_LBP_BB) {
            return MJB_BT_NO_BREAK;
        }

        // LB21a Do not break after the hyphen in Hebrew + Hyphen + non-Hebrew.
        // HL (HY | HH) × [^HL]
        if(
            (state->previous == MJB_LBP_HY || state->previous == MJB_LBP_HH) &&
            state->current != MJB_LBP_HL &&
            state->prev_prev_lbp == MJB_LBP_HL
        ) {
            return MJB_BT_NO_BREAK;
        }

        // LB21b Do not break between Solidus and Hebrew letters.
        // SY × HL
        if(state->previous == MJB_LBP_SY && state->current == MJB_LBP_HL) {
            return MJB_BT_NO_BREAK;
        }

        // LB22 Do not break before ellipses.
        // × IN
        if(state->current == MJB_LBP_IN) {
            return MJB_BT_NO_BREAK;
        }

        // LB23 Do not break between digits and letters.
        // (AL | HL) × NU
        // NU × (AL | HL)
        if(
            (
                (state->previous == MJB_LBP_AL || state->previous == MJB_LBP_HL) &&
                state->current == MJB_LBP_NU
            ) ||
            (
                state->previous == MJB_LBP_NU &&
                (state->current == MJB_LBP_AL || state->current == MJB_LBP_HL)
            )
        ) {
            return MJB_BT_NO_BREAK;
        }

        // LB23a Do not break between numeric prefixes and ideographs, or between ideographs and
        // numeric postfixes.
        // PR × (ID | EB | EM)
        // (ID | EB | EM) × PO
        if(
            state->previous == MJB_LBP_PR &&
            (
                state->current == MJB_LBP_ID ||
                state->current == MJB_LBP_EB ||
                state->current == MJB_LBP_EM
            )
        ) {
            return MJB_BT_NO_BREAK;
        }

        if(
            (
                state->previous == MJB_LBP_ID ||
                state->previous == MJB_LBP_EB ||
                state->previous == MJB_LBP_EM
            ) &&
            state->current == MJB_LBP_PO
        ) {
            return MJB_BT_NO_BREAK;
        }

        // LB24 Do not break between numeric prefix/postfix and letters, or between letters and
        // prefix / postfix.
        // (PR | PO) × (AL | HL)
        // (AL | HL) × (PR | PO)
        if(
            (state->previous == MJB_LBP_PR || state->previous == MJB_LBP_PO) &&
            (state->current == MJB_LBP_AL || state->current == MJB_LBP_HL)
        ) {
            return MJB_BT_NO_BREAK;
        }

        if(
            (state->previous == MJB_LBP_AL || state->previous == MJB_LBP_HL) &&
            (state->current == MJB_LBP_PR || state->current == MJB_LBP_PO)
        ) {
            return MJB_BT_NO_BREAK;
        }

        // LB25 Do not break numbers:
        // HY × NU
        // IS × NU
        if(
            (state->previous == MJB_LBP_HY || state->previous == MJB_LBP_IS) &&
            state->current == MJB_LBP_NU
        ) {
            return MJB_BT_NO_BREAK;
        }

        // PO × NU
        // PR × NU
        if(
            (state->previous == MJB_LBP_PO || state->previous == MJB_LBP_PR) &&
            state->current == MJB_LBP_NU
        ) {
            return MJB_BT_NO_BREAK;
        }

        // PO × OP NU  and  PR × OP NU
        // Two checks:
        // (a) At the (PO|PR) × OP position: look ahead to see if NU follows OP.
        // (b) At the OP × NU position: confirm prev_prev was PO|PR (redundant safety net).
        if(
            state->current == MJB_LBP_OP &&
            (state->previous == MJB_LBP_PO || state->previous == MJB_LBP_PR)
        ) {
            mjb_lbp next_lbp = mjb_peek_next(buffer, size, state->index, encoding, NULL);

            if(next_lbp == MJB_LBP_NU) {
                return MJB_BT_NO_BREAK;
            }
        }

        if(
            state->current == MJB_LBP_NU &&
            state->previous == MJB_LBP_OP &&
            (state->prev_prev_lbp == MJB_LBP_PO || state->prev_prev_lbp == MJB_LBP_PR)
        ) {
            return MJB_BT_NO_BREAK;
        }

        // NU (SY | IS)* × NU
        // NU (SY | IS)* × PO
        // NU (SY | IS)* × PR
        if(
            state->current == MJB_LBP_NU ||
            state->current == MJB_LBP_PO ||
            state->current == MJB_LBP_PR
        ) {
            if(state->previous == MJB_LBP_NU) {
                return MJB_BT_NO_BREAK;
            }

            if(
                (state->previous == MJB_LBP_IS || state->previous == MJB_LBP_SY) &&
                snap_num_lbp == MJB_LBP_NU
            ) {
                return MJB_BT_NO_BREAK;
            }
        }

        // NU (SY | IS)* CL × PO
        // NU (SY | IS)* CP × PO
        // NU (SY | IS)* CL × PR
        // NU (SY | IS)* CP × PR
        if(
            (state->current == MJB_LBP_PO || state->current == MJB_LBP_PR) &&
            (state->previous == MJB_LBP_CL || state->previous == MJB_LBP_CP) &&
            snap_num_lbp == MJB_LBP_NU
        ) {
            return MJB_BT_NO_BREAK;
        }

        // LB26 Do not break a Korean syllable.
        // JL × (JL | JV | H2 | H3)
        if(
            state->previous == MJB_LBP_JL &&
            (
                state->current == MJB_LBP_JL ||
                state->current == MJB_LBP_JV ||
                state->current == MJB_LBP_H2 ||
                state->current == MJB_LBP_H3
            )
        ) {
            return MJB_BT_NO_BREAK;
        }

        // (JV | H2) × (JV | JT)
        if(
            (state->previous == MJB_LBP_JV || state->previous == MJB_LBP_H2) &&
            (state->current == MJB_LBP_JV || state->current == MJB_LBP_JT)
        ) {
            return MJB_BT_NO_BREAK;
        }

        // (JT | H3) × JT
        if(
            (state->previous == MJB_LBP_JT || state->previous == MJB_LBP_H3) &&
            state->current == MJB_LBP_JT
        ) {
            return MJB_BT_NO_BREAK;
        }

        // LB27 Treat a Korean Syllable Block the same as ID.
        // (JL | JV | JT | H2 | H3) × PO
        if(
            (
                state->previous == MJB_LBP_JL ||
                state->previous == MJB_LBP_JV ||
                state->previous == MJB_LBP_JT ||
                state->previous == MJB_LBP_H2 ||
                state->previous == MJB_LBP_H3
            ) &&
            state->current == MJB_LBP_PO
        ) {
            return MJB_BT_NO_BREAK;
        }

        // PR × (JL | JV | JT | H2 | H3)
        if(
            state->previous == MJB_LBP_PR &&
            (
                state->current == MJB_LBP_JL ||
                state->current == MJB_LBP_JV ||
                state->current == MJB_LBP_JT ||
                state->current == MJB_LBP_H2 ||
                state->current == MJB_LBP_H3
            )
        ) {
            return MJB_BT_NO_BREAK;
        }

        // LB28 Do not break between alphabetics ("at").
        // (AL | HL) × (AL | HL)
        if(
            (state->previous == MJB_LBP_AL || state->previous == MJB_LBP_HL) &&
            (state->current == MJB_LBP_AL || state->current == MJB_LBP_HL)
        ) {
            return MJB_BT_NO_BREAK;
        }

        // LB28a Do not break inside the orthographic syllables of Brahmic scripts.
        // AP × (AK | [◌] | AS)
        // where [◌] = U+25CC DOTTED CIRCLE (has LBP=AL but treated specially here)
        if(
            state->previous == MJB_LBP_AP &&
            (
                state->current == MJB_LBP_AK ||
                state->current_codepoint == 0x25CC ||
                state->current == MJB_LBP_AS
            )
        ) {
            return MJB_BT_NO_BREAK;
        }

        // (AK | [◌] | AS) × (VF | VI)
        if(
            (
                state->previous == MJB_LBP_AK ||
                state->previous_codepoint == 0x25CC ||
                state->previous == MJB_LBP_AS
            ) &&
            (state->current == MJB_LBP_VF || state->current == MJB_LBP_VI)
        ) {
            return MJB_BT_NO_BREAK;
        }

        // (AK | [◌] | AS) VI × (AK | [◌])
        if(
            state->previous == MJB_LBP_VI &&
            (state->current == MJB_LBP_AK || state->current_codepoint == 0x25CC) &&
            (
                state->prev_prev_lbp == MJB_LBP_AK ||
                state->prev_prev_lbp == MJB_LBP_AS ||
                state->prev_prev_codepoint == 0x25CC
            )
        ) {
            return MJB_BT_NO_BREAK;
        }

        // (AK | [◌] | AS) × (AK | [◌] | AS) VF
        if(
            (
                state->previous == MJB_LBP_AK ||
                state->previous_codepoint == 0x25CC ||
                state->previous == MJB_LBP_AS
            ) &&
            (
                state->current == MJB_LBP_AK ||
                state->current_codepoint == 0x25CC ||
                state->current == MJB_LBP_AS
            )
        ) {
            mjb_lbp next_lbp = mjb_peek_next(buffer, size, state->index, encoding, NULL);

            if(next_lbp == MJB_LBP_VF) {
                return MJB_BT_NO_BREAK;
            }
        }

        // LB29 Do not break between numeric punctuation and alphabetics ("e.g.").
        // IS × (AL | HL)
        if(
            state->previous == MJB_LBP_IS &&
            (state->current == MJB_LBP_AL || state->current == MJB_LBP_HL)
        ) {
            return MJB_BT_NO_BREAK;
        }

        // LB30 Do not break between letters, numbers, or ordinary symbols and opening or closing
        // parentheses. $EastAsian = [\p{ea=F}\p{ea=W}\p{ea=H}]
        // (AL | HL | NU) × [OP-$EastAsian]
        // [CP-$EastAsian] × (AL | HL | NU)
        if(
            (
                state->previous == MJB_LBP_AL ||
                state->previous == MJB_LBP_HL ||
                state->previous == MJB_LBP_NU
            ) && state->current == MJB_LBP_OP && ea != MJB_EAW_FULL_WIDTH && ea != MJB_EAW_WIDE &&
                ea != MJB_EAW_HALF_WIDTH
        ) {
            return MJB_BT_NO_BREAK;
        }

        if(
            state->previous == MJB_LBP_CP &&
            (
                state->current == MJB_LBP_AL ||
                state->current == MJB_LBP_HL ||
                state->current == MJB_LBP_NU
            ) &&
            prev_ea != MJB_EAW_FULL_WIDTH && prev_ea != MJB_EAW_WIDE && prev_ea != MJB_EAW_HALF_WIDTH
        ) {
            return MJB_BT_NO_BREAK;
        }

        // LB30a Break between two regional indicator symbols if and only if there are an even
        // number of regional indicators preceding the position of the break.
        // sot (RI RI)* RI × RI
        // [^RI] (RI RI)* RI × RI
        if(state->previous == MJB_LBP_RI && state->current == MJB_LBP_RI) {
            mjb_break_type result = (state->ri_count++ % 2) == 0 ? MJB_BT_NO_BREAK : MJB_BT_ALLOWED;

            return result;
        } else {
            state->ri_count = 0;
        }

        // LB30b Do not break between an emoji base (or potential emoji) and an emoji modifier.
        // EB × EM
        // [\p{Extended_Pictographic}&\p{Cn}] × EM
        if(state->current == MJB_LBP_EM) {
            if(state->previous == MJB_LBP_EB) {
                return MJB_BT_NO_BREAK;
            }

            // Check [\p{Extended_Pictographic} & Cn]
            // extended_pictographic IS stored in property_ranges blob.
            // Cn (unassigned) means the codepoint has no row in unicode_data.
            uint8_t prev_props[MJB_PR_BUFFER_SIZE] = {0};
            mjb_codepoint_properties(state->previous_codepoint, prev_props);
            uint8_t ext_pic = mjb_codepoint_property(prev_props, MJB_PR_EXTENDED_PICTOGRAPHIC);

            if(ext_pic != 0) {
                // A codepoint is Cn iff it has no entry in unicode_data.
                sqlite3_reset(mjb_global.stmt_line_breaking);
                sqlite3_bind_int(mjb_global.stmt_line_breaking, 1, (int)state->previous_codepoint);
                bool is_assigned = (sqlite3_step(mjb_global.stmt_line_breaking) == SQLITE_ROW);
                sqlite3_reset(mjb_global.stmt_line_breaking);

                if(!is_assigned) {
                    return MJB_BT_NO_BREAK;
                }
            }
        }

        // LB31 Break everywhere else.
        // ALL ÷
        // ÷ ALL
        return MJB_BT_ALLOWED;
    }

    ++state->index;

    return MJB_BT_ALLOWED;
}
