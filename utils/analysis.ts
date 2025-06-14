import { Character } from "./character";
import { iLog, log } from "./log";
import { Categories, CountBuffer, Decomposition, Numeric } from "./types";
import { commonPrefix } from "./utils";

export class Analysis {
  categoryBuffer: { [name: string]: number } = {};
  charsCount = 0;
  codepointsCount = 0;
  combinings = 0;
  decompositions = 0;
  diffs = 0;
  hasNumber: { [name: string]: number } = {};
  maxDecimal = 0;
  maxDecompositions: number[] = [];
  maxDigit = 0;
  nameBuffer: { [name: string]: number } = {};
  names: string[] = [];
  prefixes: [string, number][] = [];
  saving = 0;
  totalSteps = 0;
  totalStepsOver16 = 0;
  totalStepsOver8 = 0;
  wordsCount = 0;

  addCharacter(char: Character): void {
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

    this.names.push(char.name);
  }

  line(diff: number, name: string, category: string, numeric: string, words: string[]): void {
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
        log(`${entry.count} :: ${buffer.join(', ')}`);

        prevCount = entry.count;
        buffer = [];
      }
    }

    log(`\nWORDS: ${Object.keys(this.nameBuffer).length}\n`);

    for(const name in this.nameBuffer) {
      ret.push({ name, count: this.nameBuffer[name] });
    }

    ret.sort(this.compareFn);
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

    log('\nMAX NUMBERS\n');
    log(`MAX DECIMAL: ${this.maxDecimal}`);
    log(`MAX DIGIT: ${this.maxDigit}`);

    iLog(`${verbose ? "\n" : ''}COUNT\n`);
    iLog(`${this.codepointsCount.toLocaleString()} codepoints (${(this.codepointsCount * 5).toLocaleString()} bytes)`);
    iLog(`${this.wordsCount.toLocaleString()} words (${(this.wordsCount * 5).toLocaleString()} bytes)`);
    iLog(`${this.charsCount.toLocaleString()} characters (${(this.charsCount).toLocaleString()} bytes)`);
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
}
