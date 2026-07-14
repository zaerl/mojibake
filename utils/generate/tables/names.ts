/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { iLog } from '../log';
import { codepointPages, formatBytes, formatHalfwords, formatPages, formatWords, indexedPages } from '../utils';
import { NameRow, PrefixRow } from './types';

// Packs NUL-terminated strings while allowing a string to share the suffix of a longer string.
function packStringData(values: string[]) {
  const data: number[] = [0];
  const offsets = new Map<string, number>([['', 0]]);
  const unique = [...new Set(values)];
  const roots = new Set<string>();
  const coveredSuffixes = new Set<string>(['']);
  const longestFirst = [...unique].sort((a, b) =>
    b.length - a.length || (a < b ? -1 : a > b ? 1 : 0)
  );

  for(const value of longestFirst) {
    if(coveredSuffixes.has(value)) {
      continue;
    }

    roots.add(value);

    for(let i = 0; i < value.length; ++i) {
      coveredSuffixes.add(value.slice(i));
    }
  }

  for(const value of unique) {
    if(!roots.has(value)) {
      continue;
    }

    const offset = data.length;

    for(let i = 0; i < value.length; ++i) {
      data.push(value.charCodeAt(i));
    }

    data.push(0);

    for(let i = 0; i < value.length; ++i) {
      const suffix = value.slice(i);

      if(!offsets.has(suffix)) {
        offsets.set(suffix, offset + i);
      }
    }
  }

  return { data, offsets };
}

// Emits packed character name and prefix lookup tables.
export function generateNames(prefixes: PrefixRow[], rows: NameRow[]) {
  iLog('Names');

  const packedPrefixes = packStringData(prefixes.map((row) => row.name));
  const packedNames = packStringData(rows.map((row) => row.name ?? ''));
  const prefixData = packedPrefixes.data;
  const nameData = packedNames.data;
  const pages = indexedPages(codepointPages(rows));

  const prefixEntries = prefixes.map((row) => {
    const offset = packedPrefixes.offsets.get(row.name);

    if(offset === undefined) {
      throw new Error(`Packed prefix is missing: ${row.name}`);
    }

    if(row.id > 0xFFFF || offset > 0xFFFF) {
      throw new Error(`Prefix entry out of bounds: id=${row.id}, offset=${offset}`);
    }

    return ((offset << 16) | row.id) >>> 0;
  });

  const nameLows = rows.map((row) => row.codepoint & 0xFF);
  const nameEntries = rows.map((row) => {
    const name = row.name ?? '';
    const offset = packedNames.offsets.get(name);

    if(offset === undefined) {
      throw new Error(`Packed name is missing: ${name}`);
    }

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

#if MJB_FEATURE_CHARACTER_NAMES
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
#endif
`;
}
