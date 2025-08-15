/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "maps.h"

static const char *category_names[] = {
    "Letter, uppercase",          // MJB_CATEGORY_LU
    "Letter, lowercase",          // MJB_CATEGORY_LL
    "Letter, titlecase",          // MJB_CATEGORY_LT
    "Letter, modifier",           // MJB_CATEGORY_LM
    "Letter, other",              // MJB_CATEGORY_LO
    "Mark, non-spacing",          // MJB_CATEGORY_MN
    "Mark, spacing combining",    // MJB_CATEGORY_MC
    "Mark, enclosing",            // MJB_CATEGORY_ME
    "Number, decimal digit",      // MJB_CATEGORY_ND
    "Number, letter",             // MJB_CATEGORY_NL
    "Number, other",              // MJB_CATEGORY_NO
    "Punctuation, connector",     // MJB_CATEGORY_PC
    "Punctuation, dash",          // MJB_CATEGORY_PD
    "Punctuation, open",          // MJB_CATEGORY_PS
    "Punctuation, close",         // MJB_CATEGORY_PE
    "Punctuation, initial quote", // MJB_CATEGORY_PI
    "Punctuation, final quote",   // MJB_CATEGORY_PF
    "Punctuation, other",         // MJB_CATEGORY_PO
    "Symbol, math",               // MJB_CATEGORY_SM
    "Symbol, currency",           // MJB_CATEGORY_SC
    "Symbol, modifier",           // MJB_CATEGORY_SK
    "Symbol, other",              // MJB_CATEGORY_SO
    "Separator, space",           // MJB_CATEGORY_ZS
    "Separator, line",            // MJB_CATEGORY_ZL
    "Separator, paragraph",       // MJB_CATEGORY_ZP
    "Other, control",             // MJB_CATEGORY_CC
    "Other, format",              // MJB_CATEGORY_CF
    "Other, surrogate",           // MJB_CATEGORY_CS
    "Other, private use",         // MJB_CATEGORY_CO
    "Other, not assigned"         // MJB_CATEGORY_CN
};

static const char *ccc_names[] = {
    "Not reordered",        // MJB_CCC_NOT_REORDERED
    "Overlays",             // MJB_CCC_OVERLAYS
    NULL,
    NULL,
    NULL,
    NULL,
    "Han reading",          // MJB_CCC_VIETNAMES_ALT
    "Nukta",                // MJB_CCC_NUKTA
    "Kana voicing",         // MJB_CCC_KANA_VOICING
    "Virama",               // MJB_CCC_VIRAMA
};

static const char *ccc_names_200_to_240[] = {
    "Below left attached",  // MJB_CCC_BELOW_LEFT_ATTACHED
    NULL,
    "Below attached",       // MJB_CCC_BELOW_ATTACHED
    NULL,
    "Below right attached", // MJB_CCC_BELOW_RIGHT_ATTACHED
    NULL,
    NULL,
    NULL,
    "Left attached",        // MJB_CCC_LEFT_ATTACHED
    NULL,
    "Right attached",       // MJB_CCC_RIGHT_ATTACHED
    NULL,
    "Above left attached",  // MJB_CCC_ABOVE_LEFT_ATTACHED
    NULL,
    "Above attached",       // MJB_CCC_ABOVE_ATTACHED
    NULL,
    "Above right attached", // MJB_CCC_ABOVE_RIGHT_ATTACHED
    NULL,
    "Below left",           // MJB_CCC_BELOW_LEFT
    NULL,
    "Below",                // MJB_CCC_BELOW
    NULL,
    "Below right",          // MJB_CCC_BELOW_RIGHT
    NULL,
    "Left",                 // MJB_CCC_LEFT
    NULL,
    "Right",                // MJB_CCC_RIGHT
    NULL,
    "Above left",           // MJB_CCC_ABOVE_LEFT
    NULL,
    "Above",                // MJB_CCC_ABOVE
    NULL,
    "Above right",          // MJB_CCC_ABOVE_RIGHT
    "Double below",         // MJB_CCC_DOUBLE_BELOW
    "Double above",         // MJB_CCC_DOUBLE_ABOVE
};

static const char *bidi_names[] = {
    "None",                       // MJB_BIDI_NONE
    "Left-to-right",              // MJB_BIDI_L
    "Right-to-left",              // MJB_BIDI_R
    "Right-to-left arabic",       // MJB_BIDI_AL
    "European number",            // MJB_BIDI_EN
    "European number separator",  // MJB_BIDI_ES
    "European number terminator", // MJB_BIDI_ET
    "Arabic number",              // MJB_BIDI_AN
    "Common number separator",    // MJB_BIDI_CS
    "Nonspacing mark",            // MJB_BIDI_NSM
    "Boundary neutral",           // MJB_BIDI_BN
    "Paragraph separator",        // MJB_BIDI_B
    "Segment separator",          // MJB_BIDI_S
    "Whitespace",                 // MJB_BIDI_WS
    "Other neutrals",             // MJB_BIDI_ON
    "Left-to-right embedding",    // MJB_BIDI_LRE
    "Left-to-right override",     // MJB_BIDI_LRO
    "Right-to-left embedding",    // MJB_BIDI_RLE
    "Right-to-left override",     // MJB_BIDI_RLO
    "Pop directional format",     // MJB_BIDI_PDF
    "Left-to-right isolate",      // MJB_BIDI_LRI
    "Right-to-left isolate",      // MJB_BIDI_RLI
    "First strong isolate",       // MJB_BIDI_FSI
    "Pop directional isolate",    // MJB_BIDI_PDI
};

