import { characterDecompositionMapping, CharacterDecompositionMappingStrings, Decomposition } from "./types";

/**
 * Example for LATIN CAPITAL LETTER D WITH DOT ABOVE
 * From UnicodeData.txt#L6883
 * 1E00 ...;0044 0307;...
 */
export function characterDecomposition(mapping: string): Decomposition {
  const map = mapping.length ? mapping.split(' ') : [];
  let type = characterDecompositionMapping['none'];
  let decomposition:number[] = [];
  let size = 0;
  let ret = { type, decomposition };

  if(map.length < 1) {
    return ret;
  }

  const canonical = map[0][0] !== '<';
  size = canonical ? map.length : map.length - 1;

  if(canonical) {
    ret.type = characterDecompositionMapping['canonical'];
  }

  for(let i = 0; i < map.length; ++i) {
    if(map[i][0] === '<') {
      ret.type = characterDecompositionMapping[map[i] as CharacterDecompositionMappingStrings];
    } else {
      ret.decomposition.push(parseInt(map[i], 16));
    }
  }

  return ret;
}
