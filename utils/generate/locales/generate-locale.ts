/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import Database from 'better-sqlite3';
import { XMLParser } from 'fast-xml-parser';
import { createWriteStream, existsSync, statSync, unlinkSync } from 'fs';
import { readFile } from 'fs/promises';
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

export function generateDB(name: string, data: any) {
  const dbPath = `./locales/${name}.db`;

  if(existsSync(dbPath)) {
    console.log(`Removing old database '${dbPath}'`);

    unlinkSync(dbPath);
  }

  const db = new Database(dbPath);

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

  for(const table of tables) {
    db.exec(`
      CREATE TABLE IF NOT EXISTS ${table[0]} (
        id TEXT PRIMARY KEY,
        name TEXT NOT NULL
      );
    `);

    const insertSmt = db.prepare(`
      INSERT INTO ${table[0]} (id, name) VALUES (?, ?);
    `);

    const items = data.localeDisplayNames[table[0]];

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

      insertSmt.run(id, item['#text']);
    }
  }

  db.pragma('synchronous = OFF');
  db.pragma('journal_mode = MEMORY');
  db.pragma('temp_store = MEMORY');

  db.pragma('optimize');
  db.exec('ANALYZE;');
  db.exec('VACUUM;');

  console.log('Database generated successfully');

  const size = statSync(dbPath).size;
  console.log('Database size:', size, 'bytes');
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

  console.log('Generating database');
  generateDB(name, data);
}
