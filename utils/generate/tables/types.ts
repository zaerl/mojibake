/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

// Unicode tables types

export type BlockRow = {
  id: number;
  start: number;
  end: number;
  name: string;
};

export type PrefixRow = {
  id: number;
  name: string;
};

export type NameRow = {
  codepoint: number;
  name: string | null;
  prefix: number | null;
};

export type EmojiRow = {
  codepoint: number;
  emoji: number;
  emoji_presentation: number;
  emoji_modifier: number;
  emoji_modifier_base: number;
  emoji_component: number;
  extended_pictographic: number;
};

export type PropertyRangeRow = {
  start_codepoint: number;
  end_codepoint: number | null;
  properties: Buffer;
};

export type NCharacterRow = {
  codepoint: number;
  category: number;
  combining: number | null;
  bidirectional: number;
  decomposition: number | null;
  quick_check: number | null;
  mirrored: number;
};

export type DecompositionRow = {
  id: number;
  value: number;
};

export type CompositionRow = {
  starter_codepoint: number;
  combining_codepoint: number;
  composite_codepoint: number;
};

export type NumericRow = {
  codepoint: number;
  decimal: number | null;
  digit: number | null;
  numeric: string | null;
};

export type SimpleCaseRow = {
  codepoint: number;
  uppercase: number | null;
  lowercase: number | null;
  titlecase: number | null;
};

export type SpecialCaseRow = {
  codepoint: number;
  case_type: number;
  new_case_1: number;
  new_case_2: number | null;
  new_case_3: number | null;
};

export type CaseFoldRow = {
  codepoint: number;
  new_case_1: number;
  new_case_2: number | null;
  new_case_3: number | null;
};

export type ConfusableRow = {
  codepoint: number;
  skeleton: Buffer;
};

export type CollationEntryRow = {
  codepoint: number;
  weights: Buffer;
};

export type CollationContractionRow = {
  first_codepoint: number;
  sequence: Buffer;
  weights: Buffer;
};
