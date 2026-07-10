/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { readFileSync, writeFileSync } from 'fs';
import functions, { MojibakeFunction } from './functions';
import { substituteBlock } from './utils';

function exampleSource(fn: MojibakeFunction): string {
  const body = (fn.example as string).split('\n');
  let output: string[] = [];

  for(const line of body) {
    if(!line.length) {
      output.push('');
      continue;
    }

    if(line.startsWith('    return 1;')) {
      output.push(`    ATT_ASSERT(0, 1, "${fn.name} test failed") // Added by the script`);
      output.push(`    return 1;`);

      continue;
    }

    if(line.startsWith('printf(')) {
      const match = line.match(/printf\((.+)\)/);

      if(!match) {
        continue;
      }

      const parameters = match[1].split(', ');
      let currentComment = output[output.length - 1] || '';
      currentComment = currentComment.replace(/^\/\/ /, '');

      output.push( `// ${line}`);
      output.push(`snprintf(test_buffer, sizeof(test_buffer), ${match[1]}); // Added by the script`);
      output.push(`ATT_ASSERT(test_buffer, "${currentComment}", "${fn.name} test failed") // Added by the script`);

      continue;
    }

    output.push(line);
  }

  const generated = output.map(
    line => line.length ? `    ${line}` : ''
  ).join('\n');

  return `
{
    // Example for ${fn.name}
    MJB_TEST_COVERAGE(${fn.name}); // Added by the script
${generated}
}
`;
}

export function generateExamples() {
  const path = '../../tests/example.c';
  let fileContent = readFileSync(path, 'utf-8');

  // Remove stale artifacts of examples that no longer exist.
  let generated = '';

  for(const fn of functions) {
    if(!fn.example) {
      continue;
    }
    generated += exampleSource(fn);
  }

  fileContent = substituteBlock(
    fileContent,
    '// This function is automatically generated. Do not edit.',
    '    return 0;',
    generated
  );

  writeFileSync(path, fileContent);
}
