import { open } from 'fs/promises';
import { log } from './log';

export async function readAliases(): Promise<{ [key: string]: string }> {
  log('READ UNICODE ALIASES');

  const file = await open('./UCD/NameAliases.txt');
  const aliases: { [key: string]: string } = {};

  for await (const line of file.readLines()) {
    if(!line || line.startsWith('#') || line.trim() === '') {
      continue;
    }

    const split = line.split(';');

    if(split.length < 3) {
      continue;
    }

    const codepoint = split[0];
    const alias = split[1];
    const type = split[2];

    if(type === 'control' || type === 'correction') {
      aliases[codepoint] = alias;
    }
  }

  return aliases;
}
