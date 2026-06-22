/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { addStringData, codepointPages, formatBytes, formatHalfwords, formatPages, formatWords, indexedPages } from '../utils';
import { NameRow, PrefixRow } from './types';

// Emits packed character name and prefix lookup tables.
export function generateNames(prefixes: PrefixRow[], rows: NameRow[]) {
  const prefixData: number[] = [0];
  const nameData: number[] = [0];
  const prefixOffsets = new Map<string, number>([['', 0]]);
  const nameOffsets = new Map<string, number>([['', 0]]);
  const pages = indexedPages(codepointPages(rows));

  const prefixEntries = prefixes.map((row) => {
    const offset = addStringData(prefixData, prefixOffsets, row.name);

    if(row.id > 0xFFFF || offset > 0xFFFF) {
      throw new Error(`Prefix entry out of bounds: id=${row.id}, offset=${offset}`);
    }

    return ((offset << 16) | row.id) >>> 0;
  });

  const nameLows = rows.map((row) => row.codepoint & 0xFF);
  const nameEntries = rows.map((row) => {
    const offset = addStringData(nameData, nameOffsets, row.name ?? '');

    if(offset >= (1 << 20)) {
      throw new Error(`Name data offset is too large to pack: ${offset}`);
    }

    const data = ((row.prefix ?? 0) << 20) | offset;

    return data;
  });

  return `typedef struct mjb_unicode_page {
    uint16_t start;
    uint16_t count;
} mjb_unicode_page;

typedef uint32_t mjb_unicode_prefix_entry;

static const char mjb_unicode_prefix_data[] = {
${formatBytes(prefixData)}
};

static const mjb_unicode_prefix_entry mjb_unicode_prefixes[] = {
${formatWords(prefixEntries)}
};

static const char mjb_unicode_name_data[] = {
${formatBytes(nameData)}
};

static const uint16_t mjb_unicode_name_page_index[] = {
${formatHalfwords(pages.index)}
};

static const mjb_unicode_page mjb_unicode_name_pages[] = {
${formatPages(pages.pages)}
};

static const uint8_t mjb_unicode_name_lows[] = {
${formatBytes(nameLows)}
};

static const uint32_t mjb_unicode_name_entries[] = {
${formatWords(nameEntries)}
};
`;
}
