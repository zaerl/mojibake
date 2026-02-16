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

// Line breaking algorithm
// see: https://www.unicode.org/reports/tr14
MJB_EXPORT mjb_break_type mjb_break_line(const char *buffer, size_t size, mjb_encoding encoding,
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

        if(first_codepoint) {
            // First codepoint
            state->current = lbp;
            state->current_codepoint = codepoint;
            first_codepoint = false;
            mjb_update_sequence_flags(state, cpb);

            continue;
        }

        // Swap previous and current codepoints
        state->previous = state->current;
        state->current = lbp;
        state->previous_codepoint = state->current_codepoint;
        state->current_codepoint = codepoint;

#define MJB_NBR mjb_update_sequence_flags(state, cpb); return MJB_BT_NO_BREAK;
#define MJB_AR mjb_update_sequence_flags(state, cpb); return MJB_BT_ALLOWED;

        // LB4 Always break after hard line breaks.
        // BK !
        if(state->previous == MJB_LBP_BK) {
            mjb_update_sequence_flags(state, cpb);

            return MJB_BT_MANDATORY;
        }

        // LB5 Treat CR followed by LF, as well as CR, LF, and NL as hard line breaks.

        // CR × LF
        if(state->previous == MJB_LBP_CR && state->current == MJB_LBP_LF) {
            MJB_NBR
        }

        // CR !
        // LF !
        // NL !
        if(
            state->previous == MJB_LBP_CR ||
            state->previous == MJB_LBP_LF ||
            state->previous == MJB_LBP_NL
        ) {
            mjb_update_sequence_flags(state, cpb);

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
            MJB_NBR
        }

        // LB7 Do not break before spaces or zero width space.
        // × SP
        // × ZW
        if(state->current == MJB_LBP_SP || state->current == MJB_LBP_ZW) {
            MJB_NBR
        }

        // LB8 Break before any character following a zero-width space, even if one or more spaces
        // intervene.
        // ZW SP* ÷
        // TODO

        // LB8a Do not break after a zero width joiner.
        // ZWJ ×
        if(state->previous == MJB_LBP_ZWJ) {
            MJB_NBR
        }

        // LB9 Do not break a combining character sequence; treat it as if it has the line breaking
        // class of the base character in all of the following rules. Treat ZWJ as if it were CM.
        // Treat X (CM | ZWJ)* as if it were X.
        // TODO

        // LB10 Treat any remaining combining mark or ZWJ as AL.
        // Treat any remaining CM or ZWJ as if it had the properties of U+0041 A LATIN CAPITAL
        // LETTER A, that is, Line_Break=AL, General_Category=Lu, East_Asian_Width=Na,
        // Extended_Pictographic=N.
        // TODO

        // LB11 Do not break before or after Word joiner and related characters.
        // × WJ
        // WJ ×
        if(state->previous == MJB_LBP_WJ || state->current == MJB_LBP_WJ) {
            MJB_NBR
        }

        // LB12 Do not break after NBSP and related characters.
        // GL ×
        if(state->previous == MJB_LBP_GL) {
            MJB_NBR
        }

        // 6.2 Tailorable Line Breaking Rules

        // LB12a Do not break before NBSP and related characters, except after spaces and hyphens.
        // [^SP BA HY HH] × GL

        // LB13 Do not break before ‘]’ or ‘!’ or ‘/’, even after spaces.
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
            MJB_NBR
        }

        // LB14 Do not break after ‘[’, even after spaces.
        // OP SP* ×

        // LB15a Do not break after an unresolved initial punctuation that lies at the start of the
        // line, after a space, after opening punctuation, or after an unresolved quotation mark,
        // even after spaces.
        // (sot | BK | CR | LF | NL | OP | QU | GL | SP | ZW) [\p{Pi}&QU] SP* ×

        // LB15b Do not break before an unresolved final punctuation that lies at the end of the
        // line, before a space, before a prohibited break, or before an unresolved quotation mark,
        // even after spaces.
        // × [\p{Pf}&QU] ( SP | GL | WJ | CL | QU | CP | EX | IS | SY | BK | CR | LF | NL | ZW | eot)

        // LB15c Break before a decimal mark that follows a space, for instance, in ‘subtract .5’.
        // SP ÷ IS NU

        // LB15d Otherwise, do not break before ‘;’, ‘,’, or ‘.’, even after spaces.
        // × IS
        if(state->current == MJB_LBP_IS) {
            MJB_NBR
        }

        // LB16 Do not break between closing punctuation and a nonstarter (lb=NS), even with
        // intervening spaces.
        // (CL | CP) SP* × NS

        // LB17 Do not break within ‘——’, even with intervening spaces.
        // B2 SP* × B2

        // LB18 Break after spaces.
        // SP ÷
        if(state->previous == MJB_LBP_SP) {
            MJB_AR
        }

        // LB19 Do not break before non-initial unresolved quotation marks, such as ‘ ” ’ or ‘ " ’,
        // nor after non-final unresolved quotation marks, such as ‘ “ ’ or ‘ " ’.
        // × [ QU - \p{Pi} ]
        // [ QU - \p{Pf} ] ×

        // LB19a Unless surrounded by East Asian characters, do not break either side of any
        // unresolved quotation marks.
        // [^$EastAsian] × QU
        // × QU ( [^$EastAsian] | eot )
        // QU × [^$EastAsian]
        // ( sot | [^$EastAsian] ) QU ×

        // LB20 Break before and after unresolved CB.
        // ÷ CB
        // CB ÷
        if(state->previous == MJB_LBP_CB || state->current == MJB_LBP_CB) {
            MJB_AR
        }

        // LB31 Break everywhere else.
        // ALL ÷
        // ÷ ALL
        mjb_update_sequence_flags(state, cpb);

        // LB20a Do not break after a word-initial hyphen.
        // ( sot | BK | CR | LF | NL | SP | ZW | CB | GL ) ( HY | HH ) × ( AL | HL )

        // LB21 Do not break before hyphen-minus, other hyphens, fixed-width spaces, small kana,
        // and other non-starters, or after acute accents.
        // × BA
        // × HH
        // × HY
        // × NS
        // BB ×
        if(
            state->current == MJB_LBP_BA ||
            state->current == MJB_LBP_HH ||
            state->current == MJB_LBP_HY ||
            state->current == MJB_LBP_NS ||
            state->current == MJB_LBP_BB
        ) {
            MJB_NBR
        }

        // LB21a Do not break after the hyphen in Hebrew + Hyphen + non-Hebrew.
        // HL (HY | HH) × [^HL]

        // LB21b Do not break between Solidus and Hebrew letters.
        // SY × HL
        if(state->previous == MJB_LBP_SY && state->current == MJB_LBP_HL) {
            MJB_NBR
        }

        // LB22 Do not break before ellipses.
        // × IN
        if(state->current == MJB_LBP_IN) {
            MJB_NBR
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
            MJB_NBR
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
            MJB_NBR
        }

        if(
            (
                state->current == MJB_LBP_ID ||
                state->current == MJB_LBP_EB ||
                state->current == MJB_LBP_EM
            ) &&
            state->previous == MJB_LBP_PO
        ) {
            MJB_NBR
        }

        // LB24 Do not break between numeric prefix/postfix and letters, or between letters and
        // prefix / postfix.
        // (PR | PO) × (AL | HL)
        // (AL | HL) × (PR | PO)

        // LB25 Do not break numbers:
        // NU ( SY | IS )* CL × PO
        // NU ( SY | IS )* CP × PO
        // NU ( SY | IS )* CL × PR
        // NU ( SY | IS )* CP × PR
        // NU ( SY | IS )* × PO
        // NU ( SY | IS )* × PR
        // PO × OP NU
        // PO × OP IS NU
        // PO × NU
        // PR × OP NU
        // PR × OP IS NU
        // PR × NU
        // HY × NU
        // IS × NU
        // NU ( SY | IS )* × NU

        // LB26 Do not break a Korean syllable.
        // JL × (JL | JV | H2 | H3)
        // (JV | H2) × (JV | JT)
        // (JT | H3) × JT

        // LB27 Treat a Korean Syllable Block the same as ID.
        // (JL | JV | JT | H2 | H3) × PO
        // PR × (JL | JV | JT | H2 | H3)

        // LB28 Do not break between alphabetics (“at”).
        // (AL | HL) × (AL | HL)
        if(
            (
                state->previous == MJB_LBP_AL || state->previous == MJB_LBP_HL
            ) &&
            (
                state->current == MJB_LBP_AL || state->current == MJB_LBP_HL
            )
        ) {
            MJB_NBR
        }

        // LB28a Do not break inside the orthographic syllables of Brahmic scripts.
        // AP × (AK | [◌] | AS)
        // (AK | [◌] | AS) × (VF | VI)
        // (AK | [◌] | AS) VI × (AK | [◌])
        // (AK | [◌] | AS) × (AK | [◌] | AS) VF

        // LB29 Do not break between numeric punctuation and alphabetics (“e.g.”).
        // IS × (AL | HL)
        if(
            state->previous == MJB_LBP_IS &&
            (
                state->current == MJB_LBP_AL || state->current == MJB_LBP_HL
            )
        ) {
            MJB_NBR
        }

        // LB30 Do not break between letters, numbers, or ordinary symbols and opening or closing
        // parentheses.
        // (AL | HL | NU) × [OP-$EastAsian]
        // [CP-$EastAsian] × (AL | HL | NU)

        // LB30a Break between two regional indicator symbols if and only if there are an even
        // number of regional indicators preceding the position of the break.
        // sot (RI RI)* RI × RI
        // [^RI] (RI RI)* RI × RI
        if(state->previous == MJB_GBP_REGIONAL_INDICATOR && state->current == MJB_GBP_REGIONAL_INDICATOR) {
            mjb_break_type result = (state->ri_count++ % 2) == 0 ? MJB_BT_NO_BREAK : MJB_BT_ALLOWED;
            mjb_update_sequence_flags(state, cpb);

            return result;
        } else {
            state->ri_count = 0;
        }

        // LB30b Do not break between an emoji base (or potential emoji) and an emoji modifier.
        // EB × EM
        // [\p{Extended_Pictographic}&\p{Cn}] × EM

        // LB31 Break everywhere else.
        // ALL ÷
        // ÷ ALL
        mjb_update_sequence_flags(state, cpb);

        return MJB_BT_ALLOWED;
    }

    ++state->index;

#undef MJB_NBR

    return MJB_BT_ALLOWED;
}
