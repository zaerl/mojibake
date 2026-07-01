/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { iLog } from '../log';
import { BidiMirroringPair } from '../types';
import { parsePropertyFile } from './utils';

export async function readBidiMirroring(): Promise<BidiMirroringPair[]> {
  iLog('Parse Bidi Mirroring data');

  const path = './unicode-data/UCD/BidiMirroring.txt';
  const pairs: BidiMirroringPair[] = [];

  for await (const split of parsePropertyFile(path)) {
    pairs.push({ cp: parseInt(split[0], 16), mirror: parseInt(split[1], 16) });
  }

  return pairs;
}
