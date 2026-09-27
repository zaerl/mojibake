/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { iLog } from '../../log';
import {
  codepointPageBitsets, codepointPages, compareBytes, formatBitsetPages, formatBytes,
  formatCodepoints, formatCompactIntegers, formatHalfwords, formatLongWords, formatWords,
  indexedPages, packCodepointSequences,
} from '../../utils';
import { SuffixAutomaton, suffixPrefixOverlap } from '../../sequence-pool';
import { CollationContractionRow, CollationEntryRow, CollationImplicitRangeRow } from '../types';

export function generateCollationImplicitRanges(rows: CollationImplicitRangeRow[]) {
  iLog('Collation implicit ranges');

  if(rows.length === 0) {
    throw new Error('Missing collation implicit ranges');
  }

  const values = rows.map((row) =>
    `    { 0x${row.start.toString(16).toUpperCase()}, 0x${row.end.toString(16).toUpperCase()}, ` +
    `0x${row.offset.toString(16).toUpperCase()}, 0x${row.base.toString(16).toUpperCase()} }`
  );

  return `typedef struct mjb_unicode_collation_implicit_range {
    uint32_t start;
    uint32_t end;
    uint32_t offset;
    uint16_t base;
} mjb_unicode_collation_implicit_range;

static const mjb_unicode_collation_implicit_range mjb_unicode_collation_implicit_ranges[] = {
${values.join(',\n')}
};

#define MJB_UNICODE_COLLATION_IMPLICIT_RANGE_COUNT ${rows.length}
`;
}

// Packs byte sequences by reusing duplicate, substring, and suffix-prefix overlaps.
export function packByteSequences(sequences: number[][]) {
  const unique = new Map<string, number[]>();
  const offsets = new Array<number>(sequences.length);
  const data: number[] = [];
  const substringIndex = new SuffixAutomaton();

  for(const bytes of sequences) {
    const buffer = Buffer.from(bytes);
    const key = buffer.toString('hex');

    if(!unique.has(key)) {
      unique.set(key, bytes);
    }
  }

  const packedOffsets = new Map<string, number>();
  const sorted = [...unique.entries()].sort(([, a], [, b]) =>
    b.length - a.length || compareBytes(a, b)
  );

  for(const [key, bytes] of sorted) {
    let offset = substringIndex.find(bytes);

    if(offset < 0) {
      const overlap = suffixPrefixOverlap(data, bytes);
      offset = data.length - overlap;
      const appended = bytes.slice(overlap);
      data.push(...appended);
      substringIndex.append(appended);
    }

    packedOffsets.set(key, offset);
  }

  sequences.forEach((bytes, index) => {
    const key = Buffer.from(bytes).toString('hex');
    const offset = packedOffsets.get(key);

    if(offset === undefined) {
      throw new Error(`Packed byte sequence is missing: ${key}`);
    }

    offsets[index] = offset;
  });

  return { data, offsets };
}

// Decodes a big-endian codepoint blob into numeric codepoints.
function codepointsFromBlob(blob: Buffer) {
  const values: number[] = [];

  for(let i = 0; i + 3 < blob.length; i += 4) {
    values.push(
      (blob[i] << 24) |
      (blob[i + 1] << 16) |
      (blob[i + 2] << 8) |
      blob[i + 3]
    );
  }

  return values;
}

