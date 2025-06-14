import { readFileSync, writeFileSync } from 'fs';
import { substituteText } from './utils';

function generateTest(value: [number, string]) {
  let hex = value[0].toString(16).padStart(4, '0').toUpperCase();
  const char = new TextEncoder().encode(String.fromCodePoint(value[0]));
  const chars = Array.from(char);
  let escaped = chars.map(byte => `\\x${byte.toString(16).toUpperCase()}`).join('');

  return `    TEST_UTF8(0x${hex}, "${escaped}", ${chars.length}, "${value[1]}");`;
}

export function generateEncodingTest() {
  let fileContent = readFileSync('../tests/encoding.c', 'utf-8');

  const tests: [number, string][] = [
    [0x007F, 'ASCII limit'],
    [0x7FF, '2-bytes limit'],
    [0x1E0A, 'LATIN CAPITAL LETTER D WITH DOT ABOVE'],
    [0xFFFD, '3-bytes limit'],
    [0x10FFFE, '4-bytes limit'],
    [0x1F642, 'SLIGHTLY SMILING FACE'],
  ];

  fileContent = substituteText(fileContent, "// UTF-8 tests\n", "\n    return NULL;", tests.map(generateTest).join("\n") + "\n");

  writeFileSync('../tests/encoding.c', fileContent);
}

generateEncodingTest();
