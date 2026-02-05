/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdlib.h>
#include <stdio.h>

#include "mojibake-internal.h"
#include "breaking.h"
#include "east-asian-width.h"
#include "utf.h"

extern mojibake mjb_global;

typedef struct mjb_lbc_result {
    mjb_line_breaking_class line_breaking_class;
    mjb_line_breaking_class original_class; // Before CM resolution
    mjb_category category;
    mjb_east_asian_width east_asian_width;
    bool extended_pictographic;
} mjb_lbc_result;

enum mjb_line_break_type {
    MJB_LBT_NOT_SET = ' ', // Not set
    MJB_LBT_MANDATORY = '!', // !
    MJB_LBT_NO_BREAK = 'x', // ×
    MJB_LBT_ALLOWED = '+' // ÷
};

// Return the codepoint character
MJB_EXPORT bool mjb_codepoint_line_breaking(mjb_codepoint codepoint,
    mjb_line_breaking *line_breaking) {
    if(!mjb_initialize()) {
        return false;
    }

    if(!mjb_codepoint_is_valid(codepoint)) {
        return false;
    }

    sqlite3_reset(mjb_global.stmt_line_breaking);
    sqlite3_bind_int(mjb_global.stmt_line_breaking, 1, codepoint);

    if(sqlite3_step(mjb_global.stmt_line_breaking) != SQLITE_ROW) {
        return false;
    }

    line_breaking->line_breaking_class = (mjb_line_breaking_class)sqlite3_column_int(
        mjb_global.stmt_line_breaking, 0);

    line_breaking->category = (mjb_category)sqlite3_column_int(mjb_global.stmt_line_breaking, 1);
    line_breaking->extended_pictographic = (bool)sqlite3_column_int(mjb_global.stmt_line_breaking, 2);

    return true;
}

// Skip forward over SP characters
static size_t skip_spaces(const mjb_lbc_result *results, size_t start, size_t max) {
    size_t pos = start;

    while(pos < max && results[pos].line_breaking_class == MJB_LBC_SP) {
        ++pos;
    }

    return pos;
}

// Check if a class is East Asian (F, W, or H)
static bool is_east_asian_class(mjb_east_asian_width width) {
    return width == MJB_EAW_FULL_WIDTH || width == MJB_EAW_WIDE || width == MJB_EAW_HALF_WIDTH;
}

