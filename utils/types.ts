export interface CountBuffer {
  name: string;
  count: number;
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
  Co,
  Cn
}

export type CategoriesStrings = keyof typeof Categories;

// C: mjb_category
export const categories = [
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
  'Other, private use',
  'Other, not assigned',
];

// C: mjb_bidi_categories
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
  string, // 10 unicode 1.0 name (ignored)
  string, // 11 10646 comment field (ignored)
  string, // 12 uppercase mapping
  string, // 13 lowercase mapping
  string // 14 titlecase mapping
];

// All blocks
export interface Block {
  name: string;
  enumName: string;
  start: number;
  end: number;
};
