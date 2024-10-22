import { createReadStream } from 'fs';
import { createInterface } from 'readline';
import { Analysis } from './analysis';
import { readBlocks } from './blocks';
import { Character } from './character';
import { dbInit, dbRun, dbRunDecompositions, dbSize } from './db';
import { characterDecomposition, generateDecomposition } from './decomposition';
import { generateHeader } from './generate-header';
import { generateReadme } from './generate-readme';
import { iLog, isVerbose, log, setVerbose } from './log';
import {
  BidirectionalCategories, Block, categories, Categories,
  UnicodeDataRow
} from './types';
import { readCompositionExclusions } from './compositition-exclusion';

let compact = false;

async function readUnicodeData(blocks: Block[]): Promise<Character[]> {
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

  const skipCodeints = [
    0xAC00,
    0xD7A3, 0x4E00, 0x9FFF ];

  for await (const line of rl) {
    const split = line.split(';') as UnicodeDataRow;
    // 10 unicode 1.0 name if Cc
    const name = split[2] === 'Cc' && split[10] !== '' ? split[10] : split[1];
    const words = name.split(' ');
    codepoint = parseInt(split[0], 16);

    // Special start end.
    if(name.startsWith('<') && name !== '<control>') {
      continue;
    }

    const diff = codepoint - previousCodepoint;

    previousCodepoint = codepoint;
    analysis.line(diff, name, split[2], split[8], words);

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
      split[14] === '' ? null : parseInt(split[14], 16) // titlecase
    );

    characters.push(char);
    analysis.addCharacter(char);
  }

  iLog('INSERT UNICODE DATA');

  analysis.beforeDB();

  // Insert characters
  for(const char of characters) {
    dbRun(char);
  }

  dbRunDecompositions(generateDecomposition(characters));

  analysis.outputGeneratedData(codepoint, isVerbose());

  return characters;
}

// Init
for(let i = 2; i < process.argv.length; ++i) {
  if(process.argv[i] === '-V' || process.argv[i] === '--verbose') {
    setVerbose(true);
  } else if(process.argv[i] === '-c') {
    compact = true;
  }
}

async function generate() {
  dbInit('../build/mojibake.db', compact);

  const blocks = readBlocks();
  readCompositionExclusions();
  await readUnicodeData(blocks);

  generateHeader(blocks, categories);
  // generateData(characters);
  generateReadme();

  const size = dbSize();
  iLog(`Database size: ${size.toLocaleString()} bytes`);
}

generate();
