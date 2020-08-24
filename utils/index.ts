import { createReadStream, writeFileSync, unlinkSync, existsSync } from 'fs';
import { createInterface } from 'readline';
import { verbose, Statement } from 'sqlite3';

interface Buffer {
  name: string;
  count: number;
}

interface Numeric {
  name: string;
  value: number;
  count: number;
}

enum Category {
  Lu,
  Ll,
  Lt,
  Lm,
  Lo,
  Mn,
  Mc,
  Me,
  Nd,
  Nl,
  No,
  Pc,
  Pd,
  Ps,
  Pe,
  Pi,
  Pf,
  Po,
  Sm,
  Sc,
  Sk,
  So,
  Zs,
  Zl,
  Zp,
  Cc,
  Cf,
  Cs,
  Co,
  Cn
}

const categories = [
  'Letter, Uppercase',
  'Letter, Lowercase',
  'Letter, Titlecase',
  'Letter, Modifier',
  'Letter, Other',
  'Mark, Non-Spacing',
  'Mark, Spacing Combining',
  'Mark, Enclosing',
  'Number, Decimal Digit',
  'Number, Letter',
  'Number, Other',
  'Punctuation, Connector',
  'Punctuation, Dash',
  'Punctuation, Open',
  'Punctuation, Close',
  'Punctuation, Initial quote',
  'Punctuation, Final quote',
  'Punctuation, Other',
  'Symbol, Math',
  'Symbol, Currency',
  'Symbol, Modifier',
  'Symbol, Other',
  'Separator, Space',
  'Separator, Line',
  'Separator, Paragraph',
  'Other, Control',
  'Other, Format',
  'Other, Surrogate',
  'Other, Private Use',
  'Other, Not Assigned',
];

type CategoriesStrings = keyof typeof Category;

enum BidirectionalCategories {
  L,
  LRE,
  LRO,
  R,
  AL,
  RLE,
  RLO,
  PDF,
  EN,
  ES,
  ET,
  AN,
  CS,
  NSM,
  BN,
  B,
  S,
  WS,
  ON
};

type BidirectionalCategoriesStrings = (keyof typeof BidirectionalCategories) | '';

enum CharacterDecompositionMapping {
  '<font>',
  '<noBreak>',
  '<initial>',
  '<medial>',
  '<final>',
  '<isolated>',
  '<circle>',
  '<super>',
  '<sub>',
  '<vertical>',
  '<wide>',
  '<narrow>',
  '<small>',
  '<square>',
  '<fraction>',
  '<compat>'
};

type CharacterDecompositionMappingStrings = (keyof typeof CharacterDecompositionMapping) | '';

type CharacterDecomposition = [
  CharacterDecompositionMappingStrings,
  ...string[]
];

type Mirrored = 'Y' | 'N';

type UnicodeDataRow = [
  string, // 0 codepoint
  string, // 1 character name
  // block
  CategoriesStrings, // 2 category
  string, // 3 canonical combining classes
  BidirectionalCategoriesStrings, // 4 bidirectional category
  string, // 5 character decomposition mapping
  string, // 6 decimal digit value
  string, // 7 digit value
  string, // 8 numeric value
  Mirrored, // 9 mirrored
  string, // 10 unicode 1.0 name
  string, // 11 10646 comment field
  string, // 12 uppercase mapping
  string, // 13 lowercase mapping
  string // 14 titlecase mapping
];

// All blocks
interface Block {
  name: string;
  enumName: string;
  start: number;
  end: number;
};

function log(message?: any, ...optionalParams: any[]) {
  console.log(message, ...optionalParams);
}

function compareFn(a: Buffer, b: Buffer): number {
  const ret = b.count - a.count;

  if(ret === 0) {
    if(a.name < b.name) {
      return -1;
    } else if(a.name > b.name) {
      return 1;
    }

    return 0;
  }

  return ret;
}

function header(name: string): string {
  name = name.toUpperCase();

  return `${license()}

#ifndef MJB_${name}_H
#define MJB_${name}_H`;
}

function footer(name: string): string {
  name = name.toUpperCase();

  return `#endif /* MJB_${name}_H */`;
}

function license(): string {
  return `/**
 * The mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */`;
}

