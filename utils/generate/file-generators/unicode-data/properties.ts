/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { iLog } from '../../log';
import { formatBytes, formatHalfwords, formatWords } from '../../utils';
import { PropertyRangeRow } from '../types';

// Emits interned property blobs and page-local property ranges.
export function generateProperties(rows: PropertyRangeRow[]) {
  iLog('Properties');

  const data: number[] = [];
  const dataOffsets = new Map<string, number>();
  const pageEntries = new Map<number, number[]>();

  for(const row of rows) {
    const start = row.start_codepoint;
    const end = row.end_codepoint ?? row.start_codepoint;
    const bytes = [...row.properties];
    const key = row.properties.toString('hex');
    let offset = dataOffsets.get(key);

    if(offset === undefined) {
      offset = data.length;
      data.push(bytes.length);
      data.push(...bytes);
      dataOffsets.set(key, offset);
    }

    if(offset > 0xFFFF) {
      throw new Error(`Property data offset is too large to pack: ${offset}`);
    }

    if(bytes.length > 0x1F) {
      throw new Error(`Property data length is too large to pack: ${bytes.length}`);
    }

    const startPage = start >> 8;
    const endPage = end >> 8;

    for(let page = startPage; page <= endPage; ++page) {
      const startLow = page === startPage ? start & 0xFF : 0;
      const endLow = page === endPage ? end & 0xFF : 0xFF;
      const delta = endLow - startLow;
      let entries = pageEntries.get(page);

      if(entries === undefined) {
        entries = [];
        pageEntries.set(page, entries);
      }

      entries.push(startLow + (delta * 0x100) + (offset * 0x10000));
    }
  }

  const pageNumbers = [...pageEntries.keys()].sort((a, b) => a - b);
  const pageStarts: number[] = [];
  const pageCounts: number[] = [];
  const entries: number[] = [];

  for(const page of pageNumbers) {
    const pageRangeEntries = pageEntries.get(page) ?? [];

    if(page > 0xFFFF) {
      throw new Error(`Property page is too large to pack: ${page}`);
    }

    if(entries.length > 0xFFFF) {
      throw new Error(`Property page start is too large to pack: ${entries.length}`);
    }

    if(pageRangeEntries.length > 0xFFFF) {
      throw new Error(`Property page count is too large to pack: ${pageRangeEntries.length}`);
    }

    pageStarts.push(entries.length);
    pageCounts.push(pageRangeEntries.length);
    entries.push(...pageRangeEntries);
  }

  return `typedef uint32_t mjb_unicode_property_range;

static const uint8_t mjb_unicode_property_data[] = {
${formatBytes(data, 24)}
};

static const uint16_t mjb_unicode_property_page_numbers[] = {
${formatHalfwords(pageNumbers, 16)}
};

static const uint16_t mjb_unicode_property_page_starts[] = {
${formatHalfwords(pageStarts, 16)}
};

static const uint16_t mjb_unicode_property_page_counts[] = {
${formatHalfwords(pageCounts, 16)}
};

static const mjb_unicode_property_range mjb_unicode_property_ranges[] = {
${formatWords(entries, 12)}
};
`;
}
