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
    const codepoint = parseInt(split[0].trim(), 16); // Parse as hex

    if(!isNaN(codepoint)) {
      exclusions.push(codepoint);
    }
  });

  return exclusions;
}
