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

  // Generate a map of all decompositions.
  for(const char of characters) {
    if(!char.decompositions.length) {
      continue;
    }

    decompositionsMap['' + char.codepoint] = [];

    for(const dec of char.decompositions) {
      decompositionsMap['' + char.codepoint].push(dec);
    }
  }

  const normalized: CalculatedDecomposition[] = [];

  for(const key in decompositionsMap) {
    if(!decompositionsMap.hasOwnProperty(key)) {
      continue;
    }

    for(const decomposed of decompositionsMap[key]) {
      // Not decomposed.
      if(typeof decompositionsMap['' + decomposed] === 'undefined') {
        normalized.push({ codepoint: parseInt(key), value: decomposed });
        continue;
      }

      // Is decomposed, add all decompositions.
      for(const innerDecomposed of decompositionsMap['' + decomposed]) {
        normalized.push({ codepoint: parseInt(key), value: innerDecomposed });
      }
    }
  }

  return normalized;
}

/*export function generateDecompositionMappings(characters: Character[]): CalculatedDecomposition[] {
  const decompositionsMap: { [key: string]: number[] } = {};

  // Generate a map of all decompositions.
  for(const char of characters) {
    if(!char.decompositions.length) {
      continue;
    }

    decompositionsMap['' + char.codepoint] = [];

    for(const dec of char.decompositions) {
      decompositionsMap['' + char.codepoint].push(dec);
    }
  }

  console.log(`decompositionsMap ${Object.keys(decompositionsMap).length}`);

  const filteredDecompositions: CalculatedDecomposition[] = [];

  for(const key in decompositionsMap) {
    if(!decompositionsMap.hasOwnProperty(key)) {
      continue;
    }

    for(const value of decompositionsMap[key]) {
      if(typeof decompositionsMap['' + value] === 'undefined') {
        // Not decomposed.
        filteredDecompositions.push({ codepoint: parseInt(key), value });

        continue;
      } else {
        // Filter all decompositions that are combining characters.
        for(const innerValue of decompositionsMap['' + value]) {
          filteredDecompositions.push({ codepoint: parseInt(key), value: innerValue });
        }
      }
    }
  }

  return filteredDecompositions;

  // dbRunDecompositions(filteredDecompositions);

  /*for(const char of characters) {
    for(const dec of char.decompositions) {
      if(typeof decompositionsMap['' + char.codepoint] === 'undefined') {
        filteredDecompositions.push({ codepoint: char.codepoint, value: dec });
        continue;
      }
      const index = `${char.codepoint}-${dec}`;
      const entry = decompositionsMap[index];

      if(typeof entry === 'undefined') {
        filteredDecompositions.push({ codepoint: char.codepoint, value: dec });
        continue;
      }

      if(entry.combining) {
        continue;
      }

      filteredDecompositions.push({ codepoint: entry.codepoint, value: entry.value });
    }
  }*
}*/
