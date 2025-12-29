/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { constants } from 'fs';
import { access, open, unlink } from 'fs/promises';
import { Analysis } from './analysis';
import { readBlocks } from './blocks';
import { generateCasefold } from './casefold';
import { Character } from './character';
import { readCompositionExclusions } from './compositition-exclusion';
import {
  dbInit, dbRun, dbRunAfter, dbRunComposition, dbRunDecompositions, dbRunEmojiProperties,
  dbRunSpecialCasing, dbSize
} from './db';
import { characterDecomposition, generateComposition, generateDecomposition } from './decomposition';
import { generateAmalgamation } from './generate-amalgamation';
import { generateAPI } from './generate-api';
import { generateBreaks, generateBreaksTest } from './generate-break';
import { generateEastAsianWidth } from './generate-east-asian-width';
import { generateEmbeddedDB } from './generate-embedded-db';
import { generateEmojiProperties } from './generate-emoji';
import { generateHeader } from './generate-header';
import { generateLocales } from './generate-locales';
import { generateNormalizationCount } from './generate-tests';
import { generateWASM } from './generate-wasm';
import { iLog, isVerbose, log, setVerbose } from './log';
import { readNormalizationProps } from './quick-check';
import { readSpecialCasingProps } from './special-casing';
import {
  BidirectionalCategories, Block, categories, Categories,
  UnicodeDataRow
} from './types';
import { updateVersion } from './update-version';
import { compressName } from './utils';

let compact = false;

async function readUnicodeData(blocks: Block[], exclusions: number[], stripSigns = true): Promise<Character[]> {
  log('READ UNICODE DATA');
  const analysis = new Analysis();

  let previousCodepoint = 0;
  let codepoint = 0;
  let currentBlock = 0;
  let characters: Character[] = [];

  const file = await open('./UCD/UnicodeData.txt');

  iLog('PARSE UNICODE DATA');

  for await (const line of file.readLines()) {
    if(!line || line.trim() === '') continue;
    const split = line.split(';') as UnicodeDataRow;
    // 10 unicode 1.0 name if Cc
    let name: string | null = split[2] === 'Cc' && split[10] !== '' ? split[10] : split[1];
    const originalName = name;
    codepoint = parseInt(split[0], 16);

    // Special start end.
    if(name.startsWith('<') && name !== '<control>') {
      continue;
    }

    if(stripSigns) {
      name = compressName(codepoint, name);
    }

    const diff = codepoint - previousCodepoint;

    previousCodepoint = codepoint;
    analysis.line(diff, name, split[2], split[8]);

    // Character decomposition mapping
    let decomposition = characterDecomposition(split[5]);
    analysis.decomposition(decomposition);

    if(codepoint > blocks[currentBlock].end) {
      ++currentBlock;
    }

    const char = new Character(
      codepoint,
      name,
      Categories[split[2]],
      parseInt(split[3], 10), // CCC
      split[4] === '' ? BidirectionalCategories.NONE : BidirectionalCategories[split[4]],
      decomposition.type,
      decomposition.decomposition,
      split[6] === '' ? null : parseInt(split[6]), // decimal
      split[7] === '' ? null : parseInt(split[7]), // digit
      split[8] === '' ? null : split[8], // numeric
      split[9] === 'Y', // mirrored
      // unicode 1.0 name
      // 10646 comment field
      split[12] === '' ? null : parseInt(split[12], 16), // uppercase
      split[13] === '' ? null : parseInt(split[13], 16), // lowercase
      split[14] === '' ? null : parseInt(split[14], 16), // titlecase
      null, // quick check
      null, // line breaking class
      null, // east asian width,
      false // extended pictographic
    );

    characters.push(char);
    analysis.addCharacter(char, originalName);
  }

  iLog('INSERT UNICODE DATA');

  analysis.beforeDB();
  await readNormalizationProps(characters);
  const newCases = await readSpecialCasingProps(characters);
  const emojis = await generateEmojiProperties(characters);
  await generateBreaks(characters);
  await generateEastAsianWidth(characters);
  await generateBreaksTest('LineBreak');
  await generateBreaksTest('GraphemeBreak');
  await generateBreaksTest('WordBreak');
  await generateBreaksTest('SentenceBreak');

  // Insert characters
  dbRun(characters);

  dbRunDecompositions(generateDecomposition(characters));
  dbRunDecompositions(generateDecomposition(characters, true), true);
  dbRunComposition(generateComposition(characters, exclusions));
  dbRunEmojiProperties(emojis);
  dbRunSpecialCasing(newCases);

  await generateCasefold();

  dbRunAfter();

  analysis.outputGeneratedData(codepoint, isVerbose());

  return characters;
}

let generateTarget: string | null = null;

// Init
for(let i = 2; i < process.argv.length; ++i) {
  if(process.argv[i] === '-v' || process.argv[i] === '--verbose') {
    setVerbose(true);
  } else if(process.argv[i] === '-c') {
    compact = true;
  } else if(process.argv[i] === 'locales') {
    generateTarget = 'locales';
  } else if(process.argv[i] === 'amalgamation') {
    generateTarget = 'amalgamation';
  } else if(process.argv[i] === 'embedded-db') {
    generateTarget = 'embedded-db';
  } else if(process.argv[i] === 'embedded-amalgamation') {
    generateTarget = 'embedded-amalgamation';
  } else if(process.argv[i] === 'update-version') {
    generateTarget = 'update-version';
  }
}

async function generate() {
  if(generateTarget === 'locales') {
    await generateLocales();
    return;
  } else if(generateTarget === 'amalgamation') {
    await generateAmalgamation();
    return;
  } else if(generateTarget === 'embedded-db') {
    await generateEmbeddedDB();
    return;
  } else if(generateTarget === 'embedded-amalgamation') {
    await generateAmalgamation(true);
    return;
  } else if(generateTarget === 'update-version') {
    await updateVersion();
    return;
  }

  const dbName = '../../mojibake.db';
  // Check if dbName file exists, if it exists, delete it
  try {
    await access(dbName, constants.F_OK);
    await unlink(dbName);
  } catch (err) {}

  dbInit(dbName, compact);

  const blocks = await readBlocks();
  await readUnicodeData(blocks, await readCompositionExclusions());

  generateHeader(blocks, categories);
  generateWASM();
  // generateData(characters);
  generateAPI();

  const size = dbSize();
  iLog(`Database size: ${size.toLocaleString()} bytes\n`);

  generateNormalizationCount();
}

generate();
