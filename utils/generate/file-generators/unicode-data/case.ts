/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { iLog } from '../../log';
import {
  codepointPageBitsets, codepointPages, formatBytes, formatCodepoints, formatCompactIntegers,
  formatHalfwords, formatLongWords, formatWords, indexedPages, packCodepointSequences
} from '../../utils';
import { CaseFoldRow, CaseFoldSimpleRow, SimpleCaseRow, SpecialCaseRow } from '../types';

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
  const pageBitsets = codepointPageBitsets(rows, pages.pages);
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

  return `static const uint8_t mjb_unicode_simple_case_page_index[] = {
${formatBytes(pages.index)}
};

static const uint16_t mjb_unicode_simple_case_page_starts[] = {
${formatHalfwords(pages.pages.starts)}
};

static const uint64_t mjb_unicode_simple_case_page_bits[] = {
${formatLongWords(pageBitsets.data, 16)}
};

static const uint32_t mjb_unicode_simple_case_page_ranks[] = {
${formatWords(pageBitsets.ranks)}
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
  const offsets: number[] = [];
  const entries = rows.map((row, index) => {
    const sequence = sequences.entries[index];

    if(row.codepoint > 0x1FFFFF || row.case_type > 0x7 || sequence.length > 0x3 ||
      sequence.offset > 0xFF) {
      throw new Error(`Special case mapping is too large to pack: ${row.codepoint}`);
    }

    offsets.push(sequence.offset);

    return (row.codepoint | (row.case_type << 21) | (sequence.length << 24)) >>> 0;
  });

  return `static const mjb_codepoint mjb_unicode_special_case_data[] = {
${formatCodepoints(sequences.data)}
};

static const uint32_t mjb_unicode_special_case_mappings[] = {
${formatWords(entries)}
};

static const uint8_t mjb_unicode_special_case_offsets[] = {
${formatCompactIntegers(offsets, 24)}
};
`;
}

// Emits packed case-fold mappings and their shared sequence payload.
export function generateCaseFoldMappings(rows: CaseFoldRow[]) {
  iLog('Case fold mappings');

  const sequences = packCodepointSequences(rows.map((row) => caseSequenceValues(row)));
  const entries = rows.map((row, index) => {
    const sequence = sequences.entries[index];

    if(row.codepoint > 0x1FFFFF || sequence.length > 0x3 || sequence.offset > 0xFF) {
      throw new Error(`Case folding mapping is too large to pack: ${row.codepoint}`);
    }

    return (row.codepoint | (sequence.length << 21) | (sequence.offset << 23)) >>> 0;
  });

  return `static const mjb_codepoint mjb_unicode_case_fold_data[] = {
${formatCodepoints(sequences.data)}
};

static const uint32_t mjb_unicode_case_fold_mappings[] = {
${formatWords(entries)}
};
`;
}

// Emits packed simple (S) case-fold mappings. BMP source codepoints keep the compact two-halfword
// layout; supplementary source codepoints use two 21-bit fields in a 64-bit entry.
export function generateCaseFoldSimpleMappings(rows: CaseFoldSimpleRow[]) {
  iLog('Simple case fold mappings');

  const bmpEntries: number[] = [];
  const supplementaryEntries: bigint[] = [];

  for(const row of rows) {
    if(row.codepoint > 0x1FFFFF || row.mapping > 0x1FFFFF) {
      throw new Error(`Simple case folding mapping is too large to pack: ${row.codepoint}`);
    }

    if(row.codepoint <= 0xFFFF) {
      if(row.mapping > 0xFFFF) {
        throw new Error(`BMP simple case folding mapping is too large to pack: ${row.codepoint}`);
      }

      bmpEntries.push(((row.codepoint << 16) | row.mapping) >>> 0);
    } else {
      supplementaryEntries.push((BigInt(row.codepoint) << 21n) | BigInt(row.mapping));
    }
  }

  return `static const uint32_t mjb_unicode_case_fold_simple_mappings[] = {
${formatWords(bmpEntries)}
};

static const uint64_t mjb_unicode_case_fold_simple_supplementary_mappings[] = {
${formatLongWords(supplementaryEntries, 11)}
};
`;
}
