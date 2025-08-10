import { Character } from './character';
import { CalculatedDecomposition, CharacterDecomposition, characterDecompositionMapping,
  CharacterDecompositionMappingStrings, Composition, Decomposition } from './types';

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
 * Codepoint: Decomposition
 * 1: 2, 4
 * 2: 3, 5
 *
 * 1: 3
 * 1: 5
 * 1: 4
 * ----
 * 2: 3
 * 2: 5
 */
export function generateDecomposition(characters: Character[], compatibility = false): CalculatedDecomposition[] {
  const characterMap: { [key: string]: Character } = {};
  const normalized: CalculatedDecomposition[] = [];

  for(const char of characters) {
    characterMap['' + char.codepoint] = char;
  }

  const addDecomposition = (currentCodepoint: number, char: Character, compatibility: boolean) => {
    if(!char.decompositions.length) {
      return;
    }

    if(!compatibility && char.decomposition !== CharacterDecomposition.Canonical) {
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
        addDecomposition(currentCodepoint, characterMap['' + dec], compatibility);
      } else {
        normalized.push({ codepoint: currentCodepoint, value: dec });
      }
    }
  }

  for(const char of characters) {
    // Add the decomposition of the character.
    addDecomposition(char.codepoint, char, compatibility);
  }

  return normalized;
}

export function generateComposition(characters: Character[], exclusions: number[]): Composition[] {
  const compositions: Composition[] = [];

  for(const char of characters) {
    if(
      char.decomposition === CharacterDecomposition.Canonical &&
      char.decompositions.length === 2 &&
      !exclusions.includes(char.codepoint)
    ) {
      compositions.push({
        starter_codepoint: char.decompositions[0],
        combining_codepoint: char.decompositions[1],
        composite_codepoint: char.codepoint
      });
    }
  }

  return compositions;
}
