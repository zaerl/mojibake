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

static inline void mjb_update_sequence_flags(mjb_next_state *state, uint8_t *buffer) {
    // Update GB11: Extended_Pictographic + ZWJ sequences
    if(mjb_codepoint_property(buffer, MJB_PR_EXTENDED_PICTOGRAPHIC)) {
        // Start of new Extended_Pictographic sequence
        state->ext_pict_seen = true;
        state->zwj_seen = false;
    } else if(state->current == MJB_GBP_ZWJ && state->ext_pict_seen) {
        // ZWJ after Extended_Pictographic, mark sequence as ExtPict...ZWJ
        state->zwj_seen = true;
    } else if(state->current != MJB_GBP_EXTEND) {
        // Break in sequence (not Extend, not ZWJ, not ExtPict)
        state->ext_pict_seen = false;
        state->zwj_seen = false;
    }

    // Update GB9c: Indic Conjunct Break sequences
    uint8_t incb_value = mjb_codepoint_property(buffer, MJB_PR_INDIC_CONJUNCT_BREAK);

    if(incb_value == MJB_INCB_CONSONANT) {
        // Start new Consonant sequence
        state->incb_consonant_seen = true;
        state->incb_linker_seen = false;
    } else if(incb_value == MJB_INCB_LINKER && state->incb_consonant_seen) {
        // Mark that we've seen a Linker in this sequence
        state->incb_linker_seen = true;
    } else if(incb_value == MJB_INCB_NOT_SET || (incb_value != MJB_INCB_EXTEND && incb_value != MJB_INCB_LINKER)) {
        // Break in sequence: character is not InCB-relevant, or is None
        // Reset unless it's Extend or Linker (which continue the sequence)
        state->incb_consonant_seen = false;
        state->incb_linker_seen = false;
    }
}

