import { createReadStream } from 'fs';
import { createInterface } from 'readline';

export async function generateCasefold() {
  const rl = createInterface({
    input: createReadStream('./UCD/CaseFolding.txt'),
    crlfDelay: Infinity
  });

  let maxFolded = 0;
  let counts = {
    'C': 0,
    'F': 0,
    'S': 0,
    'T': 0,
  }

  for await (const line of rl) {
    if(line.startsWith('#') || line.trim() === '') {
      continue;
    }

    const split = line.split(';');
    const codepoint = parseInt(split[0], 16);
    const status = split[1].trim();
    const mapping = split[2].trim().split(' ');

    ++counts[status as keyof typeof counts];

    if(mapping.length > 1) {
      maxFolded = Math.max(maxFolded, mapping.length);
    }
  }
}
