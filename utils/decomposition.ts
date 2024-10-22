import { Character } from "./character";
import { CalculatedDecomposition, characterDecompositionMapping, CharacterDecompositionMappingStrings, Decomposition } from "./types";

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

/**
 * 1: 2
 * 1: 4
 * 2: 3
 * 2: 5
 *
 * 1: 3
 * 1: 5
 * 1: 4
 * 2: 3
 * 2: 5
 */
export function generateDecomposition(characters: Character[]): CalculatedDecomposition[] {
  const decompositionsMap: { [key: string]: number[] } = {};
  const characterMap: { [key: string]: Character } = {};
  const normalized: CalculatedDecomposition[] = [];

  for(const char of characters) {
    characterMap['' + char.codepoint] = char;
  }

  const addDecomposition = (currentCodepoint: number, char: Character) => {
    if(!char.decompositions.length) {
      return;
    }

    if(!(char.decomposition == 0 || char.decomposition == 1)) {
      if(char.codepoint !== currentCodepoint) {
        normalized.push({ codepoint: currentCodepoint, value: char.codepoint });
      }

      return;
    }

    for(const dec of char.decompositions) {
      // Do not exist.
      if(typeof characterMap['' + dec] === 'undefined') {
        continue;
      }

      if(characterMap['' + dec].decompositions.length) {
        // Loop.
        addDecomposition(currentCodepoint, characterMap['' + dec]);
      } else {
        normalized.push({ codepoint: currentCodepoint, value: dec });
      }
    }
  }

  for(const char of characters) {
    addDecomposition(char.codepoint, char);
  }

  return normalized;
}
