/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdlib.h>
#include <stdio.h>

#include "mojibake-internal.h"
#include "breaking.h"
#include "utf8.h"

extern mojibake mjb_global;

typedef struct {
    mjb_line_breaking_class line_breaking_class;
    mjb_category category;
} mjb_lbc_result;

enum mjb_line_break_type {
    MJB_LBT_NOT_SET = ' ', // Not set
    MJB_LBT_MANDATORY = '!', // !
    MJB_LBT_NO_BREAK = 'x', // ×
    MJB_LBT_ALLOWED = '+' // ÷
};

// Return the codepoint character
MJB_EXPORT bool mjb_codepoint_line_breaking_class(mjb_codepoint codepoint,
    mjb_line_breaking_class *line_breaking_class, mjb_category* category) {
    if(!mjb_initialize()) {
        return false;
    }

    if(!mjb_codepoint_is_valid(codepoint)) {
        return false;
    }

    sqlite3_reset(mjb_global.stmt_line_breaking_class);
    sqlite3_bind_int(mjb_global.stmt_line_breaking_class, 1, codepoint);

    if(sqlite3_step(mjb_global.stmt_line_breaking_class) != SQLITE_ROW) {
        return false;
    }

    *line_breaking_class = (mjb_line_breaking_class)sqlite3_column_int(
        mjb_global.stmt_line_breaking_class, 0);

    if(category) {
        *category = (mjb_category)sqlite3_column_int(mjb_global.stmt_line_breaking_class, 1);
    }

    return true;
}

