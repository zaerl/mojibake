import { createReadStream } from 'fs';
import { createInterface } from 'readline';
import { Analysis } from './analysis';
import { readBlocks } from './blocks';
import { generateCasefold } from './casefold';
import { Character } from './character';
import { readCompositionExclusions } from './compositition-exclusion';
import { dbInit, dbRun, dbRunAfter, dbRunComposition, dbRunDecompositions, dbSize } from './db';
import { characterDecomposition, generateComposition, generateDecomposition } from './decomposition';
import { generateAPI } from './generate-api';
import { generateHeader } from './generate-header';
import { generateNormalizationCount } from './generate-tests';
import { iLog, isVerbose, log, setVerbose } from './log';
import { readNormalizationProps } from './quick-check';
import {
  BidirectionalCategories, Block, categories, Categories,
  UnicodeDataRow
} from './types';

let compact = false;

async function readUnicodeData(blocks: Block[], exclusions: number[]): Promise<Character[]> {
  log('READ UNICODE DATA');
  const analysis = new Analysis();

  let previousCodepoint = 0;
  let codepoint = 0;
  let currentBlock = 0;
  let characters: Character[] = [];

  const rl = createInterface({
    input: createReadStream('./UCD/UnicodeData.txt'),
    crlfDelay: Infinity
  });

  iLog('PARSE UNICODE DATA');

  for await (const line of rl) {
    const split = line.split(';') as UnicodeDataRow;
    // 10 unicode 1.0 name if Cc
    const name = split[2] === 'Cc' && split[10] !== '' ? split[10] : split[1];
    codepoint = parseInt(split[0], 16);

    // Special start end.
    if(name.startsWith('<') && name !== '<control>') {
      continue;
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
      null // quick check
    );

    characters.push(char);
    analysis.addCharacter(char);
  }

  iLog('INSERT UNICODE DATA');

  analysis.beforeDB();
  await readNormalizationProps(characters);

  // Insert characters
  dbRun(characters);

  dbRunDecompositions(generateDecomposition(characters));
  dbRunDecompositions(generateDecomposition(characters, true), true);
  dbRunComposition(generateComposition(characters, exclusions));

  await generateCasefold();

  dbRunAfter();

  analysis.outputGeneratedData(codepoint, isVerbose());

  return characters;
}

// Init
for(let i = 2; i < process.argv.length; ++i) {
  if(process.argv[i] === '-v' || process.argv[i] === '--verbose') {
    setVerbose(true);
  } else if(process.argv[i] === '-c') {
    compact = true;
  }
}

async function generate() {
  dbInit('../../mojibake.db', compact);

  const blocks = await readBlocks();
  await readUnicodeData(blocks, readCompositionExclusions());

  generateHeader(blocks, categories);
  // generateData(characters);
  generateAPI();

  const size = dbSize();
  iLog(`Database size: ${size.toLocaleString()} bytes\n`);

  generateNormalizationCount();
}

generate();
