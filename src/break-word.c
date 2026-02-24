/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <string.h>

#include "mojibake-internal.h"
#include "utf.h"

extern mojibake mjb_global;

// Word cluster breaking
// See: https://unicode.org/reports/tr29/
MJB_EXPORT mjb_break_type mjb_break_word(const char *buffer, size_t size, mjb_encoding encoding,
    mjb_next_word_state *state) {
    if(size == 0) {
        return MJB_BT_NOT_SET;
    }

    if(state->index == 0) {
        // Initialize the state.
        state->state = MJB_UTF_ACCEPT;
        state->previous = MJB_WBP_NOT_SET;
        state->current = MJB_WBP_NOT_SET;
        state->prev_prev_codepoint = MJB_CODEPOINT_NOT_VALID;
        state->previous_codepoint = MJB_CODEPOINT_NOT_VALID;
        state->current_codepoint = MJB_CODEPOINT_NOT_VALID;
        state->prev_prev_wbp = MJB_WBP_NOT_SET;
        state->in_error = false;
        state->ri_count = 0;
    }

    if(state->index == size) {
        // Reached end of string.
        ++state->index;

        // WB2 Any ÷ eot
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
        // WB1 sot ÷ Any
        // Not needed

        memset(cpb, 0, MJB_PR_BUFFER_SIZE);
        mjb_codepoint_properties(codepoint, cpb);
        mjb_wbp wbp = (mjb_wbp)mjb_codepoint_property(cpb, MJB_PR_WORD_BREAK);

        if(wbp == MJB_WBP_NOT_SET) {
            // # @missing: 0000..10FFFF; Other
            wbp = MJB_WBP_OTHER;
        }

        if(first_codepoint) {
            // First codepoint
            state->current = wbp;
            state->current_codepoint = codepoint;
            first_codepoint = false;

            continue;
        }

        state->prev_prev_wbp = state->previous;
        state->prev_prev_codepoint = state->previous_codepoint;

        // Swap previous and current codepoints
        state->previous = state->current;
        state->current = wbp;
        state->previous_codepoint = state->current_codepoint;
        state->current_codepoint = codepoint;

        // Do not break within CRLF
        // WB3 CR × LF
        if(state->previous == MJB_WBP_CR && state->current == MJB_WBP_LF) {
            return MJB_BT_NO_BREAK;
        }

        // Otherwise break before and after Newlines (including CR and LF)
        // (Newline | CR | LF) ÷
        if(
            state->previous == MJB_WBP_NEWLINE ||
            state->previous == MJB_WBP_CR ||
            state->previous == MJB_WBP_LF
        ) {
            return MJB_BT_ALLOWED;
        }

        // ÷ (Newline | CR | LF)
        if(
            state->current == MJB_WBP_NEWLINE ||
            state->current == MJB_WBP_CR ||
            state->current == MJB_WBP_LF
        ) {
            return MJB_BT_ALLOWED;
        }

        // Do not break within emoji zwj sequences.
        // WB3c ZWJ × \p{Extended_Pictographic}

        // Keep horizontal whitespace together.
        // WB3d WSegSpace × WSegSpace
        if(
            state->previous == MJB_WBP_W_SEG_SPACE ||
            state->current == MJB_WBP_W_SEG_SPACE
        ) {
            return MJB_BT_NO_BREAK;
        }

        // Ignore Format and Extend characters, except after sot, CR, LF, and Newline. (See Section
        // 6.2, Replacing Ignore Rules.) This also has the effect of: Any × (Format | Extend | ZWJ)
        // WB4 X (Extend | Format | ZWJ)* -> X

        // Do not break between most letters
        // WB5 AHLetter × AHLetter
        if(
            (state->previous == MJB_WBP_A_LETTER || state->previous == MJB_WBP_HEBREW_LETTER) &&
            (state->current == MJB_WBP_A_LETTER || state->current == MJB_WBP_HEBREW_LETTER)
        ) {
            return MJB_BT_NO_BREAK;
        }

        // Do not break letters across certain punctuation, such as within "e.g." or "example.com".
        // WB6 AHLetter × (MidLetter | MidNumLetQ) AHLetter

        // WB7 AHLetter (MidLetter | MidNumLetQ) × AHLetter
        if(
            (state->prev_prev_wbp == MJB_WBP_A_LETTER || state->prev_prev_wbp == MJB_WBP_HEBREW_LETTER) &&
            (
                state->previous == MJB_WBP_MID_LETTER ||
                (state->previous == MJB_WBP_MID_NUM_LET && state->current == MJB_WBP_SINGLE_QUOTE)
            ) &&
            (state->current == MJB_WBP_A_LETTER || state->current == MJB_WBP_HEBREW_LETTER)
        ) {
            return MJB_BT_NO_BREAK;
        }

        // WB7a Hebrew_Letter × Single_Quote
        if(state->previous == MJB_WBP_HEBREW_LETTER && state->current == MJB_WBP_SINGLE_QUOTE) {
            return MJB_BT_NO_BREAK;
        }

        // WB7b Hebrew_Letter × Double_Quote Hebrew_Letter

        // WB7c Hebrew_Letter Double_Quote × Hebrew_Letter
        if(
            state->prev_prev_wbp == MJB_WBP_HEBREW_LETTER &&
            state->previous == MJB_WBP_DOUBLE_QUOTE &&
            state->current == MJB_WBP_HEBREW_LETTER
        ) {
            return MJB_BT_NO_BREAK;
        }

        // Do not break within sequences of digits, or digits adjacent to letters ("3a", or "A3").
        // WB8 Numeric × Numeric
        if(
            state->previous == MJB_WBP_NUMERIC &&
            state->current == MJB_WBP_NUMERIC
        ) {
            return MJB_BT_NO_BREAK;
        }

        // WB9 AHLetter × Numeric
        if(
            (state->previous == MJB_WBP_A_LETTER || state->previous == MJB_WBP_HEBREW_LETTER) &&
            state->current == MJB_WBP_NUMERIC
        ) {
            return MJB_BT_NO_BREAK;
        }

        // WB10 Numeric × AHLetter
        if(
            state->previous == MJB_WBP_NUMERIC &&
            (state->current == MJB_WBP_A_LETTER || state->current == MJB_WBP_HEBREW_LETTER)
        ) {
            return MJB_BT_NO_BREAK;
        }

        // Do not break within sequences, such as "3.2" or "3,456.789".
        // WB11 Numeric (MidNum | MidNumLetQ) × Numeric
        // WB12 Numeric × (MidNum | MidNumLetQ) Numeric

        // Do not break between Katakana.
        // WB13 Katakana × Katakana
        if(state->previous == MJB_WBP_KATAKANA && state->current == MJB_WBP_KATAKANA) {
            return MJB_BT_NO_BREAK;
        }

        // Do not break from extenders.
        // WB13a (AHLetter | Numeric | Katakana | ExtendNumLet) × ExtendNumLet
        if(
            (
                (state->previous == MJB_WBP_A_LETTER || state->previous == MJB_WBP_HEBREW_LETTER) ||
                state->previous == MJB_WBP_NUMERIC ||
                state->previous == MJB_WBP_KATAKANA ||
                state->previous == MJB_WBP_EXTEND_NUM_LET
            ) &&
            state->current == MJB_WBP_EXTEND_NUM_LET
        ) {
            return MJB_BT_NO_BREAK;
        }

        // WB13b ExtendNumLet × (AHLetter | Numeric | Katakana)
        if(
            state->previous == MJB_WBP_EXTEND_NUM_LET &&
            (
                (state->current == MJB_WBP_A_LETTER || state->current == MJB_WBP_HEBREW_LETTER) ||
                state->current == MJB_WBP_NUMERIC ||
                state->current == MJB_WBP_KATAKANA
            )
        ) {
            return MJB_BT_NO_BREAK;
        }

        // Do not break within emoji flag sequences. That is, do not break between regional indicator
        // (RI) symbols if there is an odd number of RI characters before the break point.
        // WB15 sot (RI RI)* RI × RI
        // WB16 [^RI] (RI RI)* RI × RI
        if(state->previous == MJB_WBP_REGIONAL_INDICATOR && state->current == MJB_WBP_REGIONAL_INDICATOR) {
            mjb_break_type result = (state->ri_count++ % 2) == 0 ? MJB_BT_NO_BREAK : MJB_BT_ALLOWED;

            return result;
        } else {
            state->ri_count = 0;
        }

        // Otherwise, break everywhere (including around ideographs).
        // WB999 Any ÷ Any
        return MJB_BT_ALLOWED;
    }

    ++state->index;

    return MJB_BT_ALLOWED;
}
