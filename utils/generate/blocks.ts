/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { dbInsertBlock } from './db';
import { Block } from './types';
import { log } from './log';

export async function readBlocks(path = './UCD/Blocks.txt'): Promise<Block[]> {
  log('READ BLOCKS');

  const blocks: Block[] = [];
  let id = 0;

  const file = Bun.file(path);
  const content = await file.text();
  const lines = content.split('\n');

  for (const line of lines) {
    if(line.startsWith('#') || line === '') { // Comment
      continue;
    }

    const split = line.split('; ');
    const name = split[1];
    const values = split[0].split('..');
    const start = parseInt(values[0], 16);
    const end = parseInt(values[1], 16);
    const block = {
      name,
      enumName: `MJB_BLOCK_${split[1].toUpperCase().replace(/[ \-]/g, '_')}`,
      start,
      end
    };

    blocks.push(block);
    dbInsertBlock(id, block);

    ++id;
  }

  return blocks;
}
