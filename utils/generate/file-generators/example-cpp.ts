/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { readFileSync, writeFileSync } from 'fs';
import functions, { MojibakeFunction } from '../functions';
import { substituteBlock } from '../utils';

function exampleSource(fn: MojibakeFunction): string {
  const body = (fn.examples?.cpp as string).split('\n');
  let output: string[] = [];
  let stdout = [];
  let commentedStdout = [];
  let inOutput = false;
  let currentComment = '';

  for(const line of body) {
    if(!line.length) {
      output.push('');
      continue;
    }

    if(line.startsWith('    return 1;')) {
      output.push(`    ATT_ASSERT(0, 1, "${fn.name} test failed"); // Added by the script`);
      output.push(`    return;`);

      continue;
    }

    if(line.startsWith('std::cout << ')) {
      currentComment = output[output.length - 1] || '';
      currentComment = currentComment.replace(/^\/\/ /, '');
      inOutput = true;
    }

    // A std::cout statement may span multiple lines and ends with a semicolon.
    if(inOutput) {
      stdout.push(line.replace(/^std::cout/, 'oss') + ' // Added by the script');
      commentedStdout.push(`// ${line}`);

      if(line.endsWith(';')) {
        inOutput = false;

        output.push(...commentedStdout);
        commentedStdout = [];

        output.push(...stdout);
        stdout = [];

        output.push(`ATT_ASSERT(oss.str(), "${currentComment}\\n", "${fn.name} test failed"); // Added by the script`)
        output.push('oss.str(""); // Added by the script');
      }

      continue;
    }

    output.push(line);
  }

  const generated = output.map(
    line => line.length ? `    ${line}` : ''
  ).join('\n');
  const featureStart = fn.exampleFeature ? `#if ${fn.exampleFeature}\n` : '';
  const featureEnd = fn.exampleFeature ? `#endif // ${fn.exampleFeature}\n` : '';

  return `
${featureStart}\
{
    // Example for ${fn.name}
    MJB_TEST_COVERAGE(${fn.name}); // Added by the script
${generated}
    oss.clear();
    oss.copyfmt(std::ostringstream());
}
${featureEnd}\
`;
}

export function generateExampleCPP() {
  const path = '../../tests/ext/cpp/example.cpp';
  let fileContent = readFileSync(path, 'utf-8');

  // Remove stale artifacts of examples that no longer exist.
  let generated = '';

  for(const fn of functions) {
    if(!fn.examples?.cpp) {
      continue;
    }
    generated += exampleSource(fn);
  }

  fileContent = substituteBlock(
    fileContent,
    '// This function is automatically generated. Do not edit.',
    '    // End of generated examples.',
    generated
  );

  writeFileSync(path, fileContent);
}
