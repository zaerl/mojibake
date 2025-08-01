import Database, { Statement } from 'better-sqlite3';
import { statSync } from 'fs';
import { Character } from './character';
import { Block, CalculatedDecomposition } from './types';

let db: Database.Database;
let dbPath: string;
let insertDataSmt: Statement;
let insertDecompositionSmt: Statement;
let insertCompatDecompositionSmt: Statement;
let insertBlockSmt: Statement;
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
        name TEXT NOT NULL,
        flags INTEGER NOT NULL, -- category, combining, bidirectional, decomposition, decimal, digit, mirrored, block
        numeric TEXT,
        uppercase INTEGER,
        lowercase INTEGER,
        titlecase INTEGER
      );
      CREATE INDEX idx_unicode_data_codepoint ON unicode_data(codepoint);
    `);
  } else {
    db.exec(`
      CREATE TABLE IF NOT EXISTS unicode_data (
        codepoint INTEGER PRIMARY KEY,
        name TEXT NOT NULL,
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
        titlecase INTEGER
      );
      CREATE INDEX idx_unicode_data_codepoint ON unicode_data(codepoint);
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
        titlecase
      ) VALUES (?, ?, ?, ?, ?, ?, ?);
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
        titlecase
      ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
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

  insertBlockSmt = db.prepare(`
    INSERT INTO blocks (
      id,
      start,
      end,
      name
    ) VALUES (?, ?, ?, ?);
  `);

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
        char.titlecase
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
        char.titlecase
      );
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

export function dbInsertBlock(index: number, block: Block) {
  insertBlockSmt.run(index, block.start, block.end, block.name);
}

export function dbRunAfter() {
  db.pragma('optimize');
  db.exec('ANALYZE;');
  db.exec('VACUUM;');
}