// Emits three-byte first collation weights plus a sparse, bitset-ranked expansion index.
export function generateCollationEntries(rows: CollationEntryRow[]) {
  iLog('Collation entries');

  const weightsByRow = rows.map((row) => {
    const bytes = [...row.weights];
    const elements: number[] = [];

    if(bytes.length === 0 || bytes.length % 4 !== 0) {
      throw new Error(`Invalid collation weight length: ${bytes.length}`);
    }

    for(let offset = 0; offset < bytes.length; offset += 4) {
      let element = (bytes[offset] | (bytes[offset + 1] << 8) | (bytes[offset + 2] << 16) |
        (bytes[offset + 3] << 24)) >>> 0;

      // Bit 31 is unused by DUCET. Marking the last element makes sequence length implicit while
      // keeping each element directly loadable as one aligned uint32_t at runtime.
      if((element & 0x80000000) !== 0) {
        throw new Error(`Collation element uses the reserved final marker bit: ${element}`);
      }

      if(offset + 4 === bytes.length) {
        element = (element | 0x80000000) >>> 0;
      }

      elements.push(element);
    }

    return elements;
  });

  const pages = indexedPages(codepointPages(rows));
  const pageBitsets = codepointPageBitsets(rows, pages.pages);
  const primaries = weightsByRow.map((weights) => weights[0] & 0xFFFF);
  const combos = weightsByRow.map((weights) => (weights[0] >>> 16) & 0x7FFF);
  const comboCounts = new Map<number, number>();

  for(const combo of combos) {
    comboCounts.set(combo, (comboCounts.get(combo) ?? 0) + 1);
  }

  const comboEscape = 0x7F;
  const comboExpansionFlag = 0x80;
  const comboValues = [...comboCounts.entries()]
    .sort(([a, countA], [b, countB]) => countB - countA || a - b)
    .slice(0, comboEscape)
    .map(([combo]) => combo);
  const comboCodes = new Map(comboValues.map((combo, code) => [combo, code]));
  const comboExceptionIndices: number[] = [];
  const comboExceptionValues: number[] = [];
  const entryBytes: number[] = [];

  combos.forEach((combo, index) => {
    let code = comboCodes.get(combo);

    if(code === undefined) {
      if(index > 0xFFFF) {
        throw new Error(`Collation combination exception index is too large: ${index}`);
      }

      comboExceptionIndices.push(index);
      comboExceptionValues.push(combo);
      code = comboEscape;
    }

    if(weightsByRow[index].length > 1) {
      code |= comboExpansionFlag;
    }

    entryBytes.push(primaries[index] & 0xFF, primaries[index] >> 8, code);
  });

  const expansions = weightsByRow
    .map((weights, index) => ({ index, weights }))
    .filter((entry) => entry.weights.length > 1);
  const packedExpansions = packCodepointSequences(expansions.map((entry) => entry.weights.slice(1)));
  const expansionWordCount = Math.ceil(rows.length / 64);
  const expansionBits = new Array<bigint>(expansionWordCount).fill(0n);
  const expansionWordCounts = new Array<number>(expansionWordCount).fill(0);
  const expansionOffsets: number[] = [];

  expansions.forEach((entry, index) => {
    const offset = packedExpansions.entries[index].offset;

    if(offset > 0xFFFF) {
      throw new Error(`Collation expansion offset is too large: ${offset}`);
    }

    expansionBits[entry.index >> 6] |= 1n << BigInt(entry.index & 0x3F);
    ++expansionWordCounts[entry.index >> 6];
    expansionOffsets.push(offset);
  });

  const expansionRanks: number[] = [];
  let expansionRank = 0;

  for(let word = 0; word < expansionWordCount; ++word) {
    if(expansionRank > 0xFFFF) {
      throw new Error(`Collation expansion rank is too large: ${expansionRank}`);
    }

    expansionRanks.push(expansionRank);
    expansionRank += expansionWordCounts[word];
  }

  // Exception tables are never empty in practice; a placeholder keeps the arrays well-formed.
  const emittedExceptionIndices = comboExceptionIndices.length === 0 ? [0] : comboExceptionIndices;
  const emittedExceptionValues = comboExceptionValues.length === 0 ? [0] : comboExceptionValues;

  return `enum {
    MJB_UNICODE_COLLATION_COMBO_ESCAPE = ${comboEscape},
    MJB_UNICODE_COLLATION_COMBO_EXPANSION_FLAG = ${comboExpansionFlag},
    MJB_UNICODE_COLLATION_COMBO_EXCEPTION_COUNT = ${comboExceptionIndices.length}
};

static const uint8_t mjb_unicode_collation_entries[] = {
${formatCompactIntegers(entryBytes, 36)}
};

static const uint16_t mjb_unicode_collation_combo_values[] = {
${formatHalfwords(comboValues, 12)}
};

static const uint16_t mjb_unicode_collation_combo_exception_indices[] = {
${formatCompactIntegers(emittedExceptionIndices, 16)}
};

static const uint16_t mjb_unicode_collation_combo_exception_values[] = {
${formatHalfwords(emittedExceptionValues, 12)}
};

static const uint8_t mjb_unicode_collation_page_index[] = {
${formatBytes(pages.index)}
};

static const mjb_unicode_bitset_page mjb_unicode_collation_pages[] = {
${formatBitsetPages(pages.pages, pageBitsets)}
};

static const uint64_t mjb_unicode_collation_expansion_bits[] = {
${formatLongWords(expansionBits, 16)}
};

static const uint16_t mjb_unicode_collation_expansion_ranks[] = {
${formatCompactIntegers(expansionRanks, 16)}
};

static const uint16_t mjb_unicode_collation_expansion_offsets[] = {
${formatCompactIntegers(expansionOffsets, 16)}
};

static const uint32_t mjb_unicode_collation_expansion_weights[] = {
${formatWords(packedExpansions.data)}
};
`;
}

