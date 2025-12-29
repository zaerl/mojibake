/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import Database, { Statement } from 'better-sqlite3';
import { statSync } from 'fs';
import { Character } from './character';
import { Emoji } from './emoji';
import { NewCases } from './special-casing';
import { Block, CalculatedDecomposition, CaseType, Composition } from './types';

let db: Database.Database;
let dbPath: string;
let insertDataSmt: Statement;
let insertDecompositionSmt: Statement;
let insertCompatDecompositionSmt: Statement;
let insertCompositionSmt: Statement;
let insertBlockSmt: Statement;
let insertSpecialCasingSmt: Statement;
let insertEmojiPropertiesSmt: Statement;
// let insertNumericSmt: Statement;
let isCompact: boolean;

// Codepoint
// 00000000 00010000 11111111 11111111 max codepoint (1114111)

// Categories
// 00000000 00000000 00000000 00011110 category (30)
// 00000000 00000000 00000000 00111110 combining (62)
// 00000000 00000000 00000000 00011000 bidirectional (24)
// 00000000 00000000 00000000 00010000 decomposition (16)

// Numbers
// 00000000 00000000 00000000 00001001 decimal (9)
// 00000000 00000000 00000000 00001001 digit (9)

// 00000000 00000000 00000000 00000001 mirrored (1)

// 00000000 00010000 11111111 11111111 uppercase mapping (1114111)
// 00000000 00010000 11111111 11111111 lowercase mapping (1114111)
// 00000000 00010000 11111111 11111111 titlecase mapping (1114111)

