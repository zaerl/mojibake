import { createReadStream } from 'fs';
import { createInterface } from 'readline';
import { readBlocks } from './blocks';
import { Character } from './character';
import { dbInit, dbRun, dbSize } from './db';
import { characterDecomposition } from './decomposition';
import { generateHeader } from './generate-header';
import { generateReadme } from './generate-readme';
import { iLog, isVerbose, log, setVerbose } from './log';
import {
  BidirectionalCategories, Block, categories, Categories,
  CountBuffer, Numeric,
  UnicodeDataRow
} from './types';
import { commonPrefix } from './utils';

let compact = false;

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

async function readUnicodeData(blocks: Block[]): Promise<Character[]> {
  log('READ UNICODE DATA');

  const nameBuffer: { [name: string]: number } = {};
  const categoryBuffer: { [name: string]: number } = {};
  let codepointsCount = 0;
  let charsCount = 0;
  let wordsCount = 0;
  let hasNumber: { [name: string]: number } = {};
  let maxDecimal = 0;
  let maxDigit = 0;
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
  let names: string[] = [];

  const rl = createInterface({
    input: createReadStream('./UCD/UnicodeData.txt'),
    crlfDelay: Infinity
  });

  iLog('PARSE UNICODE DATA');

  for await (const line of rl) {
    const split = line.split(';') as UnicodeDataRow;
    // 10 unicode 1.0 name if Cc
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
    ++codepointsCount;
    charsCount += name.length;
    wordsCount += words.length;

    // Indexed by the `numeric` field (string)
    if(split[8] !== '') {
      if(typeof(hasNumber[split[8]]) === 'undefined') {
        hasNumber[split[8]] = 1;
      } else {
        ++hasNumber[split[8]];
      }
    }

    // Character decomposition mapping
    let decompositionType = characterDecomposition(split[5]);

    if(codepoint > blocks[currentBlock].end) {
      ++currentBlock;
    }

    const char = new Character(
      codepoint,
      name,
      1 << Categories[split[2]],
      parseInt(split[3], 10), // CCC
      split[4] === '' ? BidirectionalCategories.NONE : BidirectionalCategories[split[4]],
      decompositionType,
      split[6] === '' ? null : parseInt(split[6]), // decimal
      split[7] === '' ? null : parseInt(split[7]), // digit
      split[8] === '' ? null : split[8], // numeric
      split[9] === 'Y', // mirrored
      // unicode 1.0 name
      // 10646 comment field
      split[12] === '' ? 0 : parseInt(split[12], 16), // uppercase
      split[13] === '' ? 0 : parseInt(split[13], 16), // lowercase
      split[14] === '' ? 0 : parseInt(split[14], 16), // titlecase
      currentBlock, // Additional
    );

    characters.push(char);
    names.push(name);

    // Calculate max decimal
    if(char.decimal !== null) {
      maxDecimal = Math.max(maxDecimal, char.decimal);
    }

    // Calculate max digit
    if(char.digit !== null) {
      maxDigit = Math.max(maxDigit, char.digit);
    }

    for(const word of words) {
      if(typeof(nameBuffer[word]) === 'undefined') {
        nameBuffer[word] = 1;
      } else {
        ++nameBuffer[word];
      }

      if(typeof(categoryBuffer[split[2]]) === 'undefined') {
        categoryBuffer[split[2]] = 1;
      } else {
        ++categoryBuffer[split[2]];
      }
    }
  }

  iLog('INSERT UNICODE DATA');

  names.sort();
  const prefixesBuffer: { [name: string]: number } = {};

  for(let i = 1; i < names.length; ++i) {
    const common = commonPrefix(names[i - 1], names[i]);

    if(common !== '') {
      if(typeof(prefixesBuffer[common]) === 'undefined') {
        prefixesBuffer[common] = 1;
      } else {
        ++prefixesBuffer[common];
      }
    }
  }

  const prefixes = Object.entries(prefixesBuffer);
  let saving = 0;
  prefixes.sort((a, b) => b[1] - a[1]);
  for(const el of prefixes) {
    if(el[1] > 1) {
      saving += el[0].length * (el[1] - 1);
    }
  }

  // Insert characters
  for(const char of characters) {
    dbRun(char);
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

  // `name` is the `numeric` field
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

  log('\nMAX NUMBERS\n');
  log(`MAX DECIMAL: ${maxDecimal}`);
  log(`MAX DIGIT: ${maxDigit}`);

  iLog(`${isVerbose() ? "\n" : ''}COUNT\n`);
  iLog(`${codepointsCount.toLocaleString()} codepoints (${(codepointsCount * 5).toLocaleString()} bytes)`);
  iLog(`${wordsCount.toLocaleString()} words (${(wordsCount * 5).toLocaleString()} bytes)`);
  iLog(`${charsCount.toLocaleString()} characters (${(charsCount).toLocaleString()} bytes)`);

  return characters;
}

// Init
for(let i = 2; i < process.argv.length; ++i) {
  if(process.argv[i] === '-V') {
    setVerbose(true);
  } else if(process.argv[i] === '-c') {
    compact = true;
  }
}

async function generate() {
  dbInit('../build/mojibake.db', compact);

  const blocks = readBlocks('./UCD/Blocks.txt');
  await readUnicodeData(blocks);

  generateHeader(blocks, categories);
  // generateData(characters);
  generateReadme();

  const size = dbSize();
  iLog(`Database size: ${size.toLocaleString()} bytes`);
}

generate();