// Emits packed collation contraction entries, sequences, and shared weights.
export function generateCollationContractions(rows: CollationContractionRow[]) {
  iLog('Collation contractions');

  const sequences = rows.map((row) => codepointsFromBlob(row.sequence));
  const sequenceTails = sequences.map((sequence) => sequence.slice(1));
  const packedSequences = packCodepointSequences(sequenceTails);
  const weightsByRow = rows.map((row) => [...row.weights]);
  const packedWeights = packByteSequences(weightsByRow);
  const weightLengths = [...new Set(weightsByRow.map((weights) => weights.length))]
    .sort((a, b) => a - b);
  const entries: number[] = [];
  const firstCodepoints: number[] = [];
  const ranges: number[] = [];

  if(weightLengths.length > 4) {
    throw new Error(`Collation contractions have too many weight lengths: ${weightLengths.length}`);
  }

  for(let start = 0; start < rows.length;) {
    const firstCodepoint = rows[start].first_codepoint;
    let end = start + 1;

    while(end < rows.length && rows[end].first_codepoint === firstCodepoint) {
      ++end;
    }

    const count = end - start;

    if(firstCodepoint > 0x1FFFFF || start >= (1 << 10) || count > (1 << 6)) {
      throw new Error(
        `Collation contraction range is too large to pack: ` +
        `first=${firstCodepoint}, start=${start}, count=${count}`
      );
    }

    firstCodepoints.push(firstCodepoint);
    ranges.push(start | ((count - 1) << 10));
    start = end;
  }

  // Quick rejection bitsets: one bit per codepoint below U+1000 and one bit per 256-codepoint
  // page above it, so text without contraction starters skips the first-codepoint search.
  const lowLimit = 0x1000;
  const startLowBits = new Array<bigint>(lowLimit / 64).fill(0n);
  const startPageBits = new Array<bigint>(Math.ceil((0x110000 >> 8) / 64)).fill(0n);

  for(const firstCodepoint of firstCodepoints) {
    if(firstCodepoint < lowLimit) {
      startLowBits[firstCodepoint >> 6] |= 1n << BigInt(firstCodepoint & 0x3F);
    } else {
      const page = firstCodepoint >> 8;
      startPageBits[page >> 6] |= 1n << BigInt(page & 0x3F);
    }
  }

  rows.forEach((row, index) => {
    const sequence = sequences[index];
    const packedSequence = packedSequences.entries[index];
    const weights = weightsByRow[index];
    const weightsOffset = packedWeights.offsets[index];
    const weightLength = weightLengths.indexOf(weights.length);

    if(sequence[0] !== row.first_codepoint) {
      throw new Error(
        `Collation contraction sequence starts with ${sequence[0]}, expected ${row.first_codepoint}`
      );
    }

    if(packedSequence.offset > 0xFF) {
      throw new Error(
        `Collation contraction sequence offset is too large to pack: ${packedSequence.offset}`
      );
    }

    if(weightsOffset > 0x1FFF) {
      throw new Error(`Collation contraction weight offset is too large to pack: ${weightsOffset}`);
    }

    if(packedSequence.length < 1 || packedSequence.length > 2) {
      throw new Error(
        `Collation contraction sequence tail is too large to pack: ${packedSequence.length}`
      );
    }

    if(weightLength < 0 || weightLength > 0x3) {
      throw new Error(
        `Unknown collation contraction weight length: ${weights.length}`
      );
    }

    entries.push(packedSequence.offset |
      (weightsOffset << 8) |
      ((packedSequence.length - 1) << 21) |
      (weightLength << 22));
  });

  return `enum { MJB_UNICODE_COLLATION_CONTRACTION_LOW_LIMIT = ${lowLimit} };

static const uint64_t mjb_unicode_collation_contraction_start_low_bits[] = {
${formatLongWords(startLowBits, 16)}
};

static const uint64_t mjb_unicode_collation_contraction_start_page_bits[] = {
${formatLongWords(startPageBits, 16)}
};

static const mjb_codepoint mjb_unicode_collation_contraction_first_codepoints[] = {
${formatCodepoints(firstCodepoints)}
};

static const uint16_t mjb_unicode_collation_contraction_ranges[] = {
${formatHalfwords(ranges)}
};

static const mjb_codepoint mjb_unicode_collation_contraction_sequence_data[] = {
${formatCodepoints(packedSequences.data)}
};

static const uint8_t mjb_unicode_collation_contraction_weight_data[] = {
${formatBytes(packedWeights.data)}
};

static const uint8_t mjb_unicode_collation_contraction_weight_lengths[] = {
${formatBytes(weightLengths)}
};

static const mjb_unicode_collation_contraction_entry mjb_unicode_collation_contractions[] = {
${formatWords(entries)}
};
`;
}
