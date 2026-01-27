/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { dbInsertBlock } from '../db';
import { log } from '../log';
import { Block } from '../types';
import { parsePropertyFile, ucdCodepointRange, ucdNameToEnumName } from './utils';

export async function readBlocks(): Promise<Block[]> {
  log('READ BLOCKS');

  const path = './UCD/Blocks.txt';
  const blocks: Block[] = [];
  let id = 0;

  for await (const split of parsePropertyFile(path)) {
    if(split.length < 2) {
      continue;
    }

    const name = split[1];
    let { codepointStart, codepointEnd } = ucdCodepointRange(split[0]);

    const block = {
      name,
      enumName: ucdNameToEnumName('MJB_BLOCK', name),
      start: codepointStart,
      end: codepointEnd
    };

    blocks.push(block);
    dbInsertBlock(id, block);

    ++id;
  }

  return blocks;
}
