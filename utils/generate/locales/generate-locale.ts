/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { XMLParser } from 'fast-xml-parser';
import { existsSync, mkdirSync, statSync, unlinkSync } from 'fs';
import { readFile, writeFile } from 'fs/promises';
import { dirname } from 'path';
import { downloadFile, downloadText } from '../utils';
import { downloadLocaleIdData } from './locale-id';
import { CLDR_COMMON_URL, CLDR_MAIN_DIRECTORY_URL } from './utils';

interface LocaleKey {
  '@_type': string;
  '@_key': string | undefined;
  '@_scope': string | undefined;
  '@_alt': string | undefined;
  '@_draft': string | undefined;
  '@_menu': string | undefined;
  '#text': string;
}

interface AvailableLocales {
  locales: string[];
}

interface CldrTreeResponse {
  tree: CldrTreeEntry[];
  truncated?: boolean;
}

interface CldrTreeEntry {
  path: string;
  type: string;
}

const CLDR_DATA_DIR = './locales/cldr';
const AVAILABLE_LOCALES_PATH = `${CLDR_DATA_DIR}/available-locales.json`;

function normalizeLocaleName(name: string) {
  const parts = name.trim().replace(/-/g, '_').split('_').filter(part => part.length > 0);

  return parts.map((part, index) => {
    if(index === 0) {
      return part.toLowerCase();
    }

    if(/^[A-Za-z]{4}$/.test(part)) {
      return part[0].toUpperCase() + part.slice(1).toLowerCase();
    }

    if(/^[A-Za-z]{2}$/.test(part) || /^[0-9]{3}$/.test(part)) {
      return part.toUpperCase();
    }

    return part.toUpperCase();
  }).join('_');
}

function parseAvailableLocales(raw: string): AvailableLocales {
  const data: unknown = JSON.parse(raw);

  if(
    typeof data !== 'object' ||
    data === null ||
    !Array.isArray((data as AvailableLocales).locales)
  ) {
    throw new Error(`Invalid available locale data in '${AVAILABLE_LOCALES_PATH}'`);
  }

  const locales = (data as AvailableLocales).locales;

  if(!locales.every(locale => typeof locale === 'string')) {
    throw new Error(`Invalid available locale entry in '${AVAILABLE_LOCALES_PATH}'`);
  }

  return { locales };
}

async function generateAvailableLocales(): Promise<AvailableLocales> {
  console.log(`Downloading locales list from ${CLDR_MAIN_DIRECTORY_URL}`);
  const raw = await downloadText(CLDR_MAIN_DIRECTORY_URL);
  const response: unknown = JSON.parse(raw);

  if(
    typeof response !== 'object' ||
    response === null ||
    !Array.isArray((response as CldrTreeResponse).tree)
  ) {
    throw new Error('Unable to generate available locales: unexpected CLDR tree response');
  }

  if((response as CldrTreeResponse).truncated === true) {
    throw new Error('Unable to generate available locales: CLDR tree response was truncated');
  }

  const locales = (response as CldrTreeResponse).tree
    .filter(entry =>
      entry.type === 'blob' &&
      entry.path.startsWith('common/main/') &&
      entry.path.endsWith('.xml'))
    .map(entry => entry.path.slice('common/main/'.length, -4))
    .sort();

  if(locales.length === 0) {
    throw new Error('Unable to generate available locales: no CLDR main locale XML files found');
  }

  const data = { locales };
  mkdirSync(dirname(AVAILABLE_LOCALES_PATH), { recursive: true });

  await writeFile(AVAILABLE_LOCALES_PATH, JSON.stringify(data, null, 2) + '\n');
  console.log(`Generated '${AVAILABLE_LOCALES_PATH}' with`, locales.length, 'locales');

  return data;
}

// Get the list of available locales from the CLDR repository, either from a local JSON file or by
// generating it from the CLDR tree API.
async function availableLocales(): Promise<AvailableLocales> {
  if(existsSync(AVAILABLE_LOCALES_PATH)) {
    console.log(`Reading available locales from '${AVAILABLE_LOCALES_PATH}'`);
    return parseAvailableLocales(await readFile(AVAILABLE_LOCALES_PATH, 'utf-8'));
  }

  return generateAvailableLocales();
}

// Generate a JSON file from the CLDR locale XML data for the specified locale name.
export async function generateLocaleData(name: string, data: any) {
  const outputPath = `./locales/${name}.json`;

  if(existsSync(outputPath)) {
    console.log(`Removing old locale data '${outputPath}'`);

    unlinkSync(outputPath);
  }

  const tables = [
    ['languages', 'language'],
    ['scripts', 'script'],
    ['territories', 'territory'],
    ['subdivisions', 'subdivision'],
    ['variants', 'variant'],
    ['keys', 'key'],
    ['types', 'type'],
    ['measurementSystemNames', 'measurementSystemName'],
    ['codePatterns', 'codePattern'],
  ];
  const localeData: Record<string, Record<string, string>> = {};

  for(const table of tables) {
    const items = data.localeDisplayNames[table[0]];
    const rows: Record<string, string> = {};

    for(const item of items[table[1]] as LocaleKey[]) {
      let id = item['@_type'];

      if(item['@_key']) {
        id += `-key-${item['@_key']}`;
      }

      if(item['@_draft']) {
        id += `-draft-${item['@_draft']}`;
      }

      if(item['@_alt']) {
        id += `-alt-${item['@_alt']}`;
      }

      if(item['@_menu']) {
        id += `-menu-${item['@_menu']}`;
      }

      if(item['@_scope']) {
        id += `-scope-${item['@_scope']}`;
      }

      rows[id] = item['#text'];
    }

    localeData[table[0]] = rows;
  }

  await writeFile(outputPath, JSON.stringify(localeData, null, 2));
  console.log('Locale data generated successfully');

  const size = statSync(outputPath).size;
  console.log('Locale data size:', size, 'bytes');
}

async function parseLocale(name: string) {
  const raw = await readFile(`./locales/${name}.xml`, 'utf-8');
  // const xml = raw.replace(/<!DOCTYPE[^>]*>/s, '');

  const parser = new XMLParser({
    ignoreAttributes: false,
  });

  const data = parser.parse(raw).ldml;

  console.log(data.identity.language['@_type'], 'identifier');
  console.log(data.localeDisplayNames.languages.language.length, 'languages');
  console.log(data.localeDisplayNames.scripts.script.length, 'scripts');
  console.log(data.localeDisplayNames.territories.territory.length, 'territories');
  console.log(data.localeDisplayNames.subdivisions.subdivision.length, 'subdivisions');
  console.log(data.localeDisplayNames.variants.variant.length, 'variants');
  console.log(data.localeDisplayNames.keys.key.length, 'keys');

  console.log('Generating locale data');
  await generateLocaleData(name, data);
}

export async function generateLocale(name: string) {
  // This will download the CLDR locale ID data files if they don't already exist.
  await downloadLocaleIdData();

  // E.g. en-US to en_US
  const localeName = normalizeLocaleName(name);
  // Get the list of available locales from the CLDR repository.
  const locales = await availableLocales();

  if(!locales.locales.includes(localeName)) {
    throw new Error(`Unknown CLDR locale '${name}'`);
  }

  const url = `${CLDR_COMMON_URL}/main/${localeName}.xml`;
  const dest = `./locales/${localeName}.xml`;

  console.log(`Downloading '${localeName}'`);
  await downloadFile(url, dest);

  console.log(`Parsing '${localeName}'`);
  await parseLocale(localeName);
}
