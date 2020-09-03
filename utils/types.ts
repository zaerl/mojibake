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
  '<circle>': 7,
  '<compat>': 16,
  '<final>': 5,
  '<font>': 1,
  '<fraction>': 15,
  '<initial>': 3,
  '<isolated>': 6,
  '<medial>': 4,
  '<narrow>': 12,
  '<noBreak>': 2,
  '<small>': 13,
  '<square>': 14,
  '<sub>': 9,
  '<super>': 8,
  '<vertical>': 10,
  '<wide>': 11
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
