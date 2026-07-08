/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <string.h>

#include "mojibake-internal.h"
#include "unicode-tables.h"

/**
 * See mjb_property enum in unicode.h for the list of properties.
 */
static const char *mjb_property_names[] = {
    "kAccountingNumeric", // enumerated
    "kOtherNumeric", // enumerated
    "kPrimaryNumeric", // enumerated
    "Numeric_Value", // enumerated
    "Bidi_Mirroring_Glyph", // enumerated
    "Bidi_Paired_Bracket", // enumerated
    "Case_Folding", // enumerated
    "kCompatibilityVariant", // enumerated
    "Decomposition_Mapping", // enumerated
    "Equivalent_Unified_Ideograph", // enumerated
    "FC_NFKC_Closure", // enumerated
    "Lowercase_Mapping", // enumerated
    "NFKC_Casefold", // enumerated
    "NFKC_Simple_Casefold", // enumerated
    "Simple_Case_Folding", // enumerated
    "Simple_Lowercase_Mapping", // enumerated
    "Simple_Titlecase_Mapping", // enumerated
    "Simple_Uppercase_Mapping", // enumerated
    "Titlecase_Mapping", // enumerated
    "Uppercase_Mapping", // enumerated
    "kIICore", // enumerated
    "kIRG_GSource", // enumerated
    "kIRG_HSource", // enumerated
    "kIRG_JSource", // enumerated
    "kIRG_KPSource", // enumerated
    "kIRG_KSource", // enumerated
    "kIRG_MSource", // enumerated
    "kIRG_SSource", // enumerated
    "kIRG_TSource", // enumerated
    "kIRG_UKSource", // enumerated
    "kIRG_USource", // enumerated
    "kIRG_VSource", // enumerated
    "kMandarin", // enumerated
    "kRSUnicode", // enumerated
    "kTotalStrokes", // enumerated
    "kUnihanCore2020", // enumerated
    "ISO_Comment", // enumerated
    "Jamo_Short_Name", // enumerated
    "kEH_Cat", // enumerated
    "kEH_Desc", // enumerated
    "kEH_HG", // enumerated
    "kEH_IFAO", // enumerated
    "kEH_JSesh", // enumerated
    "Name", // enumerated
    "Name_Alias", // enumerated
    "Script_Extensions", // enumerated
    "Age", // enumerated
    "Script", // enumerated
    "Bidi_Paired_Bracket_Type", // enumerated
    "Canonical_Combining_Class", // enumerated
    "Decomposition_Type", // enumerated
    "East_Asian_Width", // enumerated
    "General_Category", // enumerated
    "Grapheme_Cluster_Break", // enumerated
    "Hangul_Syllable_Type", // enumerated
    "Indic_Conjunct_Break", // enumerated
    "Indic_Positional_Category", // enumerated
    "Indic_Syllabic_Category", // enumerated
    "Joining_Group", // enumerated
    "Joining_Type", // enumerated
    "Line_Break", // enumerated
    "NFC_Quick_Check", // enumerated
    "NFD_Quick_Check",
    "NFKC_Quick_Check", // enumerated
    "NFKD_Quick_Check",
    "Numeric_Type", // enumerated
    "Sentence_Break", // enumerated
    "Vertical_Orientation", // enumerated
    "Word_Break", // enumerated
    "ASCII_Hex_Digit",
    "Alphabetic",
    "Bidi_Control",
    "Bidi_Mirrored",
    "Cased",
    "Composition_Exclusion",
    "Case_Ignorable",
    "Full_Composition_Exclusion",
    "Changes_When_Casefolded",
    "Changes_When_Casemapped",
    "Changes_When_NFKC_Casefolded",
    "Changes_When_Lowercased",
    "Changes_When_Titlecased",
    "Changes_When_Uppercased",
    "Dash",
    "Deprecated",
    "Default_Ignorable_Code_Point",
    "Diacritic",
    "Emoji_Modifier_Base",
    "Emoji_Component",
    "Emoji_Modifier",
    "Emoji",
    "Emoji_Presentation",
    "Extender",
    "Extended_Pictographic",
    "Grapheme_Base",
    "Grapheme_Extend",
    "Grapheme_Link",
    "Hex_Digit",
    "Hyphen",
    "ID_Compat_Math_Continue",
    "ID_Compat_Math_Start",
    "ID_Continue",
    "Ideographic",
    "ID_Start",
    "IDS_Binary_Operator",
    "IDS_Trinary_Operator",
    "IDS_Unary_Operator",
    "Join_Control",
    "kEH_NoMirror",
    "kEH_NoRotate",
    "Logical_Order_Exception",
    "Lowercase",
    "Math",
    "Modifier_Combining_Mark",
    "Noncharacter_Code_Point",
    "Other_Alphabetic",
    "Other_Default_Ignorable_Code_Point",
    "Other_Grapheme_Extend",
    "Other_ID_Continue",
    "Other_ID_Start",
    "Other_Lowercase",
    "Other_Math",
    "Other_Uppercase",
    "Pattern_Syntax",
    "Pattern_White_Space",
    "Prepended_Concatenation_Mark",
    "Quotation_Mark",
    "Radical",
    "Regional_Indicator",
    "Soft_Dotted",
    "Sentence_Terminal",
    "Terminal_Punctuation",
    "Unified_Ideograph",
    "Uppercase",
    "Variation_Selector",
    "White_Space",
    "XID_Continue",
    "XID_Start",
    "Expands_On_NFC",
    "Expands_On_NFD",
    "Expands_On_NFKC",
    "Expands_On_NFKD"
};

