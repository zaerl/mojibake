import { createInterface } from 'readline';

interface Buffer {
  name: string;
  count: number;
}

const nameBuffer: { [name: string]: number } = {};
const categoryBuffer: { [name: string]: number } = {};
let charsCount = 0;
let wordsCount = 0;

const rl = createInterface({
  input: process.stdin,
  output: process.stdout,
  terminal: false
});

rl.on('line', (line) => {
  const split = line.split(';');
  const name = split[2] === 'Cc' && split[10] !== '' ? split[10] : split[1];
  const words = name.split(' ');

  charsCount += name.length;
  wordsCount += words.length;

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
}).on('close', () => {
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
});

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
