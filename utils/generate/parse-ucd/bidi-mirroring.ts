import { log } from '../log';
import { BidiMirroringPair } from '../types';
import { parsePropertyFile } from './utils';

export async function readBidiMirroring(): Promise<BidiMirroringPair[]> {
  log('READ BIDI MIRRORING');

  const path = './UCD/BidiMirroring.txt';
  const pairs: BidiMirroringPair[] = [];

  for await (const split of parsePropertyFile(path)) {
    pairs.push({ cp: parseInt(split[0], 16), mirror: parseInt(split[1], 16) });
  }

  return pairs;
}
