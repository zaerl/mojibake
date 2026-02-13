/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "mojibake-internal.h"
#include "utf.h"

#include <string.h>

extern mojibake mjb_global;

// Word and Grapheme Cluster Breaking
// See: https://unicode.org/reports/tr29/
MJB_EXPORT mjb_break_type mjb_segmentation(const char *buffer, size_t size, mjb_encoding encoding,
    mjb_next_state *state) {
    // bool has_previous_character = false;
    // bool first_character = true;

    if(size == 0) {
        return MJB_BT_NOT_SET;
    }

    if(state->index == 0) {
        // Initialize the state.
        state->state = MJB_UTF_ACCEPT;
        state->previous = MJB_GBP_NOT_SET;
        state->current = MJB_GBP_NOT_SET;
        state->in_error = false;
        state->ri_count = 0;
        state->ext_pict_seen = false;
        state->zwj_seen = false;
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

        mjb_gcb gcb = MJB_GBP_NOT_SET;
        mjb_codepoint_has_property(codepoint, MJB_PR_GRAPHEME_CLUSTER_BREAK, (uint8_t*)&gcb);

        if(gcb == MJB_GBP_NOT_SET) {
            // # @missing: 0000..10FFFF; Other
            gcb = MJB_GBP_OTHER;
        }

        if(first_codepoint) {
            // First codepoint
            state->current = gcb;
            first_codepoint = false;

            continue;
        }

        // Swap previous and current codepoints
        state->previous = state->current;
        state->current = gcb;

        bool prev_ext_pict_zwj = state->ext_pict_seen && state->zwj_seen;

        // Do not break between a CR and LF. Otherwise, break before and after controls.
        // GB3 CR × LF
        if(state->previous == MJB_GBP_CR && state->current == MJB_GBP_LF) {
            return MJB_BT_NO_BREAK;
        }

        // GB4 (Control | CR | LF) ÷
        if(
            state->previous == MJB_GBP_CONTROL ||
            state->previous == MJB_GBP_CR ||
            state->previous == MJB_GBP_LF
        ) {
            return MJB_BT_ALLOWED;
        }

        // GB5 ÷ (Control | CR | LF)
        if(
            state->current == MJB_GBP_CONTROL ||
            state->current == MJB_GBP_CR ||
            state->current == MJB_GBP_LF
        ) {
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
            return MJB_BT_NO_BREAK;
        }

        // Do not break before extending characters or ZWJ.
        // GB9 × (Extend | ZWJ)
        if(
            state->current == MJB_GBP_EXTEND ||
            state->current == MJB_GBP_ZWJ
        ) {
            return MJB_BT_NO_BREAK;
        }

        // The GB9a and GB9b rules only apply to extended grapheme clusters:
        // Do not break before SpacingMarks, or after Prepend characters.
        // GB9a × SpacingMark
        if(
            state->current == MJB_GBP_SPACING_MARK
        ) {
            return MJB_BT_NO_BREAK;
        }

        // GB9b Prepend ×
        if(
            state->previous == MJB_GBP_PREPEND
        ) {
            return MJB_BT_NO_BREAK;
        }

        // The GB9c rule only applies to extended grapheme clusters:
        // Do not break within certain combinations with Indic_Conjunct_Break (InCB)=Linker.
        // GB9c \p{InCB=Consonant} [ \p{InCB=Extend} \p{InCB=Linker} ]* \p{InCB=Linker}
        //   [ \p{InCB=Extend} \p{InCB=Linker} ]* × \p{InCB=Consonant}

        // Do not break within emoji modifier sequences or emoji zwj sequences.
        // GB11 \p{Extended_Pictographic} Extend* ZWJ × \p{Extended_Pictographic}
        if(
            prev_ext_pict_zwj &&
            mjb_codepoint_has_property(codepoint, MJB_PR_EXTENDED_PICTOGRAPHIC, NULL)
        ) {
            return MJB_BT_NO_BREAK;
        }

        // Do not break within emoji flag sequences. That is, do not break between regional
        // indicator (RI) symbols if there is an odd number of RI characters before the break point.
        // GB12 sot (RI RI)* RI × RI
        // GB13 [^RI] (RI RI)* RI × RI
        if(state->previous == MJB_GBP_REGIONAL_INDICATOR && state->current == MJB_GBP_REGIONAL_INDICATOR) {
            return (state->ri_count++ % 2) == 0 ? MJB_BT_NO_BREAK : MJB_BT_ALLOWED;
        } else {
            state->ri_count = 0;
        }

        state->ext_pict_seen = false;
        state->zwj_seen = false;

        // Otherwise, break everywhere.
        // GB999 Any ÷ Any

        if(mjb_codepoint_has_property(codepoint, MJB_PR_EXTENDED_PICTOGRAPHIC, NULL)) {
            state->ext_pict_seen = true;
        } else if(gcb != MJB_GBP_EXTEND) {
            state->ext_pict_seen = false;
        }

        state->zwj_seen = gcb == MJB_GBP_ZWJ;

        return MJB_BT_ALLOWED;
    }

    ++state->index;

    return MJB_BT_ALLOWED;
}
