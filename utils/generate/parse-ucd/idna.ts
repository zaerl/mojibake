/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { createReadStream } from 'fs';
import { createInterface } from 'readline';
import { ucdCodepointRange } from './utils';

export enum IdnaMappingStatus {
  Disallowed,
  Valid,
  Ignored,
  Mapped,
  Deviation,
}

export type IdnaMappingEntry = {
  start: number;
  end: number;
  status: IdnaMappingStatus;
  mapping: number[];
};

const statuses: Record<string, IdnaMappingStatus> = {
  disallowed: IdnaMappingStatus.Disallowed,
  valid: IdnaMappingStatus.Valid,
  ignored: IdnaMappingStatus.Ignored,
  mapped: IdnaMappingStatus.Mapped,
  deviation: IdnaMappingStatus.Deviation,
};

export async function parseIdnaMappingTable(path: string): Promise<IdnaMappingEntry[]> {
  const entries: IdnaMappingEntry[] = [];
  const lines = createInterface({
    input: createReadStream(path),
    crlfDelay: Infinity,
  });

  for await (const rawLine of lines) {
    const line = rawLine.split('#', 1)[0].trim();

    if(line.length === 0) {
      continue;
    }

    const fields = line.split(';').map(field => field.trim());

    if(fields.length < 2) {
      throw new Error(`Malformed IDNA mapping row: ${rawLine}`);
    }

    const range = ucdCodepointRange(fields[0]);
    const status = statuses[fields[1]];

    if(status === undefined) {
      throw new Error(`Unknown IDNA mapping status: ${fields[1]}`);
    }

    const mapping = (fields[2] ?? '')
      .split(/\s+/u)
      .filter(value => value.length > 0)
      .map(value => Number.parseInt(value, 16));

    if(mapping.some(codepoint => !Number.isInteger(codepoint) ||
      codepoint < 0 || codepoint > 0x10FFFF)) {
      throw new Error(`Invalid IDNA mapping: ${rawLine}`);
    }

    if(status === IdnaMappingStatus.Mapped && mapping.length === 0) {
      throw new Error(`Missing IDNA mapping: ${rawLine}`);
    }

    if(status !== IdnaMappingStatus.Mapped && status !== IdnaMappingStatus.Deviation &&
      mapping.length !== 0) {
      throw new Error(`Unexpected IDNA mapping: ${rawLine}`);
    }

    entries.push({
      start: range.codepointStart,
      end: range.codepointEnd,
      status,
      mapping,
    });
  }

  entries.sort((left, right) => left.start - right.start);

  let next = 0;

  for(const entry of entries) {
    if(entry.start !== next || entry.end < entry.start || entry.end > 0x10FFFF) {
      throw new Error(
        `IDNA mapping table has a gap or overlap at U+${next.toString(16).toUpperCase()}`
      );
    }

    next = entry.end + 1;
  }

  if(next !== 0x110000) {
    throw new Error('IDNA mapping table does not cover all Unicode code points');
  }

  return entries;
}
