/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

// mjb_bidi_class
export enum BidiClass {
  NOT_SET, // 0 is "no value"
  L,
  R,
  AL,
  EN,
  ES,
  ET,
  AN,
  CS,
  NSM,
  BN,
  B,
  S,
  WS,
  ON,
  LRE,
  LRO,
  RLE,
  RLO,
  PDF,
  LRI,
  RLI,
  FSI,
  PDI
};

// mjb_bidi_paired_bracket_type
export enum BidiPairedBracketType {
  NOT_SET, // 0 is "no value"
  CLOSE,
  NONE,
  OPEN,
};

// mjb_block
// This enum is automatically generated. Do not edit.
enum Block {
  // TODO
};

// mjb_category
// This enum is automatically generated. Do not edit.
export enum Category {
  // TODO
};

// mjb_canonical_combining_class
export enum CanonicalCombiningClass {
  NOT_REORDERED = 0,
  OVERLAY = 1,
  HAN_READING = 6,
  NUKTA = 7,
  KANA_VOICING = 8,
  VIRAMA = 9,
  CCC_10 = 10,
  CCC_11 = 11,
  CCC_12 = 12,
  CCC_13 = 13,
  CCC_14 = 14,
  CCC_15 = 15,
  CCC_16 = 16,
  CCC_17 = 17,
  CCC_18 = 18,
  CCC_19 = 19,
  CCC_20 = 20,
  CCC_21 = 21,
  CCC_22 = 22,
  CCC_23 = 23,
  CCC_24 = 24,
  CCC_25 = 25,
  CCC_26 = 26,
  CCC_27 = 27,
  CCC_28 = 28,
  CCC_29 = 29,
  CCC_30 = 30,
  CCC_31 = 31,
  CCC_32 = 32,
  CCC_33 = 33,
  CCC_34 = 34,
  CCC_35 = 35,
  CCC_36 = 36,
  CCC_84 = 84,
  CCC_91 = 91,
  CCC_103 = 103,
  CCC_107 = 107,
  CCC_118 = 118,
  CCC_122 = 122,
  CCC_129 = 129,
  CCC_130 = 130,
  CCC_132 = 132,
  // MJB_CCC_133 reserved
  ATTACHED_BELOW_LEFT = 200,
  ATTACHED_BELOW = 202,
  ATTACHED_ABOVE = 214,
  ATTACHED_ABOVE_RIGHT = 216,
  BELOW_LEFT = 218,
  BELOW = 220,
  BELOW_RIGHT = 222,
  LEFT = 224,
  RIGHT = 226,
  ABOVE_LEFT = 228,
  ABOVE = 230,
  ABOVE_RIGHT = 232,
  DOUBLE_BELOW = 233,
  DOUBLE_ABOVE = 234,
  BELOW_IOTA = 240
};

// mjb_decomposition
// This enum is automatically generated. Do not edit.
export enum Decomposition {

};

// mjb_east_asian_width
export enum EastAsianWidth {
  NOT_SET, // 0 is "no value"
  AMBIGUOUS,
  FULL_WIDTH,
  HALF_WIDTH,
  NEUTRAL,
  NARROW,
  WIDE
};

// mjb_general_category
export enum GeneralCategory {
  NOT_SET, // 0 is "no value"
  OTHER,
  CONTROL,
  FORMAT,
  UNASSIGNED,
  PRIVATE_USE,
  SURROGATE,
  LETTER,
  CASED_LETTER,
  LOWERCASE_LETTER,
  MODIFIER_LETTER,
  OTHER_LETTER,
  TITLECASE_LETTER,
  UPPERCASE_LETTER,
  MARK,
  SPACING_MARK,
  ENCLOSING_MARK,
  NONSPACING_MARK,
  NUMBER,
  DECIMAL_NUMBER,
  LETTER_NUMBER,
  OTHER_NUMBER,
  PUNCTUATION,
  CONNECTOR_PUNCTUATION,
  DASH_PUNCTUATION,
  CLOSE_PUNCTUATION,
  FINAL_PUNCTUATION,
  INITIAL_PUNCTUATION,
  OTHER_PUNCTUATION,
  OPEN_PUNCTUATION,
  SYMBOL,
  CURRENCY_SYMBOL,
  MODIFIER_SYMBOL,
  MATH_SYMBOL,
  OTHER_SYMBOL,
  SEPARATOR,
  LINE_SEPARATOR,
  PARAGRAPH_SEPARATOR,
  SPACE_SEPARATOR
};

// mjb_gcb
export enum GraphemeClusterBreak {
  NOT_SET, // 0 is "no value"
  CONTROL,
  CR,
  E_BASE, // obsolete
  E_BASE_GAZ, // obsolete
  E_MODIFIER, // obsolete
  EXTEND,
  GLUE_AFTER_ZWJ,
  L,
  LF,
  LV,
  LVT,
  PREPEND,
  REGIONAL_INDICATOR,
  SPACING_MARK,
  T,
  V,
  OTHER,
  ZWJ
};

// mjb_hangul_syllable_type
export enum HangulSyllableType {
  NOT_SET, // 0 is "no value"
  L,
  LV,
  LVT,
  NA,
  T,
  V
};

// mjb_indic_conjunct_break
export enum IndicConjunctBreak {
  NOT_SET, // 0 is "no value"
  CONSONANT,
  EXTEND,
  LINKER,
  NONE
};

// mjb_plane
export enum Plane {
  NOT_VALID = -1,
  BMP =        0,
  SMP =        1,
  SIP =        2,
  TIP =        3,
  SSP =        4,
  PUA_A =      5,
  PUA_B =     16
};

// mjb_wbp
export enum WordBreakProperty {
  NOT_SET, // 0 is "no value"
  CR,
  DOUBLE_QUOTE,
  E_BASE,
  E_BASE_GAZ,
  E_MODIFIER,
  EXTEND_NUM_LET,
  EXTEND,
  FORMAT,
  GLUE_AFTER_ZWJ,
  HEBREW_LETTER,
  KATAKANA,
  A_LETTER,
  LF,
  MID_NUM_LET,
  MID_LETTER,
  MID_NUM,
  NEWLINE,
  NUMERIC,
  REGIONAL_INDICATOR,
  SINGLE_QUOTE,
  W_SEG_SPACE,
  OTHER,
  ZWJ
};

// mjb_lbp
export enum LineBreakProperty {
  NOT_SET, // 0 is "no value"
  AI,
  AK,
  AL,
  AP,
  AS,
  B2,
  BA,
  BB,
  BK,
  CB,
  CJ,
  CL,
  CM,
  CP,
  CR,
  EB,
  EM,
  EX,
  GL,
  H2,
  H3,
  HH,
  HL,
  HY,
  ID,
  IN,
  IS,
  JL,
  JT,
  JV,
  LF,
  NL,
  NS,
  NU,
  OP,
  PO,
  PR,
  QU,
  RI,
  SA,
  SG,
  SP,
  SY,
  VF,
  VI,
  WJ,
  XX,
  ZW,
  ZWJ
};

// mjb_sbp
export enum SentenceBreakProperty {
  NOT_SET, // 0 is "no value"
  ATERM,
  CLOSE,
  CR,
  EXTEND,
  FORMAT,
  OLETTER,
  LF,
  LOWER,
  NUMERIC,
  SCONTINUE,
  SEP,
  SP,
  STERM,
  UPPER,
  OTHER
};

// mjb_script
// This enum is automatically generated. Do not edit.
export enum Script {
  // TODO
};

// mjb_property
// This enum is automatically generated. Do not edit.
export enum Property {
  // TODO
};
