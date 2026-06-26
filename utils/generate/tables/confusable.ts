/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { iLog } from '../log';
import { codepointPages, formatBytes, formatCodepoints, formatHalfwords, formatPages, formatWords, indexedPages } from '../utils';
import { ConfusableRow } from './types';

// Emits indexed confusable skeleton mappings with shared skeleton payloads.
export function generateConfusables(rows: ConfusableRow[]) {
  iLog('Confusables');

  const data: number[] = [];
  const dataOffsets = new Map<string, { offset: number; length: number }>();
  const pages = indexedPages(codepointPages(rows));
  const lows = rows.map((row) => row.codepoint & 0xFF);
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

    if(offset > 0xFFFFFF) {
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

    return ((entry.length << 24) | entry.offset) >>> 0;
  });

  return `static const mjb_codepoint mjb_unicode_confusable_data[] = {
${formatCodepoints(data)}
};

static const uint16_t mjb_unicode_confusable_page_index[] = {
${formatHalfwords(pages.index)}
};

static const mjb_unicode_page mjb_unicode_confusable_pages[] = {
${formatPages(pages.pages)}
};

static const uint8_t mjb_unicode_confusable_lows[] = {
${formatBytes(lows)}
};

static const uint32_t mjb_unicode_confusables[] = {
${formatWords(entries)}
};
`;
}
