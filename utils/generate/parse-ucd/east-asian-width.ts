/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { Character } from '../character';
import { log } from '../log';
import { EastAsianWidth, EastAsianWidthStrings } from '../types';
import { parsePropertyFile, ucdCodepointRange } from './utils';

export async function generateEastAsianWidth(characters: Character[], path = './UCD/EastAsianWidth.txt') {
  log('GENERATE EAST ASIAN WIDTH');

  const characterMap: { [key: string]: Character } = {};

  for(const char of characters) {
    characterMap['' + char.codepoint] = char;
  }

  for await (const split of parsePropertyFile(path, ['F0000', '100000'])) {
    if(split.length < 2) {
      continue;
    }

    const codepoint = split[0];
    const width = split[1];
    let { codepointStart, codepointEnd } = ucdCodepointRange(codepoint);

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
