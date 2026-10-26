/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <string.h>

#include "mojibake-internal.h"
#include "utf.h"

// Peek at the next codepoint's WBP (for look-ahead rules WB6, WB7b, WB12).
// When skip_wb4 is true, Extend/Format/ZWJ characters are skipped (WB4 transparency).
static inline mjb_wbp mjb_peek_next_word(const char *buffer, size_t byte_length, size_t peek_index,
    mjb_encoding encoding, bool skip_wb4) {
    mjb_codepoint peek_cp = 0;

    // A malformed subsequence decodes to U+FFFD, as it does in the iterator.
    while(mjb_decode_next_raw(buffer, byte_length, encoding, &peek_index, &peek_cp, NULL) !=
        MJB_STATUS_END_OF_INPUT) {
        uint8_t cpb[MJB_PR_BUFFER_SIZE] = { 0 };

        if(mjb_codepoint_properties_lookup(peek_cp, cpb) != MJB_STATUS_OK) {
            return MJB_WBP_NOT_SET;
        }

        mjb_wbp wbp = (mjb_wbp)mjb_codepoint_properties_get(cpb, MJB_PR_WORD_BREAK);

        if(wbp == MJB_WBP_NOT_SET) {
            wbp = MJB_WBP_OTHER;
        }

        // Skip WB4-transparent characters (Extend, Format, ZWJ).
        if(skip_wb4 && (wbp == MJB_WBP_EXTEND || wbp == MJB_WBP_FORMAT || wbp == MJB_WBP_ZWJ)) {
            continue;
        }

        return wbp;
    }

    return MJB_WBP_NOT_SET;
}