static const char *decomposition_names[] = {
    "None",                // MJB_DECOMPOSITION_NONE
    "Canonical",           // MJB_DECOMPOSITION_CANONICAL
    "Circle",              // MJB_DECOMPOSITION_CIRCLE
    "Compatibility",       // MJB_DECOMPOSITION_COMPAT
    "Final",               // MJB_DECOMPOSITION_FINAL
    "Font",                // MJB_DECOMPOSITION_FONT
    "Fraction",            // MJB_DECOMPOSITION_FRACTION
    "Initial",             // MJB_DECOMPOSITION_INITIAL
    "Isolated",            // MJB_DECOMPOSITION_ISOLATED
    "Medial",              // MJB_DECOMPOSITION_MEDIAL
    "Narrow",              // MJB_DECOMPOSITION_NARROW
    "No break",            // MJB_DECOMPOSITION_NOBREAK
    "Small",               // MJB_DECOMPOSITION_SMALL
    "Square",              // MJB_DECOMPOSITION_SQUARE
    "Sub",                 // MJB_DECOMPOSITION_SUB
    "Super",               // MJB_DECOMPOSITION_SUPER
    "Vertical",            // MJB_DECOMPOSITION_VERTICAL
    "Wide",                // MJB_DECOMPOSITION_WIDE
};

static const char *line_breaking_class_names[] = {
   // Non-tailorable Line Breaking Classes
   "BK", // Mandatory Break
   "CR", // Carriage Return
   "LF", // Line Feed
   "CM", // Combining Mark
   "NL", // Next Line
   "SG", // Surrogate
   "WJ", // Word Joiner
   "ZW", // Zero Width Space
   "GL", // Non-breaking
   "SP", // Space
   "ZWJ", // Zero Width Joiner

   // Break Opportunities
   "B2", // Break Opportunity Before and After
   "BA", // Break After
   "BB", // Break Before
   "HY", // Hyphen
   "CB", // Contingent Break Opportunity

   // Characters Prohibiting Certain Breaks
   "CL", // Close Punctuation
   "CP", // Close Parenthesis
   "EX", // Exclamation / Interrogation
   "IN", // Inseparable
   "NS", // Nonstarter
   "OP", // Open Punctuation
   "QU", // Quotation

   // Numeric Context
   "IS", // Infix Numeric Separator
   "NU", // Numeric
   "PO", // Postfix Numeric
   "PR", // Prefix Numeric
   "SY", // Symbols Allowing Break After

   // Other Characters
   "AI", // Ambiguous (Alphabetic or Ideographic)
   "AK", // Aksara
   "AL", // Alphabetic
   "AP", // Aksara Pre-Base
   "AS", // Aksara Start
   "CJ", // Conditional Japanese Starter
   "EB", // Emoji Base
   "EM", // Emoji Modifier
   "H2", // Hangul LV Syllable
   "H3", // Hangul LVT Syllable
   "HL", // Hebrew Letter
   "ID", // Ideographic
   "JL", // Hangul L Jamo
   "JV", // Hangul V Jamo
   "JT", // Hangul T Jamo
   "RI", // Regional Indicator
   "SA", // Complex Context Dependent (South East Asian)
   "VF", // Virama Final
   "VI", // Virama
   "XX"  // Unknown
};

const char *decomposition_name(mjb_decomposition decomposition) {
    if(decomposition > MJB_DECOMPOSITION_WIDE) {
        return "Unknown";
    }

    return decomposition_names[decomposition];
}

const char *category_name(mjb_category category) {
    if(category >= MJB_CATEGORY_COUNT) {
        return "Unknown";
    }

    return category_names[category];
}

char *ccc_name(mjb_canonical_combining_class ccc) {
    if(ccc > MJB_CCC_BELOW_IOTA) {
        return strdup("Unknown");
    }

    if(ccc < MJB_CCC_10) {
        return strdup(ccc_names[ccc] ? ccc_names[ccc] : "Unknown");
    }

    if(ccc <= MJB_CCC_36 || ccc == MJB_CCC_84 || ccc == MJB_CCC_91 || ccc == MJB_CCC_103 ||
        ccc == MJB_CCC_107 || ccc == MJB_CCC_118 || ccc == MJB_CCC_122 || ccc == MJB_CCC_129 ||
        ccc == MJB_CCC_130 || ccc == MJB_CCC_132) {
        char *str = malloc(8);

        if(str) {
            snprintf(str, 8, "CCC%d", ccc);
        }

        return str;
    }

    if(ccc >= MJB_CCC_BELOW_LEFT_ATTACHED && ccc <= MJB_CCC_DOUBLE_ABOVE) {
        return strdup(ccc_names_200_to_240[ccc - MJB_CCC_BELOW_LEFT_ATTACHED] ?
            ccc_names_200_to_240[ccc - MJB_CCC_BELOW_LEFT_ATTACHED] : "Unknown");
    }

    if(ccc == MJB_CCC_BELOW_IOTA) {
        return strdup("Iota subscript");
    }

    return strdup("Unknown");
}

const char *bidi_name(mjb_bidi_categories bidi) {
    if(bidi >= MJB_BIDI_COUNT) {
        return "Unknown";
    }

    return bidi_names[bidi];
}

const char *line_breaking_class_name(mjb_line_breaking_class line_breaking_class) {
    if(line_breaking_class >= MJB_LBC_COUNT) {
        return "XX";
    }

    return line_breaking_class_names[line_breaking_class];
}
