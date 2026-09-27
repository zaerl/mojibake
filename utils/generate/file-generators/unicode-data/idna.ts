/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { iLog } from '../../log';
import { poolSequences, SequencePool } from '../../sequence-pool';
import { formatCompactIntegers, formatHalfwords, formatLongWords, formatWords } from '../../utils';
import { IdnaMappingRow } from '../types';

// Runtime status for codepoints without an IDNA row. It never reaches callers.
const IDNA_STATUS_NONE = 5;
const IDNA_SUPPLEMENTARY_START = 0x20000;

// Emits start-only IDNA ranges.
export function generateIdnaMappings(rows: IdnaMappingRow[], corePool: SequencePool) {
  iLog('IDNA mappings');

  const mappingIds = new Map<string, number>();
  const mappings: number[][] = [];

  const addMapping = (mapping: number[]) => {
    if(mapping.length === 0) {
      return 0;
    }

    const key = mapping.join(',');
    const existing = mappingIds.get(key);

    if(existing !== undefined) {
      return existing;
    }

    mappings.push(mapping);
    mappingIds.set(key, mappings.length);

    return mappings.length;
  };

  // Fill gaps with a placeholder status and split the range crossing the supplementary boundary.
  const ranges: { start: number; status: number; mappingId: number }[] = [];
  let next = 0;

  for(const row of rows) {
    if(row.start < next || row.end < row.start || row.end > 0x10FFFF) {
      throw new Error(`IDNA ranges overlap or are unsorted: U+${row.start.toString(16)}`);
    }

    if(row.status > 0x7 || row.status === IDNA_STATUS_NONE) {
      throw new Error(`IDNA status cannot be packed: ${row.status}`);
    }

    if(row.start > next) {
      ranges.push({ start: next, status: IDNA_STATUS_NONE, mappingId: 0 });
    }

    const mappingId = addMapping(row.mapping);
    ranges.push({ start: row.start, status: row.status, mappingId });

    if(row.start < IDNA_SUPPLEMENTARY_START && row.end >= IDNA_SUPPLEMENTARY_START) {
      ranges.push({ start: IDNA_SUPPLEMENTARY_START, status: row.status, mappingId });
    }

    next = row.end + 1;
  }

  if(next <= 0x10FFFF) {
    ranges.push({ start: next, status: IDNA_STATUS_NONE, mappingId: 0 });
  }

  if(ranges[0].start !== 0) {
    throw new Error('IDNA ranges must start at U+0000');
  }

  const primary = ranges.filter((range) => range.start < IDNA_SUPPLEMENTARY_START);
  const supplementary = ranges.filter((range) => range.start >= IDNA_SUPPLEMENTARY_START);

  if(supplementary.length === 0 || supplementary[0].start !== IDNA_SUPPLEMENTARY_START) {
    throw new Error('IDNA supplementary ranges must start at U+20000');
  }

  // For each 256-codepoint page below U+20000, the index of the range covering its first
  // codepoint. A lookup only searches between one page's entry and the next one's inclusive.
  const pageFirsts: number[] = [];

  for(let page = 0; page < (IDNA_SUPPLEMENTARY_START >> 8); ++page) {
    const pageStart = page << 8;
    let first = pageFirsts.length === 0 ? 0 : pageFirsts[pageFirsts.length - 1];

    while(first + 1 < primary.length && primary[first + 1].start <= pageStart) {
      ++first;
    }

    pageFirsts.push(first);
  }

  pageFirsts.push(primary.length - 1);

  if(primary.length > 0xFFFF) {
    throw new Error(`IDNA primary ranges are too many to index: ${primary.length}`);
  }

  const primaryEntries = primary.map((range) => {
    if(range.mappingId > 0xFFF) {
      throw new Error(`IDNA mapping id is too large to pack: ${range.mappingId}`);
    }

    return (range.start | (range.status << 17) | (range.mappingId << 20)) >>> 0;
  });

  const supplementaryEntries = supplementary.map((range) => {
    if(range.mappingId > 0x1FFF) {
      throw new Error(`IDNA mapping id is too large to pack: ${range.mappingId}`);
    }

    return BigInt(range.start) |
      (BigInt(range.status) << 21n) |
      (BigInt(range.mappingId) << 24n);
  });

  const pool = new SequencePool(corePool);
  const pooled = poolSequences(pool, mappings);
  const mappingOffsets = pooled.map((entry) => {
    if(entry.offset > 0xFFFF) {
      throw new Error(`IDNA mapping sequence offset is too large to pack: ${entry.offset}`);
    }

    return entry.offset;
  });
  const mappingLengths = pooled.map((entry) => entry.length);

  return `static const uint16_t mjb_unicode_idna_sequence_units[] = {
${formatHalfwords(pool.units.length === 0 ? [0] : pool.units, 12)}
};

static const uint16_t mjb_unicode_idna_mapping_offsets[] = {
${formatCompactIntegers(mappingOffsets, 16)}
};

static const uint8_t mjb_unicode_idna_mapping_lengths[] = {
${formatCompactIntegers(mappingLengths, 32)}
};

static const uint32_t mjb_unicode_idna_ranges[] = {
${formatWords(primaryEntries, 8)}
};

static const uint16_t mjb_unicode_idna_page_firsts[] = {
${formatHalfwords(pageFirsts, 12)}
};

static const uint64_t mjb_unicode_idna_supplementary_ranges[] = {
${formatLongWords(supplementaryEntries)}
};
`;
}