// Word cluster breaking
// See: https://unicode.org/reports/tr29/
static mjb_break_type mjb_word_break_step(const mjb_text *text, mjb_word_break_state *state) {
    if(text == NULL || state == NULL) {
        return MJB_BT_NOT_SET;
    }

    const char *buffer = text->buffer;
    size_t byte_length = text->byte_length;
    mjb_encoding encoding = text->encoding;

    if(buffer == NULL || byte_length == 0 || byte_length == MJB_NUL_TERMINATED) {
        return MJB_BT_NOT_SET;
    }

    if(state->index == 0) {
        // Initialize the state.
        state->previous = MJB_WBP_NOT_SET;
        state->current = MJB_WBP_NOT_SET;
        state->previous_codepoint = MJB_CODEPOINT_NOT_VALID;
        state->current_codepoint = MJB_CODEPOINT_NOT_VALID;
        state->had_error = false;
        state->ri_count = 0;
        state->prev_prev_wbp = MJB_WBP_NOT_SET;
        state->wb4_merged = false;
        state->zwj_pending = false;
        state->prev_was_zwj = false;
    }

    bool first_codepoint = state->index == 0;

    // Resolve the byte order of generic UTF-16 and UTF-32 input and skip a leading byte order
    // mark. Without one, the byte order of generic input is unknown.
    if(!mjb_encoding_is_valid_input(encoding) ||
        !mjb_decode_begin(buffer, byte_length, &encoding, &state->index)) {
        return MJB_BT_NOT_SET;
    }

    if(state->index == byte_length) {
        // Reached end of string.
        state->position = byte_length;
        ++state->index;

        // WB2 Any ÷ eot
        return MJB_BT_ALLOWED;
    } else if(state->index > byte_length) {
        // Last step
        return MJB_BT_NOT_SET;
    }

    mjb_codepoint codepoint = 0;
    uint8_t cpb[MJB_PR_BUFFER_SIZE] = { 0 };

    for(; state->index < byte_length;) {
        // The boundary in front of the codepoint, reported when the step returns on it.
        state->position = state->index;

        // A malformed subsequence decodes to one U+FFFD for each maximal subpart.
        if(mjb_decode_next_raw(buffer, byte_length, encoding, &state->index, &codepoint, NULL) !=
            MJB_STATUS_OK) {
            state->had_error = true;
        }

        // Break at the start and end of text, unless the text is empty.
        // WB1 sot ÷ Any
        // Not needed

        memset(cpb, 0, MJB_PR_BUFFER_SIZE);

        if(mjb_codepoint_properties_lookup(codepoint, cpb) != MJB_STATUS_OK) {
            continue;
        }

        mjb_wbp wbp = (mjb_wbp)mjb_codepoint_properties_get(cpb, MJB_PR_WORD_BREAK);

        if(wbp == MJB_WBP_NOT_SET) {
            // # @missing: 0000..10FFFF; Other
            wbp = MJB_WBP_OTHER;
        }

        if(first_codepoint) {
            // First codepoint
            state->current = wbp;
            state->current_codepoint = codepoint;
            state->zwj_pending = (wbp == MJB_WBP_ZWJ);
            first_codepoint = false;

            continue;
        }

        // Track whether the character CURRENTLY becoming "previous" (old state->current)
        // was literally ZWJ before any WB4 remapping, for WB3c.
        state->prev_was_zwj = state->zwj_pending;
        state->zwj_pending = (wbp == MJB_WBP_ZWJ);

        // Save wb4_merged before clearing: used to guard WB3d so it does not fire
        // when the "previous WSegSpace" is actually a WB4-remapped Extend/Format/ZWJ.
        bool prev_was_remapped = state->wb4_merged;

        // Update prev_prev, but skip the update when WB4 just merged a character
        // (same technique as break-line.c cm_merged, ensures 3-char lookback rules
        // like WB7 and WB11 see the correct context before the WB4 base).
        if(!state->wb4_merged) {
            state->prev_prev_wbp = state->previous;
        }

        state->wb4_merged = false;

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
        if(state->previous == MJB_WBP_NEWLINE || state->previous == MJB_WBP_CR ||
            state->previous == MJB_WBP_LF) {
            return MJB_BT_ALLOWED;
        }

        // ÷ (Newline | CR | LF)
        if(state->current == MJB_WBP_NEWLINE || state->current == MJB_WBP_CR ||
            state->current == MJB_WBP_LF) {
            return MJB_BT_ALLOWED;
        }

        // Do not break within emoji zwj sequences.
        // WB3c ZWJ × \p{Extended_Pictographic}
        if(state->prev_was_zwj && mjb_codepoint_properties_get(cpb, MJB_PR_EXTENDED_PICTOGRAPHIC)) {
            return MJB_BT_NO_BREAK;
        }

        // Keep horizontal whitespace together.
        // WB3d WSegSpace × WSegSpace
        if(!prev_was_remapped && state->previous == MJB_WBP_W_SEG_SPACE &&
            state->current == MJB_WBP_W_SEG_SPACE) {
            return MJB_BT_NO_BREAK;
        }

        // Ignore Format and Extend characters, except after sot, CR, LF, and Newline. (See Section
        // 6.2, Replacing Ignore Rules.) This also has the effect of: Any × (Format | Extend | ZWJ)
        // WB4 X (Extend | Format | ZWJ)* -> X
        if((state->current == MJB_WBP_EXTEND || state->current == MJB_WBP_FORMAT ||
               state->current == MJB_WBP_ZWJ) &&
            state->previous != MJB_WBP_NOT_SET // SOT exception
        ) {
            // Re-map to the base class so subsequent calls see X as previous, not
            // Extend/Format/ZWJ.
            state->current = state->previous;
            state->current_codepoint = state->previous_codepoint;
            state->wb4_merged = true;

            return MJB_BT_NO_BREAK;
        }

        // Do not break between most letters
        // WB5 AHLetter × AHLetter
        if((state->previous == MJB_WBP_A_LETTER || state->previous == MJB_WBP_HEBREW_LETTER) &&
            (state->current == MJB_WBP_A_LETTER || state->current == MJB_WBP_HEBREW_LETTER)) {
            return MJB_BT_NO_BREAK;
        }

        // Do not break letters across certain punctuation, such as within "e.g." or "example.com".
        // WB6 AHLetter × (MidLetter | MidNumLetQ) AHLetter
        // (look-ahead: next effective char after skipping WB4-transparent chars must be AHLetter)
        if((state->previous == MJB_WBP_A_LETTER || state->previous == MJB_WBP_HEBREW_LETTER) &&
            (state->current == MJB_WBP_MID_LETTER || state->current == MJB_WBP_MID_NUM_LET ||
                state->current == MJB_WBP_SINGLE_QUOTE)) {
            mjb_wbp next_wbp = mjb_peek_next_word(buffer, byte_length, state->index, encoding,
                true);

            if(next_wbp == MJB_WBP_A_LETTER || next_wbp == MJB_WBP_HEBREW_LETTER) {
                return MJB_BT_NO_BREAK;
            }
        }

        // WB7 AHLetter (MidLetter | MidNumLetQ) × AHLetter
        if((state->prev_prev_wbp == MJB_WBP_A_LETTER ||
               state->prev_prev_wbp == MJB_WBP_HEBREW_LETTER) &&
            (state->previous == MJB_WBP_MID_LETTER || state->previous == MJB_WBP_MID_NUM_LET ||
                state->previous == MJB_WBP_SINGLE_QUOTE) &&
            (state->current == MJB_WBP_A_LETTER || state->current == MJB_WBP_HEBREW_LETTER)) {
            return MJB_BT_NO_BREAK;
        }

        // WB7a Hebrew_Letter × Single_Quote
        if(state->previous == MJB_WBP_HEBREW_LETTER && state->current == MJB_WBP_SINGLE_QUOTE) {
            return MJB_BT_NO_BREAK;
        }

        // WB7b Hebrew_Letter × Double_Quote Hebrew_Letter
        // (look-ahead: next effective char must be Hebrew_Letter)
        if(state->previous == MJB_WBP_HEBREW_LETTER && state->current == MJB_WBP_DOUBLE_QUOTE) {
            mjb_wbp next_wbp = mjb_peek_next_word(buffer, byte_length, state->index, encoding,
                true);

            if(next_wbp == MJB_WBP_HEBREW_LETTER) {
                return MJB_BT_NO_BREAK;
            }
        }

        // WB7c Hebrew_Letter Double_Quote × Hebrew_Letter
        if(state->prev_prev_wbp == MJB_WBP_HEBREW_LETTER &&
            state->previous == MJB_WBP_DOUBLE_QUOTE && state->current == MJB_WBP_HEBREW_LETTER) {
            return MJB_BT_NO_BREAK;
        }

        // Do not break within sequences of digits, or digits adjacent to letters ("3a", or "A3").
        // WB8 Numeric × Numeric
        if(state->previous == MJB_WBP_NUMERIC && state->current == MJB_WBP_NUMERIC) {
            return MJB_BT_NO_BREAK;
        }

        // WB9 AHLetter × Numeric
        if((state->previous == MJB_WBP_A_LETTER || state->previous == MJB_WBP_HEBREW_LETTER) &&
            state->current == MJB_WBP_NUMERIC) {
            return MJB_BT_NO_BREAK;
        }

        // WB10 Numeric × AHLetter
        if(state->previous == MJB_WBP_NUMERIC &&
            (state->current == MJB_WBP_A_LETTER || state->current == MJB_WBP_HEBREW_LETTER)) {
            return MJB_BT_NO_BREAK;
        }

        // Do not break within sequences, such as "3.2" or "3,456.789".
        // WB11 Numeric (MidNum | MidNumLetQ) × Numeric
        if(state->prev_prev_wbp == MJB_WBP_NUMERIC &&
            (state->previous == MJB_WBP_MID_NUM || state->previous == MJB_WBP_MID_NUM_LET ||
                state->previous == MJB_WBP_SINGLE_QUOTE) &&
            state->current == MJB_WBP_NUMERIC) {
            return MJB_BT_NO_BREAK;
        }

        // WB12 Numeric × (MidNum | MidNumLetQ) Numeric
        // (look-ahead: next effective char must be Numeric)
        if(state->previous == MJB_WBP_NUMERIC &&
            (state->current == MJB_WBP_MID_NUM || state->current == MJB_WBP_MID_NUM_LET ||
                state->current == MJB_WBP_SINGLE_QUOTE)) {
            mjb_wbp next_wbp = mjb_peek_next_word(buffer, byte_length, state->index, encoding,
                true);

            if(next_wbp == MJB_WBP_NUMERIC) {
                return MJB_BT_NO_BREAK;
            }
        }

        // Do not break between Katakana.
        // WB13 Katakana × Katakana
        if(state->previous == MJB_WBP_KATAKANA && state->current == MJB_WBP_KATAKANA) {
            return MJB_BT_NO_BREAK;
        }

        // Do not break from extenders.
        // WB13a (AHLetter | Numeric | Katakana | ExtendNumLet) × ExtendNumLet
        if(((state->previous == MJB_WBP_A_LETTER || state->previous == MJB_WBP_HEBREW_LETTER) ||
               state->previous == MJB_WBP_NUMERIC || state->previous == MJB_WBP_KATAKANA ||
               state->previous == MJB_WBP_EXTEND_NUM_LET) &&
            state->current == MJB_WBP_EXTEND_NUM_LET) {
            return MJB_BT_NO_BREAK;
        }

        // WB13b ExtendNumLet × (AHLetter | Numeric | Katakana)
        if(state->previous == MJB_WBP_EXTEND_NUM_LET &&
            ((state->current == MJB_WBP_A_LETTER || state->current == MJB_WBP_HEBREW_LETTER) ||
                state->current == MJB_WBP_NUMERIC || state->current == MJB_WBP_KATAKANA)) {
            return MJB_BT_NO_BREAK;
        }

        // Do not break within emoji flag sequences. That is, do not break between regional
        // indicator (RI) symbols if there is an odd number of RI characters before the break point.
        // WB15 sot (RI RI)* RI × RI
        // WB16 [^RI] (RI RI)* RI × RI
        if(state->previous == MJB_WBP_REGIONAL_INDICATOR &&
            state->current == MJB_WBP_REGIONAL_INDICATOR) {
            mjb_break_type result = (state->ri_count++ % 2) == 0 ? MJB_BT_NO_BREAK : MJB_BT_ALLOWED;

            return result;
        } else {
            state->ri_count = 0;
        }

        // Otherwise, break everywhere (including around ideographs).
        // WB999 Any ÷ Any
        return MJB_BT_ALLOWED;
    }

    state->position = byte_length;
    ++state->index;

    return MJB_BT_ALLOWED;
}

