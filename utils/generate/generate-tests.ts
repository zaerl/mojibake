/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { readFileSync, writeFileSync } from 'fs';
import { iLog } from './log';
import { substituteBlock } from './utils';

export function generateNormalizationCount() {
  iLog('PARSE NORMALIZATION TESTS');

  let count = 0;
  let fileContent = readFileSync('./UCD/NormalizationTest.txt', 'utf-8');
  const lines = fileContent.split('\n');

  for(const line of lines) {
    if(!line.startsWith('#') && !line.startsWith('@') && line.trim().length > 0) {
      ++count;
    }
  }

  iLog(`${count.toLocaleString()} tests\n`);

  fileContent = readFileSync('../../tests/normalization.c', 'utf-8');
  fileContent = substituteBlock(fileContent, "// CURRENT_ASSERT mjb_normalize\n",
    "bool ret = mjb_normalize(source, source_size, MJB_ENCODING_UTF_8, form, &result);",
    `    // CURRENT_COUNT ${count * 20}\n    `);

  writeFileSync('../../tests/normalization.c', fileContent);
}