// Line breaking algorithm
// see: https://www.unicode.org/reports/tr14
MJB_EXPORT mjb_line_break *mjb_break_line(const char *buffer, size_t size, mjb_encoding encoding,
    size_t *output_size) {
    size_t real_length = mjb_strnlen(buffer, size, encoding);

    if(real_length == 0) {
        return NULL;
    }

    mjb_lbc_result *results = (mjb_lbc_result*)mjb_alloc(real_length * sizeof(mjb_lbc_result));
    char *breaks = (char*)mjb_alloc(real_length + 1);  // real_length + 1 break positions

    uint8_t state = MJB_UTF_ACCEPT;
    bool in_error = false;
    mjb_codepoint codepoint;
    size_t i = 0;
    size_t j = 0;

    // Phase 1: Decode string and assign line breaking classes (LB1)
    for(i = 0; i < size;) {
        mjb_decode_result decode_status = mjb_next_codepoint(buffer, size, &state, &i, encoding,
            &codepoint, &in_error);

        if(decode_status == MJB_DECODE_END) {
            break;
        }

        if(decode_status == MJB_DECODE_OK || decode_status == MJB_DECODE_ERROR) {
            mjb_line_breaking line_breaking;
            mjb_east_asian_width eaw;

            if(!mjb_codepoint_line_breaking(codepoint, &line_breaking)) {
                line_breaking.line_breaking_class = MJB_LBC_XX;
                line_breaking.category = MJB_CATEGORY_CN;
                line_breaking.extended_pictographic = false;
            }

            if(!mjb_codepoint_east_asian_width(codepoint, &eaw)) {
                eaw = MJB_EAW_NEUTRAL;
            }

            results[j].line_breaking_class = line_breaking.line_breaking_class;
            results[j].original_class = line_breaking.line_breaking_class;
            results[j].category = line_breaking.category;
            results[j].east_asian_width = eaw;
            results[j].extended_pictographic = line_breaking.extended_pictographic;
            ++j;
        }
    }

    // Phase 2: Resolve combining marks (LB9, LB10)
    // LB9: Treat X (CM | ZWJ)* as if it were X
    // LB10: Treat any remaining CM or ZWJ as AL
    for(i = 0; i < real_length; ++i) {
        if(results[i].line_breaking_class == MJB_LBC_CM ||
           results[i].line_breaking_class == MJB_LBC_ZWJ) {
            if(i == 0) {
                // LB10: Treat standalone CM/ZWJ at start as AL
                results[i].line_breaking_class = MJB_LBC_AL;
            } else {
                mjb_line_breaking_class prev = results[i - 1].line_breaking_class;

                // If previous is BK, CR, LF, NL, SP, ZW, treat CM as AL (LB10)
                if(prev == MJB_LBC_BK || prev == MJB_LBC_CR || prev == MJB_LBC_LF ||
                   prev == MJB_LBC_NL || prev == MJB_LBC_SP || prev == MJB_LBC_ZW) {
                    results[i].line_breaking_class = MJB_LBC_AL;
                } else {
                    // LB9: Inherit class from previous character
                    results[i].line_breaking_class = prev;
                }
            }
        }
    }

    // Phase 3: Initialize break opportunities
    // breaks[i] = break opportunity BEFORE character i
    for(i = 0; i <= real_length; ++i) {
        breaks[i] = MJB_LBT_NOT_SET;
    }

    breaks[0] = MJB_LBT_NO_BREAK;  // LB2: Never break at start of text
    breaks[real_length] = MJB_LBT_MANDATORY;  // LB3: Always break at end of text

    // Phase 4: Apply line breaking rules
    for(i = 0; i < real_length; ++i) {
        mjb_line_breaking_class cc = results[i].line_breaking_class;
        mjb_line_breaking_class prev = (i > 0) ? results[i - 1].line_breaking_class : MJB_LBC_XX;

        // LB4: Always break after hard line breaks
        // BK !
        if(cc == MJB_LBC_BK) {
            if(i + 1 <= real_length) {
                breaks[i + 1] = MJB_LBT_MANDATORY;
            }
        }

        // LB5: Treat CR followed by LF, as well as CR, LF, and NL as hard line breaks
        // CR × LF
        if(prev == MJB_LBC_CR && cc == MJB_LBC_LF) {
            breaks[i] = MJB_LBT_NO_BREAK;
        }

        // CR !, LF !, NL !
        if(cc == MJB_LBC_CR || cc == MJB_LBC_LF || cc == MJB_LBC_NL) {
            if(i + 1 <= real_length) {
                breaks[i + 1] = MJB_LBT_MANDATORY;
            }
        }

        // LB6: Do not break before hard line breaks
        // × ( BK | CR | LF | NL )
        if(cc == MJB_LBC_BK || cc == MJB_LBC_CR || cc == MJB_LBC_LF || cc == MJB_LBC_NL) {
            breaks[i] = MJB_LBT_NO_BREAK;
        }

        // LB7: Do not break before spaces or zero width space
        // × SP, × ZW
        if(cc == MJB_LBC_SP || cc == MJB_LBC_ZW) {
            breaks[i] = MJB_LBT_NO_BREAK;
        }

        // LB8: Break before any character following a zero-width space, even if one or more spaces
        // intervene
        // ZW SP* ÷
        if(cc == MJB_LBC_ZW) {
            size_t next = skip_spaces(results, i + 1, real_length);

            if(next < real_length) {
                breaks[next] = MJB_LBT_ALLOWED;
            }
        }

        // LB8a: Do not break after a zero width joiner
        // ZWJ ×
        if(cc == MJB_LBC_ZWJ) {
            if(i + 1 <= real_length) {
                breaks[i + 1] = MJB_LBT_NO_BREAK;
            }
        }

        // LB11: Do not break before or after Word joiner and related characters
        // × WJ, WJ ×
        if(cc == MJB_LBC_WJ) {
            breaks[i] = MJB_LBT_NO_BREAK;
            if(i + 1 <= real_length) {
                breaks[i + 1] = MJB_LBT_NO_BREAK;
            }
        }

        // LB12: Do not break after NBSP and related characters
        // GL ×
        if(cc == MJB_LBC_GL) {
            if(i + 1 <= real_length) {
                breaks[i + 1] = MJB_LBT_NO_BREAK;
            }
        }

        // LB12a: Do not break before NBSP and related characters, except after spaces and hyphens
        // [^SP BA HY] × GL
        if(cc == MJB_LBC_GL && prev != MJB_LBC_SP && prev != MJB_LBC_BA && prev != MJB_LBC_HY) {
            breaks[i] = MJB_LBT_NO_BREAK;
        }

        // LB13: Do not break before ']' or '!' or ';' or '/', even after spaces
        // × CL, × CP, × EX, × IS, × SY
        if(cc == MJB_LBC_CL || cc == MJB_LBC_CP || cc == MJB_LBC_EX || cc == MJB_LBC_IS ||
            cc == MJB_LBC_SY) {
            breaks[i] = MJB_LBT_NO_BREAK;
        }

        // LB14: Do not break after '[', even after spaces
        // OP SP* ×
        if(cc == MJB_LBC_OP) {
            size_t next = skip_spaces(results, i + 1, real_length);

            if(next < real_length) {
                breaks[next] = MJB_LBT_NO_BREAK;
            }
        }

        // LB15: Do not break within '"[', even with intervening spaces
        // QU SP* × OP
        if(cc == MJB_LBC_QU) {
            size_t next = skip_spaces(results, i + 1, real_length);

            if(next < real_length && results[next].line_breaking_class == MJB_LBC_OP) {
                breaks[next] = MJB_LBT_NO_BREAK;
            }
        }

        // LB15a: Do not break after an unresolved initial punctuation that lies at the start of the
        // line, after a space, after opening punctuation, or after an unresolved quotation mark,
        // even after spaces.
        // (sot | BK | CR | LF | NL | OP | QU | GL | SP | ZW) [\p{Pi}&QU] SP* ×
        if(results[i].category == MJB_CATEGORY_PI && cc == MJB_LBC_QU) {
            if(i == 0 || prev == MJB_LBC_BK || prev == MJB_LBC_CR || prev == MJB_LBC_LF ||
               prev == MJB_LBC_NL || prev == MJB_LBC_OP || prev == MJB_LBC_QU ||
               prev == MJB_LBC_GL || prev == MJB_LBC_SP || prev == MJB_LBC_ZW) {
                size_t next = skip_spaces(results, i + 1, real_length);

                if(next < real_length) {
                    breaks[next] = MJB_LBT_NO_BREAK;
                }
            }
        }

        // LB15b: Do not break before an unresolved final punctuation that lies at the end of the
        // line, before a space, before a prohibited break, or before an unresolved quotation mark,
        // even after spaces.
        // × [\p{Pf}&QU] ( SP | GL | WJ | CL | QU | CP | EX | IS | SY | BK | CR | LF | NL | ZW | eot)
        if(results[i].category == MJB_CATEGORY_PF && cc == MJB_LBC_QU) {
            if(i + 1 >= real_length) {
                breaks[i] = MJB_LBT_NO_BREAK;
            } else {
                mjb_line_breaking_class next_class = results[i + 1].line_breaking_class;

                if(next_class == MJB_LBC_SP || next_class == MJB_LBC_GL || next_class == MJB_LBC_WJ ||
                   next_class == MJB_LBC_CL || next_class == MJB_LBC_QU || next_class == MJB_LBC_CP ||
                   next_class == MJB_LBC_EX || next_class == MJB_LBC_IS || next_class == MJB_LBC_SY ||
                   next_class == MJB_LBC_BK || next_class == MJB_LBC_CR || next_class == MJB_LBC_LF ||
                   next_class == MJB_LBC_NL || next_class == MJB_LBC_ZW) {
                    breaks[i] = MJB_LBT_NO_BREAK;
                }
            }
        }

        // TODO
        // LB15c: Break before a decimal mark that follows a space, for instance, in ‘subtract .5’.
        // SP ÷ IS NU

        // TODO
        // LB15d: Otherwise, do not break before ‘;’, ‘,’, or ‘.’, even after spaces.
        // x IS

        // LB16: Do not break between closing punctuation and a nonstarter (lb=NS), even with
        // intervening spaces.
        // (CL | CP) SP* × NS
        if(cc == MJB_LBC_CL || cc == MJB_LBC_CP) {
            size_t next = skip_spaces(results, i + 1, real_length);

            if(next < real_length && results[next].line_breaking_class == MJB_LBC_NS) {
                breaks[next] = MJB_LBT_NO_BREAK;
            }
        }

        // LB17: Do not break within '——', even with intervening spaces
        // B2 SP* × B2
        if(cc == MJB_LBC_B2) {
            size_t next = skip_spaces(results, i + 1, real_length);

            if(next < real_length && results[next].line_breaking_class == MJB_LBC_B2) {
                breaks[next] = MJB_LBT_NO_BREAK;
            }
        }

        // LB18: Break after spaces
        // SP ÷
        if(cc == MJB_LBC_SP) {
            if(i + 1 <= real_length) {
                breaks[i + 1] = MJB_LBT_ALLOWED;
            }
        }

        // Special case rules:

        // LB19: Do not break before non-initial unresolved quotation marks, such as ‘ ” ’ or ‘ " ’,
        // nor after non-final unresolved quotation marks, such as ‘ “ ’ or ‘ " ’.
        // × QU, QU ×
        if(cc == MJB_LBC_QU) {
            if(results[i].category != MJB_CATEGORY_PI) {
                breaks[i] = MJB_LBT_NO_BREAK;
            }

            if(results[i].category != MJB_CATEGORY_PF && i + 1 <= real_length) {
                breaks[i + 1] = MJB_LBT_NO_BREAK;
            }
        }

        // TODO
        // LB19a: Unless surrounded by East Asian characters, do not break either side of any
        // unresolved quotation marks.
        // [^$EastAsian] × QU
        // × QU ( [^$EastAsian] | eot )
        // QU × [^$EastAsian]
        // ( sot | [^$EastAsian] ) QU ×

        // LB20: Break before and after unresolved CB
        // ÷ CB, CB ÷
        if(cc == MJB_LBC_CB) {
            breaks[i] = MJB_LBT_ALLOWED;
            if(i + 1 <= real_length) {
                breaks[i + 1] = MJB_LBT_ALLOWED;
            }
        }

        // TODO
        // LB20a: Do not break after a word-initial hyphen.
        // ( sot | BK | CR | LF | NL | SP | ZW | CB | GL ) ( HY | HH ) × ( AL | HL )

        // LB21: Do not break before hyphen-minus, other hyphens, fixed-width spaces, small kana,
        // and other non-starters, or after acute accents.
        // × BA, × HH, × HY, × NS, BB ×
        if(cc == MJB_LBC_BA || cc == MJB_LBC_HH || cc == MJB_LBC_HY || cc == MJB_LBC_NS) {
            breaks[i] = MJB_LBT_NO_BREAK;
        }

        // BB ×
        if(cc == MJB_LBC_BB) {
            if(i + 1 <= real_length) {
                breaks[i + 1] = MJB_LBT_NO_BREAK;
            }
        }

        // LB21a: Do not break after the hyphen in Hebrew + Hyphen + non-Hebrew.
        // HL (HY | HH) × [^HL]
        if(prev == MJB_LBC_HL && (cc == MJB_LBC_HY || cc == MJB_LBC_HH)) {
            if(i + 1 < real_length && results[i + 1].line_breaking_class != MJB_LBC_HL) {
                breaks[i + 1] = MJB_LBT_NO_BREAK;
            }
        }

        // LB21b: Do not break between Solidus and Hebrew letters
        // SY × HL
        if(prev == MJB_LBC_SY && cc == MJB_LBC_HL) {
            breaks[i] = MJB_LBT_NO_BREAK;
        }

        // LB22: Do not break before ellipses
        // × IN
        if(cc == MJB_LBC_IN) {
            breaks[i] = MJB_LBT_NO_BREAK;
        }

        // Numbers:

        // LB23: Do not break between digits and letters
        // (AL | HL) × NU
        if((prev == MJB_LBC_AL || prev == MJB_LBC_HL) && cc == MJB_LBC_NU) {
            breaks[i] = MJB_LBT_NO_BREAK;
        }

        // NU × (AL | HL)
        if(prev == MJB_LBC_NU && (cc == MJB_LBC_AL || cc == MJB_LBC_HL)) {
            breaks[i] = MJB_LBT_NO_BREAK;
        }

        // LB23a: Do not break between numeric prefixes and ideographs, or between ideographs and
        // numeric postfixes.
        // PR × (ID | EB | EM)
        if(prev == MJB_LBC_PR && (cc == MJB_LBC_ID || cc == MJB_LBC_EB || cc == MJB_LBC_EM)) {
            breaks[i] = MJB_LBT_NO_BREAK;
        }

        // (ID | EB | EM) × PO
        if((prev == MJB_LBC_ID || prev == MJB_LBC_EB || prev == MJB_LBC_EM) && cc == MJB_LBC_PO) {
            breaks[i] = MJB_LBT_NO_BREAK;
        }

        // LB24: Do not break between numeric prefix/postfix and letters, or between letters and
        // prefix/postfix.
        // (PR | PO) × (AL | HL)
        if((prev == MJB_LBC_PR || prev == MJB_LBC_PO) && (cc == MJB_LBC_AL || cc == MJB_LBC_HL)) {
            breaks[i] = MJB_LBT_NO_BREAK;
        }

        // (AL | HL) × (PR | PO)
        if((prev == MJB_LBC_AL || prev == MJB_LBC_HL) && (cc == MJB_LBC_PR || cc == MJB_LBC_PO)) {
            breaks[i] = MJB_LBT_NO_BREAK;
        }

        // LB25: Do not break numbers
        // (PR | PO) × ( OP | HY )? NU
        if((prev == MJB_LBC_PR || prev == MJB_LBC_PO) && cc == MJB_LBC_NU) {
            breaks[i] = MJB_LBT_NO_BREAK;
        }

        if((prev == MJB_LBC_PR || prev == MJB_LBC_PO) && (cc == MJB_LBC_OP || cc == MJB_LBC_HY) &&
            i + 1 < real_length && results[i + 1].line_breaking_class == MJB_LBC_NU) {
            breaks[i] = MJB_LBT_NO_BREAK;
            breaks[i + 1] = MJB_LBT_NO_BREAK;
        }

        // HY × NU, IS × NU, NU × NU, SY × NU
        if((prev == MJB_LBC_HY || prev == MJB_LBC_IS || prev == MJB_LBC_NU || prev == MJB_LBC_SY) &&
           cc == MJB_LBC_NU) {
            breaks[i] = MJB_LBT_NO_BREAK;
        }

        // LB26: Do not break a Korean syllable
        // JL × (JL | JV | H2 | H3)
        if(prev == MJB_LBC_JL && (cc == MJB_LBC_JL || cc == MJB_LBC_JV || cc == MJB_LBC_H2 ||
            cc == MJB_LBC_H3)) {
            breaks[i] = MJB_LBT_NO_BREAK;
        }

        // (JV | H2) × (JV | JT)
        if((prev == MJB_LBC_JV || prev == MJB_LBC_H2) && (cc == MJB_LBC_JV || cc == MJB_LBC_JT)) {
            breaks[i] = MJB_LBT_NO_BREAK;
        }

        // (JT | H3) × JT
        if((prev == MJB_LBC_JT || prev == MJB_LBC_H3) && cc == MJB_LBC_JT) {
            breaks[i] = MJB_LBT_NO_BREAK;
        }

        // LB27: Treat a Korean Syllable Block the same as ID
        // (JL | JV | JT | H2 | H3) × PO
        if((prev == MJB_LBC_JL || prev == MJB_LBC_JV || prev == MJB_LBC_JT || prev == MJB_LBC_H2 ||
            prev == MJB_LBC_H3) && cc == MJB_LBC_PO) {
            breaks[i] = MJB_LBT_NO_BREAK;
        }

        // (JL | JV | JT | H2 | H3) × IN
        if((prev == MJB_LBC_JL || prev == MJB_LBC_JV || prev == MJB_LBC_JT || prev == MJB_LBC_H2 ||
            prev == MJB_LBC_H3) && cc == MJB_LBC_IN) {
            breaks[i] = MJB_LBT_NO_BREAK;
        }

        // PR × (JL | JV | JT | H2 | H3)
        if(prev == MJB_LBC_PR && (cc == MJB_LBC_JL || cc == MJB_LBC_JV || cc == MJB_LBC_JT ||
            cc == MJB_LBC_H2 || cc == MJB_LBC_H3)) {
            breaks[i] = MJB_LBT_NO_BREAK;
        }

        // LB28: Do not break between alphabetics (“at”).
        // (AL | HL) × (AL | HL)
        if((prev == MJB_LBC_AL || prev == MJB_LBC_HL) && (cc == MJB_LBC_AL || cc == MJB_LBC_HL)) {
            breaks[i] = MJB_LBT_NO_BREAK;
        }

        // LB28a: Do not break inside Brahmic scripts
        // AP × (AK | AS)
        if(prev == MJB_LBC_AP && (cc == MJB_LBC_AK || cc == MJB_LBC_AS)) {
            breaks[i] = MJB_LBT_NO_BREAK;
        }

        // (AK | AS) × (VF | VI)
        if((prev == MJB_LBC_AK || prev == MJB_LBC_AS) && (cc == MJB_LBC_VF || cc == MJB_LBC_VI)) {
            breaks[i] = MJB_LBT_NO_BREAK;
        }

        // LB29: Do not break between numeric punctuation and alphabetics
        // IS × (AL | HL)
        if(prev == MJB_LBC_IS && (cc == MJB_LBC_AL || cc == MJB_LBC_HL)) {
            breaks[i] = MJB_LBT_NO_BREAK;
        }

        // LB30: Do not break between letters, numbers, or ordinary symbols and opening or closing
        // parentheses
        // (AL | HL | NU) × [OP-$EastAsian]
        if((prev == MJB_LBC_AL || prev == MJB_LBC_HL || prev == MJB_LBC_NU) &&
           cc == MJB_LBC_OP && !is_east_asian_class(results[i].east_asian_width)) {
            breaks[i] = MJB_LBT_NO_BREAK;
        }

        // [CP-$EastAsian] × (AL | HL | NU)
        if(prev == MJB_LBC_CP && !is_east_asian_class(results[i - 1].east_asian_width) &&
           (cc == MJB_LBC_AL || cc == MJB_LBC_HL || cc == MJB_LBC_NU)) {
            breaks[i] = MJB_LBT_NO_BREAK;
        }

        // LB30a: Break between two regional indicator symbols if and only if there are an even
        // number preceding
        // sot (RI RI)* RI × RI
        // [^RI] (RI RI)* RI × RI
        if(prev == MJB_LBC_RI && cc == MJB_LBC_RI) {
            // Count preceding RI
            size_t ri_count = 0;

            for(size_t j = i - 1; results[j].line_breaking_class == MJB_LBC_RI; --j) {
                ++ri_count;
                if(j == 0) {
                    break;
                }
            }

            if(ri_count % 2 == 1) {
                breaks[i] = MJB_LBT_NO_BREAK;
            }
        }

        // LB30b: Do not break between an emoji base and an emoji modifier
        // EB × EM
        // [\p{Extended_Pictographic}&\p{Cn}] × EM
        if(prev == MJB_LBC_EB && cc == MJB_LBC_EM) {
            breaks[i] = MJB_LBT_NO_BREAK;
        }

        // TODO: missing LB30b second rule
    }

    // Phase 5: Apply default rule (LB31) - Break everywhere else
    for(i = 1; i < real_length; ++i) {
        if(breaks[i] == MJB_LBT_NOT_SET) {
            breaks[i] = MJB_LBT_ALLOWED;
        }
    }

    // Phase 6: Collect break opportunities
    // breaks[i] represents a break before character i (or after character i-1)
    // We report breaks from position 1 onwards (after char 0, after char 1, ...)
    // The index in the output represents: "break after character at this index"

    size_t num_breaks = 0;

    for(size_t i = 1; i <= real_length; ++i) {
        if(breaks[i] == MJB_LBT_ALLOWED || breaks[i] == MJB_LBT_MANDATORY) {
            ++num_breaks;
        }
    }

    mjb_line_break *line_breaks = (mjb_line_break*)mjb_alloc(num_breaks * sizeof(mjb_line_break));
    j = 0;

    for(size_t i = 1; i <= real_length; ++i) {
        if(breaks[i] == MJB_LBT_ALLOWED || breaks[i] == MJB_LBT_MANDATORY) {
            // Report as "break after character i-1"
            line_breaks[j].index = i - 1;
            line_breaks[j].mandatory = breaks[i] == MJB_LBT_MANDATORY;

            ++j;
        }
    }

    *output_size = num_breaks;

    mjb_free(results);
    mjb_free(breaks);

    return line_breaks;
}
