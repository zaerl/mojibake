/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { iLog } from '../../log';
import { formatBytes, formatCompactIntegers, formatHalfwords } from '../../utils';
import { PropertyRangeRow } from '../types';

// Codepoints per property block; each populated page indexes its runs by block.
const PROPERTY_BLOCK_SHIFT = 5;
const PROPERTY_BLOCKS_PER_PAGE = 256 >> PROPERTY_BLOCK_SHIFT;

type PropertyRecord = { bools: number[]; enums: [number, number][] };

// Decodes a source range blob: [bool count][bool ids][enum count][id, value]...
function decodeBlob(blob: Buffer): PropertyRecord {
  let offset = 0;
  const boolCount = blob[offset++];
  const bools = [...blob.subarray(offset, offset + boolCount)];
  offset += boolCount;
  const enumCount = blob[offset++];
  const enums: [number, number][] = [];

  for(let i = 0; i < enumCount; ++i) {
    enums.push([blob[offset], blob[offset + 1]]);
    offset += 2;
  }

  return { bools, enums };
}

// Merges the records of every source range covering a codepoint. Earlier ranges win for
// enumerated values, matching the scan order of the previous overlapping-range lookup.
function mergeRecords(records: PropertyRecord[]) {
  const bools = new Set<number>();
  const enums = new Map<number, number>();

  for(const record of records) {
    for(const id of record.bools) {
      bools.add(id);
    }

    for(const [id, value] of record.enums) {
      if(!enums.has(id)) {
        enums.set(id, value);
      }
    }
  }

  const boolIds = [...bools].sort((a, b) => a - b);
  const enumIds = [...enums.keys()].sort((a, b) => a - b);
  const encoded = [boolIds.length, ...boolIds, enumIds.length];

  for(const id of enumIds) {
    encoded.push(id, enums.get(id)!);
  }

  return encoded;
}

