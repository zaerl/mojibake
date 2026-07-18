/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { iLog } from '../log';
import {
  codepointPageBitsets, codepointPages, formatBytes, formatCodepoints, formatCompactIntegers,
  formatHalfwords, formatLongWords, formatWords, indexedPages,
} from '../utils';
import { ConfusableRow } from './types';

// Emits indexed confusable skeleton mappings with shared skeleton payloads.
export function generateConfusables(rows: ConfusableRow[]) {
  iLog('Confusables');

  const data: number[] = [];
  const dataOffsets = new Map<string, { offset: number; length: number }>();
  const pages = indexedPages(codepointPages(rows));
  const pageBitsets = codepointPageBitsets(rows, pages.pages);
  const skeletons = rows.map((row) => {
    const values: number[] = [];

    for(let i = 0; i + 3 < row.skeleton.length; i += 4) {
      values.push(
        (row.skeleton[i] << 24) |
        (row.skeleton[i + 1] << 16) |
        (row.skeleton[i + 2] << 8) |
        row.skeleton[i + 3]
      );
    }

    return values;
  });

  const uniqueSkeletons = [...new Map(skeletons.map((values) => [values.join(','), values])).values()]
    .sort((a, b) => b.length - a.length);

  // Lengths 1-8 fit directly in three bits. The longest skeleton is stored first at offset zero,
  // allowing that one entry to share the length-8 encoding without widening every mapping.
  const encodeLength = (offset: number, length: number) => {
    if(length >= 1 && length <= 8) {
      return length - 1;
    }

    if(offset === 0) {
      return 7;
    }

    throw new Error(`Confusable mapping length cannot be packed: ${length}`);
  };

  // Finds an existing payload offset for a confusable skeleton.
  const findDataOffset = (values: number[]) => {
    for(let offset = 0; offset <= data.length - values.length; ++offset) {
      let matches = true;

      for(let i = 0; i < values.length; ++i) {
        if(data[offset + i] !== values[i]) {
          matches = false;
          break;
        }
      }

      if(matches) {
        return offset;
      }
    }

    return -1;
  };

  for(const values of uniqueSkeletons) {
    const key = values.join(',');
    let offset = findDataOffset(values);

    if(offset < 0) {
      offset = data.length;
      data.push(...values);
    }

    if(offset > 0x1FFF) {
      throw new Error(`Confusable data offset is too large to pack: ${offset}`);
    }

    if(values.length > 0xFF) {
      throw new Error(`Confusable length is too large to pack: ${values.length}`);
    }

    dataOffsets.set(key, { offset, length: values.length });
  }

  const entries = skeletons.map((values) => {
    const entry = dataOffsets.get(values.join(','));

    if(entry === undefined) {
      throw new Error('Missing confusable skeleton entry');
    }

    return entry.offset | (encodeLength(entry.offset, values.length) << 13);
  });

  return `#define MJB_UNICODE_CONFUSABLE_LONG_LENGTH ${uniqueSkeletons[0]?.length ?? 0}

static const mjb_codepoint mjb_unicode_confusable_data[] = {
${formatCodepoints(data)}
};

static const uint8_t mjb_unicode_confusable_page_index[] = {
${formatBytes(pages.index)}
};

static const uint16_t mjb_unicode_confusable_page_starts[] = {
${formatHalfwords(pages.pages.starts)}
};

static const uint64_t mjb_unicode_confusable_page_bits[] = {
${formatLongWords(pageBitsets.data, 16)}
};

static const uint32_t mjb_unicode_confusable_page_ranks[] = {
${formatWords(pageBitsets.ranks)}
};

static const uint16_t mjb_unicode_confusables[] = {
${formatCompactIntegers(entries, 16)}
};
`;
}
