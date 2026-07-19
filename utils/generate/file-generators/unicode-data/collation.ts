/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { iLog } from '../../log';
import {
  codepointPageBitsets, codepointPages, compareBytes, formatBytes, formatCodepoints,
  formatCompactIntegers, formatHalfwords, formatLongWords, formatWords, indexedPages,
  packCodepointSequences,
} from '../../utils';
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

  const packedWeights = packCodepointSequences(weightsByRow);
  const pages = indexedPages(codepointPages(rows));
  const pageBitsets = codepointPageBitsets(rows, pages.pages);
  const entryOffsets: number[] = [];

  weightsByRow.forEach((weights, index) => {
    const offset = packedWeights.entries[index].offset;

    if(offset > 0xFFFF) {
      throw new Error(`Collation weight offset is too large to pack: ${offset}`);
    }

    entryOffsets.push(offset);
  });

  return `static const uint32_t mjb_unicode_collation_weight_data[] = {
${formatWords(packedWeights.data)}
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

static const uint16_t mjb_unicode_collation_entry_offsets[] = {
${formatCompactIntegers(entryOffsets, 16)}
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

  return `static const mjb_codepoint mjb_unicode_collation_contraction_first_codepoints[] = {
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
