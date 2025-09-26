/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#pragma once

#ifndef MJB_SEGMENTATION_H
#define MJB_SEGMENTATION_H

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

typedef enum mjb_word_break_property {
    MJB_WBP_CR,
    MJB_WBP_LF,
    MJB_WBP_NEWLINE,
    MJB_WBP_EXTEND,
    MJB_WBP_ZWJ,
    MJB_WBP_REGIONAL_INDICATOR,
    MJB_WBP_FORMAT,
    MJB_WBP_KATAKANA,
    MJB_WBP_HEBREW_LETTER,
    MJB_WBP_A_LETTER,
    MJB_WBP_SINGLE_QUOTE,
    MJB_WBP_DOUBLE_QUOTE,
    MJB_WBP_MID_NUM_LET,
    MJB_WBP_MID_LETTER,
    MJB_WBP_MID_NUM,
    MJB_WBP_NUMERIC,
    MJB_WBP_EXTEND_NUM_LET,
    MJB_WBP_E_BASE,
    MJB_WBP_E_MODIFIER,
    MJB_WBP_GLUE_AFTER_ZWJ,
    MJB_WBP_E_BASE_GAZ,
    MJB_WBP_W_SEG_SPACE,
    MJB_WBP_ANY
} mjb_word_break_property;

#endif // MJB_SEGMENTATION_H
