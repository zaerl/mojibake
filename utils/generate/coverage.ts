/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { existsSync, readFileSync, readdirSync, statSync, writeFileSync } from 'fs';
import path, { join } from 'path';

// Types
type FuncCoverage = {
  u: number;  // usage count
  p: number;  // parameter count
};

type Coverage = { [key: string]: FuncCoverage };

const coverage: Coverage = {};

function findExports(): void {
  const jsonPath = path.join(__dirname, 'functions.json');
  const jsonData = readFileSync(jsonPath, 'utf8');
  const data: any[] = JSON.parse(jsonData);

  for(const item of data) {
    coverage[`mjb_${item.name}`] = { u: 0, p: item.args.length };
  }
}

function scanTestFile(filepath: string): void {
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

function findTests(directory: string): void {
  function walkDir(dir: string): void {
    const items = readdirSync(dir);

    for(const item of items) {
      const fullPath = join(dir, item);
      const stat = statSync(fullPath);

      if(stat.isDirectory()) {
        walkDir(fullPath);
      } else if(item.endsWith('.c')) {
        scanTestFile(fullPath);
      }
    }
  }

  walkDir(directory);
}

function printCoverage(): void {
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

  let output = '# Test coverage\n\n';
  output += `| Test ${post} | Coverage   |\n`;
  output += `| ${bar} | ---------- |\n`;

  for(const [key, value] of sortedCov) {
    const post = ' '.repeat(maxNameLength - key.length - 2);
    const countStr = value.u.toString();
    const countPost = ' '.repeat(10 - countStr.length);
    output += `| \`${key}\`${post} | ${countStr}${countPost} |\n`;
  }

  const totalPost = ' '.repeat(maxNameLength - 10);
  const totalStr = total.toString();
  const totalCountPost = ' '.repeat(10 - totalStr.length - 4);
  output += `| **Total** ${totalPost} | **${totalStr}**${totalCountPost} |\n`;

  writeFileSync('../../TESTS.md', output);
}

function main(): void {
  const testDir = process.argv.length < 4 ? '../../tests' : process.argv[3];

  if(!existsSync(testDir) || !statSync(testDir).isDirectory()) {
    console.error(`Error: ${testDir} is not a valid directory`);
    process.exit(1);
  }

  findExports();
  findTests(testDir);
  printCoverage();
}

if(require.main === module) {
  main();
}
