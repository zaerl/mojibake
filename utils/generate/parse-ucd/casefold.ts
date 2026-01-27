/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { log } from '../log';
import { parsePropertyFile, ucdInt } from './utils';

export async function generateCasefold() {
  log('GENERATE CASEFOLD');

  const path = './UCD/CaseFolding.txt';
  let maxFolded = 0;
  let counts = {
    'C': 0,
    'F': 0,
    'S': 0,
    'T': 0,
  }

  for await (const split of parsePropertyFile(path) {
    const codepoint = ucdInt(split[0]);
    const status = split[1];
    const mapping = split[2].split(' ');

    ++counts[status as keyof typeof counts];

    if(mapping.length > 1) {
      maxFolded = Math.max(maxFolded, mapping.length);
    }
  }
}
