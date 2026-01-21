/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { writeFileSync } from 'fs';
import { Character } from '../character';
import { log } from '../log';
import { parsePropertyFile } from './parse-property-file';
import { LineBreakingClass, LineBreakingClassStrings } from '../types';

export async function generateBreaks(characters: Character[], path = './UCD/LineBreak.txt') {
  log('GENERATE BREAKS');

  const characterMap: { [key: string]: Character } = {};

  for(const char of characters) {
    characterMap['' + char.codepoint] = char;
  }

  for await (const split of parsePropertyFile(path, ['F0000', '100000'])) {
    if(split.length < 2) {
      continue;
    }

    const codepoint = split[0];
    const breakClass = split[1];
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

    if(LineBreakingClass[breakClass as keyof typeof LineBreakingClass] === undefined) {
      console.log(`Unknown line breaking class: ${breakClass}`);
      continue;
    } else {
      for(let cp = codepointStart; cp <= codepointEnd; ++cp) {
        const index = '' + cp;

        if(characterMap[index]) {
          characterMap[index].lineBreakingClass = LineBreakingClass[breakClass as LineBreakingClassStrings];
        }
      }
    }
  }
}

export async function generateBreaksTest(path: string) {
  log(`GENERATE BREAKS TEST ${path}`);

  let max = 0;
  let output: string[] = [];

  for await (const split of parsePropertyFile(`./UCD/auxiliary/${path}Test.txt`, [], '#', false)) {
    if(split.length < 2) {
      continue;
    }

    const rule = split[0];
    const withSlash = rule.replace(/÷/g, '+');
    const final = withSlash.replace(/×/g, 'x');

    max = Math.max(max, final.length);
    // Remove first ×
    output.push(final.slice(1).trim());
  }

  writeFileSync(`./UCD/auxiliary/${path}TestModified.txt`, output.join('\n'));
}
