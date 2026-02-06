/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { log } from '../log';
import { parsePropertyFile } from './utils';

export async function readAliases(): Promise<{ [key: string]: string }> {
  log('READ UNICODE ALIASES');

  const aliases: { [key: string]: string } = {};

  for await (const split of parsePropertyFile('./UCD/NameAliases.txt')) {
    if(split.length < 3) {
      continue;
    }

    const codepoint = split[0];
    const alias = split[1];
    const type = split[2];

    if(type === 'control' || type === 'correction') {
      aliases[codepoint] = alias;
    }
  }

  return aliases;
}
