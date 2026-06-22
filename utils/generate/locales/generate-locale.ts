/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { XMLParser } from 'fast-xml-parser';
import { createWriteStream, existsSync, statSync, unlinkSync } from 'fs';
import { readFile, writeFile } from 'fs/promises';
import { get } from 'https';
import { pipeline } from 'stream';
import { promisify } from 'util';

interface LocaleKey {
  '@_type': string;
  '@_key': string | undefined;
  '@_scope': string | undefined;
  '@_alt': string | undefined;
  '@_draft': string | undefined;
  '@_menu': string | undefined;
  '#text': string;
}

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

export async function generateLocale(name: string) {
  const url = `https://raw.githubusercontent.com/unicode-org/cldr/refs/heads/main/common/main/${name}.xml`;
  const dest = `./locales/${name}.xml`;

  const streamPipeline = promisify(pipeline);

  return new Promise((resolve, reject) => {
    get(url, async (res) => {
      if(res.statusCode !== 200) {
        reject(new Error(`Failed to get '${url}' (${res.statusCode})`));
        res.resume();

        return;
      }

      const fileStream = createWriteStream(dest);

      try {
        await streamPipeline(res, fileStream);
        console.log(`Downloaded '${name}'`);
        await parseLocale(name);
        resolve(undefined);
      } catch(err) {
        reject(err);
      }
    }).on('error', reject);
  });
}

async function parseLocale(name: string) {
  const raw = await readFile(`./locales/${name}.xml`, 'utf-8');
  // const xml = raw.replace(/<!DOCTYPE[^>]*>/s, '');

  const parser = new XMLParser({
    ignoreAttributes: false,
  });

  const data = parser.parse(raw).ldml;

  console.log(data.identity.language['@_type']);
  console.log(data.localeDisplayNames.languages.language.length, 'languages');
  console.log(data.localeDisplayNames.scripts.script.length, 'scripts');
  console.log(data.localeDisplayNames.territories.territory.length, 'territories');
  console.log(data.localeDisplayNames.subdivisions.subdivision.length, 'subdivisions');
  console.log(data.localeDisplayNames.variants.variant.length, 'variants');
  console.log(data.localeDisplayNames.keys.key.length, 'keys');

  console.log('Generating locale data');
  await generateLocaleData(name, data);
}
