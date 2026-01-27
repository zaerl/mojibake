/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { Character } from '../character';
import { log } from '../log';
import { parsePropertyFile, ucdCodepointRange } from './utils';

export enum QuickCheckResult {
  YES        = 0x0,
  NO         = 0x1,
  MAYBE      = 0x2,
  // See: DerivedNormalizationProps.txt
  NFD_NO     = 0x4,
  NFD_MAYBE  = 0x8,
  NFC_NO     = 0x10,
  NFC_MAYBE  = 0x20,
  NFKC_NO    = 0x40,
  NFKC_MAYBE = 0x80,
  NFKD_NO    = 0x100,
  NFKD_MAYBE = 0x200
}

export type QuickCheck = {
  NFD_QC: { N: number; M: number; }; // Yes by default
  NFC_QC: { N: number; M: number; }; // Yes by default
  NFKC_QC: { N: number; M: number; }; // Yes by default
  NFKD_QC: { N: number; M: number; }; // Yes by default
};

function quickCheckResultToNumber(normalization: keyof QuickCheck, result: 'N' | 'M'): QuickCheckResult {
  switch(normalization) {
    case 'NFD_QC':
      return result === 'N' ? QuickCheckResult.NFD_NO : QuickCheckResult.NFD_MAYBE;
    case 'NFC_QC':
      return result === 'N' ? QuickCheckResult.NFC_NO : QuickCheckResult.NFC_MAYBE;
    case 'NFKC_QC':
      return result === 'N' ? QuickCheckResult.NFKC_NO : QuickCheckResult.NFKC_MAYBE;
    case 'NFKD_QC':
      return result === 'N' ? QuickCheckResult.NFKD_NO : QuickCheckResult.NFKD_MAYBE;
  }
}

export async function readNormalizationProps(characters: Character[]): Promise<QuickCheck> {
  log('READ NORMALIZATION PROPS');

  const path = './UCD/DerivedNormalizationProps.txt';
  const characterMap: { [key: string]: Character } = {};

  for(const char of characters) {
    characterMap['' + char.codepoint] = char;
  }

  let id = 0;

  const quickCheck: QuickCheck = {
    NFD_QC: { N: 0, M: 0 },
    NFC_QC: { N: 0, M: 0 },
    NFKC_QC: { N: 0, M: 0 },
    NFKD_QC: { N: 0, M: 0 }
  };

  for await (const split of parsePropertyFile(path, [], ';', false)) {
    if(split.length < 3) {
      continue;
    }

    const codepoint = split[0];
    const decomposition = split[1] as keyof QuickCheck;

    if(!(decomposition in quickCheck)) {
      continue;
    }

    const add = split[2].split('#')[0].trim() as keyof QuickCheck[typeof decomposition];
    ++quickCheck[decomposition][add];

    let { codepointStart, codepointEnd } = ucdCodepointRange(codepoint);

    for(let cp = codepointStart; cp <= codepointEnd; ++cp) {
      const index = '' + cp;

      if(characterMap[index]) {
        const qc = quickCheckResultToNumber(decomposition, add);

        if(characterMap[index].quickCheck === null) {
          characterMap[index].quickCheck = qc;
        } else {
          characterMap[index].quickCheck |= qc;
        }
      }
    }

    ++id;
  }

  return quickCheck;
}
