/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { constants } from 'fs';
import { access, unlink } from 'fs/promises';
import { generateAmalgamation } from './amalgamation';
import { Analysis } from './analysis';
import { Character } from './character';
import { dbInit, dbRun, dbRunAfter, dbRunComposition, dbRunDecompositions, dbRunEmojiProperties, dbRunPropertyRanges, dbRunSpecialCasing, dbSize } from './db';
import { characterDecomposition, generateComposition, generateDecomposition } from './decomposition';
import { generateAPI } from './generate-api';
import { generateEmbeddedDB } from './generate-embedded-db';
import { generateNormalizationCount } from './generate-tests';
import { generateHeader } from './header';
import { generateLocales } from './locales';
import { iLog, isVerbose, log, setVerbose } from './log';
import { readAliases } from './parse-ucd/aliases';
import { readBlocks } from './parse-ucd/blocks';
import { generateBreaksTest } from './parse-ucd/breaks';
import { generateCasefold } from './parse-ucd/casefold';
import { readCompositionExclusions } from './parse-ucd/compositition-exclusion';
import { generateEmojiProperties } from './parse-ucd/emoji-properties';
import { buildPropertyRanges, Property } from './parse-ucd/properties';
import { readNormalizationProps } from './parse-ucd/quick-check';
import { readSpecialCasingProps } from './parse-ucd/special-casing';
import { parsePropertyFile, ucdBool, ucdInt, ucdString } from './parse-ucd/utils';
import { PrefixCompressor } from './prefix-compressor';
import {
  BidirectionalCategories, Block, categories, Categories,
  UnicodeDataRow
} from './types';
import { updateVersion } from './update-version';
import { CodepointsRangeMap, compressName, isCodepointOnRanges } from './utils';
import { generateWASM } from './wasm';

let compact = false;

async function readUnicodeData(blocks: Block[], exclusions: number[], stripSigns = true):
  Promise<{ characters: Character[], properties: Property[] }> {
  log('READ UNICODE DATA');
  const analysis = new Analysis();

  let previousCodepoint = 0;
  let codepoint = 0;
  let currentBlock = 0;
  let characters: Character[] = [];
  let ranges: CodepointsRangeMap = {};

  iLog('PARSE UNICODE DATA');
  const aliases = await readAliases();

  log('READ UNICODE DATA');
  for await (const line of parsePropertyFile('./UCD/UnicodeData.txt', [], ';', false)) {
    const split = line as UnicodeDataRow;

    let name: string | null = aliases[split[0]] ? aliases[split[0]] : split[1];
    const originalName = name;
    codepoint = parseInt(split[0], 16);

    // Special start end.
    const codepointsRange = isCodepointOnRanges(codepoint, name ?? '');

    if(codepointsRange === null) {
      continue;
    }

    if(codepointsRange.range) {
      if(ranges[codepointsRange.name] === undefined) {
        codepointsRange.rangeStart = codepoint;
        ranges[codepointsRange.name] = codepointsRange;
      } else {
        // Start already exists, update the end.
        ranges[codepointsRange.name].rangeEnd = codepoint;
      }

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
      ucdInt(split[6]), // decimal
      ucdInt(split[7]), // digit
      ucdString(split[8]), // numeric
      ucdBool(split[9]), // mirrored
      // unicode 1.0 name
      // 10646 comment field
      ucdInt(split[12], 16), // uppercase
      ucdInt(split[13], 16), // lowercase
      ucdInt(split[14], 16), // titlecase
      null, // quick check
      false, // extended pictographic
      null, // prefix
    );

    characters.push(char);
    analysis.addCharacter(char, originalName);
  }

  iLog('INSERT UNICODE DATA');

  analysis.beforeDB();
  const { propertyRanges, properties } = await buildPropertyRanges();
  await readNormalizationProps(characters);
  const newCases = await readSpecialCasingProps(characters);
  const emojis = await generateEmojiProperties(characters);
  await generateBreaksTest('LineBreak');
  await generateBreaksTest('GraphemeBreak');
  await generateBreaksTest('WordBreak');
  await generateBreaksTest('SentenceBreak');

  const prefixCompressor = new PrefixCompressor(characters);
  const prefixes = prefixCompressor.compress();

  // Insert characters
  dbRun(characters, prefixes);

  dbRunDecompositions(generateDecomposition(characters));
  dbRunDecompositions(generateDecomposition(characters, true), true);
  dbRunComposition(generateComposition(characters, exclusions));
  dbRunEmojiProperties(emojis);
  dbRunSpecialCasing(newCases);
  dbRunPropertyRanges(propertyRanges);

  await generateCasefold();

  dbRunAfter();

  analysis.outputGeneratedData(codepoint, isVerbose());

  return { characters, properties };
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
  const { properties } = await readUnicodeData(blocks, await readCompositionExclusions());

  generateHeader(blocks, categories, properties);
  generateWASM();
  // generateData(characters);
  generateAPI();

  const size = dbSize();
  iLog(`Database size: ${size.toLocaleString()} bytes\n`);

  generateNormalizationCount();
}

generate();