// 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
// block             mir      dig dec  deco     bidi     comb     cat
export function dbInit(path = '../../mojibake.db', compact = false) {
  dbPath = path;
  db = new Database(dbPath);
  isCompact = compact;

  if(isCompact) {
    db.exec(`
      CREATE TABLE IF NOT EXISTS unicode_data (
        codepoint INTEGER PRIMARY KEY,
        name TEXT,
        flags INTEGER NOT NULL, -- category, combining, bidirectional, decomposition, decimal, digit, mirrored, block
        numeric TEXT,
        uppercase INTEGER,
        lowercase INTEGER,
        titlecase INTEGER,
        quick_check INTEGER,
        line_breaking_class INTEGER,
        east_asian_width INTEGER,
        extended_pictographic INTEGER -- emoji properties
      );
    `);
  } else {
    db.exec(`
      CREATE TABLE IF NOT EXISTS unicode_data (
        codepoint INTEGER PRIMARY KEY,
        name TEXT,
        category INTEGER NOT NULL,
        combining INTEGER,
        bidirectional INTEGER,
        decomposition INTEGER,
        decimal INTEGER,
        digit INTEGER,
        numeric TEXT,
        mirrored INTEGER,
        -- unicode 1.0 name
        -- 10646 comment
        uppercase INTEGER,
        lowercase INTEGER,
        titlecase INTEGER,
        quick_check INTEGER,
        line_breaking_class INTEGER,
        east_asian_width INTEGER,
        extended_pictographic INTEGER -- emoji properties
      );
    `);
  }

  db.exec(`
    CREATE TABLE IF NOT EXISTS decompositions (
      id INTEGER NOT NULL,
      value INTEGER NOT NULL
    );
    CREATE INDEX idx_decompositions_id ON decompositions(id)
  `);

  db.exec(`
    CREATE TABLE IF NOT EXISTS compatibility_decompositions (
      id INTEGER NOT NULL,
      value INTEGER NOT NULL
    );
    CREATE INDEX idx_compatibility_decompositions_id ON compatibility_decompositions(id)
  `);

  db.exec(`
    CREATE TABLE IF NOT EXISTS blocks (
      id INTEGER PRIMARY KEY,
      start INTEGER NOT NULL,
      end INTEGER NOT NULL,
      name TEXT NOT NULL
    );
    CREATE INDEX idx_blocks_start_end ON blocks(start, end);
  `);

  db.exec(`
    CREATE TABLE IF NOT EXISTS compositions (
      starter_codepoint INTEGER NOT NULL,
      combining_codepoint INTEGER NOT NULL,
      composite_codepoint INTEGER NOT NULL,
      PRIMARY KEY (starter_codepoint, combining_codepoint)
    );
  `);

  db.exec(`
    CREATE TABLE IF NOT EXISTS special_casing (
      codepoint INTEGER NOT NULL,
      case_type INTEGER NOT NULL,
      new_case_1 INTEGER NOT NULL,
      new_case_2 INTEGER,
      new_case_3 INTEGER,
      PRIMARY KEY (codepoint, case_type)
    );
  `);
  db.exec(`
    CREATE TABLE IF NOT EXISTS emoji_properties (
      codepoint INTEGER PRIMARY KEY,
      emoji INTEGER,
      emoji_presentation INTEGER,
      emoji_modifier INTEGER,
      emoji_modifier_base INTEGER,
      emoji_component INTEGER,
      extended_pictographic INTEGER
    );
  `);

  /*db.exec(`
    CREATE TABLE IF NOT EXISTS numerics (
      codepoint INTEGER PRIMARY KEY,
      decimal INTEGER,
      digit INTEGER,
      numeric TEXT
    );
    CREATE INDEX idx_numerics_codepoint ON numerics(codepoint);
  `);*/

  process.on('exit', () => db.close());
  process.on('SIGHUP', () => process.exit(128 + 1));
  process.on('SIGINT', () => process.exit(128 + 2));
  process.on('SIGTERM', () => process.exit(128 + 15));

  if(isCompact) {
    insertDataSmt = db.prepare(`
      INSERT INTO unicode_data (
        codepoint,
        name,
        flags,
        numeric,
        uppercase,
        lowercase,
        titlecase,
        quick_check,
        line_breaking_class,
        east_asian_width,
        extended_pictographic
      ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
    `);
  } else {
    insertDataSmt = db.prepare(`
      INSERT INTO unicode_data (
        codepoint,
        name,
        category,
        combining,
        bidirectional,
        decomposition,
        decimal,
        digit,
        numeric,
        mirrored,
        -- unicode 1.0 name
        -- 10646 comment
        uppercase,
        lowercase,
        titlecase,
        quick_check,
        line_breaking_class,
        east_asian_width,
        extended_pictographic
      ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
    `);
  }

  insertDecompositionSmt = db.prepare(`
    INSERT INTO decompositions (
      id,
      value
    ) VALUES (?, ?);
  `);

  insertCompatDecompositionSmt = db.prepare(`
    INSERT INTO compatibility_decompositions (
      id,
      value
    ) VALUES (?, ?);
  `);

  insertCompositionSmt = db.prepare(`
    INSERT INTO compositions (
      starter_codepoint,
      combining_codepoint,
      composite_codepoint
    ) VALUES (?, ?, ?);
  `);

  insertBlockSmt = db.prepare(`
    INSERT INTO blocks (
      id,
      start,
      end,
      name
    ) VALUES (?, ?, ?, ?);
  `);

  insertSpecialCasingSmt = db.prepare(`
    INSERT INTO special_casing (
      codepoint,
      case_type,
      new_case_1,
      new_case_2,
      new_case_3)
      VALUES (?, ?, ?, ?, ?);
  `);

  insertEmojiPropertiesSmt = db.prepare(`
    INSERT INTO emoji_properties (
      codepoint,
      emoji,
      emoji_presentation,
      emoji_modifier,
      emoji_modifier_base,
      emoji_component,
      extended_pictographic
    ) VALUES (?, ?, ?, ?, ?, ?, ?);
  `);

  /*insertNumericSmt = db.prepare(`
    INSERT INTO numerics (
      codepoint,
      decimal,
      digit,
      numeric
    ) VALUES (?, ?, ?, ?);
  `);*/

  db.pragma('synchronous = OFF');
  db.pragma('journal_mode = MEMORY');
  db.pragma('temp_store = MEMORY');
}

export function dbSize() {
  return statSync(dbPath).size;
}

