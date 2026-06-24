/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { readFileSync, readdirSync, writeFileSync } from 'fs';
import path from 'path';

const mojibakeFileLicense = `/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */`;

function getFileLicense(description: string[] = []) {
  if(description.length) {
    return mojibakeFileLicense.replace('*/', `\n * ${description.join('\n * ')}\n */`);
  }

  return mojibakeFileLicense;
}

function loadFile(file: string) {
  const srcDir = '../../src';
  let fileContent = readFileSync(path.join(srcDir, file), 'utf-8');
  console.log(`Loading ${file}`);

  // Replace #pragma once with comment
  fileContent = fileContent.replace(getFileLicense() + '\n\n#pragma once\n', '');

  return fileContent;
}

export async function generateAmalgamation() {
  console.log('Generating amalgamation...');
  const baseFolder = '../../build-amalgamation';

  let header = getFileLicense([
    'This file is an amalgamation of all Mojibake source files. It is automatically generated. Do not',
    'edit. If you want to generate it, run the following command:',
    '',
    'make amalgamation'
  ]);

  header += '\n\n#pragma once\n';
  header += loadFile('mojibake.h');

  let unicode_file = loadFile('unicode.h');
  header = header.replace('#include "unicode.h"\n', `// unicode.h\n${unicode_file}`);

  let locales_file = loadFile('locales.h');
  header = header.replace('#include "locales.h"\n', `\n// locales.h\n${locales_file}`);

  // Generate main header
  writeFileSync(baseFolder + '/mojibake.h', header);

  let source = `/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 *
 * This file is an amalgamation of all Mojibake source files. It is automatically generated. Do not
 * edit. If you want to generate it, run the following command:
 *
 * make amalgamation
 */\n\n#include "mojibake.h"\n`;

  // Headers that need to be included before the sources
  const headers = [
    'buffer.h',
  ];

  for(const header of headers) {
    let content = `\n// ----------\n// ${header}\n// ----------\n\n` + loadFile(header);
    content = content.replace(/#include "mojibake\.h"/g, match => `// ${match}`);

    source += content;
  }

  source += `\n// ----------\n// Start of sources\n// ----------\n
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>\n\n`;

  const srcDir = '../../src';
  const cFiles = readdirSync(srcDir, { withFileTypes: true })
    .filter(entry => entry.isFile() && entry.name.endsWith('.c') && entry.name !== 'mojibake.c')
    .map(entry => entry.name)
    .sort();

  const sources = [
    'mojibake-internal.h',
    'unicode-tables.h',
    'unicode-data.h',
    'mojibake.c',
    'utf8.h',
    'utf16.h',
    'utf32.h',
    'utf.h',
    ...cFiles,
  ];

  for(const file of sources) {
    let content = `\n// ----------\n// ${file}\n// ----------\n\n` + loadFile(file);
    content = content.replace(/#include .+/g, match => `// ${match}`);
    content = content.replace(/extern mojibake mjb_global;/g, match => `// ${match}`);

    source += content;
  }

  writeFileSync(baseFolder + '/mojibake.c', source);
}
