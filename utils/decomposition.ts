import { characterDecompositionMapping, CharacterDecompositionMappingStrings } from "./types";

export function characterDecomposition(mapping: string): number {
  const decomposition = mapping.length ? mapping.split(' ') : [];
  let decompositionType: number = 0;
  let decompositions = 0;
  let maxDecomposition = 0;

  if(decomposition.length <= 1) {
    return decompositionType;
  }

  const canonical = decomposition[0][0] !== '<';
  const decompositionSize = canonical ? decomposition.length : decomposition.length - 1;
  decompositions += decompositionSize;

  maxDecomposition = Math.max(maxDecomposition, decompositionSize);

  if(canonical) {
    decompositionType = characterDecompositionMapping['canonical'];
  }

  for(let i = 0; i < decomposition.length; ++i) {
    if(decomposition[i][0] === '<') {
      decompositionType = characterDecompositionMapping[decomposition[i] as CharacterDecompositionMappingStrings];
    } else {
      /* decompositionStmt.run(
        codepoint,
        decompositionType,
        parseInt(decomposition[i], 16));*/
    }
  }

  /* if(decomposition.length >= 16) {
    log('' + codepoint, name, decomposition);
  } */
  // return { decompositionType, decompositions, maxDecomposition };
  return decompositionType;
}