MJB_EXPORT mjb_break_type mjb_word_break_next(const mjb_text *text, mjb_word_break_state *state) {
    if(state == NULL) {
        return MJB_BT_NOT_SET;
    }

    // A new iteration: initialize what callers read even if the iterator returns early, before
    // it sets up the rest of the state.
    if(state->index == 0) {
        state->position = 0;
        state->had_error = false;
    }

    return mjb_word_break_step(text, state);
}

// Return the number of bytes that form the first max_segments word-break segments.
MJB_EXPORT size_t mjb_word_prefix_length(const mjb_text *text, size_t max_segments) {
    if(text == NULL) {
        return 0;
    }

    const char *buffer = text->buffer;
    size_t byte_length = text->byte_length;
    mjb_encoding encoding = text->encoding;

    if(buffer == NULL || byte_length == 0 || max_segments == 0) {
        return 0;
    }

    if(mjb_resolve_input_byte_length(buffer, &byte_length, encoding) != MJB_STATUS_OK ||
        byte_length == 0) {
        return 0;
    }

    mjb_text resolved = mjb_text_make(buffer, byte_length, encoding);
    mjb_word_break_state state;
    state.index = 0;

    mjb_break_type bt;
    size_t segment_count = 0;

    while((bt = mjb_word_break_next(&resolved, &state)) != MJB_BT_NOT_SET) {
        if(bt == MJB_BT_NO_BREAK) {
            continue;
        }

        if(++segment_count >= max_segments) {
            return state.position;
        }
    }

    return state.position;
}

