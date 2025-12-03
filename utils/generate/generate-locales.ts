/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { readFileSync, writeFileSync } from 'fs';
import { substituteBlock } from './utils';

export async function generateLocales() {
  const file = Bun.file('./locales/ISO-639-2.txt');
  const content = await file.text();
  const lines = content.split('\n');
  const locales = [];
  const names = [];

  for (const line of lines) {
    const split = line.split('|');

    if(split[2].length) {
      locales.push(split[2]);
      names.push(split[3]);
    }
  }

  const localeEnums = [];

  for(let i = 0; i < locales.length; ++i) {
    localeEnums.push(`    MJB_LOCALE_${locales[i].toUpperCase()}${ i === locales.length - 1 ? ' ' : ','} // ${names[i]}`);
  }

  let fileContent = readFileSync('../../src/locales.h', 'utf-8');

  fileContent = substituteBlock(fileContent, "typedef enum mjb_locale {\n", "\n} mjb_locale;", localeEnums.join('\n'));
  fileContent = substituteBlock(fileContent, '#define MJB_LOCALE_NUM ', "\n", '' + locales.length);

  writeFileSync('../../src/locales.h', fileContent);
}
