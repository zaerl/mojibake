/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { writeFileSync } from 'fs';
import { log } from '../log';
import { parsePropertyFile } from './utils';

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
