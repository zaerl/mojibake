/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { iLog } from '../../log';
import { poolSequences, SequencePool } from '../../sequence-pool';
import {
  codepointPageBitsets, codepointPages, formatBitsetPages, formatCompactIntegers, formatHalfwords,
  indexedPages,
} from '../../utils';
import { ConfusableRow } from '../types';

// Emits indexed confusable skeleton mappings. Skeletons live in an extension of the core
// sequence pool, so single-codepoint and decomposition-like skeletons reuse existing units.
export function generateConfusables(rows: ConfusableRow[], corePool: SequencePool) {
  iLog('Confusables');

  const pages = indexedPages(codepointPages(rows), true);
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

  const pool = new SequencePool(corePool);
  const pooled = poolSequences(pool, skeletons);
  const entries: number[] = [];
  const lengthNibbles: number[] = [];
  const exceptionIndices: number[] = [];
  const exceptionLengths: number[] = [];

  pooled.forEach((entry, index) => {
    if(entry.offset > 0xFFFF) {
      throw new Error(`Confusable sequence offset is too large to pack: ${entry.offset}`);
    }

    entries.push(entry.offset);

    // Lengths 1..14 fit in a nibble; 15 escapes to a tiny exception table.
    let nibble = entry.length;

    if(entry.length >= 0xF) {
      if(index > 0xFFFF || entry.length > 0xFF) {
        throw new Error(`Confusable length exception is too large to pack: ${index}`);
      }

      exceptionIndices.push(index);
      exceptionLengths.push(entry.length);
      nibble = 0xF;
    }

    if((index & 1) === 0) {
      lengthNibbles.push(nibble);
    } else {
      lengthNibbles[lengthNibbles.length - 1] |= nibble << 4;
    }
  });

  return `enum { MJB_UNICODE_CONFUSABLE_EXCEPTION_COUNT = ${exceptionIndices.length} };

static const uint16_t mjb_unicode_confusable_sequence_units[] = {
${formatHalfwords(pool.units.length === 0 ? [0] : pool.units, 12)}
};

static const uint16_t mjb_unicode_confusable_page_index[] = {
${formatHalfwords(pages.index)}
};

static const mjb_unicode_bitset_page mjb_unicode_confusable_pages[] = {
${formatBitsetPages(pages.pages, pageBitsets)}
};

static const uint16_t mjb_unicode_confusables[] = {
${formatCompactIntegers(entries, 16)}
};

static const uint8_t mjb_unicode_confusable_lengths[] = {
${formatCompactIntegers(lengthNibbles, 32)}
};

static const uint16_t mjb_unicode_confusable_exception_indices[] = {
${formatCompactIntegers(exceptionIndices.length === 0 ? [0] : exceptionIndices, 16)}
};

static const uint8_t mjb_unicode_confusable_exception_lengths[] = {
${formatCompactIntegers(exceptionLengths.length === 0 ? [0] : exceptionLengths, 16)}
};
`;
}
