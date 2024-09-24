import { readFileSync, writeFileSync } from "fs";
import { Character } from "./types";
import { substituteText } from "./utils";

export function generateData(characters: Character[]) {
  let fileContent = readFileSync('../src/data.c', 'utf-8');

  fileContent = substituteText(fileContent, 'mjb_characters[', ']', '' + characters.length);
  fileContent = substituteText(fileContent, "= {\n", "\n};", characters.map(value => '    ' + value.formatC()).join(',\n'));

  writeFileSync('../src/data.c', fileContent);
}
