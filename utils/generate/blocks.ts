/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { dbInsertBlock } from './db';
import { log } from './log';
import { parsePropertyFile } from './parse-property-file';
import { Block } from './types';

export async function readBlocks(path = './UCD/Blocks.txt'): Promise<Block[]> {
  log('READ BLOCKS');

  const blocks: Block[] = [];
  let id = 0;

  for await (const split of parsePropertyFile(path)) {
    if(split.length < 2) {
      continue;
    }

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