export function dbRun(characters: Character[]) {
  for(const char of characters) {
    if(isCompact) {
      // 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
      //                    mir     dig dec  deco     bidi     comb     cat
      const flags = BigInt(char.category) |
        (BigInt(char.combining) << BigInt(8)) |
        (BigInt(char.bidirectional ?? 0) << BigInt(16)) |
        (BigInt(char.decomposition) << BigInt(24)) |
        ((BigInt(char.decimal ?? BigInt(0))) << BigInt(32)) |
        ((BigInt(char.digit ?? BigInt(0))) << BigInt(36)) |
        (char.mirrored ? (BigInt(1) << BigInt(47)) : BigInt(0));

      insertDataSmt.run(
        char.codepoint,
        char.name,
        flags,
        char.numeric,
        char.uppercase,
        char.lowercase,
        char.titlecase,
        char.quickCheck,
        char.lineBreakingClass,
        char.eastAsianWidth,
        char.extendedPictographic ? 1 : 0
        );
    } else {
      insertDataSmt.run(
        char.codepoint,
        char.name,
        char.category,
        char.combining,
        char.bidirectional,
        char.decomposition,
        char.decimal,
        char.digit,
        char.numeric,
        char.mirrored ? 1 : 0,
        char.uppercase,
        char.lowercase,
        char.titlecase,
        char.quickCheck,
        char.lineBreakingClass,
        char.eastAsianWidth,
        char.extendedPictographic ? 1 : 0
      );

      /*if(char.decimal !== null || char.digit !== null || char.numeric !== null) {
        insertNumericSmt.run(
          char.codepoint,
          char.decimal,
          char.digit,
          char.numeric
        );
      }*/
    }
  }
}

export function dbRunDecompositions(decompositions: CalculatedDecomposition[], compat = false) {
  for(const value of decompositions) {
    if(compat) {
      insertCompatDecompositionSmt.run(value.codepoint, value.value);
    } else {
      insertDecompositionSmt.run(value.codepoint, value.value);
    }
  }
}

export function dbRunComposition(compositions: Composition[]) {
  for(const comp of compositions) {
    insertCompositionSmt.run(comp.starter_codepoint, comp.combining_codepoint, comp.composite_codepoint);
  }
}

export function dbRunSpecialCasing(newCases: NewCases) {
  for(let i = 0; i < newCases.length; ++i) {
    if(newCases[i].hasLowercase > 1) {
      insertSpecialCasingSmt.run(
        newCases[i].codepoint,
        CaseType.LowerCase,
        newCases[i].lowercase[0],
        newCases[i].lowercase[1],
        newCases[i].lowercase[2],
      );
    }

    if(newCases[i].hasTitlecase > 1) {
      insertSpecialCasingSmt.run(
        newCases[i].codepoint,
        CaseType.TitleCase,
        newCases[i].titlecase[0],
        newCases[i].titlecase[1],
        newCases[i].titlecase[2],
      );
    }

    if(newCases[i].hasUppercase > 1) {
      insertSpecialCasingSmt.run(
        newCases[i].codepoint,
        CaseType.UpperCase,
        newCases[i].uppercase[0],
        newCases[i].uppercase[1],
        newCases[i].uppercase[2],
      );
    }
  }
}

export function dbInsertBlock(index: number, block: Block) {
  insertBlockSmt.run(index, block.start, block.end, block.name);
}

export function dbRunEmojiProperties(emojiProperties: Emoji[]) {
  for(const emoji of emojiProperties) {
    insertEmojiPropertiesSmt.run(
      emoji.codepoint,
      emoji.emoji ? 1 : 0,
      emoji.emoji_presentation ? 1 : 0,
      emoji.emoji_modifier ? 1 : 0,
      emoji.emoji_modifier_base ? 1 : 0,
      emoji.emoji_component ? 1 : 0,
      emoji.extended_pictographic ? 1 : 0
    );
  }
}

export function dbRunAfter() {
  db.pragma('optimize');
  db.exec('ANALYZE;');
  db.exec('VACUUM;');
}
