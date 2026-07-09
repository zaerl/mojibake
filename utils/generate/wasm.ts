/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { readFileSync, writeFileSync } from 'fs';
import { cfns } from './html-function';
import { substituteBlock } from './utils';

function getFunctions() {
  return cfns().filter(value => value.isWASM()).map(
    value => '        "' + value.formatWASM() + '"').join("\n");
}

export function generateWASM() {
  const path = '../../src/CMakeLists.txt';
  let fileContent = readFileSync(path, 'utf-8');

  fileContent = substituteBlock(fileContent,
    "set(EXPORTED_FUNCTIONS\n",
    "\n        # Core memory functions",
    getFunctions());

  writeFileSync(path, fileContent);
}
