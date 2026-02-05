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
        state->previous_codepoint = MJB_CODEPOINT_NOT_VALID;
        state->codepoint = MJB_CODEPOINT_NOT_VALID;
        state->in_error = false;
        memset(state->previous_properties, 0, MJB_PR_BUFFER_SIZE);
        memset(state->properties, 0, MJB_PR_BUFFER_SIZE);
    }

    mjb_codepoint codepoint;

    #define MJB_PREVIOUS_CP(VALUE) mjb_codepoint_property(state->previous_properties, MJB_PR_GRAPHEME_CLUSTER_BREAK) == VALUE
    #define MJB_CURRENT_CP(VALUE) mjb_codepoint_property(state->properties, MJB_PR_GRAPHEME_CLUSTER_BREAK) == VALUE

    for(; state->index < size; ) {  // No loop increment - mjb_next_codepoint manages index
        mjb_decode_result decode_status = mjb_next_codepoint(buffer, size, &state->state,
            &state->index, encoding, &codepoint, &state->in_error);

        if(decode_status == MJB_DECODE_END) {
            return false;
        }

        if(decode_status == MJB_DECODE_INCOMPLETE) {
            continue;
        }

        ++state->count;

        // Break at the start and end of text, unless the text is empty.
        // GB1 sot ÷ Any
        if(state->count == 1) {
            mjb_codepoint_properties(state->codepoint, state->properties);
            state->codepoint = codepoint;

            return true;
        }

        // Swap previous and current codepoints
        state->previous_codepoint = state->codepoint;
        memcpy(state->previous_properties, state->properties, MJB_PR_BUFFER_SIZE);

        state->codepoint = codepoint;
        mjb_codepoint_properties(state->codepoint, state->properties);

        // GB2 Any ÷ eot
        // Not handled

        // Do not break between a CR and LF. Otherwise, break before and after controls.
        // GB3 CR × LF
        if(state->previous_codepoint == 0x0D && state->codepoint == 0x0A) {
            continue;
        }

        mjb_character character;

        if(!mjb_codepoint_character(state->previous_codepoint, &character)) {
            continue;
        }

        // GB4 (Control | CR | LF) ÷
        if(
            MJB_PREVIOUS_CP(MJB_GBP_CONTROL) ||
            MJB_PREVIOUS_CP(MJB_GBP_CONTROL) ||
            state->previous_codepoint == 0x0D ||
            state->previous_codepoint == 0x0A
        ) {
            return true;
        }

        // GB5 ÷ (Control | CR | LF)
        if(
            MJB_CURRENT_CP(MJB_GBP_CONTROL) ||
            state->codepoint == 0x0D ||
            state->codepoint == 0x0A
        ) {
            return true;
        }

        // Do not break Hangul syllable or other conjoining sequences.
        // GB6 L × (L | V | LV | LVT)
        if(
            MJB_PREVIOUS_CP(MJB_GBP_L) &&
            (
                MJB_CURRENT_CP(MJB_GBP_L) ||
                MJB_CURRENT_CP(MJB_GBP_V) ||
                MJB_CURRENT_CP(MJB_GBP_LV) ||
                MJB_CURRENT_CP(MJB_GBP_LVT)
            )
        ) {
            continue;
        }

        // GB7 (LV | V) × (V | T)
        if(
            (
                MJB_PREVIOUS_CP(MJB_GBP_LV) ||
                MJB_PREVIOUS_CP(MJB_GBP_V)
            ) &&
            (
                MJB_CURRENT_CP(MJB_GBP_V) ||
                MJB_CURRENT_CP(MJB_GBP_T)
            )
        ) {
            continue;
        }

        // GB8 (LVT | T) × T
        if(
            (
                MJB_PREVIOUS_CP(MJB_GBP_LVT) ||
                MJB_PREVIOUS_CP(MJB_GBP_T)
            ) &&
            MJB_CURRENT_CP(MJB_GBP_T)
        ) {
            continue;
        }

        // Do not break before extending characters or ZWJ.
        // GB9 × (Extend | ZWJ)
        if(
            MJB_CURRENT_CP(MJB_GBP_EXTEND) ||
            MJB_CURRENT_CP(MJB_GBP_ZWJ)
        ) {
            continue;
        }

        // The GB9a and GB9b rules only apply to extended grapheme clusters:
        // Do not break before SpacingMarks, or after Prepend characters.
        // GB9a × SpacingMark
        if(
            MJB_CURRENT_CP(MJB_GBP_SPACING_MARK)
        ) {
            continue;
        }

        // GB9b Prepend ×
        if(
            MJB_PREVIOUS_CP(MJB_GBP_PREPEND)
        ) {
            continue;
        }

        // The GB9c rule only applies to extended grapheme clusters:
        // Do not break within certain combinations with Indic_Conjunct_Break (InCB)=Linker.
        // GB9c \p{InCB=Consonant} [ \p{InCB=Extend} \p{InCB=Linker} ]* \p{InCB=Linker}
        //   [ \p{InCB=Extend} \p{InCB=Linker} ]* × \p{InCB=Consonant}

        // Do not break within emoji modifier sequences or emoji zwj sequences.
        // GB11 \p{Extended_Pictographic} Extend* ZWJ × \p{Extended_Pictographic}

        // Do not break within emoji flag sequences. That is, do not break between regiona
        // indicator (RI) symbols if there is an odd number of RI characters before the break point.
        // GB12 sot (RI RI)* RI × RI
        // GB13 [^RI] (RI RI)* RI × RI

        // Otherwise, break everywhere.
        // GB999 Any ÷ Any

        return true;

        // Word Boundary Rules
        // Break at the start and end of text, unless the text is empty.
        // WB1 sot ÷ Any
        // WB2 Any ÷ eot

        // Do not break within CRLF.
        // WB3 CR × LF

        // Otherwise break before and after Newlines (including CR and LF)
        // WB3a (Newline | CR | LF) ÷
        // WB3b ÷ (Newline | CR | LF)

        // Do not break within emoji zwj sequences.
        // WB3c ZWJ × \p{Extended_Pictographic}

        // Keep horizontal whitespace together.
        // WB3d WSegSpace × WSegSpace

        // Ignore Format and Extend characters, except after sot, CR, LF, and Newline. (See Section
        // 6.2, Replacing Ignore Rules.) This also has the effect of: Any × (Format | Extend | ZWJ)
        // WB4 X (Extend | Format | ZWJ)* → X

        // Do not break between most letters.
        // WB5 AHLetter × AHLetter

        // Do not break letters across certain punctuation, such as within “e.g.” or “example.com”.
        // WB6 AHLetter × (MidLetter | MidNumLetQ) AHLetter
        // WB7 AHLetter (MidLetter | MidNumLetQ) × AHLetter
        // WB7a Hebrew_Letter × Single_Quote
        // WB7b Hebrew_Letter × Double_Quote Hebrew_Letter
        // WB7c Hebrew_Letter Double_Quote × Hebrew_Letter

        // Do not break within sequences of digits, or digits adjacent to letters (“3a”, or “A3”).
        // WB8 Numeric × Numeric
        // WB9 AHLetter × Numeric
        // WB10 Numeric × AHLetter

        // Do not break within sequences, such as “3.2” or “3,456.789”.
        // WB11 Numeric (MidNum | MidNumLetQ) × Numeric
        // WB12 Numeric × (MidNum | MidNumLetQ) Numeric

        // Do not break between Katakana.
        // WB13 Katakana × Katakana

        // Do not break from extenders.
        // WB13a (AHLetter | Numeric | Katakana | ExtendNumLet) × ExtendNumLet
        // WB13b ExtendNumLet × (AHLetter | Numeric | Katakana)

        // Do not break within emoji flag sequences. That is, do not break between regional
        //   indicator (RI) symbols if there is an odd number of RI characters before the break
        //   point.
        // WB15 sot (RI RI)* RI × RI
        // WB16 [^RI] (RI RI)* RI × RI

        // Otherwise, break everywhere (including around ideographs).
        // WB999 Any ÷ Any
    }

    return false;
}
