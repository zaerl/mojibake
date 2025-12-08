/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { open } from 'fs/promises';
import { log } from './log';

export async function generateCasefold() {
  log('GENERATE CASEFOLD');
  const file = await open('./UCD/CaseFolding.txt');

  let maxFolded = 0;
  let counts = {
    'C': 0,
    'F': 0,
    'S': 0,
    'T': 0,
  }

  for await (const line of file.readLines()) {
    if(line.startsWith('#') || line.trim() === '') {
      continue;
    }

    const split = line.split(';');
    const codepoint = parseInt(split[0], 16);
    const status = split[1].trim();
    const mapping = split[2].trim().split(' ');

    ++counts[status as keyof typeof counts];

    if(mapping.length > 1) {
      maxFolded = Math.max(maxFolded, mapping.length);
    }
  }
}
