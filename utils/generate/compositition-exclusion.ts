import { createReadStream } from 'fs';
import { createInterface } from 'readline';
import { log } from './log';

export function readCompositionExclusions(path = './UCD/CompositionExclusions.txt'): number[] {
  log('READ COMPOSITION EXCLUSIONS');

  const exclusions: number[] = [];

  const rl = createInterface({
    input: createReadStream(path),
    crlfDelay: Infinity
  });

  rl.on('line', (line: string) => {
    if(line.startsWith('#') || line === '') { // Comment
      return;
    }

    const split = line.split('#');
    const name = parseInt(split[0].trim());

    exclusions.push(name);
  });

  return exclusions;
}
