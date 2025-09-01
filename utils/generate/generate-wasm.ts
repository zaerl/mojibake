import { readFileSync, writeFileSync } from 'fs';
import { cfns } from './function';
import { substituteText } from './utils';

function getFunctions() {
  return cfns().filter(value => value.isWASM()).map(
    value => '        "' + value.formatWASM() + '"').join("\n");
}

export function generateWASM() {
  let fileContent = readFileSync('../../src/CMakeLists.txt', 'utf-8');

  fileContent = substituteText(fileContent,
    "set(EXPORTED_FUNCTIONS\n",
    "\n        # Core memory functions",
    getFunctions());

  writeFileSync('../../src/CmakeLists.txt', fileContent);
}
