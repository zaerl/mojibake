/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { iLog } from '../log';
import {
  codepointPageBitsets, codepointPages, compareBytes, formatBytes, formatCodepoints,
  formatHalfwords, formatLongWords, formatWords, indexedPages
} from '../utils';
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

type ByteSuffixState = {
  length: number;
  link: number;
  transitions: Map<number, number>;
  earliestEnd: number;
};

// Incrementally indexes every substring in a byte stream. Keeping the earliest end position on
// each state makes find() match Buffer.indexOf() semantics without rescanning the packed data.
class ByteSuffixAutomaton {
  private states: ByteSuffixState[] = [{
    length: 0,
    link: -1,
    transitions: new Map(),
    earliestEnd: -1,
  }];
  private last = 0;
  private dataLength = 0;

  find(bytes: number[]) {
    let state = 0;

    for(const byte of bytes) {
      const next = this.states[state].transitions.get(byte);

      if(next === undefined) {
        return -1;
      }

      state = next;
    }

    return this.states[state].earliestEnd - bytes.length + 1;
  }

  append(bytes: number[]) {
    for(const byte of bytes) {
      this.appendByte(byte);
    }
  }

  private appendByte(byte: number) {
    const end = this.dataLength++;
    const current = this.states.length;
    this.states.push({
      length: this.states[this.last].length + 1,
      link: 0,
      transitions: new Map(),
      earliestEnd: end,
    });

    let state = this.last;

    while(state >= 0 && !this.states[state].transitions.has(byte)) {
      this.states[state].transitions.set(byte, current);
      state = this.states[state].link;
    }

    if(state < 0) {
      this.states[current].link = 0;
    } else {
      const next = this.states[state].transitions.get(byte)!;

      if(this.states[state].length + 1 === this.states[next].length) {
        this.states[current].link = next;
      } else {
        const clone = this.states.length;
        this.states.push({
          length: this.states[state].length + 1,
          link: this.states[next].link,
          transitions: new Map(this.states[next].transitions),
          earliestEnd: this.states[next].earliestEnd,
        });

        while(state >= 0 && this.states[state].transitions.get(byte) === next) {
          this.states[state].transitions.set(byte, clone);
          state = this.states[state].link;
        }

        this.states[next].link = clone;
        this.states[current].link = clone;
      }
    }

    this.last = current;
  }
}

// Packs byte sequences by reusing duplicate, substring, and suffix-prefix overlaps.
export function packByteSequences(sequences: number[][]) {
  const unique = new Map<string, number[]>();
  const offsets = new Array<number>(sequences.length);
  const data: number[] = [];
  const substringIndex = new ByteSuffixAutomaton();

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
      const overlap = byteSuffixPrefixOverlap(data, bytes);
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

// Emits indexed collation entries with shared packed weight data.
export function generateCollationEntries(rows: CollationEntryRow[]) {
  iLog('Collation entries');

  const weightsByRow = rows.map((row) => [...row.weights]);
  const packedWeights = packByteSequences(weightsByRow);
  const pages = indexedPages(codepointPages(rows));
  const pageBitsets = codepointPageBitsets(rows, pages.pages);
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

static const uint8_t mjb_unicode_collation_page_index[] = {
${formatBytes(pages.index)}
};

static const uint16_t mjb_unicode_collation_page_starts[] = {
${formatHalfwords(pages.pages.starts)}
};

static const uint64_t mjb_unicode_collation_page_bits[] = {
${formatLongWords(pageBitsets.data, 16)}
};

static const uint32_t mjb_unicode_collation_page_ranks[] = {
${formatWords(pageBitsets.ranks)}
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
    const sequenceTail = sequence.slice(1);
    const weights = weightsByRow[index];
    const weightsOffset = packedWeights.offsets[index];

    if(sequence[0] !== row.first_codepoint) {
      throw new Error(
        `Collation contraction sequence starts with ${sequence[0]}, expected ${row.first_codepoint}`
      );
    }

    sequenceData.push(...sequenceTail);

    if(row.first_codepoint > 0x1FFFFF) {
      throw new Error(
        `Collation contraction first codepoint is too large to pack: ${row.first_codepoint}`
      );
    }

    if(sequenceOffset > 0xFFFF) {
      throw new Error(
        `Collation contraction sequence offset is too large to pack: ${sequenceOffset}`
      );
    }

    if(weightsOffset > 0xFFFF) {
      throw new Error(`Collation contraction weight offset is too large to pack: ${weightsOffset}`);
    }

    if(sequenceTail.length > 0x7) {
      throw new Error(
        `Collation contraction sequence tail is too large to pack: ${sequenceTail.length}`
      );
    }

    if(weights.length > 0x3F) {
      throw new Error(
        `Collation contraction weight length is too large to pack: ${weights.length}`
      );
    }

    entries.push(BigInt(row.first_codepoint) |
      (BigInt(sequenceOffset) << 21n) |
      (BigInt(weightsOffset) << 37n) |
      (BigInt(sequenceTail.length) << 53n) |
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
