/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { existsSync, readFileSync, readdirSync, statSync, writeFileSync } from 'fs';
import { join, relative } from 'path';
import functions from './functions';
import { CFunction } from './html-function';
import { substituteBlock } from './utils';

// Types
type FuncCoverage = {
  u: number; // usage count
  p: number; // parameter count
};

type Coverage = { [key: string]: FuncCoverage };

const notAllowedFiles = new Set([
  'attractor',
  'utils',
]);

function createCoverage(names: string[]): Coverage {
  const coverage: Coverage = {};

  for(const name of names) {
    coverage[name] = { u: 0, p: 0 };
  }

  return coverage;
}

function findExports(): Coverage {
  const coverage: Coverage = {};

  for(const item of functions) {
    coverage[`${item.name}`] = { u: 0, p: item.args.length };
  }

  return coverage;
}

function javascriptName(cName: string): string {
  return cName.replace(/^mjb_/, '');
}

function findJavaScriptExports(): Coverage {
  const names = functions
    .filter(item => item.wasm)
    .map(item => `Mojibake.${CFunction.snakeToCamel(javascriptName(item.name))}`);

  return createCoverage(['Mojibake.create', ...names]);
}

function findJavaScriptCoverageKey(method: string): string {
  return method === 'create' ? 'Mojibake.create' : `Mojibake.${method}`;
}

function scanTestFile(filepath: string, coverage: Coverage): void {
  const content = readFileSync(filepath, 'utf8');
  const lines = content.split('\n');

  let currentResult = '';
  let currentCount = 1;
  const prog = /ATT_ASSERT\(.*(mjb_[a-z0-9_.]+).+$/;
  const attAssertRegex = /ATT_ASSERT\(/;

  for(const line of lines) {
    const trimmedLine = line.trim();
    let result = trimmedLine.match(/\/\/ CURRENT_ASSERT (.+)$/);

    if(result) {
      currentResult = result[1];

      continue;
    }

    result = trimmedLine.match(/\/\/ CURRENT_COUNT (\d+)$/);

    if(result) {
      currentCount = parseInt(result[1], 10);

      continue;
    }

    // Check if this line contains an ATT_ASSERT call
    if(attAssertRegex.test(trimmedLine)) {
      result = trimmedLine.match(prog);

      if(result) {
        const key = result[1].trim();

        if(key in coverage) {
          coverage[key].u += currentCount;
          currentResult = key;
          currentCount = 1;
        } else if(currentResult) {
          if(currentResult in coverage) {
            coverage[currentResult].u += currentCount;
            currentCount = 1;
          }
        }
      } else if(currentResult) {
        // ATT_ASSERT found but doesn't match function name pattern. Use currentResult if available.
        if(currentResult in coverage) {
          coverage[currentResult].u += currentCount;
          currentCount = 1;
        }
      }
    }
  }
}

function scanJavaScriptTestFile(filepath: string, coverage: Coverage): void {
  const content = readFileSync(filepath, 'utf8');
  const assertions = content.matchAll(/ATT_ASSERT\(([\s\S]*?)\);/g);

  for(const assertion of assertions) {
    const body = assertion[1];
    const method = body.match(/\bmojibake\.([A-Za-z0-9_]+)\s*\(/);
    const description = body.match(/,\s*['"]([A-Za-z0-9_]+)['"]\s*$/);
    const key = findJavaScriptCoverageKey(method?.[1] ?? description?.[1] ?? '');

    if(key in coverage) {
      ++coverage[key].u;
    }
  }
}

function findTests(directory: string, coverage: Coverage): void {
  function walkDir(dir: string): void {
    const items = readdirSync(dir);

    for(const item of items) {
      const fullPath = join(dir, item);
      const stat = statSync(fullPath);

      if(stat.isDirectory()) {
        if(notAllowedFiles.has(relative(directory, fullPath))) {
          continue;
        }

        walkDir(fullPath);
      } else if(item.endsWith('.c')) {
        scanTestFile(fullPath, coverage);
      }
    }
  }

  walkDir(directory);
}

function renderCoverageSection(title: string, coverage: Coverage): { output: string; total: number } {
  let total = 0;
  let maxNameLength = 0;

  for(const key in coverage) {
    total += coverage[key].u;
    maxNameLength = Math.max(maxNameLength, key.length + 2);
  }

  const sortedCov = Object.entries(coverage).sort((a, b) => {
    if(b[1].u !== a[1].u) {
      return b[1].u - a[1].u;
    } else {
      return a[0].localeCompare(b[0]);
    }
  });

  const post = ' '.repeat(maxNameLength - 5);
  const bar = '-'.repeat(maxNameLength);
  const totalStr = total.toString();
  // Width of the count column: the widest of "Coverage", the counts, and the bold total.
  const countWidth = Math.max(10, totalStr.length + 4);

  let output = `## ${title}\n\n`;
  output += `| Test ${post} | Coverage${' '.repeat(countWidth - 8)} |\n`;
  output += `| ${bar} | ${'-'.repeat(countWidth)} |\n`;

  for(const [key, value] of sortedCov) {
    const post = ' '.repeat(maxNameLength - key.length - 2);
    const countStr = value.u.toString();
    const countPost = ' '.repeat(countWidth - countStr.length);
    output += `| \`${key}\`${post} | ${countStr}${countPost} |\n`;
  }

  const totalPost = ' '.repeat(maxNameLength - 10);
  const totalCountPost = ' '.repeat(countWidth - totalStr.length - 4);
  output += `| **Total** ${totalPost} | **${totalStr}**${totalCountPost} |\n`;

  return { output, total };
}

function printCoverage(cCoverage: Coverage, javascriptCoverage: Coverage): void {
  const cSection = renderCoverageSection('C', cCoverage);
  const javascriptSection = renderCoverageSection('JavaScript', javascriptCoverage);
  const total = cSection.total; // + javascriptSection.total;
  let output = '# Test coverage\n\n';
  output += `${cSection.output}\n`;
  output += javascriptSection.output;

  writeFileSync('../../TESTS.md', output);

  // Update README.md with the total coverage count
  const readmePath = '../../README.md';
  const readmeContent = readFileSync(readmePath, 'utf8');
  const updatedReadmeContent = substituteBlock(readmeContent, 'a total of **', `** tests including`,
    total.toLocaleString('en-US')
  );
  writeFileSync(readmePath, updatedReadmeContent);
}

function main(): void {
  const testDir = process.argv.length < 4 ? '../../tests' : process.argv[3];
  const javascriptTestFile = '../../src/api/tests/index.ts';

  if(!existsSync(testDir) || !statSync(testDir).isDirectory()) {
    console.error(`Error: ${testDir} is not a valid directory`);
    process.exit(1);
  }

  if(!existsSync(javascriptTestFile) || !statSync(javascriptTestFile).isFile()) {
    console.error(`Error: ${javascriptTestFile} is not a valid file`);
    process.exit(1);
  }

  const cCoverage = findExports();
  const javascriptCoverage = findJavaScriptExports();

  findTests(testDir, cCoverage);
  scanJavaScriptTestFile(javascriptTestFile, javascriptCoverage);
  printCoverage(cCoverage, javascriptCoverage);
}

if(require.main === module) {
  main();
}
