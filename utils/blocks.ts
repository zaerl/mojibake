import { createReadStream } from 'fs';
import { createInterface } from 'readline';
import { log } from './log';
import { Block } from './types';

export function readBlocks(path = './UCD/Blocks.txt'): Block[] {
  log('READ BLOCKS');

  const blocks: Block[] = [];

  const rl = createInterface({
    input: createReadStream(path),
    crlfDelay: Infinity
  });

  rl.on('line', (line: string) => {
    if(line.startsWith('#') || line === '') { // Comment
      return;
    }

    const split = line.split('; ');
    const name = split[1];
    const values = split[0].split('..');
    const start = parseInt(values[0], 16);
    const end = parseInt(values[1], 16);

    blocks.push({
      name,
      enumName: `MJB_BLOCK_${split[1].toUpperCase().replace(/[ \-]/g, '_')}`,
      start,
      end
    });
  });

  return blocks;
}