// Emits disjoint property runs. Every codepoint maps to one merged record, so a lookup finds a
// single run in its page (narrowed by a per-block index) and scans one sorted record.
export function generateProperties(rows: PropertyRangeRow[]) {
  iLog('Properties');

  // Sweep the range boundaries, keeping the set of active source ranges.
  type Event = { codepoint: number; row: number; start: boolean };
  const events: Event[] = [];
  const decoded = rows.map((row) => decodeBlob(row.properties));

  rows.forEach((row, index) => {
    const end = row.end_codepoint ?? row.start_codepoint;
    events.push({ codepoint: row.start_codepoint, row: index, start: true });
    events.push({ codepoint: end + 1, row: index, start: false });
  });

  events.sort((a, b) => a.codepoint - b.codepoint || (a.start === b.start ? 0 : a.start ? 1 : -1));

  const recordData: number[] = [];
  const recordOffsets = new Map<string, number>();
  const active = new Set<number>();
  const runs: { start: number; end: number; record: number }[] = [];
  let eventIndex = 0;

  // Interns a merged record and returns its offset, or -1 for an empty record.
  const internRecord = () => {
    if(active.size === 0) {
      return -1;
    }

    const ordered = [...active].sort((a, b) =>
      rows[a].start_codepoint - rows[b].start_codepoint ||
      (rows[a].end_codepoint ?? rows[a].start_codepoint) -
      (rows[b].end_codepoint ?? rows[b].start_codepoint)
    );
    const encoded = mergeRecords(ordered.map((index) => decoded[index]));
    const key = encoded.join(',');
    let offset = recordOffsets.get(key);

    if(offset === undefined) {
      offset = recordData.length;
      recordData.push(...encoded);
      recordOffsets.set(key, offset);
    }

    return offset;
  };

  while(eventIndex < events.length) {
    const codepoint = events[eventIndex].codepoint;

    while(eventIndex < events.length && events[eventIndex].codepoint === codepoint) {
      const event = events[eventIndex++];

      if(event.start) {
        active.add(event.row);
      } else {
        active.delete(event.row);
      }
    }

    const record = internRecord();
    const next = eventIndex < events.length ? events[eventIndex].codepoint : codepoint;

    if(record < 0 || next <= codepoint) {
      continue;
    }

    // Split runs at page boundaries so each one packs into a page-local halfword.
    for(let start = codepoint; start < next;) {
      const end = Math.min(next - 1, (start | 0xFF));
      const last = runs[runs.length - 1];

      if(last !== undefined && last.record === record && last.end + 1 === start &&
        (last.start >> 8) === (start >> 8)) {
        last.end = end;
      } else {
        runs.push({ start, end, record });
      }

      start = end + 1;
    }
  }

  if(recordData.length > 0xFFFF) {
    throw new Error(`Property record data is too large to index: ${recordData.length}`);
  }

  const pageCount = runs.length === 0 ? 0 : (runs[runs.length - 1].end >> 8) + 1;
  const pageIndex = new Array<number>(pageCount).fill(0xFFFF);
  const pageStarts: number[] = [];
  const blockStarts: number[] = [];
  const runEntries: number[] = [];
  const runRecords: number[] = [];
  // Pages with identical page-local runs (uniform CJK, Hangul, and private-use pages) share one
  // compact page.
  const compactPages = new Map<string, number>();

  for(let index = 0; index < runs.length;) {
    const page = runs[index].start >> 8;
    const pageStart = index;

    while(index < runs.length && (runs[index].start >> 8) === page) {
      ++index;
    }

    const pageRuns = runs.slice(pageStart, index);
    const pageEntries = pageRuns.map((run) => (run.start & 0xFF) | ((run.end - run.start) << 8));
    const key = pageRuns.map((run, i) => `${pageEntries[i]}:${run.record}`).join(',');
    const existing = compactPages.get(key);

    if(existing !== undefined) {
      pageIndex[page] = existing;
      continue;
    }

    if(pageStarts.length >= 0xFFFF || runEntries.length + pageRuns.length > 0xFFFF) {
      throw new Error('Property pages or runs are too many to index');
    }

    if(pageRuns.length > 0xFF) {
      throw new Error(`Property page ${page} has too many runs to index: ${pageRuns.length}`);
    }

    compactPages.set(key, pageStarts.length);
    pageIndex[page] = pageStarts.length;
    pageStarts.push(runEntries.length);

    // Each block starts at the last run beginning at or before the block's first codepoint.
    let run = 0;

    for(let block = 0; block < PROPERTY_BLOCKS_PER_PAGE; ++block) {
      const blockStart = block << PROPERTY_BLOCK_SHIFT;

      while(run + 1 < pageRuns.length && (pageRuns[run + 1].start & 0xFF) <= blockStart) {
        ++run;
      }

      blockStarts.push(run);
    }

    runEntries.push(...pageEntries);
    runRecords.push(...pageRuns.map((run) => run.record));
  }

  pageStarts.push(runEntries.length);

  return `enum {
    MJB_UNICODE_PROPERTY_BLOCK_SHIFT = ${PROPERTY_BLOCK_SHIFT},
    MJB_UNICODE_PROPERTY_BLOCKS_PER_PAGE = ${PROPERTY_BLOCKS_PER_PAGE}
};

// Merged records: [bool count][sorted bool ids][enum count][id, value]...
static const uint8_t mjb_unicode_property_records[] = {
${formatCompactIntegers(recordData, 32)}
};

static const uint16_t mjb_unicode_property_page_index[] = {
${formatHalfwords(pageIndex, 12)}
};

static const uint16_t mjb_unicode_property_page_starts[] = {
${formatCompactIntegers(pageStarts, 16)}
};

static const uint8_t mjb_unicode_property_block_starts[] = {
${formatCompactIntegers(blockStarts, 32)}
};

static const uint16_t mjb_unicode_property_runs[] = {
${formatHalfwords(runEntries, 12)}
};

static const uint16_t mjb_unicode_property_run_records[] = {
${formatCompactIntegers(runRecords, 16)}
};
`;
}
