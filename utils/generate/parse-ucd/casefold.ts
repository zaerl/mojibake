/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { Character } from '../character';
import { log } from '../log';
import { parsePropertyFile, ucdInt } from './utils';

export type CaseFoldEntry = {
  codepoint: number;
  mapping: number[];
};

export async function generateCasefold(characters: Character[]): Promise<CaseFoldEntry[]> {
  log('GENERATE CASEFOLD');

  const path = './UCD/CaseFolding.txt';

  // Build map from codepoint to unicode_data.lowercase
  const lowercaseMap = new Map<number, number | null>();

  for(const char of characters) {
    lowercaseMap.set(char.codepoint, char.lowercase);
  }

  const entries: CaseFoldEntry[] = [];

  for await (const split of parsePropertyFile(path)) {
    const codepoint = ucdInt(split[0]) as number;
    const status = split[1];
    const mapping = split[2].split(' ').map(s => parseInt(s, 16));

    if(status === 'C') {
      // Only store if different from unicode_data.lowercase (the fallback handles the common case)
      const dbLower = lowercaseMap.get(codepoint) ?? null;

      if(dbLower !== mapping[0]) {
        entries.push({ codepoint, mapping });
      }
    } else if(status === 'F') {
      // Always store multi-char full folds (ß→ss, ﬃ→ffi, etc.)
      entries.push({ codepoint, mapping });
    }

    // Skip S (simple) and T (Turkic)
  }

  return entries;
}
