/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { iLog } from '../../log';
import {
  codepointPageBitsets, codepointPages, formatBytes, formatCompactIntegers, formatLongWords,
  formatWords, indexedPages,
} from '../../utils';
import { NameRow, PrefixRow } from '../types';

// Packs 7-bit strings with the high bit marking the final byte. A string can share the suffix of a
// longer string, but each stored root no longer needs a trailing NUL byte.
function packStringData(values: string[]) {
  // Offset zero represents the empty string.
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
      const code = value.charCodeAt(i);

      if(code >= 0x80) {
        // Impossible.
        throw new Error(`Name data is not 7-bit: ${value}`);
      }

      data.push(code | (i + 1 === value.length ? 0x80 : 0));
    }

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
  const pageBitsets = codepointPageBitsets(rows, pages.pages);

  const prefixOffsets = [0, ...prefixes.map((row, index) => {
    const offset = packedPrefixes.offsets.get(row.name);

    if(offset === undefined) {
      throw new Error(`Packed prefix is missing: ${row.name}`);
    }

    if(row.id !== index + 1 || offset > 0xFFFF) {
      throw new Error(`Prefix entry out of bounds: id=${row.id}, offset=${offset}`);
    }

    return offset;
  })];

  const pagePrefixStarts: number[] = [];
  const pagePrefixOffsets: number[] = [];
  const nameTags: number[] = [];
  const nameOffsets: number[] = [];

  pages.pages.starts.forEach((start, page) => {
    const prefixIds = new Map<number, number>();
    const end = start + pages.pages.counts[page];

    pagePrefixStarts.push(pagePrefixOffsets.length);

    for(let index = start; index < end; ++index) {
      const row = rows[index];
      const name = row.name ?? '';
      const prefix = row.prefix ?? 0;
      const offset = packedNames.offsets.get(name);
      const prefixName = prefix === 0 ? '' : prefixes[prefix - 1]?.name;

      if(prefixName === undefined) {
        throw new Error(`Unknown name prefix ID: ${prefix}`);
      }

      if(name.length + prefixName.length >= 128) {
        throw new Error(`Character name is too long for the runtime buffer: ${name}`);
      }

      if(offset === undefined) {
        throw new Error(`Packed name is missing: ${name}`);
      }

      if(offset >= (1 << 17)) {
        throw new Error(`Name data offset is too large to pack: ${offset} ` +
          `(data=${packedNames.data.length}, prefixes=${prefixes.length})`);
      }

      let localPrefix = prefixIds.get(prefix);

      if(localPrefix === undefined) {
        localPrefix = prefixIds.size;

        if(localPrefix >= (1 << 7)) {
          throw new Error(`Page ${page} has too many name prefixes: ${localPrefix + 1}`);
        }

        const prefixValue = prefixOffsets[prefix];

        if(prefixValue === undefined) {
          throw new Error(`Unknown name prefix ID: ${prefix}`);
        }

        prefixIds.set(prefix, localPrefix);
        pagePrefixOffsets.push(prefixValue);
      }

      nameTags.push((localPrefix << 1) | (offset >> 16));
      nameOffsets.push(offset & 0xFFFF);
    }
  });

  if(pagePrefixOffsets.length > 0xFFFF) {
    throw new Error(`Name page-prefix table is too large: ${pagePrefixOffsets.length}`);
  }

  const pageStarts = pages.pages.starts.map((start, page) => {
    const prefixStart = pagePrefixStarts[page];

    if(start > 0xFFFF || prefixStart > 0xFFFF) {
      throw new Error(`Name page starts are too large: names=${start}, prefixes=${prefixStart}`);
    }

    return ((prefixStart << 16) | start) >>> 0;
  });

  return `typedef struct mjb_unicode_page {
    uint16_t start;
    uint16_t count;
} mjb_unicode_page;

#if MJB_FEATURE_CHARACTER_NAMES
static const uint8_t mjb_unicode_prefix_data[] = {
${formatBytes(prefixData)}
};

static const uint16_t mjb_unicode_name_page_prefix_offsets[] = {
${formatCompactIntegers(pagePrefixOffsets, 16)}
};

static const uint8_t mjb_unicode_name_data[] = {
${formatBytes(nameData)}
};

static const uint8_t mjb_unicode_name_page_index[] = {
${formatBytes(pages.index)}
};

static const uint32_t mjb_unicode_name_page_starts[] = {
${formatWords(pageStarts)}
};

static const uint64_t mjb_unicode_name_page_bits[] = {
${formatLongWords(pageBitsets.data, 16)}
};

static const uint32_t mjb_unicode_name_page_ranks[] = {
${formatWords(pageBitsets.ranks)}
};

static const uint8_t mjb_unicode_name_tags[] = {
${formatCompactIntegers(nameTags, 24)}
};

static const uint16_t mjb_unicode_name_offsets[] = {
${formatCompactIntegers(nameOffsets, 16)}
};
#endif
`;
}