// Return if a codepoint has a property
MJB_EXPORT mjb_status mjb_codepoint_has_property(mjb_codepoint codepoint, mjb_property property,
    uint8_t *value) {
    if(!mjb_codepoint_is_valid(codepoint)) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    if(!mjb_unicode_has_property(codepoint, property, value)) {
        return MJB_STATUS_NOT_FOUND;
    }

    return MJB_STATUS_OK;
}

MJB_EXPORT mjb_status mjb_codepoint_properties(mjb_codepoint codepoint, uint8_t *buffer) {
    if(buffer == NULL || !mjb_codepoint_is_valid(codepoint)) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    // Reset the buffer to zero before filling it with properties.
    memset(buffer, 0, MJB_PR_BUFFER_SIZE);

    if(!mjb_unicode_properties(codepoint, buffer)) {
        return MJB_STATUS_NOT_FOUND;
    }

    return MJB_STATUS_OK;
}

MJB_EXPORT uint8_t mjb_codepoint_property(const uint8_t *properties, mjb_property property) {
    if(properties == NULL) {
        return 0;
    }

    return properties[property];
}

MJB_EXPORT mjb_script mjb_codepoint_script(mjb_codepoint codepoint) {
    if(!mjb_codepoint_is_valid(codepoint)) {
        return MJB_SC_ZZZZ;
    }

    uint8_t raw = 0;

    if(mjb_codepoint_has_property(codepoint, MJB_PR_SCRIPT, &raw) != MJB_STATUS_OK ||
        raw == MJB_SC_NOT_SET) {
        return MJB_SC_ZZZZ;
    }

    return (mjb_script)raw;
}

MJB_EXPORT bool mjb_codepoint_is_id_start(mjb_codepoint codepoint) {
    return mjb_codepoint_has_property(codepoint, MJB_PR_ID_START, NULL) == MJB_STATUS_OK;
}

MJB_EXPORT bool mjb_codepoint_is_id_continue(mjb_codepoint codepoint) {
    return mjb_codepoint_has_property(codepoint, MJB_PR_ID_CONTINUE, NULL) == MJB_STATUS_OK;
}

MJB_EXPORT bool mjb_codepoint_is_xid_start(mjb_codepoint codepoint) {
    return mjb_codepoint_has_property(codepoint, MJB_PR_XID_START, NULL) == MJB_STATUS_OK;
}

MJB_EXPORT bool mjb_codepoint_is_xid_continue(mjb_codepoint codepoint) {
    return mjb_codepoint_has_property(codepoint, MJB_PR_XID_CONTINUE, NULL) == MJB_STATUS_OK;
}

MJB_EXPORT bool mjb_codepoint_is_pattern_syntax(mjb_codepoint codepoint) {
    return mjb_codepoint_has_property(codepoint, MJB_PR_PATTERN_SYNTAX, NULL) == MJB_STATUS_OK;
}

MJB_EXPORT bool mjb_codepoint_is_pattern_white_space(mjb_codepoint codepoint) {
    return mjb_codepoint_has_property(codepoint, MJB_PR_PATTERN_WHITE_SPACE, NULL) ==
        MJB_STATUS_OK;
}

MJB_EXPORT const char *mjb_property_name(mjb_property property) {
    if(property >= MJB_PR_COUNT) {
        return "Unknown";
    }

    return mjb_property_names[property];
}
