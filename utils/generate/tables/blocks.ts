/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { iLog } from '../log';
import { cStringData, formatLongWords } from '../utils';
import { BlockRow } from './types';

// Emits packed Unicode block tables and shared block-name chunks.
export function generateBlocks(rows: BlockRow[]) {
  iLog('Blocks');

  const nameOffsets = new Map<string, { chunk: number; offset: number }>();
  const nameChunks: string[][] = [[]];
  let chunkSize = 0;
  const entries = rows.map((row) => {
    let name = nameOffsets.get(row.name);

    if(name === undefined) {
      if(chunkSize + row.name.length + 1 > 3500) {
        nameChunks.push([]);
        chunkSize = 0;
      }

      name = { chunk: nameChunks.length - 1, offset: chunkSize };
      nameOffsets.set(row.name, name);
      nameChunks[name.chunk].push(row.name);
      chunkSize += row.name.length + 1;
    }

    const delta = row.end - row.start;

    if(row.start > 0x1FFFFF || delta > 0xFFFF || row.id > 0x1FF ||
      name.offset > 0xFFF || name.chunk > 0x3F) {
      throw new Error(
        `Block entry out of bounds: start=${row.start}, delta=${delta}, id=${row.id}, ` +
        `chunk=${name.chunk}, offset=${name.offset}`
      );
    }

    return BigInt(row.start) |
      (BigInt(delta) << 21n) |
      (BigInt(row.id) << 37n) |
      (BigInt(name.offset) << 46n) |
      (BigInt(name.chunk) << 58n);
  });

  const chunkTables = nameChunks.map((chunk, index) =>
    `static const char mjb_unicode_block_name_data_${index}[] =\n${cStringData(chunk)};`
  ).join('\n\n');
  const chunkPointers = nameChunks.map((_, index) =>
    `    mjb_unicode_block_name_data_${index},`
  );

  return `typedef uint64_t mjb_unicode_block_entry;

${chunkTables}

static const char *mjb_unicode_block_name_data[] = {
${chunkPointers.join('\n')}
};

static const mjb_unicode_block_entry mjb_unicode_blocks[] = {
${formatLongWords(entries)}
};
`;
}
