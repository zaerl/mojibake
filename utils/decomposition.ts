import { characterDecompositionMapping, CharacterDecompositionMappingStrings, Decomposition } from "./types";

export function characterDecomposition(mapping: string): Decomposition {
  const map = mapping.length ? mapping.split(' ') : [];
  let type: number = 0;
  let decomposition:number[] = [];
  let size = 0;
  let ret = { type, decomposition };

  if(map.length <= 1) {
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
      /* decompositionStmt.run(
        codepoint,
        type,
        parseInt(decomposition[i], 16));*/
    }
  }

  /* if(decomposition.length >= 16) {
    log('' + codepoint, name, decomposition);
  } */
  // return { type, decompositions, maxDecomposition };
  return ret;
}
