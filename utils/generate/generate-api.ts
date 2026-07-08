/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { readFileSync, writeFileSync } from 'fs';
import { cfns, CFunction } from './html-function';
import { substituteBlock } from './utils';

export function generateAPI() {
  let fileContent = readFileSync('../../API.md', 'utf-8');

  const functions = '\n' + cfns().map((value: CFunction) => value.formatMD()).join('\n\n');
  fileContent = substituteBlock(fileContent, "# Functions\n", null, functions);

  writeFileSync('../../API.md', fileContent);
}
