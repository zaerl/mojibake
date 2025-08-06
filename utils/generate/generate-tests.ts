import { readFileSync, writeFileSync } from 'fs';
import { iLog } from './log';
import { substituteText } from './utils';

export function generateNormalizationCount() {
  iLog('PARSE NORMALIZATION TESTS');

  let count = 0;
  let fileContent = readFileSync('./UCD/NormalizationTest.txt', 'utf-8');
  const lines = fileContent.split('\n');

  for(const line of lines) {
    if (!line.startsWith('#') && !line.startsWith('@') && line.trim().length > 0) {
      ++count;
    }
  }

  iLog(`${count.toLocaleString()} tests\n`);

  fileContent = readFileSync('../../tests/normalization.c', 'utf-8');
  fileContent = substituteText(fileContent, "// CURRENT_ASSERT mjb_normalize\n", "char *normalized_res", `    // CURRENT_COUNT ${count * 20}\n    `);

  writeFileSync('../../tests/normalization.c', fileContent);
}
