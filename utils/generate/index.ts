/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { generateAmalgamation } from './amalgamation';
import { Analysis } from './analysis';
import { Character } from './character';
import { characterDecomposition, generateComposition, generateDecomposition } from './decomposition';
import { generateAPI } from './generate-api';
import { generateExamples } from './generate-examples';
import { generateUnicodeTables } from './generate-unicode-tables';
import { generateWasmTD } from './generate-wasm-td';
import { generateHeader } from './header';
import { generateLocale } from './locales/generate-locale';
import { iLog, setVerbose } from './log';
import { readAliases } from './parse-ucd/aliases';
import { readBidiBrackets } from './parse-ucd/bidi-brackets';
import { readBidiMirroring } from './parse-ucd/bidi-mirroring';
import { readBlocks } from './parse-ucd/blocks';
import { generateCasefold } from './parse-ucd/casefold';
import { parseCollationAllKeys } from './parse-ucd/collation';
import { readCompositionExclusions } from './parse-ucd/compositition-exclusion';
import { parseConfusables } from './parse-ucd/confusables';
import { generateEmojiProperties } from './parse-ucd/emoji-properties';
import { generateEmojiSequences } from './parse-ucd/emoji-sequences';
import { buildPropertyRanges, Property } from './parse-ucd/properties';
import { readNormalizationProps } from './parse-ucd/quick-check';
import { readSpecialCasingProps } from './parse-ucd/special-casing';
import { readScriptExtensions } from './parse-ucd/script-extensions';
import { parsePropertyFile, ucdBool, ucdInt, ucdString } from './parse-ucd/utils';
import { PrefixCompressor } from './prefix-compressor';
import {
  BidirectionalCategories, Block, categories, Categories,
  UnicodeDataRow
} from './types';
import {
  addCaseFolding, addCharacters, addCollation, addCompositions, addConfusables,
  addDecompositions, addEmojiProperties, addEmojiSequences, addPropertyRanges, addSimpleCaseFolding,
  addSpecialCasing, addScriptExtensions, resetUnicodeTableData
} from './unicode-data-store';
import { updateVersion } from './update-version';
import { CodepointsRangeMap, compressName, isCodepointOnRanges } from './utils';
import { generateWASM } from './wasm';

async function readUnicodeData(blocks: Block[], exclusions: number[], stripSigns = true):
  Promise<{ characters: Character[], properties: Property[] }> {
  const analysis = new Analysis();

  let previousCodepoint = 0;
  let codepoint = 0;
  let currentBlock = 0;
  let characters: Character[] = [];
  let ranges: CodepointsRangeMap = {};

  const aliases = await readAliases();

  for await (const line of parsePropertyFile(
    './unicode-data/UCD/UnicodeData.txt', [], ';', false
  )) {
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
      // Unicode 1.0 name
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

  iLog('Build Unicode data');

  analysis.beforeDB();
  const { propertyRanges, properties } = await buildPropertyRanges();
  await readNormalizationProps(characters);
  const newCases = await readSpecialCasingProps(characters);
  const emojis = await generateEmojiProperties(characters);
  const emojiSequences = await generateEmojiSequences();

  const prefixCompressor = new PrefixCompressor(characters);
  const prefixes = prefixCompressor.compress();

  addCharacters(characters, prefixes);

  addDecompositions(generateDecomposition(characters));
  addDecompositions(generateDecomposition(characters, true), true);
  addCompositions(generateComposition(characters, exclusions));
  addEmojiProperties(emojis);
  addEmojiSequences(emojiSequences);
  addSpecialCasing(newCases);
  addPropertyRanges(propertyRanges);
  addScriptExtensions(await readScriptExtensions(properties));

  const caseFolds = await generateCasefold(characters);
  addCaseFolding(caseFolds.full);
  addSimpleCaseFolding(caseFolds.simple);

  // analysis.outputGeneratedData(codepoint, isVerbose());

  return { characters, properties };
}

let generateTarget: string | null = null;

// Init
for(let i = 2; i < process.argv.length; ++i) {
  if(process.argv[i] === '-v' || process.argv[i] === '--verbose') {
    setVerbose(true);
  } else if(process.argv[i] === 'generate-locale') {
    generateTarget = 'locale';
  } else if(process.argv[i] === 'amalgamation') {
    generateTarget = 'amalgamation';
  } else if(process.argv[i] === 'unicode-tables') {
    generateTarget = 'unicode-tables';
  } else if(process.argv[i] === 'update-version') {
    generateTarget = 'update-version';
  }
}

async function buildUnicodeTableData() {
  iLog('Build Unicode table data');
  resetUnicodeTableData();

  const blocks = await readBlocks();
  const { properties } = await readUnicodeData(blocks, await readCompositionExclusions());

  iLog('Parse collation data');
  const { entries: collationEntries } =
    await parseCollationAllKeys('./unicode-data/collation/allkeys.txt');
  addCollation(collationEntries);

  iLog('Parse confusables data');
  const confusableEntries = await parseConfusables('./unicode-data/security/confusables.txt');
  addConfusables(confusableEntries);

  return { blocks, properties };
}

async function generate() {
  if(generateTarget === 'locale') {
    await generateLocale('it');
    return;
  } else if(generateTarget === 'amalgamation') {
    await generateAmalgamation();
    return;
  } else if(generateTarget === 'unicode-tables') {
    await buildUnicodeTableData();
    await generateUnicodeTables();
    return;
  } else if(generateTarget === 'update-version') {
    await updateVersion();
    return;
  }

  const { blocks, properties } = await buildUnicodeTableData();
  const bidiBrackets = await readBidiBrackets();
  const bidiMirroring = await readBidiMirroring();

  await generateUnicodeTables();
  generateHeader(blocks, categories, properties, bidiBrackets, bidiMirroring);
  generateWASM();
  // generateData(characters);
  generateAPI();
  generateExamples();

  // const summary = unicodeTableDataSummary();
  // iLog(`Unicode table rows: ${JSON.stringify(summary)}\n`);

  generateWasmTD();
}

generate();
