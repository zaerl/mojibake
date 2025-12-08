/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { open } from 'fs/promises';
import { Character } from './character';
import { log } from './log';
import { EastAsianWidth, EastAsianWidthStrings } from './types';

export async function generateEastAsianWidth(characters: Character[], path = './UCD/EastAsianWidth.txt') {
  log('GENERATE EAST ASIAN WIDTH');
  const file = await open(path);
  const characterMap: { [key: string]: Character } = {};

  for(const char of characters) {
    characterMap['' + char.codepoint] = char;
  }

  for await (const line of file.readLines()) {
    if(line.length === 0 || line.startsWith('#') || line.startsWith('F0000') ||
      line.startsWith('100000')) {
      continue;
    }

    const split = line.split(';');

    if(split.length < 2) continue;

    const codepoint = split[0].trim();
    const width = split[1].trim().split('#')[0].trim();
    let codepointStart = 0;
    let codepointEnd = 0;

    if(codepoint.includes('..')) {
      const codepoints = codepoint.split('..');

      if(codepoints.length === 2) {
        codepointStart = parseInt(codepoints[0], 16);
        codepointEnd = parseInt(codepoints[1], 16);
      }
    } else {
      codepointStart = parseInt(codepoint, 16);
      codepointEnd = codepointStart;
    }

    if(EastAsianWidth[width as EastAsianWidthStrings] === undefined) {
      console.log(`Unknown east asian width: ${width}`);
      continue;
    } else {
      for(let cp = codepointStart; cp <= codepointEnd; ++cp) {
        const index = '' + cp;

        if(characterMap[index]) {
          characterMap[index].eastAsianWidth = EastAsianWidth[width as EastAsianWidthStrings];
        }
      }
    }
  }
}
