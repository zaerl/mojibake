/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { iLog } from '../log';
import { codepointPages, formatBytes, formatCodepoints, formatHalfwords, formatLongWords, formatPages, indexedPages, packCodepointSequences } from '../utils';
import { CaseFoldRow, SimpleCaseRow, SpecialCaseRow } from './types';

// Returns the non-null codepoint sequence for special casing or case folding.
function caseSequenceValues(row: SpecialCaseRow | CaseFoldRow) {
  const values = [row.new_case_1];

  if(row.new_case_2 !== null) {
    values.push(row.new_case_2);
  }

  if(row.new_case_3 !== null) {
    values.push(row.new_case_3);
  }

  return values;
}

// Emits indexed simple case mappings with packed signed deltas.
export function generateSimpleCaseMappings(rows: SimpleCaseRow[]) {
  iLog('Simple case mappings');

  const pages = indexedPages(codepointPages(rows));
  const lows = rows.map((row) => row.codepoint & 0xFF);
  const data: bigint[] = [];
  const dataOffsets = new Map<string, number>();
  const entries: number[] = [];
  // Packs one signed case delta into its fixed-width two's-complement field.
  const packDelta = (value: number) => {
    if(value < -0x20000 || value > 0x1FFFF) {
      throw new Error(`Simple case delta is too large to pack: ${value}`);
    }

    return BigInt(value & 0x3FFFF);
  };

  for(const row of rows) {
    const mask =
      (row.uppercase !== null ? 1 : 0) |
      (row.lowercase !== null ? 2 : 0) |
      (row.titlecase !== null ? 4 : 0);
    const upperDelta = row.uppercase !== null ? row.uppercase - row.codepoint : 0;
    const lowerDelta = row.lowercase !== null ? row.lowercase - row.codepoint : 0;
    const titleDelta = row.titlecase !== null ? row.titlecase - row.codepoint : 0;
    const key = `${mask},${upperDelta},${lowerDelta},${titleDelta}`;
    let offset = dataOffsets.get(key);

    if(offset === undefined) {
      offset = data.length;
      data.push(packDelta(upperDelta) |
        (packDelta(lowerDelta) << 18n) |
        (packDelta(titleDelta) << 36n) |
        (BigInt(mask) << 54n));
      dataOffsets.set(key, offset);
    }

    if(offset > 0xFF) {
      throw new Error(`Simple case mapping data offset is too large to pack: ${offset}`);
    }

    entries.push(offset);
  }

  return `static const uint16_t mjb_unicode_simple_case_page_index[] = {
${formatHalfwords(pages.index)}
};

static const mjb_unicode_page mjb_unicode_simple_case_pages[] = {
${formatPages(pages.pages)}
};

static const uint8_t mjb_unicode_simple_case_lows[] = {
${formatBytes(lows)}
};

static const uint64_t mjb_unicode_simple_case_mapping_data[] = {
${formatLongWords(data, 0)}
};

static const uint8_t mjb_unicode_simple_case_mappings[] = {
${formatBytes(entries, 24)}
};
`;
}

// Emits packed full special-casing mappings and their shared sequence payload.
export function generateSpecialCaseMappings(rows: SpecialCaseRow[]) {
  iLog('Special case mappings');

  const sequences = packCodepointSequences(rows.map((row) => caseSequenceValues(row)));
  const entries = rows.map((row, index) => {
    const sequence = sequences.entries[index];

    if(row.codepoint > 0x1FFFFF || row.case_type > 0x7 || sequence.length > 0x3 ||
      sequence.offset > 0xFFFFFFFF) {
      throw new Error(`Special case mapping is too large to pack: ${row.codepoint}`);
    }

    return BigInt(row.codepoint) |
      (BigInt(row.case_type) << 21n) |
      (BigInt(sequence.length) << 24n) |
      (BigInt(sequence.offset) << 26n);
  });

  return `typedef uint64_t mjb_unicode_special_case_entry;

static const mjb_codepoint mjb_unicode_special_case_data[] = {
${formatCodepoints(sequences.data)}
};

static const mjb_unicode_special_case_entry mjb_unicode_special_case_mappings[] = {
${formatLongWords(entries)}
};
`;
}

// Emits packed case-fold mappings and their shared sequence payload.
export function generateCaseFoldMappings(rows: CaseFoldRow[]) {
  iLog('Case fold mappings');

  const sequences = packCodepointSequences(rows.map((row) => caseSequenceValues(row)));
  const entries = rows.map((row, index) => {
    const sequence = sequences.entries[index];

    if(row.codepoint > 0x1FFFFF || sequence.length > 0x3 || sequence.offset > 0xFFFFFFFF) {
      throw new Error(`Case folding mapping is too large to pack: ${row.codepoint}`);
    }

    return BigInt(row.codepoint) |
      (BigInt(sequence.length) << 21n) |
      (BigInt(sequence.offset) << 23n);
  });

  return `typedef uint64_t mjb_unicode_case_fold_entry;

static const mjb_codepoint mjb_unicode_case_fold_data[] = {
${formatCodepoints(sequences.data)}
};

static const mjb_unicode_case_fold_entry mjb_unicode_case_fold_mappings[] = {
${formatLongWords(entries)}
};
`;
}
