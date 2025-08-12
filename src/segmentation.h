/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

/**
 * Grapheme_Cluster_Break Property Values
 * See: https://www.unicode.org/reports/tr29/#Grapheme_Cluster_Break_Property_Values
 */
typedef enum mjb_grapheme_break_property {
    MJB_GBP_CR,
    MJB_GBP_LF,
    MJB_GBP_CONTROL,
    MJB_GBP_EXTEND,
    MJB_GBP_ZWJ,
    MJB_GBP_REGIONAL_INDICATOR,
    MJB_GBP_PREPEND,
    MJB_GBP_SPACING_MARK,
    MJB_GBP_L,
    MJB_GBP_V,
    MJB_GBP_T,
    MJB_GBP_LV,
    MJB_GBP_LVT,
    MJB_GBP_ANY
} mjb_grapheme_break_property;
