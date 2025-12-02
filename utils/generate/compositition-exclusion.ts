/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { log } from './log';

export async function readCompositionExclusions(path = './UCD/CompositionExclusions.txt'): Promise<number[]> {
  log('READ COMPOSITION EXCLUSIONS');

  const exclusions: number[] = [];

  const file = Bun.file(path);
  const content = await file.text();
  const lines = content.split('\n');

  for (const line of lines) {
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
