/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { readFileSync, writeFileSync } from 'fs';
import { cfns } from './function';
import { getVersion, substituteBlock, substituteText } from './utils';

function getFunctions() {
  const functs = cfns();

  return functs.filter(fn => fn.isWASM() && !fn.isInternal());
}

export async function generateSite() {
  let fileContent = readFileSync('../../src/site/index.html', 'utf-8');
  const functs = getFunctions();

  fileContent = substituteBlock(fileContent,
    'const functions = {',
    '};',
    functs.map(fn => `"${fn.getName()}": ${fn.formatJSON()}`).join(',\n'));

  fileContent = substituteBlock(fileContent,
    '<section id="functions" class="loading">',
    '</section>',
    functs.map(fn => '    ' + fn.formatHTML()).join('\n'));

  const version = getVersion();
  const fileName = `mojibake-amalgamation-${version.major}${version.minor}${version.revision}.zip`;
  const embeddedFileName = `mojibake-embedded-amalgamation-${version.major}${version.minor}${version.revision}.zip`;
  const wasmFileName = `mojibake-wasm-${version.major}${version.minor}${version.revision}.zip`;

  fileContent = substituteText(fileContent, '[AM_HREF]', fileName);
  fileContent = substituteText(fileContent, '[AM_NAME]', fileName);
  fileContent = substituteText(fileContent, '[EMBEDDED_HREF]', embeddedFileName);
  fileContent = substituteText(fileContent, '[EMBEDDED_NAME]', embeddedFileName);
  fileContent = substituteText(fileContent, '[WASM_HREF]', wasmFileName);
  fileContent = substituteText(fileContent, '[WASM_NAME]', wasmFileName);
  fileContent = substituteText(fileContent, '[VERSION]', version.version);

  writeFileSync('../../build-wasm/src/index.html', fileContent);

  // Copy style.css to build-wasm directory
  const styleContent = readFileSync('../../src/site/style.css', 'utf-8');
  writeFileSync('../../build-wasm/src/style.css', styleContent);
}
