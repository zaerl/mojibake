/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#pragma once

#ifndef MJB_BREAK_H
#define MJB_BREAK_H

#include "mojibake.h"

/**
 * Unicode encoding
 * [see: https://www.unicode.org/glossary/#character_encoding_scheme]
 */
typedef enum mjb_line_breaking_class {
    // Non-tailorable Line Breaking Classes
    MJB_LBC_BK, // Mandatory Break
    MJB_LBC_CR, // Carriage Return
    MJB_LBC_LF, // Line Feed
    MJB_LBC_CM, // Combining Mark
    MJB_LBC_NL, // Next Line
    MJB_LBC_SG, // Surrogate
    MJB_LBC_WJ, // Word Joiner
    MJB_LBC_ZW, // Zero Width Space
    MJB_LBC_GL, // Non-breaking ("glue")
    MJB_LBC_SP, // Space
    MJB_LBC_ZWJ, // Zero Width Joiner

    // Break Opportunities
    MJB_LBC_B2, // Break Opportunity Before and After
    MJB_LBC_BA, // Break After
    MJB_LBC_BB, // Break Before
    MJB_LBC_HY, // Hyphen
    MJB_LBC_HH, // Unambiguous Hyphen
    MJB_LBC_CB, // Contingent Break Opportunity

    // Characters Prohibiting Certain Breaks
    MJB_LBC_CL, // Close Punctuation
    MJB_LBC_CP, // Close Parenthesis
    MJB_LBC_EX, // Exclamation / Interrogation
    MJB_LBC_IN, // Inseparable
    MJB_LBC_NS, // Nonstarter
    MJB_LBC_OP, // Open Punctuation
    MJB_LBC_QU, // Quotation

    // Numeric Context
    MJB_LBC_IS, // Infix Numeric Separator
    MJB_LBC_NU, // Numeric
    MJB_LBC_PO, // Postfix Numeric
    MJB_LBC_PR, // Prefix Numeric
    MJB_LBC_SY, // Symbols Allowing Break After

    // Other Characters
    MJB_LBC_AI, // Ambiguous (Alphabetic or Ideographic)
    MJB_LBC_AK, // Aksara
    MJB_LBC_AL, // Alphabetic
    MJB_LBC_AP, // Aksara Pre-Base
    MJB_LBC_AS, // Aksara Start
    MJB_LBC_CJ, // Conditional Japanese Starter
    MJB_LBC_EB, // Emoji Base
    MJB_LBC_EM, // Emoji Modifier
    MJB_LBC_H2, // Hangul LV Syllable
    MJB_LBC_H3, // Hangul LVT Syllable
    MJB_LBC_HL, // Hebrew Letter
    MJB_LBC_ID, // Ideographic
    MJB_LBC_JL, // Hangul L Jamo
    MJB_LBC_JV, // Hangul V Jamo
    MJB_LBC_JT, // Hangul T Jamo
    MJB_LBC_RI, // Regional Indicator
    MJB_LBC_SA, // Complex Context Dependent (South East Asian)
    MJB_LBC_VF, // Virama Final
    MJB_LBC_VI, // Virama
    MJB_LBC_XX  // Unknown
} mjb_line_breaking_class;

#define MJB_LBC_COUNT 48

MJB_NONNULL(2) bool mjb_codepoint_line_breaking_class(mjb_codepoint codepoint,
    mjb_line_breaking_class *line_breaking_class, mjb_category *category);

#endif // MJB_BREAK_H
