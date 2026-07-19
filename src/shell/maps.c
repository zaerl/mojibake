/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "shell.h"

// # @missing: 0000..10FFFF; General_Category; Unassigned
static const char *mjbsh_category_names[] = {
    "Other, not assigned",        // MJB_CATEGORY_CN
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
    "Other, private use"          // MJB_CATEGORY_CO
};

static const char *mjbsh_ccc_names[] = {
    "Not Reordered",
    "Overlay",
    NULL,
    NULL,
    NULL,
    NULL,
    "Han Reading",
    "Nukta",
    "Kana Voicing",
    "Virama",
};

static const char *mjbsh_ccc_names_200_to_240[] = {
    "Attached Below Left", // MJB_CCC_ATTACHED_BELOW_LEFT
    NULL,
    "Attached Below", // MJB_CCC_ATTACHED_BELOW
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    "Attached Above", // MJB_CCC_ATTACHED_ABOVE
    NULL, "Attached Above Right", NULL, "Below Left", NULL, "Below", NULL, "Below Right", NULL,
    "Left", NULL, "Right", NULL, "Above Left", NULL, "Above", NULL, "Above Right", "Double Below",
    "Double Above"
};

static const char *mjbsh_bidi_names[] = {
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

static const char *mjbsh_decomposition_names[] = {
    "None",          // MJB_DECOMPOSITION_NONE
    "Canonical",     // MJB_DECOMPOSITION_CANONICAL
    "Circle",        // MJB_DECOMPOSITION_CIRCLE
    "Compatibility", // MJB_DECOMPOSITION_COMPAT
    "Final",         // MJB_DECOMPOSITION_FINAL
    "Font",          // MJB_DECOMPOSITION_FONT
    "Fraction",      // MJB_DECOMPOSITION_FRACTION
    "Initial",       // MJB_DECOMPOSITION_INITIAL
    "Isolated",      // MJB_DECOMPOSITION_ISOLATED
    "Medial",        // MJB_DECOMPOSITION_MEDIAL
    "Narrow",        // MJB_DECOMPOSITION_NARROW
    "No break",      // MJB_DECOMPOSITION_NOBREAK
    "Small",         // MJB_DECOMPOSITION_SMALL
    "Square",        // MJB_DECOMPOSITION_SQUARE
    "Sub",           // MJB_DECOMPOSITION_SUB
    "Super",         // MJB_DECOMPOSITION_SUPER
    "Vertical",      // MJB_DECOMPOSITION_VERTICAL
    "Wide",          // MJB_DECOMPOSITION_WIDE
};

static const char *mjbsh_east_asian_width_names[] = {
    "Not set",    // MJB_EAW_NOT_SET
    "Ambiguous",  // MJB_EAW_AMBIGUOUS
    "Full-width", // MJB_EAW_FULL_WIDTH
    "Half-width", // MJB_EAW_HALF_WIDTH
    "Neutral",    // MJB_EAW_NEUTRAL
    "Narrow",     // MJB_EAW_NARROW
    "Wide"        // MJB_EAW_WIDE
};

const char *mjbsh_decomposition_name(mjb_decomposition decomposition) {
    size_t decomposition_index = (size_t)(unsigned int)decomposition;

    if(decomposition_index > MJB_DECOMPOSITION_WIDE) {
        return "Unknown";
    }

    return mjbsh_decomposition_names[decomposition_index];
}

const char *mjbsh_category_name(mjb_category category) {
    size_t category_index = (size_t)(unsigned int)category;

    if(category_index >= MJB_CATEGORY_COUNT) {
        return "Unknown";
    }

    return mjbsh_category_names[category_index];
}

char *mjbsh_ccc_name(mjb_canonical_combining_class ccc) {
    size_t ccc_index = (size_t)(unsigned int)ccc;

    if(ccc_index > MJB_CCC_BELOW_IOTA) {
        return strdup("Unknown");
    }

    if(ccc_index < MJB_CCC_10) {
        return strdup(mjbsh_ccc_names[ccc_index] ? mjbsh_ccc_names[ccc_index] : "Unknown");
    }

    if(ccc_index <= MJB_CCC_36 || ccc_index == MJB_CCC_84 || ccc_index == MJB_CCC_91 ||
        ccc_index == MJB_CCC_103 || ccc_index == MJB_CCC_107 || ccc_index == MJB_CCC_118 ||
        ccc_index == MJB_CCC_122 || ccc_index == MJB_CCC_129 || ccc_index == MJB_CCC_130 ||
        ccc_index == MJB_CCC_132) {
        char *str = (char *)malloc(8);

        if(str) {
            snprintf(str, 8, "CCC%zu", ccc_index);
        }

        return str;
    }

    if(ccc_index >= MJB_CCC_ATTACHED_BELOW_LEFT && ccc_index <= MJB_CCC_DOUBLE_ABOVE) {
        size_t index = ccc_index - MJB_CCC_ATTACHED_BELOW_LEFT;

        return strdup(mjbsh_ccc_names_200_to_240[index] ? mjbsh_ccc_names_200_to_240[index] :
                                                          "Unknown");
    }

    if(ccc_index == MJB_CCC_BELOW_IOTA) {
        return strdup("Iota_Subscript");
    }

    return strdup(mjbsh_ccc_names[0]);
}

const char *mjbsh_bidi_name(mjb_bidi_class bidi) {
    size_t bidi_index = (size_t)(unsigned int)bidi;

    if(bidi_index >= MJB_BIDI_CLASS_COUNT) {
        return "Unknown";
    }

    return mjbsh_bidi_names[bidi_index];
}

const char *mjbsh_east_asian_width_name(mjb_east_asian_width east_asian_width) {
    size_t east_asian_width_index = (size_t)(unsigned int)east_asian_width;

    if(east_asian_width_index >= MJB_EAW_COUNT) {
        return "Unknown";
    }

    return mjbsh_east_asian_width_names[east_asian_width_index];
}
