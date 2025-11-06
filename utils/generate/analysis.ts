import { Character } from './character';
import { iLog, log } from './log';
import { Categories, CountBuffer, Decomposition, Numeric } from './types';
import { commonPrefix } from './utils';

export class Analysis {
  categoryBuffer: { [name: string]: number } = {};
  nameBuffer: { [name: string]: number } = {};

  combinings = 0;
  decompositions = 0;
  diffs = 0;
  hasNumber: { [name: string]: number } = {};
  maxDecimal = 0;
  maxDecompositions: number[] = [];
  maxDigit = 0;
  names: string[] = [];
  prefixes: [string, number][] = [];
  saving = 0;
  totalSteps = 0;
  totalStepsOver16 = 0;
  totalStepsOver8 = 0;

  charsCount = 0;
  charsBaseSixCount = 0;
  codepointsCount = 0;
  spacesCount = 0;
  wordsCount = 0;

  addCharacter(char: Character, originalName: string | null): void {
    if(char.category === Categories.Mn || char.category === Categories.Me || char.category === Categories.Mc) {
        if(char.combining !== 0) {
          ++this.combinings;
        }
    }

    // Calculate max decimal
    if(char.decimal !== null) {
      this.maxDecimal = Math.max(this.maxDecimal, char.decimal);
    }

    // Calculate max digit
    if(char.digit !== null) {
      this.maxDigit = Math.max(this.maxDigit, char.digit);
    }

    if(originalName !== null) {
      this.names.push(originalName);
    }
  }

  line(diff: number, name: string | null, category: string, numeric: string): void {
    if(name === null) {
      return;
    }

    const words = name.split(' ');
    const spaces = words.length - 1;

    if(diff > 1) {
      this.diffs += diff;
      ++this.totalSteps;

      if(diff > 8) {
        ++this.totalStepsOver8;

        if(diff > 16) {
          ++this.totalStepsOver16;
        }
      }

      // log(`STEP (${split[0]} -- ${diff})`);
    }

    ++this.codepointsCount;

    this.charsCount += name.length;
    this.wordsCount += words.length;
    this.spacesCount += spaces;
    this.charsBaseSixCount += this.countCharsBaseSix(name);

    // Indexed by the `numeric` field (string)
    if(numeric !== '') {
      if(typeof(this.hasNumber[numeric]) === 'undefined') {
        this.hasNumber[numeric] = 1;
      } else {
        ++this.hasNumber[numeric];
      }
    }

    for(const word of words) {
      if(typeof(this.nameBuffer[word]) === 'undefined') {
        this.nameBuffer[word] = 1;
      } else {
        ++this.nameBuffer[word];
      }

      if(typeof(this.categoryBuffer[category]) === 'undefined') {
        this.categoryBuffer[category] = 1;
      } else {
        ++this.categoryBuffer[category];
      }
    }
  }

  decomposition(decomposition: Decomposition): void {
    this.decompositions += decomposition.decomposition.length;

    if(typeof this.maxDecompositions[decomposition.decomposition.length] === 'undefined') {
      this.maxDecompositions[decomposition.decomposition.length] = 1;
    } else {
      ++this.maxDecompositions[decomposition.decomposition.length];
    }
  }

  beforeDB() {
    this.names.sort();
    const prefixesBuffer: { [name: string]: number } = {};

    for(let i = 1; i < this.names.length; ++i) {
      const common = commonPrefix(this.names[i - 1], this.names[i]);

      if(common !== '') {
        if(typeof(prefixesBuffer[common]) === 'undefined') {
          prefixesBuffer[common] = 1;
        } else {
          ++prefixesBuffer[common];
        }
      }
    }

    this.prefixes.sort((a, b) => b[1] - a[1]);

    for(const el of this.prefixes) {
      if(el[1] > 1) {
        this.saving += el[0].length * (el[1] - 1);
      }
    }
  }

