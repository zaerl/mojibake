export interface CountBuffer {
  name: string;
  count: number;
}

export interface Numeric {
  name: string;
  value: number;
  count: number;
}

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

export const categories = [
  'Letter, Uppercase',
  'Letter, Lowercase',
  'Letter, Titlecase',
  'Letter, Modifier',
  'Letter, Other',
  'Mark, Non-Spacing',
  'Mark, Spacing Combining',
  'Mark, Enclosing',
  'Number, Decimal Digit',
  'Number, Letter',
  'Number, Other',
  'Punctuation, Connector',
  'Punctuation, Dash',
  'Punctuation, Open',
  'Punctuation, Close',
  'Punctuation, Initial quote',
  'Punctuation, Final quote',
  'Punctuation, Other',
  'Symbol, Math',
  'Symbol, Currency',
  'Symbol, Modifier',
  'Symbol, Other',
  'Separator, Space',
  'Separator, Line',
  'Separator, Paragraph',
  'Other, Control',
  'Other, Format',
  'Other, Surrogate',
  'Other, Private Use',
  'Other, Not Assigned',
];

export type CategoriesStrings = keyof typeof Categories;

export enum BidirectionalCategories {
  L,
  LRE,
  LRO,
  R,
  AL,
  RLE,
  RLO,
  PDF,
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
  ON
};

export type BidirectionalCategoriesStrings = (keyof typeof BidirectionalCategories) | '';

export const characterDecompositionMapping = {
  'canonical': 0,
  '<circle>': 1,
  '<compat>': 2,
  '<final>': 3,
  '<font>': 4,
  '<fraction>': 5,
  '<initial>': 6,
  '<isolated>': 7,
  '<medial>': 8,
  '<narrow>': 9,
  '<noBreak>': 10,
  '<small>': 11,
  '<square>': 12,
  '<sub>': 13,
  '<super>': 14,
  '<vertical>': 15,
  '<wide>': 16
};

export type CharacterDecompositionMappingStrings = keyof typeof characterDecompositionMapping;

export type Mirrored = 'Y' | 'N';

export type UnicodeDataRow = [
  string, // 0 codepoint
  string, // 1 character name
  // block
  CategoriesStrings, // 2 category
  string, // 3 canonical combining classes
  BidirectionalCategoriesStrings, // 4 bidirectional category
  // decomposition type
  string, // 5 character decomposition mapping
  string, // 6 decimal digit value
  string, // 7 digit value
  string, // 8 numeric value
  Mirrored, // 9 mirrored
  string, // 10 unicode 1.0 name
  string, // 11 10646 comment field
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