// A segment is word-like when it contains at least one alphabetic or numeric codepoint.
static bool mjb_segment_is_word_like(const char *buffer, size_t byte_length,
    mjb_encoding encoding) {
    size_t index = 0;
    mjb_codepoint codepoint = 0;

    // A malformed subsequence decodes to U+FFFD, which is neither alphabetic nor numeric.
    while(mjb_decode_next_raw(buffer, byte_length, encoding, &index, &codepoint, NULL) !=
        MJB_STATUS_END_OF_INPUT) {
        uint8_t cpb[MJB_PR_BUFFER_SIZE] = { 0 };

        if(mjb_codepoint_properties_lookup(codepoint, cpb) != MJB_STATUS_OK) {
            continue;
        }

        if(mjb_codepoint_properties_get(cpb, MJB_PR_ALPHABETIC) ||
            (mjb_wbp)mjb_codepoint_properties_get(cpb, MJB_PR_WORD_BREAK) == MJB_WBP_NUMERIC) {
            return true;
        }
    }

    return false;
}

static mjb_status mjb_word_count_process(const char *buffer, size_t byte_length,
    mjb_encoding encoding, size_t *count) {
    if(byte_length == 0) {
        *count = 0;

        return MJB_STATUS_OK;
    }

    mjb_text resolved = mjb_text_make(buffer, byte_length, encoding);
    mjb_word_break_state state;
    state.index = 0;

    mjb_break_type bt;
    size_t word_count = 0;
    size_t last_break = 0;
    // Segments after the first do not start with the byte order mark, so decode them with the
    // resolved byte order.
    size_t data_start = 0;
    mjb_encoding segment_encoding = encoding;

    if(!mjb_decode_begin(buffer, byte_length, &segment_encoding, &data_start)) {
        return MJB_STATUS_INVALID_ENCODING;
    }

    while((bt = mjb_word_break_next(&resolved, &state)) != MJB_BT_NOT_SET) {
        if(bt == MJB_BT_NO_BREAK) {
            continue;
        }

        size_t break_pos = state.position;

        if(break_pos > last_break &&
            mjb_segment_is_word_like(buffer + last_break, break_pos - last_break,
                segment_encoding)) {
            ++word_count;
        }

        last_break = break_pos;
    }

    if(state.had_error) {
        return MJB_STATUS_MALFORMED_INPUT;
    }

    *count = word_count;

    return MJB_STATUS_OK;
}

