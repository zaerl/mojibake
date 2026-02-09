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
MJB_EXPORT bool mjb_segmentation(const char *buffer, size_t size, mjb_encoding encoding,
    mjb_next_state *state) {
    // bool has_previous_character = false;
    // bool first_character = true;

    if(size == 0) {
        return false;
    }

    if(state->count == 0) {
        // Initialize the state.
        state->state = MJB_UTF_ACCEPT;
        state->index = 0;
        state->count = 0;
        state->previous = MJB_GBP_OTHER;
        state->current = MJB_GBP_OTHER;
        state->in_error = false;
        state->ri_count = 0;
        state->ext_pict_seen = false;
        state->zwj_seen = false;
    }

    mjb_codepoint codepoint;
    mjb_gcb gcb;

    for(; state->index < size; ) {  // No loop increment - mjb_next_codepoint manages index
        mjb_decode_result decode_status = mjb_next_codepoint(buffer, size, &state->state,
            &state->index, encoding, &codepoint, &state->in_error);

        if(decode_status == MJB_DECODE_END) {
            return false;
        }

        if(decode_status == MJB_DECODE_INCOMPLETE) {
            continue;
        }

        // Break at the start and end of text, unless the text is empty.
        // GB1 sot ÷ Any
        // Not needed

        mjb_codepoint_has_property(codepoint, MJB_PR_GRAPHEME_CLUSTER_BREAK, (uint8_t*)&gcb);

        if(gcb == MJB_GBP_NOT_SET) {
            // # @missing: 0000..10FFFF; Other
            gcb = MJB_GBP_OTHER;
        }

        bool prev_ext_pict_zwj = state->ext_pict_seen && state->zwj_seen;

        // Swap previous and current codepoints
        state->previous = state->current;
        state->current = gcb;
        ++state->count;

        if(state->count > 1) {
            // Do not break between a CR and LF. Otherwise, break before and after controls.
            // GB3 CR × LF
            if(state->previous == MJB_GBP_CR && state->current == MJB_GBP_LF) {
                return false;
            }

            // GB4 (Control | CR | LF) ÷
            if(
                state->previous == MJB_GBP_CONTROL ||
                state->previous == MJB_GBP_CONTROL ||
                state->previous == MJB_GBP_CR ||
                state->previous == MJB_GBP_LF
            ) {
                return true;
            }

            // GB5 ÷ (Control | CR | LF)
            if(
                state->current == MJB_GBP_CONTROL ||
                state->current == MJB_GBP_CR ||
                state->current == MJB_GBP_LF
            ) {
                return true;
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
                return false;
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
                return false;
            }

            // GB8 (LVT | T) × T
            if(
                (
                    state->previous == MJB_GBP_LVT ||
                    state->previous == MJB_GBP_T
                ) &&
                state->current == MJB_GBP_T
            ) {
                return false;
            }

            // Do not break before extending characters or ZWJ.
            // GB9 × (Extend | ZWJ)
            if(
                state->current == MJB_GBP_EXTEND ||
                state->current == MJB_GBP_ZWJ
            ) {
                return false;
            }

            // The GB9a and GB9b rules only apply to extended grapheme clusters:
            // Do not break before SpacingMarks, or after Prepend characters.
            // GB9a × SpacingMark
            if(
                state->current == MJB_GBP_SPACING_MARK
            ) {
                return false;
            }

            // GB9b Prepend ×
            if(
                state->previous == MJB_GBP_PREPEND
            ) {
                return false;
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
                return false;
            }

            // Do not break within emoji flag sequences. That is, do not break between regional
            // indicator (RI) symbols if there is an odd number of RI characters before the break point.
            // GB12 sot (RI RI)* RI × RI
            // GB13 [^RI] (RI RI)* RI × RI
            if(state->previous == MJB_GBP_REGIONAL_INDICATOR && state->current == MJB_GBP_REGIONAL_INDICATOR) {
                ++state->ri_count;

                return (state->ri_count % 2) == 0;
            } else {
                state->ri_count = 0;
            }

            state->ext_pict_seen = false;
            state->zwj_seen = false;
        }

        if(mjb_codepoint_has_property(codepoint, MJB_PR_EXTENDED_PICTOGRAPHIC, NULL)) {
            state->ext_pict_seen = true;
        } else if(gcb != MJB_GBP_EXTEND) {
            state->ext_pict_seen = false;
        }

        state->zwj_seen = gcb == MJB_GBP_ZWJ;

        // Otherwise, break everywhere.
        // GB999 Any ÷ Any
        return true;
    }

    return false;
}
