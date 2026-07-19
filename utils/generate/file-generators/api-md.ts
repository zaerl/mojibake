/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { readFileSync, writeFileSync } from 'fs';
import { cfns, CFunction } from '../html-function';
import { substituteBlock } from '../utils';

export function generateAPI() {
  const path = '../../API.md';
  let fileContent = readFileSync(path, 'utf-8');

  const functions = '\n' + cfns().map((value: CFunction) => value.formatMD()).join('\n\n');
  fileContent = substituteBlock(fileContent, "# Functions\n", "\n\n# Unicode references", functions);

  writeFileSync(path, fileContent);
}
