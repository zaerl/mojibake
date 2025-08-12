import { readFileSync, writeFileSync } from 'fs';
import { open } from 'fs/promises';
import { substituteText } from './utils';

async function generateLocales() {
  const file = await open('./locales/ISO-639-2.txt');
  const locales = [];
  const names = [];

  for await (const line of file.readLines()) {
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

  fileContent = substituteText(fileContent, "typedef enum mjb_locale {\n", "\n} mjb_locale;", localeEnums.join('\n'));
  fileContent = substituteText(fileContent, '#define MJB_LOCALE_NUM ', "\n", '' + locales.length);

  writeFileSync('../../src/locales.h', fileContent);
}

generateLocales();
