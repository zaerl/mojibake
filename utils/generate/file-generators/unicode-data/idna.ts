/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { iLog } from '../../log';
import { formatCodepoints, formatHalfwords, formatLongWords } from '../../utils';
import { IdnaMappingRow } from '../types';

export function generateIdnaMappings(rows: IdnaMappingRow[]) {
  iLog('IDNA mappings');

  const mappingData: number[] = [];
  const mappingIds = new Map<string, number>();
  const mappingOffsets = [0];

  const addMapping = (mapping: number[]) => {
    if(mapping.length > 0xFF) {
      throw new Error(`IDNA mapping is too long to pack: ${mapping.length}`);
    }

    if(mapping.length === 0) {
      return 0;
    }

    const key = mapping.join(',');
    const existing = mappingIds.get(key);

    if(existing !== undefined) {
      return existing;
    }

    const id = mappingOffsets.length;
    mappingData.push(...mapping);
    mappingOffsets.push(mappingData.length);
    mappingIds.set(key, id);

    return id;
  };

  const entries = rows.map(row => {
    const mappingId = addMapping(row.mapping);
    const delta = row.end - row.start;

    if(row.start > 0x1FFFFF || delta > 0x1FFFFF) {
      throw new Error(`IDNA range is too large to pack: U+${row.start.toString(16)}`);
    }

    if(row.status > 0x7 || mappingId > 0x1FFF) {
      throw new Error(`IDNA entry is too large to pack: U+${row.start.toString(16)}`);
    }

    return BigInt(row.start) |
      (BigInt(delta) << 21n) |
      (BigInt(row.status) << 42n) |
      (BigInt(mappingId) << 45n);
  });

  if(mappingData.length > 0xFFFF) {
    throw new Error(`IDNA mapping data is too large to index: ${mappingData.length}`);
  }

  return `typedef uint64_t mjb_unicode_idna_range;

static const mjb_codepoint mjb_unicode_idna_mapping_data[] = {
${formatCodepoints(mappingData)}
};

static const uint16_t mjb_unicode_idna_mapping_offsets[] = {
${formatHalfwords(mappingOffsets)}
};

static const mjb_unicode_idna_range mjb_unicode_idna_ranges[] = {
${formatLongWords(entries)}
};
`;
}
