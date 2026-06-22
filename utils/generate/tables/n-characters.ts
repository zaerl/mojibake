/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { bitset, formatBytes, formatHalfwords, formatPages, formatWords, indexedPages } from '../utils';
import { NCharacterRow } from './types';

// Builds per-256-codepoint page start/count metadata for codepoint ranges.
function codepointRangePages(rows: { start: number; end: number }[]) {
  const pageCount = rows.length === 0 ? 0 : (rows[rows.length - 1].end >> 8) + 1;
  const starts: number[] = [];
  const counts: number[] = [];
  let index = 0;

  for(let page = 0; page < pageCount; ++page) {
    const pageStart = page << 8;
    const pageEnd = pageStart + 0xFF;

    while(index < rows.length && rows[index].end < pageStart) {
      ++index;
    }

    let count = 0;

    while(index + count < rows.length && rows[index + count].start <= pageEnd) {
      ++count;
    }

    starts.push(index);
    counts.push(count);
  }

  return { starts, counts };
}

// Emits compressed general character metadata runs and mirrored bitsets.
export function generateNCharacters(rows: NCharacterRow[]) {
  const runs: Array<{
    start: number;
    end: number;
    category: number;
    combining: number;
    bidirectional: number;
    decomposition: number;
    quickCheck: number;
    mirrored: boolean;
  }> = [];

  // Checks whether a character row can extend the current metadata run.
  const sameRun = (run: (typeof runs)[number], row: NCharacterRow) =>
    run.end + 1 === row.codepoint &&
    run.category === row.category &&
    run.combining === (row.combining ?? 0) &&
    run.bidirectional === row.bidirectional &&
    run.decomposition === (row.decomposition ?? 0) &&
    run.quickCheck === (row.quick_check ?? 0) &&
    run.mirrored === (row.mirrored !== 0);

  // Starts a new metadata run from a character row.
  const pushRun = (row: NCharacterRow) => {
    runs.push({
      start: row.codepoint,
      end: row.codepoint,
      category: row.category,
      combining: row.combining ?? 0,
      bidirectional: row.bidirectional,
      decomposition: row.decomposition ?? 0,
      quickCheck: row.quick_check ?? 0,
      mirrored: row.mirrored !== 0,
    });
  };

  for(const row of rows) {
    const current = runs[runs.length - 1];

    if(current !== undefined && sameRun(current, row) && row.codepoint - current.start <= 0x7FF) {
      current.end = row.codepoint;
    } else {
      pushRun(row);
    }
  }

  const pages = indexedPages(codepointRangePages(runs));
  const ranges = runs.map((run) => {
    const delta = run.end - run.start;

    if(delta > 0x7FF) {
      throw new Error(`N-character run is too large to pack: ${run.start}-${run.end}`);
    }

    return ((delta << 21) | run.start) >>> 0;
  });
  const mirrored = bitset(runs.map((run) => run.mirrored));
  const entries = runs.map((run) => {
    const packed = run.quickCheck |
      (run.category << 9) |
      (run.combining << 14) |
      (run.bidirectional << 22) |
      (run.decomposition << 27);

    return packed >>> 0;
  });

  return `static const uint16_t mjb_unicode_n_character_page_index[] = {
${formatHalfwords(pages.index)}
};

static const mjb_unicode_page mjb_unicode_n_character_pages[] = {
${formatPages(pages.pages)}
};

static const uint32_t mjb_unicode_n_character_ranges[] = {
${formatWords(ranges)}
};

static const uint32_t mjb_unicode_n_character_entries[] = {
${formatWords(entries)}
};

static const uint8_t mjb_unicode_n_character_mirrored[] = {
${formatBytes(mirrored)}
};
`;
}
