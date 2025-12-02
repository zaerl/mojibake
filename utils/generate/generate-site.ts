/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { readFileSync, writeFileSync } from 'fs';
import { cfns } from './function';
import { substituteText } from './utils';

function getFunctions() {
  const functs = cfns();

  return functs.filter(fn => fn.isWASM() && !fn.isInternal());
}

export async function generateSite() {
  let fileContent = readFileSync('../../build-wasm/src/index.html', 'utf-8');
  const functs = getFunctions();

  fileContent = substituteText(fileContent,
    "const functions = {",
    "};",
    functs.map(fn => `"${fn.getName()}": ${fn.formatJSON()}`).join(',\n'));

  fileContent = substituteText(fileContent,
    "<section id=\"functions\" class=\"loading\">",
    "</section>",
    functs.map(fn => '    ' + fn.formatHTML()).join('\n'));

  writeFileSync('../../build-wasm/src/index.html', fileContent);
}
