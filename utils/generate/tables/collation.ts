/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { iLog } from '../log';
import { codepointPages, compareBytes, formatBytes, formatCodepoints, formatHalfwords, formatLongWords, formatPages, formatWords, indexedPages } from '../utils';
import { CollationContractionRow, CollationEntryRow } from './types';

// Finds how many bytes at the end of data overlap the start of bytes.
function byteSuffixPrefixOverlap(data: number[], bytes: number[]) {
  const max = Math.min(data.length, bytes.length - 1);

  outer:
  for(let length = max; length > 0; --length) {
    const start = data.length - length;

    for(let i = 0; i < length; ++i) {
      if(data[start + i] !== bytes[i]) {
        continue outer;
      }
    }

    return length;
  }

  return 0;
}

// Packs byte sequences by reusing duplicate, substring, and suffix-prefix overlaps.
export function packByteSequences(sequences: number[][]) {
  const unique = new Map<string, { bytes: number[]; buffer: Buffer }>();
  const offsets = new Array<number>(sequences.length);
  const data: number[] = [];
  let dataBuffer = Buffer.alloc(0);

  for(const bytes of sequences) {
    const buffer = Buffer.from(bytes);
    const key = buffer.toString('hex');

    if(!unique.has(key)) {
      unique.set(key, { bytes, buffer });
    }
  }

  const packedOffsets = new Map<string, number>();
  const sorted = [...unique.entries()].sort(([, a], [, b]) =>
    b.bytes.length - a.bytes.length || compareBytes(a.bytes, b.bytes)
  );

  for(const [key, sequence] of sorted) {
    let offset = dataBuffer.indexOf(sequence.buffer);

    if(offset < 0) {
      const overlap = byteSuffixPrefixOverlap(data, sequence.bytes);
      offset = data.length - overlap;
      data.push(...sequence.bytes.slice(overlap));
      dataBuffer = Buffer.from(data);
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

// Emits indexed collation entries with shared packed weight data.
export function generateCollationEntries(rows: CollationEntryRow[]) {
  iLog('Collation entries');

  const weightsByRow = rows.map((row) => [...row.weights]);
  const packedWeights = packByteSequences(weightsByRow);
  const pages = indexedPages(codepointPages(rows));
  const lows = rows.map((row) => row.codepoint & 0xFF);
  const entries: number[] = [];

  weightsByRow.forEach((weights, index) => {
    const offset = packedWeights.offsets[index];

    if(offset > 0xFFFFFF) {
      throw new Error(`Collation weight offset is too large to pack: ${offset}`);
    }

    if(weights.length > 0xFF) {
      throw new Error(`Collation weight length is too large to pack: ${weights.length}`);
    }

    const packed = (weights.length << 24) | offset;

    entries.push(packed >>> 0);
  });

  return `static const uint8_t mjb_unicode_collation_weight_data[] = {
${formatBytes(packedWeights.data)}
};

static const uint16_t mjb_unicode_collation_page_index[] = {
${formatHalfwords(pages.index)}
};

static const mjb_unicode_page mjb_unicode_collation_pages[] = {
${formatPages(pages.pages)}
};

static const uint8_t mjb_unicode_collation_lows[] = {
${formatBytes(lows)}
};

static const uint32_t mjb_unicode_collation_entries[] = {
${formatWords(entries)}
};
`;
}

// Emits packed collation contraction entries, sequences, and shared weights.
export function generateCollationContractions(rows: CollationContractionRow[]) {
  iLog('Collation contractions');

  const sequenceData: number[] = [];
  const weightsByRow = rows.map((row) => [...row.weights]);
  const packedWeights = packByteSequences(weightsByRow);
  const entries: bigint[] = [];

  rows.forEach((row, index) => {
    const sequenceOffset = sequenceData.length;
    const sequence = codepointsFromBlob(row.sequence);
    const weights = weightsByRow[index];
    const weightsOffset = packedWeights.offsets[index];

    sequenceData.push(...sequence);

    if(row.first_codepoint > 0x1FFFFF) {
      throw new Error(
        `Collation contraction first codepoint is too large to pack: ${row.first_codepoint}`
      );
    }

    if(sequenceOffset > 0xFFFF) {
      throw new Error(`Collation contraction sequence offset is too large to pack: ${sequenceOffset}`);
    }

    if(weightsOffset > 0xFFFF) {
      throw new Error(`Collation contraction weight offset is too large to pack: ${weightsOffset}`);
    }

    if(sequence.length > 0x7) {
      throw new Error(`Collation contraction sequence length is too large to pack: ${sequence.length}`);
    }

    if(weights.length > 0x3F) {
      throw new Error(`Collation contraction weight length is too large to pack: ${weights.length}`);
    }

    entries.push(BigInt(row.first_codepoint) |
      (BigInt(sequenceOffset) << 21n) |
      (BigInt(weightsOffset) << 37n) |
      (BigInt(sequence.length) << 53n) |
      (BigInt(weights.length) << 56n));
  });

  return `static const mjb_codepoint mjb_unicode_collation_contraction_sequence_data[] = {
${formatCodepoints(sequenceData)}
};

static const uint8_t mjb_unicode_collation_contraction_weight_data[] = {
${formatBytes(packedWeights.data)}
};

static const mjb_unicode_collation_contraction_entry mjb_unicode_collation_contractions[] = {
${formatLongWords(entries)}
};
`;
}
