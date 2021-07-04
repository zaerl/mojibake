import { writeFileSync } from "fs";
import { license } from "./format";
import { Character } from "./types";

export function generateData(characters: Character[]) {
  const data =
`${license()}

#include "mojibake.h"

MJB_EXPORT const mjb_character mjb_characters[${characters.length}] = {
${characters.map(value => '    ' + value.formatC()).join(',\n')}
};
`;

  writeFileSync('../src/data.c', data);
}
