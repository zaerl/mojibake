/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <string.h>

#include "mojibake-internal.h"
#include "unicode-tables.h"

// clang-format off
/**
 * See mjb_property enum in unicode.h for the list of properties.
 * This array is automatically generated. Do not edit.
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
    "kJURC_Src", // enumerated
    "kNSHU_DubenSrc", // enumerated
    "kSEAL_CCZSrc", // enumerated
    "kSEAL_DYCSrc", // enumerated
    "kSEAL_QJZSrc", // enumerated
    "kSEAL_THXSrc", // enumerated
    "kTGT_MergedSrc", // enumerated
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

// See PropertyAliases.txt / PropertyValueAliases.txt.
// This array is automatically generated. Do not edit.
static const bool mjb_property_is_binary[] = {
    false, // MJB_PR_KACCOUNTINGNUMERIC
    false, // MJB_PR_KOTHERNUMERIC
    false, // MJB_PR_KPRIMARYNUMERIC
    false, // MJB_PR_NUMERIC_VALUE
    false, // MJB_PR_BIDI_MIRRORING_GLYPH
    false, // MJB_PR_BIDI_PAIRED_BRACKET
    false, // MJB_PR_CASE_FOLDING
    false, // MJB_PR_KCOMPATIBILITYVARIANT
    false, // MJB_PR_DECOMPOSITION_MAPPING
    false, // MJB_PR_EQUIVALENT_UNIFIED_IDEOGRAPH
    false, // MJB_PR_FC_NFKC_CLOSURE
    false, // MJB_PR_LOWERCASE_MAPPING
    false, // MJB_PR_NFKC_CASEFOLD
    false, // MJB_PR_NFKC_SIMPLE_CASEFOLD
    false, // MJB_PR_SIMPLE_CASE_FOLDING
    false, // MJB_PR_SIMPLE_LOWERCASE_MAPPING
    false, // MJB_PR_SIMPLE_TITLECASE_MAPPING
    false, // MJB_PR_SIMPLE_UPPERCASE_MAPPING
    false, // MJB_PR_TITLECASE_MAPPING
    false, // MJB_PR_UPPERCASE_MAPPING
    false, // MJB_PR_KIICORE
    false, // MJB_PR_KIRG_GSOURCE
    false, // MJB_PR_KIRG_HSOURCE
    false, // MJB_PR_KIRG_JSOURCE
    false, // MJB_PR_KIRG_KPSOURCE
    false, // MJB_PR_KIRG_KSOURCE
    false, // MJB_PR_KIRG_MSOURCE
    false, // MJB_PR_KIRG_SSOURCE
    false, // MJB_PR_KIRG_TSOURCE
    false, // MJB_PR_KIRG_UKSOURCE
    false, // MJB_PR_KIRG_USOURCE
    false, // MJB_PR_KIRG_VSOURCE
    false, // MJB_PR_KMANDARIN
    false, // MJB_PR_KRSUNICODE
    false, // MJB_PR_KTOTALSTROKES
    false, // MJB_PR_KUNIHANCORE2020
    false, // MJB_PR_ISO_COMMENT
    false, // MJB_PR_JAMO_SHORT_NAME
    false, // MJB_PR_KEH_CAT
    false, // MJB_PR_KEH_DESC
    false, // MJB_PR_KEH_HG
    false, // MJB_PR_KEH_IFAO
    false, // MJB_PR_KEH_JSESH
    false, // MJB_PR_KJURC_SRC
    false, // MJB_PR_KNSHU_DUBENSRC
    false, // MJB_PR_KSEAL_CCZSRC
    false, // MJB_PR_KSEAL_DYCSRC
    false, // MJB_PR_KSEAL_QJZSRC
    false, // MJB_PR_KSEAL_THXSRC
    false, // MJB_PR_KTGT_MERGEDSRC
    false, // MJB_PR_NAME
    false, // MJB_PR_NAME_ALIAS
    false, // MJB_PR_SCRIPT_EXTENSIONS
    false, // MJB_PR_AGE
    false, // MJB_PR_SCRIPT
    false, // MJB_PR_BIDI_PAIRED_BRACKET_TYPE
    false, // MJB_PR_CANONICAL_COMBINING_CLASS
    false, // MJB_PR_DECOMPOSITION_TYPE
    false, // MJB_PR_EAST_ASIAN_WIDTH
    false, // MJB_PR_GENERAL_CATEGORY
    false, // MJB_PR_GRAPHEME_CLUSTER_BREAK
    false, // MJB_PR_HANGUL_SYLLABLE_TYPE
    false, // MJB_PR_INDIC_CONJUNCT_BREAK
    false, // MJB_PR_INDIC_POSITIONAL_CATEGORY
    false, // MJB_PR_INDIC_SYLLABIC_CATEGORY
    false, // MJB_PR_JOINING_GROUP
    false, // MJB_PR_JOINING_TYPE
    false, // MJB_PR_LINE_BREAK
    false, // MJB_PR_NFC_QUICK_CHECK
    true, // MJB_PR_NFD_QUICK_CHECK
    false, // MJB_PR_NFKC_QUICK_CHECK
    true, // MJB_PR_NFKD_QUICK_CHECK
    false, // MJB_PR_NUMERIC_TYPE
    false, // MJB_PR_SENTENCE_BREAK
    false, // MJB_PR_VERTICAL_ORIENTATION
    false, // MJB_PR_WORD_BREAK
    true, // MJB_PR_ASCII_HEX_DIGIT
    true, // MJB_PR_ALPHABETIC
    true, // MJB_PR_BIDI_CONTROL
    true, // MJB_PR_BIDI_MIRRORED
    true, // MJB_PR_CASED
    true, // MJB_PR_COMPOSITION_EXCLUSION
    true, // MJB_PR_CASE_IGNORABLE
    true, // MJB_PR_FULL_COMPOSITION_EXCLUSION
    true, // MJB_PR_CHANGES_WHEN_CASEFOLDED
    true, // MJB_PR_CHANGES_WHEN_CASEMAPPED
    true, // MJB_PR_CHANGES_WHEN_NFKC_CASEFOLDED
    true, // MJB_PR_CHANGES_WHEN_LOWERCASED
    true, // MJB_PR_CHANGES_WHEN_TITLECASED
    true, // MJB_PR_CHANGES_WHEN_UPPERCASED
    true, // MJB_PR_DASH
    true, // MJB_PR_DEPRECATED
    true, // MJB_PR_DEFAULT_IGNORABLE_CODE_POINT
    true, // MJB_PR_DIACRITIC
    true, // MJB_PR_EMOJI_MODIFIER_BASE
    true, // MJB_PR_EMOJI_COMPONENT
    true, // MJB_PR_EMOJI_MODIFIER
    true, // MJB_PR_EMOJI
    true, // MJB_PR_EMOJI_PRESENTATION
    true, // MJB_PR_EXTENDER
    true, // MJB_PR_EXTENDED_PICTOGRAPHIC
    true, // MJB_PR_GRAPHEME_BASE
    true, // MJB_PR_GRAPHEME_EXTEND
    true, // MJB_PR_GRAPHEME_LINK
    true, // MJB_PR_HEX_DIGIT
    true, // MJB_PR_HYPHEN
    true, // MJB_PR_ID_COMPAT_MATH_CONTINUE
    true, // MJB_PR_ID_COMPAT_MATH_START
    true, // MJB_PR_ID_CONTINUE
    true, // MJB_PR_IDEOGRAPHIC
    true, // MJB_PR_ID_START
    true, // MJB_PR_IDS_BINARY_OPERATOR
    true, // MJB_PR_IDS_TRINARY_OPERATOR
    true, // MJB_PR_IDS_UNARY_OPERATOR
    true, // MJB_PR_JOIN_CONTROL
    true, // MJB_PR_KEH_NOMIRROR
    true, // MJB_PR_KEH_NOROTATE
    true, // MJB_PR_LOGICAL_ORDER_EXCEPTION
    true, // MJB_PR_LOWERCASE
    true, // MJB_PR_MATH
    true, // MJB_PR_MODIFIER_COMBINING_MARK
    true, // MJB_PR_NONCHARACTER_CODE_POINT
    true, // MJB_PR_OTHER_ALPHABETIC
    true, // MJB_PR_OTHER_DEFAULT_IGNORABLE_CODE_POINT
    true, // MJB_PR_OTHER_GRAPHEME_EXTEND
    true, // MJB_PR_OTHER_ID_CONTINUE
    true, // MJB_PR_OTHER_ID_START
    true, // MJB_PR_OTHER_LOWERCASE
    true, // MJB_PR_OTHER_MATH
    true, // MJB_PR_OTHER_UPPERCASE
    true, // MJB_PR_PATTERN_SYNTAX
    true, // MJB_PR_PATTERN_WHITE_SPACE
    true, // MJB_PR_PREPENDED_CONCATENATION_MARK
    true, // MJB_PR_QUOTATION_MARK
    true, // MJB_PR_RADICAL
    true, // MJB_PR_REGIONAL_INDICATOR
    true, // MJB_PR_SOFT_DOTTED
    true, // MJB_PR_SENTENCE_TERMINAL
    true, // MJB_PR_TERMINAL_PUNCTUATION
    true, // MJB_PR_UNIFIED_IDEOGRAPH
    true, // MJB_PR_UPPERCASE
    true, // MJB_PR_VARIATION_SELECTOR
    true, // MJB_PR_WHITE_SPACE
    true, // MJB_PR_XID_CONTINUE
    true, // MJB_PR_XID_START
    true, // MJB_PR_EXPANDS_ON_NFC
    true, // MJB_PR_EXPANDS_ON_NFD
    true, // MJB_PR_EXPANDS_ON_NFKC
    true  // MJB_PR_EXPANDS_ON_NFKD
};
// End generated property types.
// clang-format on

static bool mjb_property_valid(mjb_property property) {
    return (unsigned int)property < MJB_PR_COUNT;
}

static bool mjb_property_binary(mjb_property property) {
    return mjb_property_valid(property) && mjb_property_is_binary[property];
}

// Return a binary property value. A missing binary property has the Unicode default value false.
MJB_EXPORT mjb_status mjb_codepoint_property_binary(mjb_codepoint codepoint, mjb_property property,
    bool *value) {
    if(value == NULL || !mjb_codepoint_is_valid(codepoint) || !mjb_property_binary(property)) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    *value = mjb_unicode_has_property(codepoint, property, NULL);
    return MJB_STATUS_OK;
}

bool mjb_codepoint_has_binary_property(mjb_codepoint codepoint, mjb_property property) {
    bool value = false;

    return mjb_codepoint_property_binary(codepoint, property, &value) == MJB_STATUS_OK && value;
}

// Return an enumerated or integer property value.
MJB_EXPORT mjb_status mjb_codepoint_property_int(mjb_codepoint codepoint, mjb_property property,
    int32_t *value) {
    if(value == NULL || !mjb_codepoint_is_valid(codepoint) || !mjb_property_valid(property) ||
        mjb_property_binary(property)) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    uint8_t raw = 0;

    if(!mjb_unicode_has_property(codepoint, property, &raw)) {
        return MJB_STATUS_NOT_FOUND;
    }

    *value = (int32_t)raw;

    return MJB_STATUS_OK;
}

mjb_status mjb_codepoint_properties_lookup(mjb_codepoint codepoint, uint8_t *buffer) {
    if(buffer == NULL || !mjb_codepoint_is_valid(codepoint)) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    memset(buffer, 0, MJB_PR_BUFFER_SIZE);

    if(!mjb_unicode_properties(codepoint, buffer)) {
        return MJB_STATUS_NOT_FOUND;
    }

    return MJB_STATUS_OK;
}

uint8_t mjb_codepoint_properties_get(const uint8_t *properties, mjb_property property) {
    if(properties == NULL || (unsigned int)property >= MJB_PR_COUNT) {
        return 0;
    }

    return properties[property];
}

MJB_EXPORT mjb_script mjb_codepoint_script(mjb_codepoint codepoint) {
    if(!mjb_codepoint_is_valid(codepoint)) {
        return MJB_SC_ZZZZ;
    }

    int32_t raw = 0;

    if(mjb_codepoint_property_int(codepoint, MJB_PR_SCRIPT, &raw) != MJB_STATUS_OK ||
        raw == MJB_SC_NOT_SET) {
        return MJB_SC_ZZZZ;
    }

    return (mjb_script)raw;
}

MJB_EXPORT mjb_status mjb_codepoint_script_extensions(mjb_codepoint codepoint, mjb_script *scripts,
    size_t *count) {
    if(count == NULL || !mjb_codepoint_is_valid(codepoint)) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    const uint8_t *values = NULL;
    uint8_t value_count = 0;
    uint8_t fallback = (uint8_t)MJB_SC_ZZZZ;

    if(!mjb_unicode_script_extensions_lookup(codepoint, &values, &value_count)) {
        fallback = (uint8_t)mjb_codepoint_script(codepoint);
        values = &fallback;
        value_count = 1;
    }

    size_t capacity = *count;
    *count = value_count;

    if(scripts == NULL) {
        return MJB_STATUS_OK;
    }

    if(capacity < value_count) {
        return MJB_STATUS_OUTPUT_TOO_SMALL;
    }

    for(uint8_t i = 0; i < value_count; ++i) {
        scripts[i] = (mjb_script)values[i];
    }

    return MJB_STATUS_OK;
}

MJB_EXPORT bool mjb_codepoint_is_id_start(mjb_codepoint codepoint) {
    return mjb_codepoint_has_binary_property(codepoint, MJB_PR_ID_START);
}

MJB_EXPORT bool mjb_codepoint_is_id_continue(mjb_codepoint codepoint) {
    return mjb_codepoint_has_binary_property(codepoint, MJB_PR_ID_CONTINUE);
}

MJB_EXPORT bool mjb_codepoint_is_xid_start(mjb_codepoint codepoint) {
    return mjb_codepoint_has_binary_property(codepoint, MJB_PR_XID_START);
}

MJB_EXPORT bool mjb_codepoint_is_xid_continue(mjb_codepoint codepoint) {
    return mjb_codepoint_has_binary_property(codepoint, MJB_PR_XID_CONTINUE);
}

MJB_EXPORT bool mjb_codepoint_is_pattern_syntax(mjb_codepoint codepoint) {
    return mjb_codepoint_has_binary_property(codepoint, MJB_PR_PATTERN_SYNTAX);
}

MJB_EXPORT bool mjb_codepoint_is_pattern_white_space(mjb_codepoint codepoint) {
    return mjb_codepoint_has_binary_property(codepoint, MJB_PR_PATTERN_WHITE_SPACE);
}

MJB_EXPORT const char *mjb_property_name(mjb_property property) {
    if((unsigned int)property >= MJB_PR_COUNT) {
        return "Unknown";
    }

    return mjb_property_names[property];
}
