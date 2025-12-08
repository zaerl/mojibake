/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { open } from 'fs/promises';
import { log } from './log';

export async function readCompositionExclusions(path = './UCD/CompositionExclusions.txt'): Promise<number[]> {
  log('READ COMPOSITION EXCLUSIONS');

  const exclusions: number[] = [];

  const file = await open(path);

  for await (const line of file.readLines()) {
    if(line.startsWith('#') || line === '') { // Comment
      continue;
    }

    const split = line.split('#');
    const codepoint = parseInt(split[0].trim(), 16); // Parse as hex

    if(!isNaN(codepoint)) {
      exclusions.push(codepoint);
    }
  }

  return exclusions;
}
