/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { writeFileSync } from 'fs';
import { Character } from './character';
import { log } from './log';
import { LineBreakingClass, LineBreakingClassStrings } from './types';

export async function generateBreaks(characters: Character[], path = './UCD/LineBreak.txt') {
  log('GENERATE BREAKS');
  const file = Bun.file(path);
  const content = await file.text();
  const lines = content.split('\n');
  const characterMap: { [key: string]: Character } = {};

  for(const char of characters) {
    characterMap['' + char.codepoint] = char;
  }

  for (const line of lines) {
    if(line.length === 0 || line.startsWith('#') || line.startsWith('F0000') ||
      line.startsWith('100000')) {
      continue;
    }

    const split = line.split(';');

    if(split.length < 2) continue;

    const codepoint = split[0].trim();
    const breakClass = split[1].trim().split('#')[0].trim();
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
  const file = Bun.file(`./UCD/auxiliary/${path}Test.txt`);
  const content = await file.text();
  const lines = content.split('\n');
  let max = 0;
  let output: string[] = [];

  for (const line of lines) {
    if(line.length === 0 || line.startsWith('#')) {
      continue;
    }

    const split = line.split('#');
    if(split.length < 2) continue;

    const rule = split[0].trim();
    const withSlash = rule.replace(/÷/g, '+');
    const final = withSlash.replace(/×/g, 'x');

    max = Math.max(max, final.length);
    // Remove first ×
    output.push(final.slice(1).trim());
  }

  writeFileSync(`./UCD/auxiliary/${path}TestModified.txt`, output.join('\n'));
}
