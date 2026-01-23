/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { log } from './log';
import { parsePropertyFile } from './parse-ucd/utils';

export async function readCompositionExclusions(path = './UCD/CompositionExclusions.txt'): Promise<number[]> {
  log('READ COMPOSITION EXCLUSIONS');

  const exclusions: number[] = [];

  for await (const split of parsePropertyFile(path)) {
    if(split.length < 1) {
      continue;
    }

    const codepoint = parseInt(split[0], 16);

    if(!isNaN(codepoint)) {
      exclusions.push(codepoint);
    }
  }

  return exclusions;
}
