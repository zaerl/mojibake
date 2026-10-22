/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { readFileSync, writeFileSync } from 'fs';
import functions, { MojibakeFunction } from '../functions';
import { substituteBlock } from '../utils';

// Split the leading `static` function definitions of an example, with their comments, from its
// body. C does not allow nested functions, so they are placed at file scope.
function splitExample(fn: MojibakeFunction): { definitions: string[], body: string[] } {
  const lines = (fn.examples?.c as string).split('\n');
  let start = 0;

  for(;;) {
    let line = start;

    while(line < lines.length && lines[line].startsWith('//')) {
      ++line;
    }

    if(line >= lines.length || !lines[line].startsWith('static ') || !lines[line].endsWith('{')) {
      break;
    }

    const end = lines.indexOf('}', line);

    if(end === -1) {
      throw new Error(`Unterminated definition in the C example of ${fn.name}`);
    }

    start = end + 1;

    while(start < lines.length && !lines[start].length) {
      ++start;
    }
  }

  return {
    definitions: lines.slice(0, start),
    body: lines.slice(start),
  };
}

function featureGuard(fn: MojibakeFunction, source: string): string {
  return fn.exampleFeature ?
    `#if ${fn.exampleFeature}\n${source}#endif // ${fn.exampleFeature}\n` : source;
}

function definitionsSource(fn: MojibakeFunction): string {
  const { definitions } = splitExample(fn);

  if(!definitions.length) {
    return '';
  }

  return '\n' + featureGuard(fn, `// Example definitions for ${fn.name}\n` +
    definitions.join('\n').trimEnd() + '\n');
}

function exampleSource(fn: MojibakeFunction): string {
  const { body } = splitExample(fn);
  let output: string[] = [];

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

    if(line.startsWith('printf(')) {
      const match = line.match(/printf\((.+)\)/);

      if(!match) {
        continue;
      }

      let currentComment = output[output.length - 1] || '';
      currentComment = currentComment.replace(/^\/\/ /, '');

      output.push( `// ${line}`);
      output.push(`snprintf(test_buffer, sizeof(test_buffer), ${match[1]}); // Added by the script`);
      output.push(`ATT_ASSERT(test_buffer, "${currentComment}", "${fn.name} test failed"); // Added by the script`);

      continue;
    }

    output.push(line);
  }

  const indent = fn.exampleCompileOnly ? '        ' : '    ';
  let generated = output.map(
    line => line.length ? `${indent}${line}` : ''
  ).join('\n');

  if(fn.exampleCompileOnly) {
    // volatile, so compilers do not warn about a constant condition
    generated = `    volatile bool run_example = false; // Added by the script

    if(run_example) { // Added by the script
${generated}
    } // Added by the script`;
  }

  return '\n' + featureGuard(fn, `{
    // Example for ${fn.name}
    MJB_TEST_COVERAGE(${fn.name}); // Added by the script
${generated}
}
`);
}

export function generateExampleC() {
  const path = '../../tests/example.c';
  let fileContent = readFileSync(path, 'utf-8');

  // Remove stale artifacts of examples that no longer exist.
  let definitions = '';
  let generated = '';

  for(const fn of functions) {
    if(!fn.examples?.c) {
      continue;
    }
    definitions += definitionsSource(fn);
    generated += exampleSource(fn);
  }

  fileContent = substituteBlock(
    fileContent,
    '// Example definitions are automatically generated. Do not edit.\n',
    '\n// End of generated example definitions.',
    definitions
  );

  fileContent = substituteBlock(
    fileContent,
    '// This function is automatically generated. Do not edit.',
    '    // End of generated examples.',
    generated
  );

  writeFileSync(path, fileContent);
}