// Word and Grapheme Cluster Breaking
// See: https://unicode.org/reports/tr29/
MJB_EXPORT mjb_break_type mjb_segmentation(const char *buffer, size_t size, mjb_encoding encoding,
    mjb_next_state *state) {
    if(size == 0) {
        return MJB_BT_NOT_SET;
    }

    if(state->index == 0) {
        // Initialize the state.
        state->state = MJB_UTF_ACCEPT;
        state->previous = MJB_GBP_NOT_SET;
        state->current = MJB_GBP_NOT_SET;
        state->previous_codepoint = MJB_CODEPOINT_NOT_VALID;
        state->current_codepoint = MJB_CODEPOINT_NOT_VALID;
        state->in_error = false;
        state->ri_count = 0;
        state->ext_pict_seen = false;
        state->zwj_seen = false;
        state->incb_consonant_seen = false;
        state->incb_linker_seen = false;
    }

    if(state->index == size) {
        // Reached end of string.
        ++state->index;

        // GB2 Any ÷ eot
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
        // GB1 sot ÷ Any
        // Not needed

        memset(cpb, 0, MJB_PR_BUFFER_SIZE);
        mjb_codepoint_properties(codepoint, cpb);
        mjb_gcb gcb = (mjb_gcb)mjb_codepoint_property(cpb, MJB_PR_GRAPHEME_CLUSTER_BREAK);

        if(gcb == MJB_GBP_NOT_SET) {
            // # @missing: 0000..10FFFF; Other
            gcb = MJB_GBP_OTHER;
        }

        if(first_codepoint) {
            // First codepoint
            state->current = gcb;
            state->current_codepoint = codepoint;
            first_codepoint = false;
            mjb_update_sequence_flags(state, cpb);

            continue;
        }

        // Swap previous and current codepoints
        state->previous = state->current;
        state->current = gcb;
        state->previous_codepoint = state->current_codepoint;
        state->current_codepoint = codepoint;

        bool prev_ext_pict_zwj = state->ext_pict_seen && state->zwj_seen;

        // Do not break between a CR and LF. Otherwise, break before and after controls.
        // GB3 CR × LF
        if(state->previous == MJB_GBP_CR && state->current == MJB_GBP_LF) {
            mjb_update_sequence_flags(state, cpb);

            return MJB_BT_NO_BREAK;
        }

        // GB4 (Control | CR | LF) ÷
        if(
            state->previous == MJB_GBP_CONTROL ||
            state->previous == MJB_GBP_CR ||
            state->previous == MJB_GBP_LF
        ) {
            mjb_update_sequence_flags(state, cpb);

            return MJB_BT_ALLOWED;
        }

        // GB5 ÷ (Control | CR | LF)
        if(
            state->current == MJB_GBP_CONTROL ||
            state->current == MJB_GBP_CR ||
            state->current == MJB_GBP_LF
        ) {
            mjb_update_sequence_flags(state, cpb);

            return MJB_BT_ALLOWED;
        }

        // Do not break Hangul syllable or other conjoining sequences.
        // GB6 L × (L | V | LV | LVT)
        if(
            state->previous == MJB_GBP_L &&
            (
                state->current == MJB_GBP_L ||
                state->current == MJB_GBP_V ||
                state->current == MJB_GBP_LV ||
                state->current == MJB_GBP_LVT
            )
        ) {
            mjb_update_sequence_flags(state, cpb);

            return MJB_BT_NO_BREAK;
        }

        // GB7 (LV | V) × (V | T)
        if(
            (
                state->previous == MJB_GBP_LV ||
                state->previous == MJB_GBP_V
            ) &&
            (
                state->current == MJB_GBP_V ||
                state->current == MJB_GBP_T
            )
        ) {
            mjb_update_sequence_flags(state, cpb);

            return MJB_BT_NO_BREAK;
        }

        // GB8 (LVT | T) × T
        if(
            (
                state->previous == MJB_GBP_LVT ||
                state->previous == MJB_GBP_T
            ) &&
            state->current == MJB_GBP_T
        ) {
            mjb_update_sequence_flags(state, cpb);

            return MJB_BT_NO_BREAK;
        }

        // Do not break before extending characters or ZWJ.
        // GB9 × (Extend | ZWJ)
        if(
            state->current == MJB_GBP_EXTEND ||
            state->current == MJB_GBP_ZWJ
        ) {
            mjb_update_sequence_flags(state, cpb);

            return MJB_BT_NO_BREAK;
        }

        // The GB9a and GB9b rules only apply to extended grapheme clusters:
        // Do not break before SpacingMarks, or after Prepend characters.
        // GB9a × SpacingMark
        if(
            state->current == MJB_GBP_SPACING_MARK
        ) {
            mjb_update_sequence_flags(state, cpb);

            return MJB_BT_NO_BREAK;
        }

        // GB9b Prepend ×
        if(
            state->previous == MJB_GBP_PREPEND
        ) {
            mjb_update_sequence_flags(state, cpb);

            return MJB_BT_NO_BREAK;
        }

        // The GB9c rule only applies to extended grapheme clusters:
        // Do not break within certain combinations with Indic_Conjunct_Break (InCB)=Linker.
        // GB9c \p{InCB=Consonant} [ \p{InCB=Extend} \p{InCB=Linker} ]* \p{InCB=Linker}
        //   [ \p{InCB=Extend} \p{InCB=Linker} ]* × \p{InCB=Consonant}
        uint8_t curr_incb = mjb_codepoint_property(cpb, MJB_PR_INDIC_CONJUNCT_BREAK);

        if(curr_incb != MJB_INCB_NOT_SET &&
            state->incb_consonant_seen &&
            state->incb_linker_seen &&
            curr_incb == MJB_INCB_CONSONANT) {
            mjb_update_sequence_flags(state, cpb);

            return MJB_BT_NO_BREAK;
        }

        // Do not break within emoji modifier sequences or emoji zwj sequences.
        // GB11 \p{Extended_Pictographic} Extend* ZWJ × \p{Extended_Pictographic}
        if(
            prev_ext_pict_zwj &&
            mjb_codepoint_property(cpb, MJB_PR_EXTENDED_PICTOGRAPHIC)
        ) {
            mjb_update_sequence_flags(state, cpb);

            return MJB_BT_NO_BREAK;
        }

        // Do not break within emoji flag sequences. That is, do not break between regional
        // indicator (RI) symbols if there is an odd number of RI characters before the break point.
        // GB12 sot (RI RI)* RI × RI
        // GB13 [^RI] (RI RI)* RI × RI
        if(state->previous == MJB_GBP_REGIONAL_INDICATOR && state->current == MJB_GBP_REGIONAL_INDICATOR) {
            mjb_break_type result = (state->ri_count++ % 2) == 0 ? MJB_BT_NO_BREAK : MJB_BT_ALLOWED;
            mjb_update_sequence_flags(state, cpb);

            return result;
        } else {
            state->ri_count = 0;
        }

        // Otherwise, break everywhere.
        // GB999 Any ÷ Any
        mjb_update_sequence_flags(state, cpb);

        return MJB_BT_ALLOWED;
    }

    ++state->index;

    return MJB_BT_ALLOWED;
}