// Line breaking algorithm
// see: https://www.unicode.org/reports/tr14
MJB_EXPORT mjb_line_break *mjb_break_line(const char *buffer, size_t length, mjb_encoding encoding, size_t *output_size) {
    size_t real_length = mjb_strnlen(buffer, length, encoding);

    if(real_length == 0) {
        return NULL;
    }

    mjb_lbc_result *results = (mjb_lbc_result*)malloc(real_length * sizeof(mjb_lbc_result));
    char *breaks = (char*)malloc(real_length + 1);  // real_length + 1 break positions + null terminator

    // Initialize all breaks to allowed (LB31 default) except first position
    for(size_t j = 0; j <= real_length; ++j) {
        breaks[j] = MJB_LBT_NOT_SET;
    }

    breaks[0] = MJB_LBT_NO_BREAK;  // LB2: Never break at start of text
    breaks[real_length] = MJB_LBT_MANDATORY;

    uint8_t state = MJB_UTF8_ACCEPT;
    mjb_codepoint codepoint;
    const char *current = buffer;
    size_t i = 0;

    // https://www.unicode.org/reports/tr14/#LB1
    while(*current && (size_t)(current - buffer) < length) {
        state = mjb_utf8_decode_step(state, *current, &codepoint);

        if(state == MJB_UTF8_REJECT) {
            continue;
        }

        if(state == MJB_UTF8_ACCEPT) {
            // LB1 Assign a line breaking class to each code point of the input.
            mjb_line_breaking_class line_breaking_class;
            mjb_category category;

            if(!mjb_codepoint_line_breaking_class(codepoint, &line_breaking_class, &category)) {
                // If lookup fails, use unknown class
                line_breaking_class = MJB_LBC_XX;
            }

            results[i].line_breaking_class = line_breaking_class;
            results[i].category = category;
            ++i;

            // Resolve AI, CB, CJ, SA, SG, and XX into other line breaking classes depending on criteria
            // outside the scope of this algorithm.
            // TODO: not implemented
        }

        ++current;
    }

    // LB2: Never break at start of text (implicitly handled by the algorithm)

    mjb_line_breaking_class previous_c = MJB_LBC_XX;  // Start of text - no previous character

    for(i = 0; i < real_length; ++i) {
        // Mandatory breaks:
        mjb_line_breaking_class cc = results[i].line_breaking_class;

        // LB4 Always break after hard line breaks.
        // BK !
        if(cc == MJB_LBC_BK) {
            breaks[i + 1] = MJB_LBT_MANDATORY;
        }

        // LB5 Treat CR followed by LF, as well as CR, LF, and NL as hard line breaks.
        // CR × LF
        if(i > 0 && previous_c == MJB_LBC_CR && cc == MJB_LBC_LF) {
            // \r \n
            // 0  1
            //    ^
            breaks[i] = MJB_LBT_NO_BREAK;
        }

        // LB5
        // CR !
        // LF !
        // NL !
        if(
            cc == MJB_LBC_CR ||
            cc == MJB_LBC_LF ||
            cc == MJB_LBC_NL) {
            // \n E
            // 0  1
            //    !
            breaks[i + 1] = MJB_LBT_MANDATORY;
        }

        // LB6 Do not break before hard line breaks.
        // × ( BK | CR | LF | NL )
        if(i > 0 && (cc == MJB_LBC_BK || cc == MJB_LBC_CR || cc == MJB_LBC_LF || cc == MJB_LBC_NL)) {
            breaks[i] = MJB_LBT_NO_BREAK;
        }

        // LB7 Do not break before spaces or zero width space.
        // × SP
        // x ZW
        if(cc == MJB_LBC_SP || cc == MJB_LBC_ZW) {
            breaks[i] = MJB_LBT_NO_BREAK;
        }

        // LB8 Break before any character following a zero-width space, even if one or more spaces
        // intervene.
        // ZW SP* ÷
        if(cc == MJB_LBC_ZW) {
            for(size_t j = i; j < real_length; ++j) {
                if(results[j].line_breaking_class == MJB_LBC_SP) {
                    continue;
                } else {
                    breaks[j] = MJB_LBT_ALLOWED;
                    break; // Stop at the first non-space character
                }
            }
        }

        // LB8a Do not break after a zero width joiner.
        if(cc == MJB_LBC_ZWJ) {
            // ZWJ ×
            breaks[i + 1] = MJB_LBT_NO_BREAK;
        }

        // Combining marks:

        // LB9 Do not break a combining character sequence; treat it as if it has the line breaking
        // class of the base character in all of the following rules. Treat ZWJ as if it were CM.
        // Treat X (CM | ZWJ)* as if it were X.

        // LB10 Treat any remaining combining mark or ZWJ as AL.

        // LB11 Do not break before or after Word joiner and related characters.
        if(cc == MJB_LBC_WJ) {
            // × WJ
            // WJ ×
            breaks[i] = MJB_LBT_NO_BREAK;
            breaks[i + 1] = MJB_LBT_NO_BREAK;
        }

        // Non-breaking characters:

        // LB12 Do not break after NBSP and related characters.
        if(cc == MJB_LBC_GL) {
            // GL ×
            breaks[i + 1] = MJB_LBT_NO_BREAK;
        }

        // 6.2 Tailorable Line Breaking Rules
        // LB12a Do not break before NBSP and related characters, except after spaces and hyphens.
        // [^SP BA HY] × GL

        // LB13 Do not break before ‘]’ or ‘!’ or ‘/’, even after spaces.
        if(cc == MJB_LBC_CL || cc == MJB_LBC_CP || cc == MJB_LBC_EX || cc == MJB_LBC_SY) {
            // × CL
            // × CP
            // × EX
            // × SY
            breaks[i] = MJB_LBT_NO_BREAK;
        }

        // LB14 Do not break after '[', even after spaces.
        // OP SP* ×
        if(cc == MJB_LBC_OP) {
            for(size_t j = i; j < real_length; ++j) {
                if(results[j].line_breaking_class != MJB_LBC_SP) {
                    continue;
                } else {
                    breaks[j] = MJB_LBT_NO_BREAK;
                    break; // Stop at the first non-space character
                }
            }
        }

        // LB15a Do not break after an unresolved initial punctuation that lies at the start of the
        // line, after a space, after opening punctuation, or after an unresolved quotation mark,
        // even after spaces.
        // (sot | BK | CR | LF | NL | OP | QU | GL | SP | ZW) [\p{Pi}&QU] SP* ×
        if(
            (i == 0 ||
            previous_c == MJB_LBC_BK || previous_c == MJB_LBC_CR || previous_c == MJB_LBC_LF ||
            previous_c == MJB_LBC_NL || previous_c == MJB_LBC_OP || previous_c == MJB_LBC_QU ||
            previous_c == MJB_LBC_GL || previous_c == MJB_LBC_SP || previous_c == MJB_LBC_ZW) &&
            results[i].category == MJB_CATEGORY_PI && cc == MJB_LBC_QU) {
            for(size_t j = i; j < real_length; ++j) {
                if(results[j].line_breaking_class == MJB_LBC_SP) {
                    continue;
                } else {
                    breaks[j] = MJB_LBT_NO_BREAK;
                    break; // Stop at the first non-space character
                }
            }
        }

        // LB15b Do not break before an unresolved final punctuation that lies at the end of the
        // line, before a space, before a prohibited break, or before an unresolved quotation mark,
        // even after spaces.
        // × [\p{Pf}&QU] ( SP | GL | WJ | CL | QU | CP | EX | IS | SY | BK | CR | LF | NL | ZW | eot)

        // LB15c Break before a decimal mark that follows a space, for instance, in ‘subtract .5’.
        // SP ÷ IS NU

        // LB15d Otherwise, do not break before ‘;’, ‘,’, or ‘.’, even after spaces.
        // × IS

        // LB16 Do not break between closing punctuation and a nonstarter (lb=NS), even with
        // intervening spaces.
        // (CL | CP) SP* × NS
        if(cc == MJB_LBC_CL || cc == MJB_LBC_CP) {
            for(size_t j = i; j < real_length; ++j) {
                if(results[j].line_breaking_class == MJB_LBC_SP) {
                    continue;
                } else if(results[j].line_breaking_class == MJB_LBC_NS) {
                    breaks[j] = MJB_LBT_NO_BREAK;
                    break; // Stop at the first non-space character
                } else {
                    break; // Stop at the first non-NS character
                }
            }
        }

        // LB17 Do not break within ‘——’, even with intervening spaces.
        // B2 SP* × B2

        // Spaces

        // LB18 Break after spaces.
        if(cc == MJB_LBC_SP) {
            // SP ÷
            breaks[i + 1] = MJB_LBT_ALLOWED;
        }

        // Special case rules:

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
        if(cc == MJB_LBC_CB) {
            // ÷ CB
            // CB ÷
            breaks[i] = MJB_LBT_ALLOWED;
            breaks[i + 1] = MJB_LBT_ALLOWED;
        }

        // LB20a Do not break after a word-initial hyphen.
        // ( sot | BK | CR | LF | NL | SP | ZW | CB | GL ) ( HY | [\u2010] ) × AL

        // LB21 Do not break before hyphen-minus, other hyphens, fixed-width spaces, small kana, and
        // other non-starters, or after acute accents.
        if(cc == MJB_LBC_BA || cc == MJB_LBC_HY || cc == MJB_LBC_NS) {
            // × BA
            // × HY
            // × NS
            breaks[i] = MJB_LBT_NO_BREAK;
        }

        // LB21
        if(cc == MJB_LBC_BB) {
            // BB ×
            breaks[i + 1] = MJB_LBT_NO_BREAK;
        }

        // LB21a Do not break after the hyphen in Hebrew + Hyphen + non-Hebrew.
        // HL (HY | [ BA - $EastAsian ]) × [^HL]

        // LB21b Do not break between Solidus and Hebrew letters.
        if(i > 0 && previous_c == MJB_LBC_SY && cc == MJB_LBC_HL) {
            // SY × HL
            breaks[i] = MJB_LBT_NO_BREAK;
        }

        // LB22 Do not break before ellipses.
        if(cc == MJB_LBC_IN) {
            // × IN
            breaks[i] = MJB_LBT_NO_BREAK;
        }

        // Numbers:

        // LB23 Do not break between digits and letters.
        if(i > 0 && (previous_c == MJB_LBC_AL || previous_c == MJB_LBC_HL) && cc == MJB_LBC_NU) {
            // (AL | HL) × NU
            breaks[i] = MJB_LBT_NO_BREAK;
        }

        // LB23
        if(i > 0 && previous_c == MJB_LBC_NU && (cc == MJB_LBC_AL || cc == MJB_LBC_HL)) {
            // NU × (AL | HL)
            breaks[i] = MJB_LBT_NO_BREAK;
        }

        // LB23a Do not break between numeric prefixes and ideographs, or between ideographs and
        // numeric postfixes.
        if(i > 0 && previous_c == MJB_LBC_PR && (cc == MJB_LBC_ID || cc == MJB_LBC_EB ||
            cc == MJB_LBC_EM)) {
            // PR × (ID | EB | EM)
            breaks[i] = MJB_LBT_NO_BREAK;
        }

        // LB23a
        if(i > 0 && (previous_c == MJB_LBC_ID || previous_c == MJB_LBC_EB ||
            previous_c == MJB_LBC_EM) && cc == MJB_LBC_PO) {
            // (ID | EB | EM) × PO
            breaks[i] = MJB_LBT_NO_BREAK;
        }

        // LB24 Do not break between numeric prefix/postfix and letters, or between letters and
        // prefix/postfix.
        if(i > 0 && (previous_c == MJB_LBC_PR || previous_c == MJB_LBC_PO) &&
            (cc == MJB_LBC_AL || cc == MJB_LBC_HL)) {
            // (PR | PO) × (AL | HL)
            breaks[i] = MJB_LBT_NO_BREAK;
        }

        // LB24
        if(i > 0 && (previous_c == MJB_LBC_AL || previous_c == MJB_LBC_HL) &&
            (cc == MJB_LBC_PR || cc == MJB_LBC_PO)) {
            // (AL | HL) × (PR | PO)
            breaks[i] = MJB_LBT_NO_BREAK;
        }

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

        // Korean syllable blocks:

        // LB26 Do not break a Korean syllable.
        // JL × (JL | JV | H2 | H3)
        // (JV | H2) × (JV | JT)
        // (JT | H3) × JT

        // LB27 Treat a Korean Syllable Block the same as ID.
        // (JL | JV | JT | H2 | H3) × PO

        // Finally, join alphabetic letters into words and break everything else.

        // LB28 Do not break between alphabetics (“at”).
        // (AL | HL) × (AL | HL)
        if(i > 0 && (previous_c == MJB_LBC_AL || previous_c == MJB_LBC_HL) &&
            (cc == MJB_LBC_AL || cc == MJB_LBC_HL)) {
            // (AL | HL) × (AL | HL)
            breaks[i] = MJB_LBT_NO_BREAK;
        }

        // LB28a Do not break inside the orthographic syllables of Brahmic scripts.
        // AP × (AK | [◌] | AS)
        // (AK | [◌] | AS) × (VF | VI)
        // (AK | [◌] | AS) VI × (AK | [◌])
        // (AK | [◌] | AS) × (AK | [◌] | AS) VF

        // LB29 Do not break between numeric punctuation and alphabetics (“e.g.”).
        if(i > 0 && previous_c == MJB_LBC_IS && (cc == MJB_LBC_AL || cc == MJB_LBC_HL)) {
            // IS × (AL | HL)
            breaks[i] = MJB_LBT_NO_BREAK;
        }

        // LB30 Do not break between letters, numbers, or ordinary symbols and opening or closing
        // parentheses.
        // (AL | HL | NU) × [OP-$EastAsian]
        // [CP-$EastAsian] × (AL | HL | NU)

        // LB30a Break between two regional indicator symbols if and only if there are an even
        // number of regional indicators preceding the position of the break.
        // sot (RI RI)* RI × RI
        // [^RI] (RI RI)* RI × RI

        // LB30b Do not break between an emoji base (or potential emoji) and an emoji modifier.
        // EB × EM
        // [\p{Extended_Pictographic}&\p{Cn}] × EM

        // LB31 Break everywhere else.
        // ALL ÷
        // ÷ ALL
        // breaks[prev] = MJB_LBT_ALLOWED;
        // breaks[i] = MJB_LBT_ALLOWED;

        previous_c = results[i].line_breaking_class;
    }

    // LB3: Always break at the end of text - but keep whatever rule set it
    // The test expects the actual break type, not always mandatory

    size_t num_breaks = 0;

    for(size_t i = 0; i < real_length; ++i) {
        if(breaks[i] == MJB_LBT_ALLOWED || breaks[i] == MJB_LBT_MANDATORY) {
            ++num_breaks;
        }
    }

    mjb_line_break *line_breaks = (mjb_line_break*)malloc(num_breaks * sizeof(mjb_line_break));
    size_t j = 0;

    for(size_t i = 0; i < real_length; ++i) {
        if(breaks[i] == MJB_LBT_ALLOWED || breaks[i] == MJB_LBT_MANDATORY) {
            line_breaks[j].index = i;
            line_breaks[j].mandatory = breaks[i] == MJB_LBT_MANDATORY;
            ++j;
        }
    }

    *output_size = num_breaks;

    free(results);
    free(breaks);

    return line_breaks;
}
