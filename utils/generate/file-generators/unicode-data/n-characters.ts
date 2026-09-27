/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { iLog } from '../../log';
import {
  bitset, formatBytes, formatCompactIntegers, formatHalfwords, formatPages, formatWords,
} from '../../utils';
import { NCharacterRow } from '../types';

// Emits page-local character metadata runs, deduplicated packed values, and mirrored bitsets.
export function generateNCharacters(rows: NCharacterRow[]) {
  iLog('N-Characters');

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

    // Runs never cross a 256-codepoint page, so a range packs into one page-local halfword.
    if(current !== undefined && sameRun(current, row) && (row.codepoint >> 8) === (current.start >> 8)) {
      current.end = row.codepoint;
    } else {
      pushRun(row);
    }
  }

  const values: number[] = [];
  const valueIndexes = new Map<number, number>();
  // Interns a packed metadata value and returns its index.
  const valueIndex = (run: (typeof runs)[number]) => {
    const packed = (run.quickCheck |
      (run.category << 9) |
      (run.combining << 14) |
      (run.bidirectional << 22) |
      (run.decomposition << 27)) >>> 0;
    let index = valueIndexes.get(packed);

    if(index === undefined) {
      index = values.length;

      if(index > 0xFFFF) {
        throw new Error('N-character values are too many to index');
      }

      values.push(packed);
      valueIndexes.set(packed, index);
    }

    return index;
  };

  // Pages with identical page-local runs (such as uniform ideograph pages) share one compact
  // page; each populated page indexes its runs by 16-codepoint block.
  const pageCount = runs.length === 0 ? 0 : (runs[runs.length - 1].end >> 8) + 1;
  const pageIndex = new Array<number>(pageCount).fill(0xFF);
  const pageStarts: number[] = [];
  const pageCounts: number[] = [];
  const blockStarts: number[] = [];
  const ranges: number[] = [];
  const entries: number[] = [];
  const mirroredFlags: boolean[] = [];
  const compactPages = new Map<string, number>();

  for(let index = 0; index < runs.length;) {
    const page = runs[index].start >> 8;
    const pageStart = index;

    while(index < runs.length && (runs[index].start >> 8) === page) {
      ++index;
    }

    const pageRuns = runs.slice(pageStart, index);
    const pageRanges = pageRuns.map((run) => {
      const delta = run.end - run.start;

      if(delta > 0xFF || (run.start >> 8) !== (run.end >> 8)) {
        throw new Error(`N-character run is too large to pack: ${run.start}-${run.end}`);
      }

      return (run.start & 0xFF) | (delta << 8);
    });
    const pageValues = pageRuns.map(valueIndex);
    const key = pageRuns.map((run, i) => `${pageRanges[i]}:${pageValues[i]}:${run.mirrored}`)
      .join(',');
    const existing = compactPages.get(key);

    if(existing !== undefined) {
      pageIndex[page] = existing;
      continue;
    }

    if(pageStarts.length >= 0xFF || ranges.length + pageRuns.length > 0xFFFF) {
      throw new Error('N-character pages or runs are too many to index');
    }

    if(pageRuns.length > 0xFF) {
      throw new Error(`N-character page has too many runs to index: ${pageRuns.length}`);
    }

    compactPages.set(key, pageStarts.length);
    pageIndex[page] = pageStarts.length;
    pageStarts.push(ranges.length);
    pageCounts.push(pageRuns.length);

    let run = 0;

    for(let block = 0; block < 16; ++block) {
      while(run + 1 < pageRuns.length && (pageRuns[run + 1].start & 0xFF) <= (block << 4)) {
        ++run;
      }

      blockStarts.push(run);
    }

    ranges.push(...pageRanges);
    entries.push(...pageValues);
    mirroredFlags.push(...pageRuns.map((run) => run.mirrored));
  }

  const pages = { index: pageIndex, pages: { starts: pageStarts, counts: pageCounts } };
  const mirrored = bitset(mirroredFlags);

  return `static const uint8_t mjb_unicode_n_character_page_index[] = {
${formatBytes(pages.index)}
};

static const mjb_unicode_page mjb_unicode_n_character_pages[] = {
${formatPages(pages.pages)}
};

static const uint8_t mjb_unicode_n_character_block_starts[] = {
${formatCompactIntegers(blockStarts, 32)}
};

static const uint16_t mjb_unicode_n_character_ranges[] = {
${formatHalfwords(ranges, 12)}
};

static const uint16_t mjb_unicode_n_character_entries[] = {
${formatCompactIntegers(entries, 24)}
};

static const uint32_t mjb_unicode_n_character_values[] = {
${formatWords(values)}
};

static const uint8_t mjb_unicode_n_character_mirrored[] = {
${formatBytes(mirrored)}
};
`;
}
