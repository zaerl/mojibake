import { open } from 'fs/promises';
import { Character } from './character';
import { log } from './log';

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

export async function readNormalizationProps(characters: Character[], path = './UCD/DerivedNormalizationProps.txt'): Promise<QuickCheck> {
  log('READ NORMALIZATION PROPS');
  const characterMap: { [key: string]: Character } = {};

  for(const char of characters) {
    characterMap['' + char.codepoint] = char;
  }

  let id = 0;

  const file = await open(path);

  const quickCheck: QuickCheck = {
    NFD_QC: { N: 0, M: 0 },
    NFC_QC: { N: 0, M: 0 },
    NFKC_QC: { N: 0, M: 0 },
    NFKD_QC: { N: 0, M: 0 }
  };

  for await (const line of file.readLines()) {
    if(line.startsWith('#') || line === '') { // Comment
      continue
    }

    const split = line.split('; ');
    const codepoint = split[0].trim();
    const decomposition = split[1].trim() as keyof QuickCheck;

    if(!(decomposition in quickCheck)) {
      continue;
    }

    const add = split[2].split('#')[0].trim() as keyof QuickCheck[typeof decomposition];
    ++quickCheck[decomposition][add];

    let codepointStart = 0;
    let codepointEnd = 0;

    if(codepoint.includes('..')) {
      const codepoints = codepoint.split('..');

      if(codepoints.length === 2) {
        codepointStart = parseInt(codepoints[0], 16);
        codepointEnd = parseInt(codepoints[1], 16);
      }
    } else {
      codepointStart = parseInt(codepoint, 16);
      codepointEnd = codepointStart;
    }

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