async function readBlocks(stmt: Statement): Promise<Block[]> {
  log('READ BLOCKS');

  const blocks: Block[] = [];

  const rl = createInterface({
    input: createReadStream('./UCD/Blocks.txt'),
    crlfDelay: Infinity
  });

  let i = 0;

  for await (const line of rl) {
    if(line.startsWith('#') || line === '') { // Comment
      continue;
    }

    const split = line.split('; ');
    const name = split[1];
    const values = split[0].split('..');
    const start = parseInt(values[0], 16);
    const end = parseInt(values[1], 16);

    blocks.push({
      name,
      enumName: `MJB_BLOCK_${split[1].toUpperCase().replace(/[ \-]/g, '_')}`,
      start,
      end
    });

    stmt.run(
      split[1],
      start,
      end
    );
  }

  stmt.finalize();

  return blocks;
}

async function readUnicodeData(stmt: Statement, blocks: Block[]) {
  log('READ UNICODE DATA');

  const nameBuffer: { [name: string]: number } = {};
  const categoryBuffer: { [name: string]: number } = {};
  let charsCount = 0;
  let wordsCount = 0;
  let hasNumber: { [name: string]: number } = {};
  let previousCodepoint = 0;
  let diffs = 0;
  let codepoint = 0;
  let currentBlock = 0;
  let maxDecomposition = 0;

  const rl = createInterface({
    input: createReadStream('./UCD/UnicodeData.txt'),
    crlfDelay: Infinity
  });

  for await (const line of rl) {
    const split = line.split(';') as UnicodeDataRow;
    const name = split[2] === 'Cc' && split[10] !== '' ? split[10] : split[1];
    const words = name.split(' ');
    codepoint = parseInt(split[0], 16);
    const diff = codepoint - previousCodepoint;

    if(diff > 1) {
      diffs += diff;
      // log(`STEP (${split[0]} -- ${codepoint - previousCodepoint})`);
    }

    previousCodepoint = codepoint;
    charsCount += name.length;
    wordsCount += words.length;

    if(split[8] !== '') {
      if(typeof(hasNumber[split[8]]) === 'undefined') {
        hasNumber[split[8]] = 1;
      } else {
        ++hasNumber[split[8]];
      }
    }

    const decomposition = split[5].split(' ') as CharacterDecomposition;
    maxDecomposition = Math.max(maxDecomposition, decomposition.length);

    if(decomposition.length === 19) {
      log(codepoint, name, decomposition);
    }

    if(codepoint > blocks[currentBlock].end) {
      ++currentBlock;
    }

    stmt.run(
      codepoint, // 0
      name, // 1
      currentBlock, // Block
      1 << Category[split[2]],
      parseInt(split[3], 10),
      split[4] === '' ? null : BidirectionalCategories[split[4]],
      decomposition[0] === '' ? null : CharacterDecompositionMapping[decomposition[0]],

      split[6] === '' ? null : split[6],
      split[7] === '' ? null : split[7],
      split[8] === '' ? null : split[8],

      split[9] === 'Y' ? 1 : 0,
      // unicode 1.0 name
      // 10646 comment field
      split[12] === '' ? null : parseInt(split[12], 16),
      split[13] === '' ? null : parseInt(split[13], 16),
      split[14] === '' ? null : parseInt(split[14], 16)
    );

    for(const word of words) {
      if(typeof(nameBuffer[word]) === 'undefined') {
        nameBuffer[word] = 1;
      } else {
        ++nameBuffer[word];
      }

      if(typeof(categoryBuffer[split[2]]) === 'undefined') {
        categoryBuffer[split[2]] = 1;
      } else {
        ++categoryBuffer[split[2]];
      }
    }
  };

  stmt.finalize();

  log(`MAX DECOMPOSITION ${maxDecomposition}\n`);

  log(`STEP TOTAL ${diffs}/${codepoint}\n`);

  const ret: Buffer[] = [];
  const ret2: Buffer[] = [];

  for(const name in categoryBuffer) {
    ret2.push({ name, count: categoryBuffer[name] });
  }

  log('CATEGORIES\n');

  ret2.sort(compareFn);
  let buffer: string[] = [];
  let prevCount = ret2[0].count;

  for(const entry of ret2) {
    buffer.push(entry.name);

    if(entry.count !== prevCount) {
      log(`${entry.count} :: ${buffer.join(', ')}`);

      prevCount = entry.count;
      buffer = [];
    }
  }

  log('\nWORDS\n');

  for(const name in nameBuffer) {
    ret.push({ name, count: nameBuffer[name] });
  }

  ret.sort(compareFn);
  buffer = [];
  prevCount = ret[0].count;

  for(const entry of ret) {
    buffer.push(entry.name);

    if(entry.count !== prevCount) {
      const line = buffer.length > 10 ? buffer.slice(0, 10).join(', ') + '...' : buffer.join(', ');

      log(`${entry.count} :: ${line}`);

      prevCount = entry.count;
      buffer = [];
    }
  }

  log('\nNUMBERS\n');

  const numbersBuffer: Numeric[] = [];

  for(const name in hasNumber) {
    const values = name.split('/'); // Check if it's a fraction
    const value = values.length === 1 ? parseFloat(values[0]) :
      Math.floor((parseFloat(values[0]) / parseFloat(values[1])) * 100) / 100;
    const count = hasNumber[name];

    numbersBuffer.push({ name, value, count });
  }

  numbersBuffer.sort((a: Numeric, b: Numeric) => b.count - a.count);

  for(const num of numbersBuffer) {
    log(`${num.name} (${num.value}): ${num.count}`);
  }

  log('\nCOUNT\n');
  log(`${wordsCount.toLocaleString()} words (${(wordsCount * 5).toLocaleString()} bytes)`);
  log(`${charsCount.toLocaleString()} characters (${(charsCount).toLocaleString()} bytes)`);
}

