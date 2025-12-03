/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { readFileSync, writeFileSync } from 'fs';
import { cfns } from './function';
import { substituteBlock, substituteText } from './utils';

function getFunctions() {
  const functs = cfns();

  return functs.filter(fn => fn.isWASM() && !fn.isInternal());
}

function getVersion() {
  return readFileSync('../../VERSION', 'utf-8').trim();
}

export async function generateSite() {
  let fileContent = readFileSync('../../src/site/index.html', 'utf-8');
  const functs = getFunctions();

  fileContent = substituteBlock(fileContent,
    "const functions = {",
    "};",
    functs.map(fn => `"${fn.getName()}": ${fn.formatJSON()}`).join(',\n'));

  fileContent = substituteBlock(fileContent,
    "<section id=\"functions\" class=\"loading\">",
    "</section>",
    functs.map(fn => '    ' + fn.formatHTML()).join('\n'));

  const version = getVersion();
  const fileName = `mojibake-amalgamation-${version.replace(/\./g, '')}.zip`;

  fileContent = substituteText(fileContent, '[AM_HREF]', fileName);
  fileContent = substituteText(fileContent, '[AM_NAME]', fileName);
  fileContent = substituteText(fileContent, '[VERSION]', version);

  writeFileSync('../../build-wasm/src/index.html', fileContent);
}
