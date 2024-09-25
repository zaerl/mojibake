export interface CountBuffer {
  name: string;
  count: number;
}

export interface Numeric {
  name: string;
  value: number;
  count: number;
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

export type CategoriesStrings = keyof typeof Categories;

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

// C: mjb_decomposition
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
  // Block additional
];

export class Character {
  constructor(
    public codepoint: number,
    public name: string,
    public category: number,
    public combining: number,
    public bidirectional: BidirectionalCategories | null,
    public decomposition: number,
    public decimal: number | null,
    public digit: number | null,
    public numeric: string | null,
    public mirrored: boolean,
    // unicode 1.0 name
    // comment
    public uppercase: number,
    public lowercase: number,
    public titlecase: number,
    public block: number, // Additional
  ) {}

  formatC(): string {
    return `{ ${this.fmt(this.codepoint)}, ${this.fmt(this.name)}, ${this.fmt(this.category)}, ` +
      `${this.fmt(this.combining)}, ${this.fmt(this.bidirectional)}, ${this.fmt(this.decomposition)}, ` +
      `${this.fmt(this.decimal)}, ${this.fmt(this.digit)}, ` + `${this.fmt(this.numeric)}, ${this.mirrored}, ` +
      `${this.fmt(this.uppercase)}, ${this.fmt(this.lowercase)}, ${this.fmt(this.titlecase)}, ${this.fmt(this.block)} }`;
  }

  public fmt(value: string | number | null, defaultC = 'NULL'): string {
    if(value === null) {
      return defaultC;
    } else {
      return typeof value === 'number' ? `${value === 0 ? 0 : '0x' + value.toString(16).toUpperCase()}` : `"${value}"`;
    }
  }
}

// All blocks
export interface Block {
  name: string;
  enumName: string;
  start: number;
  end: number;
};
