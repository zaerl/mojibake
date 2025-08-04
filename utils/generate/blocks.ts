import { createReadStream } from 'fs';
import { createInterface } from 'readline';
import { dbInsertBlock } from './db';
import { log } from './log';
import { Block } from './types';

export async function readBlocks(path = './UCD/Blocks.txt'): Promise<Block[]> {
  log('READ BLOCKS');

  const blocks: Block[] = [];
  let id = 0;

  const rl = createInterface({
    input: createReadStream(path),
    crlfDelay: Infinity
  });

  return new Promise((resolve) => {
    rl.on('line', (line: string) => {
      if(line.startsWith('#') || line === '') { // Comment
        return;
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
    });

    rl.on('close', () => {
      resolve(blocks);
    });
  });
}
