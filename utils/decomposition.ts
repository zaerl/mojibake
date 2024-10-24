import { Character } from "./character";
import { CalculatedDecomposition, CharacterDecomposition, characterDecompositionMapping, CharacterDecompositionMappingStrings, Decomposition } from "./types";

/**
 * Example for LATIN CAPITAL LETTER D WITH DOT ABOVE
 * From UnicodeData.txt#L6883
 * 1E00 ...;0044 0307;...
 */
export function characterDecomposition(mapping: string): Decomposition {
  const map = mapping.length ? mapping.split(' ') : [];
  let decomposition:number[] = [];
  let ret: Decomposition = { type: CharacterDecomposition.None, decomposition };
  let start = 0;

  if(map.length < 1) {
    return ret;
  }

  if(map[0][0] === '<') {
    ret.type = characterDecompositionMapping[map[0] as CharacterDecompositionMappingStrings];
    ++start;
  } else {
    ret.type = CharacterDecomposition.Canonical;
  }

  for(let i = start; i < map.length; ++i) {
    ret.decomposition.push(parseInt(map[i], 16));
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

    if(char.decomposition !== CharacterDecomposition.Canonical) {
      if(char.codepoint !== currentCodepoint) {
        normalized.push({ codepoint: currentCodepoint, value: char.codepoint });
      }

      return;
    }

    for(const dec of char.decompositions) {
      // It is not a normal character (CJK, Hangul, etc).
      if(typeof characterMap['' + dec] === 'undefined') {
        normalized.push({ codepoint: currentCodepoint, value: dec });

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
