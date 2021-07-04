import { createReadStream } from 'fs';
import { createInterface } from 'readline';
import { generateData } from './data';
import { generateHeader } from './header';
import { generateReadme } from './readme';
import {
  BidirectionalCategories, Block, categories, Categories, Character, characterDecompositionMapping, CharacterDecompositionMappingStrings,
  CountBuffer, Numeric, UnicodeDataRow
} from './types';

let verbose = false;

function log(message?: any, ...optionalParams: any[]) {
  if(verbose) {
    console.log(message, ...optionalParams);
  }
}

function iLog(message?: any, ...optionalParams: any[]) {
  console.log(message, ...optionalParams);
}

function compareFn(a: CountBuffer, b: CountBuffer): number {
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

function readBlocks(): Block[] {
  log('READ BLOCKS');

  const blocks: Block[] = [];

  const rl = createInterface({
    input: createReadStream('./UCD/Blocks.txt'),
    crlfDelay: Infinity
  });

  rl.on('line', (line: string) => {
    if(line.startsWith('#') || line === '') { // Comment
      return;
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
  });

  return blocks;
}

async function readUnicodeData(blocks: Block[]): Promise<Character[]> {
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
  let decompositions = 0;
  let totalSteps = 0;
  let totalStepsOver8 = 0;
  let totalStepsOver16 = 0;
  let characters: Character[] = [];

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
      ++totalSteps;

      if(diff > 8) {
        ++totalStepsOver8;

        if(diff > 16) {
          ++totalStepsOver16;
        }
      }

      log(`STEP (${split[0]} -- ${diff})`);
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

    const decomposition = split[5].split(' ');
    let decompositionType = null;

    if(decomposition.length > 1) {
      const canonical = decomposition[0][0] !== '<';
      const decompositionSize = canonical ? decomposition.length : decomposition.length - 1;
      decompositions += decompositionSize;

      maxDecomposition = Math.max(maxDecomposition, decompositionSize);

      if(canonical) {
        decompositionType = characterDecompositionMapping.canonical;
      }

      for(let i = 0; i < decomposition.length; ++i) {
        if(decomposition[i][0] === '<') {
          decompositionType = characterDecompositionMapping[decomposition[i] as CharacterDecompositionMappingStrings];
        } else {
          /* decompositionStmt.run(
            codepoint,
            decompositionType,
            parseInt(decomposition[i], 16));*/
        }
      }
    }

    /*if(decomposition.length === 19) {
      log(codepoint, name, decomposition);
    }*/

    if(codepoint > blocks[currentBlock].end) {
      ++currentBlock;
    }

    characters.push(new Character(
      codepoint,
      name,
      currentBlock, // Additional
      1 << Categories[split[2]],
      parseInt(split[3], 10), // CCC
      split[4] === '' ? BidirectionalCategories.NONE : BidirectionalCategories[split[4]],

      split[6] === '' ? null : split[6], // decimal
      split[7] === '' ? null : split[7], // digit
      split[8] === '' ? null : split[8], // numeric

      split[9] === 'Y', // mirrored
      // unicode 1.0 name
      // 10646 comment field
      split[12] === '' ? 0 : parseInt(split[12], 16), // uppercase
      split[13] === '' ? 0 : parseInt(split[13], 16), // lowercase
      split[14] === '' ? 0 : parseInt(split[14], 16) // titlecase
    ));

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
  }

  log(`\nDECOMPOSITION COUNT: ${decompositions}\n`);

  log(`MAX DECOMPOSITION: ${maxDecomposition}\n`);

  log(`STEPS COUNT: ${totalSteps}\n`);
  log(`STEPS COUNT OVER 8: ${totalStepsOver8}\n`);
  log(`STEPS COUNT OVER 16: ${totalStepsOver16}\n`);

  log(`STEPS TOTAL: ${diffs}/${codepoint}\n`);

  const ret: CountBuffer[] = [];
  const ret2: CountBuffer[] = [];

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

  log(`\nWORDS: ${Object.keys(nameBuffer).length}\n`);

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

  iLog(`${verbose ? '\n' : ''}COUNT\n`);
  iLog(`${wordsCount.toLocaleString()} words (${(wordsCount * 5).toLocaleString()} bytes)`);
  iLog(`${charsCount.toLocaleString()} characters (${(charsCount).toLocaleString()} bytes)`);

  return characters;
}

// Init

if(process.argv[2] === '-V') {
  verbose = true;
}

async function generate() {
  const blocks = readBlocks();
  const characters = await readUnicodeData(blocks);

  generateHeader(blocks, categories);
  generateData(characters);
  generateReadme();
}

generate();
