import { cp, open } from 'fs/promises';
import { Character } from './character';
import { log } from './log';

export type NewCase = {
  codepoint: number;
  lowercase: (number | null)[];
  titlecase: (number | null)[];
  uppercase: (number | null)[];
  hasLowercase: number;
  hasTitlecase: number;
  hasUppercase: number;
}

export type NewCases = Array<NewCase>;

export async function readSpecialCasingProps(characters: Character[], path = './UCD/SpecialCasing.txt'): Promise<NewCases> {
  log('READ SPECIAL CASING');
  const characterMap: { [key: string]: Character } = {};

  for(const char of characters) {
    characterMap['' + char.codepoint] = char;
  }

  const file = await open(path);
  let newCases: NewCases = [];
  let count = 0;
  let hasMultiple = 0;
  let maxNewCases = 0;
  let maxNewCasesCount = 0;

  for await (const line of file.readLines()) {
    if(line.startsWith('#') || line === '') { // Comment
      continue;
    }

    // TODO: add support for conditional mappings.
    if(line === '# Conditional Mappings') {
      break;
    }

    const filter = (field: string) => {
      let ret = field.trim().split(' ').filter(cp => cp !== '');

      return ret.map(cp => cp === '' ? null : parseInt(cp, 16));
    }

    const split = line.split('; ');
    const codepoint = parseInt(split[0].trim(), 16);
    const lowercase = filter(split[1]);
    const titlecase = filter(split[2]);
    const uppercase = filter(split[3]);

    if(codepoint in characterMap) {
      const char = characterMap[codepoint];
      const previousCount = newCases.length;
      let index = 0;

      if(!newCases[codepoint]) {
        newCases[codepoint] = {
          codepoint,
          lowercase: [null, null, null],
          titlecase: [null, null, null],
          uppercase: [null, null, null],
          hasLowercase: lowercase.length,
          hasTitlecase: titlecase.length,
          hasUppercase: uppercase.length
        };
      }

      for(const cp of lowercase) {
        newCases[codepoint].lowercase[index] = cp;

        ++index;
      }

      index = 0;

      for(const cp of titlecase) {
        newCases[codepoint].titlecase[index] = cp;
        ++index;
      }

      index = 0;

      for(const cp of uppercase) {
        newCases[codepoint].uppercase[index] = cp;
        ++index;
      }

      if(newCases.length !== previousCount) {
        ++count;
      }

      if(lowercase.length > 1 || titlecase.length > 1 || uppercase.length > 1) {
        ++hasMultiple;
      }

      const max = lowercase.length + titlecase.length + uppercase.length;

      if(max > maxNewCases) {
        maxNewCases = max;
      }

      maxNewCasesCount = Math.max(maxNewCasesCount, lowercase.length, titlecase.length, uppercase.length);
    }
  }

  newCases = newCases.filter(x => x !== undefined);

  for(const char of newCases) {
    // Override parent field if only one case is present
    if(char.hasLowercase === 1) {
      characterMap['' + char.codepoint].lowercase = char.lowercase[0];
    } else if(char.hasTitlecase === 1) {
      characterMap['' + char.codepoint].titlecase = char.titlecase[0];
    } else if(char.hasUppercase === 1) {
      characterMap['' + char.codepoint].uppercase = char.uppercase[0];
    }
  }

  const showList = (cases: NewCase[], type: 'lowercase' | 'titlecase' | 'uppercase') => {
    // console.log(type);
    for(const char of cases) {
      if(char[type].length > 1) {
        // console.log(char.codepoint);
      }
    }
  }

  showList(newCases, 'lowercase');
  showList(newCases, 'titlecase');
  showList(newCases, 'uppercase');

  log(`${count} characters have special casing`);
  log(`${hasMultiple} characters have multiple special casing`);
  log(`Max new codepoint cases: ${maxNewCases}`);
  log(`Max new normalizations: ${maxNewCasesCount}`);

  return newCases;
}
