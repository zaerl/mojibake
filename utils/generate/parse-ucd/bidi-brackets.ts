import { log } from '../log';
import { BidiBracket } from '../types';
import { parsePropertyFile } from './utils';

export async function readBidiBrackets(): Promise<BidiBracket[]> {
  log('READ BIDI BRACKETS');

  const path = './UCD/BidiBrackets.txt';
  const bidiBrackets: BidiBracket[] = [];

  for await (const split of parsePropertyFile(path)) {
    const cp = parseInt(split[0], 16);
    const pair = parseInt(split[1], 16);
    const isOpen = split[2] === 'o';

    bidiBrackets.push({ cp, pair, isOpen });
  }

  return bidiBrackets;
}
