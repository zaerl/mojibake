/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

export interface CountBuffer {
  name: string;
  count: number;
  countTotal?: number;
}

export interface PrefixCalc {
  name: string;
  count: number;
}

export interface Numeric {
  name: string;
  value: number;
  count: number;
}

export interface Composition {
  starter_codepoint: number;
  combining_codepoint: number;
  composite_codepoint: number;
}

export interface CalculatedDecomposition {
  codepoint: number;
  value: number;
}

export interface Decomposition {
  type: CharacterDecomposition;
  decomposition: number[];
}

// C: mjb_category
export enum Categories {
  Cn,
  Lu,
  Ll,
  Lt,
  Lm,
  Lo,
  Mn,
  Mc,
  Me,
  Nd,
  Nl,
  No,
  Pc,
  Pd,
  Ps,
  Pe,
  Pi,
  Pf,
  Po,
  Sm,
  Sc,
  Sk,
  So,
  Zs,
  Zl,
  Zp,
  Cc,
  Cf,
  Cs,
  Co
}

export type CategoriesStrings = keyof typeof Categories;

// C: mjb_category
export const categories = [
  'Other, not assigned',
  'Letter, uppercase',
  'Letter, lowercase',
  'Letter, titlecase',
  'Letter, modifier',
  'Letter, other',
  'Mark, non-spacing',
  'Mark, spacing combining',
  'Mark, enclosing',
  'Number, decimal digit',
  'Number, letter',
  'Number, other',
  'Punctuation, connector',
  'Punctuation, dash',
  'Punctuation, open',
  'Punctuation, close',
  'Punctuation, initial quote',
  'Punctuation, final quote',
  'Punctuation, other',
  'Symbol, math',
  'Symbol, currency',
  'Symbol, modifier',
  'Symbol, other',
  'Separator, space',
  'Separator, line',
  'Separator, paragraph',
  'Other, control',
  'Other, format',
  'Other, surrogate',
  'Other, private use'
];

// C: mjb_bidi_class
export enum BidirectionalCategories {
  NONE,
  // Strong
  L,
  R,
  AL,
  // Weak
  EN,
  ES,
  ET,
  AN,
  CS,
  NSM,
  BN,
  // Neutral
  B,
  S,
  WS,
  ON,
  // Explicit formatting
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

export type BidirectionalCategoriesStrings = (keyof typeof BidirectionalCategories) | '';

export enum CharacterDecomposition {
  None,
  Canonical,
  Cicle,
  Compat,
  Final,
  Font,
  Fraction,
  Initial,
  Isolated,
  Medial,
  Narrow,
  NoBreak,
  Small,
  Square,
  Sub,
  Super,
  Vertical,
  Wide,
};

// C: mjb_decomposition
export const characterDecompositionMapping = {
  'none': CharacterDecomposition.None,
  'canonical': 1,
  '<circle>': 2,
  '<compat>': 3,
  '<final>': 4,
  '<font>': 5,
  '<fraction>': 6,
  '<initial>': 7,
  '<isolated>': 8,
  '<medial>': 9,
  '<narrow>': 10,
  '<noBreak>': 11,
  '<small>': 12,
  '<square>': 13,
  '<sub>': 14,
  '<super>': 15,
  '<vertical>': 16,
  '<wide>': 17
};

export type CharacterDecompositionMappingStrings = keyof typeof characterDecompositionMapping;

// C: bool
export type Mirrored = 'Y' | 'N';

// UnicodeData.txt raw values
export type UnicodeDataRow = [
  string, // 0 codepoint
  string, // 1 character name
  CategoriesStrings, // 2 category
  string, // 3 canonical combining classes
  BidirectionalCategoriesStrings, // 4 bidirectional category
  string, // 5 character decomposition mapping
  string, // 6 decimal digit value
  string, // 7 digit value
  string, // 8 numeric value
  Mirrored, // 9 mirrored
  string, // 10 Unicode 1.0 name (ignored)
  string, // 11 10646 comment field (ignored)
  string, // 12 uppercase mapping
  string, // 13 lowercase mapping
  string // 14 titlecase mapping
];

// All blocks
export interface Block {
  name: string;
  enumName: string;
  wasmEnumName: string;
  start: number;
  end: number;
};

export enum CaseType {
  None,
  UpperCase,
  LowerCase,
  TitleCase,
  CaseFold
};

export enum EmojiProperties {
  Emoji,
  Emoji_Presentation,
  Emoji_Modifier,
  Emoji_Modifier_Base,
  Emoji_Component,
  Extended_Pictographic
};

export type EmojiPropertiesStrings = keyof typeof EmojiProperties;

export interface BidiBracket {
  cp: number;
  pair: number;
  isOpen: boolean;
}

export interface BidiMirroringPair {
  cp: number;
  mirror: number;
}

// See mjb_caseless_mode on mojibake.h
export const caseModes = [
  'MJB_CASELESS_CANONICAL',
  'MJB_CASELESS_UNNORMALIZED',
  'MJB_CASELESS_COMPATIBILITY',
  'MJB_CASELESS_IDENTIFIER',
];

export const collationModes = [
  'MJB_COLLATION_DEFAULT',
  'MJB_COLLATION_NUMERIC',
];

export const directions = [
  'MJB_DIRECTION_LTR',
  'MJB_DIRECTION_RTL',
  'MJB_DIRECTION_TTB',
  'MJB_DIRECTION_BTT',
];

export const encodings = [
  'MJB_ENC_UTF_8',
  'MJB_ENC_UTF_16BE',
  'MJB_ENC_UTF_16LE',
  'MJB_ENC_UTF_32BE',
  'MJB_ENC_UTF_32LE',
];

export const encodingValues = [
  0x2,
  0x8,
  0x10,
  0x40,
  0x80,
];

export const filterFlags = [
  'MJB_FILTER_NORMALIZE',
  'MJB_FILTER_SPACES',
  'MJB_FILTER_COLLAPSE_SPACES',
  'MJB_FILTER_CONTROLS',
  'MJB_FILTER_NUMERIC',
  'MJB_FILTER_LIMIT_COMBINING',
];

export const filterFlagValues = [
  0x1,
  0x2,
  0x4,
  0x8,
  0x10,
  0x20,
];

export const identifierProfiles = [
  'MJB_IDENTIFIER_DEFAULT',
  'MJB_IDENTIFIER_NFKC'
];

export const caseType = [
  'MJB_CASE_UPPER',
  'MJB_CASE_LOWER',
  'MJB_CASE_TITLE',
  'MJB_CASE_CASEFOLD',
];

// See mjb_encoding on mojibake.h
export const caseTypeValues = [
  1,
  2,
  3,
  4,
];

export const normalizations = [
  'MJB_NORMALIZATION_NFC',
  'MJB_NORMALIZATION_NFD',
  'MJB_NORMALIZATION_NFKC',
  'MJB_NORMALIZATION_NFKD',
];

// See mjb_plane on unicode.h
export const planes = [
  'MJB_PLANE_BMP',
  'MJB_PLANE_SMP',
  'MJB_PLANE_SIP',
  'MJB_PLANE_TIP',
  'MJB_PLANE_SSP',
  'MJB_PLANE_PUA_A',
  'MJB_PLANE_PUA_B',
];

// See mjb_plane on unicode.h
export const planeValues = [
  0,
  1,
  2,
  3,
  4,
  5,
  16,
];

export const widthContexts = [
  'MJB_WIDTH_CONTEXT_WESTERN',
  'MJB_WIDTH_CONTEXT_EAST_ASIAN',
  'MJB_WIDTH_CONTEXT_AUTO',
];
