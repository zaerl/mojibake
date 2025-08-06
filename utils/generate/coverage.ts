import { existsSync, readFileSync, readdirSync, statSync, writeFileSync } from "fs";
import { join } from "path";

const exclude = [
  'mjb_alloc',
  'mjb_free',
  'mjb_initialize',
  'mjb_realloc',
  'mjb_shutdown',
];

// Types
type FuncCoverage = {
  u: number;  // usage count
  p: number;  // parameter count
};

type Coverage = { [key: string]: FuncCoverage };

const coverage: Coverage = {};

function scanFile(filepath: string): void {
  try {
    const content = readFileSync(filepath, 'utf8');
    const lines = content.split('\n');

    const prog = /MJB_EXPORT.+[ \*]([a-z_]+)\(([^)]*)\)\s*\{/;

    for(const line of lines) {
      const result = line.trim().match(prog);

      if(result) {
        const params = result[2];
        let count = 0;

        if(params !== 'void') {
          count = (params.match(/,/g) || []).length + 1;
        }

        if(!exclude.includes(result[1])) {
          coverage[result[1]] = { u: 0, p: count };
        }
      }
    }
  } catch (error) {
    console.error(`Error reading file ${filepath}:`, error);
  }
}

function walkDir(dir: string): void {
  const items = readdirSync(dir);

  for(const item of items) {
    const fullPath = join(dir, item);
    const stat = statSync(fullPath);

    if(stat.isDirectory()) {
      walkDir(fullPath);
    } else if(item.endsWith('.c') || item.endsWith('.h')) {
      scanFile(fullPath);
    }
  }
}

function findExports(directory: string): void {
  walkDir(directory);
}

function scanTestFile(filepath: string): void {
  const content = readFileSync(filepath, 'utf8');
  const lines = content.split('\n');

  let currentResult = '';
  let currentCount = 1;
  const prog = /ATT_ASSERT\(([a-z_.]+)[\(\,].+$/;

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
  const sourceDir = process.argv.length < 3 ? '../../src' : process.argv[2];
  const testDir = process.argv.length < 4 ? '../../tests' : process.argv[3];

  if(!existsSync(sourceDir) || !statSync(sourceDir).isDirectory()) {
    console.error(`Error: ${sourceDir} is not a valid directory`);
    process.exit(1);
  }

  if(!existsSync(testDir) || !statSync(testDir).isDirectory()) {
    console.error(`Error: ${testDir} is not a valid directory`);
    process.exit(1);
  }

  findExports(sourceDir);
  findTests(testDir);
  printCoverage();
}

if(require.main === module) {
  main();
}
