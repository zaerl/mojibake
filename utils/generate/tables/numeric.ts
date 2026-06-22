/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { codepointPages, formatBytes, formatHalfwords, formatPages, formatWords, indexedPages } from '../utils';
import { NumericRow } from './types';

// Emits packed numeric value lookups and shared numeric string data.
export function generateNumericValues(rows: NumericRow[]) {
  const data: number[] = [0];
  const stringOffsets = new Map<string, number>([['', 0]]);
  const pages = indexedPages(codepointPages(rows));
  const lows = rows.map((row) => row.codepoint & 0xFF);
  const entries: number[] = [];
  // Encodes nullable decimal and digit fields with zero as the missing sentinel.
  const encodeNumber = (value: number | null) => value === null ? 0 : value + 1;

  for(const row of rows) {
    let numericOffset = 0;

    if(row.numeric !== null) {
      let offset = stringOffsets.get(row.numeric);

      if(offset === undefined) {
        offset = data.length;

        for(let i = 0; i < row.numeric.length; ++i) {
          data.push(row.numeric.charCodeAt(i));
        }

        data.push(0);
        stringOffsets.set(row.numeric, offset);
      }

      numericOffset = offset;
    }

    if(numericOffset > 0x3FF) {
      throw new Error(`Numeric string offset is too large to pack: ${numericOffset}`);
    }

    entries.push(numericOffset | (encodeNumber(row.decimal) << 10) |
      (encodeNumber(row.digit) << 14));
  }

  return `static const uint16_t mjb_unicode_numeric_page_index[] = {
${formatHalfwords(pages.index)}
};

static const mjb_unicode_page mjb_unicode_numeric_pages[] = {
${formatPages(pages.pages)}
};

static const char mjb_unicode_numeric_data[] = {
${formatBytes(data)}
};

static const uint8_t mjb_unicode_numeric_lows[] = {
${formatBytes(lows)}
};

static const uint32_t mjb_unicode_numeric_values[] = {
${formatWords(entries)}
};
`;
}