const dbName = '../src/mojibake.db';
const sqlite = verbose();

// Remove old database
if(existsSync(dbName)) {
  unlinkSync(dbName);
}

const db = new sqlite.Database(dbName, (err: Error | null) => {
  console.error(err?.message);
});

db.serialize(async () => {

  db.run('BEGIN TRANSACTION');

  db.run(
`CREATE TABLE characters(
  codepoint INTEGER NOT NULL PRIMARY KEY,
  name TEXT NOT NULL,
  block INTEGER NOT NULL,
  category INTEGER NOT NULL,
  combining INTEGER NOT NULL,
  bidirectional INTEGER,
  decomposition INTEGER,
  decimal TEXT,
  digit TEXT,
  numeric TEXT,
  mirrored INTEGER,
  uppercase INTEGER,
  lowercase INTEGER,
  titlecase INTEGER
) WITHOUT ROWID`);

db.run(
`CREATE TABLE blocks(
  name TEXT NOT NULL,
  start INTEGER NOT NULL,
  end INTEGER NOT NULL
)`);

  const blockStmt = db.prepare('INSERT INTO blocks VALUES (?, ?, ?)');
  const dataStmt = db.prepare('INSERT INTO characters VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)');

  const blocks = await readBlocks(blockStmt);
  await readUnicodeData(dataStmt, blocks);

  const categoryEnums: string[] = [];

  for(let i = 0; i < categories.length; ++i) {
    categoryEnums.push(`    MJB_CATEGORY_${Category[i].toUpperCase()} = 0x${(1 << i).toString(16)}${ i === categories.length - 1 ? '' : ','} /* ${i} (${Category[i]}) ${categories[i]} */`);
  }

  const fheader =
`${header('mojibake')}

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MJB_VERSION "1.0.0"
#define MJB_VERSION_NUMBER 0x100 /* MAJOR << 8 && MINOR << 4 && REVISION */
#define MJB_VERSION_MAJOR 1
#define MJB_VERSION_MINOR 0
#define MJB_VERSION_REVISION 0

#define MJB_UNICODE_VERSION "13.0"
#define MJB_UNICODE_VERSION_MAJOR 13
#define MJB_UNICODE_VERSION_MINOR 0

#ifndef MJB_EXTERN
#define MJB_EXTERN extern
#endif

#ifndef MJB_EXPORT
#define MJB_EXPORT __attribute__((visibility("default")))
#endif

/* See c standard memory allocation functions */
typedef void* (*mjb_alloc)(size_t size);
typedef void* (*mjb_realloc)(void* ptr, size_t new_size);
typedef void (*mjb_free)(void* ptr);

/*
 A unicode codepoint
 [see: https://www.unicode.org/glossary/#code_point]
 */
typedef uint32_t mjb_codepoint;

#define MJB_CODEPOINT_MIN 0x0
#define MJB_CODEPOINT_MAX 0x10FFFF /* Maximum valid unicode code point */
#define MJB_CODEPOINT_REPLACEMENT 0xFFFD /* The character used when there is invalid data */

#define MJB_BLOCK_NUM ${blocks.length}

typedef enum mjb_block_name {
${blocks.map((value: Block, index: number) => `    ${value.enumName} = ${index}`).join(',\n')}
} mjb_block_name;

/*
 Unicode block
 [see: https://www.unicode.org/glossary/#block]
*/
typedef struct mjb_block {
    char* name;
    uint32_t start;
    uint32_t end;
} mjb_block;

#define MJB_CATEGORY_COUNT ${categoryEnums.length}

/*
 Unicode codepoint general category
 [see: https://www.unicode.org/glossary/#general_category]
 */
typedef enum mjb_category {
${categoryEnums.join('\n')}
} mjb_category;

/*
 A unicode character
 [see: https://www.unicode.org/glossary/#character]
 */
typedef struct mjb_character {
    mjb_codepoint codepoint;
    unsigned char name[128];
    unsigned short block;
    mjb_category category;
    unsigned short combining;
    unsigned short bidirectional;
    unsigned short decomposition;
    unsigned char decimal[128];
    unsigned char digit[128];
    unsigned char numeric[128];
    bool mirrored;
    mjb_codepoint uppercase;
    mjb_codepoint lowercase;
    mjb_codepoint titlecase;
} mjb_character;

/*
 Unicode plane
 [see: https://www.unicode.org/glossary/#plane]
*/
typedef uint8_t mjb_plane;

#define MJB_PLANE_NUM 17 /* 17 planes */
#define MJB_PLANE_SIZE 65536 /* 2^16 code points per plane */

/*
 Unicode encoding
 [see: https://www.unicode.org/glossary/#character_encoding_scheme]
 */
typedef enum mjb_encoding {
    MJB_ENCODING_UNKNOWN = 0,
    MJB_ENCODING_ASCII = 0x1,
    MJB_ENCODING_UTF_8 = 0x2,
    MJB_ENCODING_UTF_16 = 0x4,
    MJB_ENCODING_UTF_16_BE = 0x8,
    MJB_ENCODING_UTF_16_LE = 0x10,
    MJB_ENCODING_UTF_32 = 0x20,
    MJB_ENCODING_UTF_32_BE = 0x40,
    MJB_ENCODING_UTF_32_LE = 0x80
} mjb_encoding;

/*
 Normalization form
 https://www.unicode.org/glossary/#normalization_form
*/
typedef enum mjb_normalization {
    MJB_NORMALIZATION_NFD = 0,
    MJB_NORMALIZATION_NFC = 1,
    MJB_NORMALIZATION_NFKD = 2,
    MJB_NORMALIZATION_NFKC = 3
} mjb_normalization;

/* Initialize the library */
bool mjb_initialize(const char* filename);

/* The library is ready */
bool mjb_ready();

/* Close the library */
bool mjb_close();

/* Output the current library version (MJB_VERSION) */
char* mjb_version();

/* Output the current library version number (MJB_VERSION_NUMBER) */
unsigned int mjb_version_number();

/* Output the current supported unicode version (MJB_UNICODE_VERSION) */
char* mjb_unicode_version();

/* Return true if the codepoint is valid */
bool mjb_codepoint_is_valid(mjb_codepoint codepoint);

/* Return true if the plane is valid */
bool mjb_plane_is_valid(mjb_plane plane);

/* Return the name of a plane, NULL if the place specified is not valid */
const char* mjb_plane_name(mjb_plane plane, bool abbreviation);

/* Return the string encoding (the most probable) */
mjb_encoding mjb_string_encoding(const char* buffer, size_t size);

/* Return true if the string is encoded in UTF-8 */
bool mjb_string_is_utf8(const char* buffer, size_t size);

/* Return true if the string is encoded in ASCII */
bool mjb_string_is_ascii(const char* buffer, size_t size);

/* Return the codepoint character */
bool mjb_codepoint_character(mjb_character* character, mjb_codepoint codepoint);

/* Return true if the codepoint has the category */
bool mjb_codepoint_is(mjb_codepoint codepoint, mjb_category category);

/* Return true if the codepoint is graphic */
bool mjb_codepoint_is_graphic(mjb_codepoint codepoint);

/* Return the codepoint lowercase codepoint */
mjb_codepoint mjb_codepoint_to_lowercase(mjb_codepoint codepoint);

/* Return the codepoint uppercase codepoint */
mjb_codepoint mjb_codepoint_to_uppercase(mjb_codepoint codepoint);

/* Return the codepoint titlecase codepoint */
mjb_codepoint mjb_codepoint_to_titlecase(mjb_codepoint codepoint);

/* Normalize a string */
void mjb_normalize(const char* buffer, size_t size, mjb_normalization form);

#ifdef __cplusplus
}
#endif

${footer('mojibake')}
`;

  writeFileSync('../src/mojibake.h', fheader);

  db.run('END');

  db.close();
});
