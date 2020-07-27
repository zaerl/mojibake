import { writeFileSync, createReadStream } from 'fs';
import { createInterface } from 'readline';

interface Buffer {
  name: string;
  count: number;
}

const nameBuffer: { [name: string]: number } = {};
const categoryBuffer: { [name: string]: number } = {};
let charsCount = 0;
let wordsCount = 0;
let lines = 0;
let entries: string[] = [];

readUnicodeData();

async function readUnicodeData() {
  const rl = createInterface({
    input: createReadStream('./UCD/UnicodeData.txt'),
    crlfDelay: Infinity
  });

  for await (const line of rl) {
    const split = line.split(';');
    const name = split[2] === 'Cc' && split[10] !== '' ? split[10] : split[1];
    const words = name.split(' ');

    ++lines;
    charsCount += name.length;
    wordsCount += words.length;

    entries.push(`    { 0x${split[0]}, UCX_GENERAL_CATEGORY_${split[2].toUpperCase()}, "${name}" }`);

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
  };
  const ret: Buffer[] = [];
  const ret2: Buffer[] = [];

  for(const name in categoryBuffer) {
    ret2.push({ name, count: categoryBuffer[name] });
  }

  console.log('GENERAL CATEGORIES\n');

  for(const entry of ret2.sort(compareFn)) {
    console.log(`${entry.count} :: ${entry.name}`);
  }

  console.log('\nWORDS\n');

  for(const name in nameBuffer) {
    ret.push({ name, count: nameBuffer[name] });
  }

  for(const entry of ret.sort(compareFn)) {
    console.log(`${entry.count} :: ${entry.name}`);
  }

  console.log('\nCOUNT\n');
  console.log(`${wordsCount.toLocaleString()} words (${(wordsCount * 5).toLocaleString()} bytes)`);
  console.log(`${charsCount.toLocaleString()} characters (${(charsCount).toLocaleString()} bytes)`);

  const fheader = `${header('unicode_data')}

#include "ucx.h"

#define UCX_CHARACTER_MAX ${lines}

extern ucx_character ucx_characters[UCX_CHARACTER_MAX];

${footer('unicode_data')}
`;

  const ffile = `${license()}

#include "unicode_data.h"

ucx_character ucx_characters[] = {
${entries.join(',\n')}
};\n`;

  writeFileSync('../src/unicode_data.h', fheader);
  writeFileSync('../src/unicode_data.c', ffile);
}

function compareFn(a: Buffer, b: Buffer): number {
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

function header(name: string): string {
  name = name.toUpperCase();

  return `${license()}

#ifndef UCX_${name}_H
#define UCX_${name}_H`;
}

function footer(name: string): string {
  name = name.toUpperCase();

  return `#endif /* UCX_${name}_H */`;
}

function license(): string {
  return `/**
 * The UCX library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */`;
}