// Count the word-break segments that contain at least one alphabetic or numeric character.
// Scripts that other implementations segment by dictionary lookup, such as Chinese, Japanese,
// Thai, Lao, Khmer, and Burmese, count roughly one word per character: Mojibake does not use
// frequency dictionaries to keep the size of the library small.
MJB_EXPORT mjb_status mjb_word_count(const mjb_text *text, mjb_malformed_policy malformed_policy,
    size_t *count, mjb_diagnostic *diagnostic) {
    if(count == NULL) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    *count = 0;
    mjb_diagnostic_reset(diagnostic);

    if(text == NULL || !mjb_malformed_policy_is_valid(malformed_policy)) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    const char *buffer = text->buffer;
    size_t byte_length = text->byte_length;
    mjb_encoding encoding = text->encoding;

    if(!mjb_encoding_is_valid_input(encoding)) {
        return MJB_STATUS_INVALID_ENCODING;
    }

    if(byte_length == 0) {
        return MJB_STATUS_OK;
    }

    if(buffer == NULL) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    mjb_status status = mjb_resolve_input_byte_length(buffer, &byte_length, encoding);

    if(status != MJB_STATUS_OK || byte_length == 0) {
        return status;
    }

    status = mjb_check_input_encoding_byte_order(buffer, byte_length, encoding);

    if(status != MJB_STATUS_OK) {
        return status;
    }

    status = mjb_word_count_process(buffer, byte_length, encoding, count);
    mjb_result sanitized = { NULL, 0, 0, false };

    if(status == MJB_STATUS_MALFORMED_INPUT) {
        status = mjb_repair_text_input(&buffer, &byte_length, &encoding, malformed_policy,
            diagnostic, &sanitized);

        if(status == MJB_STATUS_OK) {
            status = mjb_word_count_process(buffer, byte_length, encoding, count);
        }
    }

    mjb_result_free(&sanitized);

    return status;
}

// Return the number of bytes whose word-break segments fit within max_columns terminal cells.
MJB_EXPORT size_t mjb_word_prefix_length_for_width(const mjb_text *text,
    mjb_terminal_width_profile profile, size_t max_columns) {
    if(text == NULL) {
        return 0;
    }

    const char *buffer = text->buffer;
    size_t byte_length = text->byte_length;
    mjb_encoding encoding = text->encoding;

    if(buffer == NULL || byte_length == 0 || max_columns == 0) {
        return 0;
    }

    if(mjb_resolve_input_byte_length(buffer, &byte_length, encoding) != MJB_STATUS_OK ||
        byte_length == 0) {
        return 0;
    }

    mjb_text resolved = mjb_text_make(buffer, byte_length, encoding);
    mjb_word_break_state state;
    state.index = 0;

    mjb_break_type bt;
    size_t total_width = 0;
    size_t prev_break = 0;

    while((bt = mjb_word_break_next(&resolved, &state)) != MJB_BT_NOT_SET) {
        if(bt == MJB_BT_NO_BREAK) {
            continue;
        }

        size_t break_pos = state.position;
        size_t segment_width = 0;

        mjb_text segment = mjb_text_make(buffer + prev_break, break_pos - prev_break, encoding);

        if(mjb_terminal_width(&segment, MJB_MALFORMED_STOP, profile, &segment_width, NULL) !=
            MJB_STATUS_OK) {
            return prev_break;
        }

        if(total_width > max_columns || segment_width > max_columns - total_width) {
            return prev_break;
        }

        total_width += segment_width;
        prev_break = break_pos;
    }

    return prev_break;
}