  outputGeneratedData(codepoint: number, verbose = false): void {
    const ret: CountBuffer[] = [];
    const ret2: CountBuffer[] = [];

    for(const name in this.categoryBuffer) {
      ret2.push({ name, count: this.categoryBuffer[name] });
    }

    log('CATEGORIES\n');

    ret2.sort(this.compareFn);
    let buffer: string[] = [];
    let prevCount = ret2[0].count;

    for(const entry of ret2) {
      buffer.push(entry.name);

      if(entry.count !== prevCount) {
        log(`${entry.count}: ${buffer.join(', ')}`);

        prevCount = entry.count;
        buffer = [];
      }
    }

    iLog(`\nWORDS: ${Object.keys(this.nameBuffer).length}`);
    iLog(`SPACES: ${this.spacesCount}\n`);

    // Calculate compressed count words by characters
    // Biggest word is the space character
    let compressedCount = this.spacesCount;
    // First codepoint is START OF HEADING, not 0 (NULL)
    let compressedCodepoint = 1;

    for(const name in this.nameBuffer) {
      ret.push({ name, count: this.nameBuffer[name] });
    }

    ret.sort(this.compareFn);
    buffer = [];
    prevCount = ret[0].count;

    for(const entry of ret) {
      buffer.push(entry.name);
      ++compressedCodepoint;

      // Get UTF-8 byte length using TextEncoder
      const utf8Length = new TextEncoder().encode(String.fromCodePoint(compressedCodepoint)).length;
      compressedCount += utf8Length * entry.count;

      if(entry.count !== prevCount) {
        const line = buffer.length > 10 ? buffer.slice(0, 10).join(', ') + '...' : buffer.join(', ');
        log(`${entry.count}: ${line}`);

        prevCount = entry.count;
        buffer = [];
      }
    }

    iLog(`COMPRESSED BYTES: ${compressedCount}\n`);

    log('\nNUMBERS\n');

    const numbersBuffer: Numeric[] = [];

    // `name` is the `numeric` field
    for(const name in this.hasNumber) {
      const values = name.split('/'); // Check if it's a fraction
      const value = values.length === 1 ? parseFloat(values[0]) :
        Math.floor((parseFloat(values[0]) / parseFloat(values[1])) * 100) / 100;
      const count = this.hasNumber[name];

      numbersBuffer.push({ name, value, count });
    }

    numbersBuffer.sort((a: Numeric, b: Numeric) => b.count - a.count);

    for(const num of numbersBuffer) {
      log(`${num.name} (${num.value}): ${num.count}`);
    }

    log(`\nDECOMPOSITION COUNT: ${this.decompositions}\n`);

    log(`DECOMPOSITIONS\n`);

    for(let i = 0; i < this.maxDecompositions.length; ++i) {
      if(typeof this.maxDecompositions[i] !== 'undefined') {
        log(`${i}: ${this.maxDecompositions[i]}`);
      }
    }

    log(`\nCOMBINING CHARACTERS: ${this.combinings}\n`);

    log(`STEPS COUNT: ${this.totalSteps}\n`);
    log(`STEPS COUNT OVER 8: ${this.totalStepsOver8}\n`);
    log(`STEPS COUNT OVER 16: ${this.totalStepsOver16}\n`);

    log(`STEPS TOTAL: ${this.diffs}/${codepoint}\n`);

    log(`${verbose ? '' : '\n'}MAX NUMBERS\n`);
    log(`MAX DECIMAL: ${this.maxDecimal}`);
    log(`MAX DIGIT: ${this.maxDigit}`);

    iLog(`${verbose ? "\n" : ''}COUNTS\n`);
    iLog(`${this.codepointsCount.toLocaleString()} codepoints (${(this.codepointsCount * 5).toLocaleString()} bytes)`);
    iLog(`${this.wordsCount.toLocaleString()} words (${(this.wordsCount * 5).toLocaleString()} bytes)`);
    iLog(`${this.charsCount.toLocaleString()} characters (${(this.charsCount).toLocaleString()} bytes)`);
    iLog(`${this.charsCount.toLocaleString()} characters (packed: ${(this.charsBaseSixCount).toLocaleString()} bytes)`);
  }

  compareFn(a: CountBuffer, b: CountBuffer): number {
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

  private countCharsBaseSix(chars: string): number {
    const totalBits = chars.length * 6;
    const totalBytes = Math.ceil(totalBits / 8);

    return totalBytes;
  }
}
