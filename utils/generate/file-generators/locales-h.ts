/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { existsSync, readFileSync, writeFileSync } from 'fs';
import { getScriptEnumNames } from '../header';
import { Property } from '../parse-ucd/properties';
import { downloadText, substituteBlock } from '../utils';

// ISO 639-2 code list as maintained by the Debian iso-codes project. The Library of Congress
// list (https://www.loc.gov/standards/iso639-2/) is the normative source but its download is
// behind a bot check; the two carry the same codes.
const ISO_639_2_URL = 'https://salsa.debian.org/iso-codes-team/iso-codes/-/raw/main/data/iso_639-2.json';
const ISO_639_2_PATH = './locales/iso_639-2.json';

// Same as ColumnLimit in .clang-format
const COLUMN_LIMIT = 100;

interface Iso6392Entry {
  alpha_2?: string;
  alpha_3: string;
  bibliographic?: string;
  name: string;
}

interface Language {
  code: string; // ISO 639-1 two-letter code
  name: string;
}

// Read the languages that have a two-letter ISO 639-1 code, ordered by their bibliographic
// ISO 639-2 code, the same order used by the Library of Congress list.
async function readLanguages(): Promise<Language[]> {
  if(!existsSync(ISO_639_2_PATH)) {
    writeFileSync(ISO_639_2_PATH, await downloadText(ISO_639_2_URL));
  }

  const entries = JSON.parse(readFileSync(ISO_639_2_PATH, 'utf-8'))['639-2'] as Iso6392Entry[];
  const sortKey = (entry: Iso6392Entry) => entry.bibliographic ?? entry.alpha_3;

  return entries
    .filter(entry => entry.alpha_2 !== undefined)
    .sort((a, b) => sortKey(a) < sortKey(b) ? -1 : sortKey(a) > sortKey(b) ? 1 : 0)
    .map(entry => ({ code: entry.alpha_2 as string, name: entry.name }));
}

// Format a C string table, one entry per line, with the trailing comments aligned the way
// clang-format expects them.
function formatCodeTable(codes: string[], comments: string[]) {
  const literals = codes.map((code, i) => `"${code}"${i === codes.length - 1 ? '' : ','}`);
  const width = Math.max(...literals.map(literal => literal.length));

  return literals.map((literal, i) => `    ${literal.padEnd(width)} // ${comments[i]}`).join('\n');
}

export async function generateLocales(properties: Property[]) {
  const languages = await readLanguages();

  // mjb_locale: 0 is "no value", then one entry per ISO 639-1 language.
  const localeEnumNames = ['NOT_SET', ...languages.map(language => language.code.toUpperCase())];
  const localeComments = ['0 is "no value"', ...languages.map(language => language.name)];
  const localeCodes = ['', ...languages.map(language => language.code)];

  // Pad the entries so the trailing comments line up, and wrap the comments that would exceed
  // the column limit, the way clang-format expects them.
  const enumEntries = (prefix: string) => localeEnumNames.map((name, i) =>
    `${prefix}${name}${i === localeEnumNames.length - 1 ? '' : ','}`);
  const alignComments = (entries: string[], indent: string) => {
    const width = Math.max(...entries.map(entry => entry.length));
    const lines: string[] = [];

    entries.forEach((entry, i) => {
      const words = localeComments[i].split(' ');
      let line = `${indent}${entry.padEnd(width)} //`;

      for(const word of words) {
        if(line.length + 1 + word.length > COLUMN_LIMIT) {
          lines.push(line);
          line = `${indent}${''.padEnd(width)} //`;
        }

        line += ` ${word}`;
      }

      lines.push(line);
    });

    return lines.join('\n');
  };

  const localeEnums = alignComments(enumEntries('MJB_LOCALE_'), '    ');

  // Fill locales.h mjb_locale.
  let fileContent = readFileSync('../../src/locales.h', 'utf-8');

  fileContent = substituteBlock(fileContent, "typedef enum mjb_locale {\n", "\n} mjb_locale;", localeEnums);
  fileContent = substituteBlock(fileContent, '#define MJB_LOCALE_NUM ', "\n", '' + localeEnumNames.length);

  writeFileSync('../../src/locales.h', fileContent);

  // Fill locales.ts Locale.
  fileContent = readFileSync('../../src/api/locales.ts', 'utf-8');

  fileContent = substituteBlock(fileContent, "export enum Locale {\n", "\n};", alignComments(enumEntries(''), '  '));

  writeFileSync('../../src/api/locales.ts', fileContent);

  // mjb_script codes, in the same order as the unicode.h enum: 0 is "no value", the Script
  // property values, then the UTS #39 augmented scripts.
  const scriptProperty = properties.find(property => property.name === 'Script');

  if(!scriptProperty) {
    throw new Error('Script property not found');
  }

  const scriptCodes = ['', ...getScriptEnumNames(scriptProperty.values, key => key), 'Hanb', 'Jpan', 'Kore'];
  const scriptComments = scriptCodes.map(code => `MJB_SC_${code === '' ? 'NOT_SET' : code.toUpperCase()}`);

  // Fill locales.c code tables.
  fileContent = readFileSync('../../src/locales.c', 'utf-8');

  fileContent = substituteBlock(fileContent, "static const char *mjb_locale_codes[] = {\n", "\n};",
    formatCodeTable(localeCodes, localeEnumNames.map(name => `MJB_LOCALE_${name}`)));
  fileContent = substituteBlock(fileContent, "static const char *mjb_script_codes[] = {\n", "\n};",
    formatCodeTable(scriptCodes, scriptComments));

  writeFileSync('../../src/locales.c', fileContent);
}
