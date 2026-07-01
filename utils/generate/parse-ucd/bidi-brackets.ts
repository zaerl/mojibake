/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { iLog } from '../log';
import { BidiBracket } from '../types';
import { parsePropertyFile } from './utils';

export async function readBidiBrackets(): Promise<BidiBracket[]> {
  iLog('Parse Bidi Brackets');

  const path = './unicode-data/UCD/BidiBrackets.txt';
  const bidiBrackets: BidiBracket[] = [];

  for await (const split of parsePropertyFile(path)) {
    const cp = parseInt(split[0], 16);
    const pair = parseInt(split[1], 16);
    const isOpen = split[2] === 'o';

    bidiBrackets.push({ cp, pair, isOpen });
  }

  return bidiBrackets;
}
