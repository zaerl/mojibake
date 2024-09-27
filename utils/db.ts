import Database, { Statement } from 'better-sqlite3';
import { statSync } from 'fs';
import { Character } from './character';

let db: Database.Database;
let dbPath: string;
let insertDataSmt: Statement;
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

// Block (additional)
// 00000000 00000000 00000001 01010010 block (338)

// 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
// block             mir      dig dec  deco     bidi     comb     cat
export function dbInit(path = '../build/mojibake.db', compact = false) {
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
        titlecase INTEGER,
        block INTEGER NOT NULL -- Additional
      );
    `);
  }

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
        titlecase,
        block -- Additional
      ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
    `);
  }

  // Persistent
  db.pragma('journal_mode = OFF');
  db.pragma('temp_store = MEMORY');
  db.pragma('cache_size = -1000000');

  // Not persistent
  /*db.pragma('synchronous = OFF');
  db.pragma('locking_mode = EXCLUSIVE');
  db.pragma('mmap_size = 30000000000');*/

  db.exec('ANALYZE;');
  db.exec('VACUUM;');
}

export function dbSize() {
  return statSync(dbPath).size;
}

export function dbRun(char: Character) {
  if(isCompact) {
    // 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
    //  block             mir     dig dec  deco     bidi     comb     cat
    const flags = BigInt(char.category) |
      (BigInt(char.combining) << BigInt(8)) |
      (BigInt(char.bidirectional ?? 0) << BigInt(16)) |
      (BigInt(char.decomposition) << BigInt(24)) |
      ((BigInt(char.decimal ?? BigInt(0))) << BigInt(32)) |
      ((BigInt(char.digit ?? BigInt(0))) << BigInt(36)) |
      (char.mirrored ? (BigInt(1) << BigInt(47)) : BigInt(0)) |
      (BigInt(char.block) << BigInt(48));

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
      char.titlecase,
      char.block
    );
  }
}
