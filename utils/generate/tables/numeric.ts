/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { iLog } from '../log';
import {
  codepointPages, formatBytes, formatCompactIntegers, formatPages, formatWords, indexedPages,
} from '../utils';
import { NumericRow } from './types';

// Emits packed numeric value lookups and shared numeric string data.
export function generateNumericValues(rows: NumericRow[]) {
  iLog('Numeric values');

  const data: number[] = [0];
  const stringOffsets = new Map<string, number>([['', 0]]);
  const pages = indexedPages(codepointPages(rows));
  const lows = rows.map((row) => row.codepoint & 0xFF);
  const valueData: number[] = [];
  const valueOffsets = new Map<number, number>();
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

    const packed = numericOffset | (encodeNumber(row.decimal) << 10) |
      (encodeNumber(row.digit) << 14);
    let valueOffset = valueOffsets.get(packed);

    if(valueOffset === undefined) {
      valueOffset = valueData.length;
      valueData.push(packed);
      valueOffsets.set(packed, valueOffset);
    }

    if(valueOffset > 0xFF) {
      throw new Error(`Numeric value dictionary is too large to pack: ${valueOffset}`);
    }

    entries.push(valueOffset);
  }

  return `static const uint8_t mjb_unicode_numeric_page_index[] = {
${formatBytes(pages.index)}
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

static const uint32_t mjb_unicode_numeric_value_data[] = {
${formatWords(valueData)}
};

static const uint8_t mjb_unicode_numeric_values[] = {
${formatCompactIntegers(entries, 24)}
};
`;
}
