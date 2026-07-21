/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { spawnSync } from 'child_process';
import { existsSync, mkdtempSync, readFileSync, rmSync, writeFileSync } from 'fs';
import { tmpdir } from 'os';
import { join, resolve } from 'path';
import functions from '../functions';
import { CFunction } from '../html-function';

// Types
type FuncCoverage = {
  u: number; // usage count
  p: number; // parameter count
};

type Coverage = { [key: string]: FuncCoverage };

type RuntimeCoverageEntry = {
  name: string;
  count: number;
};

type RuntimeCoverage = {
  coverage: RuntimeCoverageEntry[];
};

const repoRoot = resolve(__dirname, '../../..');

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

function findJavaScriptExports(): Coverage {
  const names = functions
    .filter(item => item.wasm)
    .map(item => `Mojibake.${CFunction.wasmFunctionName(item)}`);

  return createCoverage(['Mojibake.create', ...names]);
}

function findJavaScriptCoverageKey(method: string): string {
  return method === 'create' ? 'Mojibake.create' : `Mojibake.${method}`;
}

function resolveInputPath(filepath: string): string {
  const cwdPath = resolve(process.cwd(), filepath);

  if(existsSync(cwdPath)) {
    return cwdPath;
  }

  return resolve(repoRoot, filepath);
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

function findTestExecutable(args: string[]): string {
  const executableName = process.platform === 'win32' ? 'mojibake-test.exe' : 'mojibake-test';

  for(let i = 0; i < args.length; ++i) {
    if(args[i] === '--test-executable' && args[i + 1]) {
      return resolveInputPath(args[i + 1]);
    }

    const match = args[i].match(/^--test-executable=(.+)$/);

    if(match) {
      return resolveInputPath(match[1]);
    }
  }

  if(process.env.MJB_TEST_EXECUTABLE) {
    return resolveInputPath(process.env.MJB_TEST_EXECUTABLE);
  }

  const candidates = [
    join(repoRoot, 'build-test', 'tests', executableName),
    join(repoRoot, 'build', 'tests', executableName)
  ];

  for(const candidate of candidates) {
    if(existsSync(candidate)) {
      return candidate;
    }
  }

  return candidates[0];
}

function readRuntimeCoverage(filepath: string, coverage: Coverage): void {
  const parsed = JSON.parse(readFileSync(filepath, 'utf8')) as RuntimeCoverage;

  if(!Array.isArray(parsed.coverage)) {
    throw new Error(`Invalid runtime coverage file: ${filepath}`);
  }

  for(const entry of parsed.coverage) {
    if(typeof entry.name !== 'string' || typeof entry.count !== 'number') {
      throw new Error(`Invalid runtime coverage entry in ${filepath}`);
    }

    if(entry.name in coverage) {
      coverage[entry.name].u += entry.count;
    }
  }
}

function captureRuntimeCoverage(testExecutable: string, coverage: Coverage): void {
  if(!existsSync(testExecutable)) {
    throw new Error(`Test executable not found: ${testExecutable}`);
  }

  // The attractor test framework will write the coverage to a temporary file, which we will read
  // after the test executable runs.
  const tempDir = mkdtempSync(join(tmpdir(), 'mojibake-coverage-'));
  const coveragePath = join(tempDir, 'coverage.json');

  try {
    const result = spawnSync(testExecutable, ['--coverage', coveragePath], {
      cwd: repoRoot,
      stdio: 'inherit'
    });

    if(result.error) {
      throw result.error;
    }

    if(result.status !== 0) {
      throw new Error(`Runtime coverage command failed with exit code ${result.status ?? 1}`);
    }

    readRuntimeCoverage(coveragePath, coverage);
  } finally {
    rmSync(tempDir, { recursive: true, force: true });
  }
}

function printCoverage(cCoverage: Coverage, javascriptCoverage: Coverage): void {
  const cSection = renderCoverageSection('C', cCoverage);
  const javascriptSection = renderCoverageSection('JavaScript', javascriptCoverage);
  const total = cSection.total.toLocaleString('en-US');
  const totalJS = javascriptSection.total.toLocaleString('en-US');
  let output = `# Test coverage

Mojibake run a total of **${total}** C assertions and **${totalJS}** JavaScript assertions, including all
the official tests included in the standard:

1. [auxiliary/GraphemeBreakTest.txt](https://www.unicode.org/Public/18.0.0/ucd/auxiliary/GraphemeBreakTest.txt)
2. [auxiliary/LineBreakTest.txt](https://www.unicode.org/Public/18.0.0/ucd/auxiliary/LineBreakTest.txt)
3. [auxiliary/SentenceBreakTest.txt](https://www.unicode.org/Public/18.0.0/ucd/auxiliary/SentenceBreakTest.txt)
4. [auxiliary/WordBreakTest.txt](https://www.unicode.org/Public/18.0.0/ucd/auxiliary/WordBreakTest.txt)
5. [BidiCharacterTest.txt](https://www.unicode.org/Public/18.0.0/ucd/BidiCharacterTest.txt)
6. [BidiTest.txt](https://www.unicode.org/Public/18.0.0/ucd/BidiTest.txt)
7. [CaseFolding.txt](https://www.unicode.org/Public/18.0.0/ucd/CaseFolding.txt)
8. [CollationTest/CollationTest_NON_IGNORABLE.txt](https://www.unicode.org/Public/18.0.0/uca/CollationTest.zip)
9. [CollationTest/CollationTest_SHIFTED.txt](https://www.unicode.org/Public/18.0.0/uca/CollationTest.zip)
10. [emoji-test.txt](https://www.unicode.org/Public/18.0.0/emoji/emoji-test.txt)
11. [UTS #39 data (confusables.txt and intentional.txt)](https://www.unicode.org/Public/draft/security/uts39-data-18.0.0.zip)
12. [NormalizationTest.txt](https://www.unicode.org/Public/18.0.0/ucd/NormalizationTest.txt)
13. [SpecialCasing.txt](https://www.unicode.org/Public/18.0.0/ucd/SpecialCasing.txt)\n
`;
  output += `${cSection.output}\n`;
  output += javascriptSection.output;

  writeFileSync(join(repoRoot, 'TESTS.md'), output);
}

function main(): void {
  const args = process.argv.slice(2);
  const testExecutable = findTestExecutable(args);
  const javascriptTestFile = join(repoRoot, 'src', 'api', 'tests', 'index.ts');

  if(!existsSync(javascriptTestFile)) {
    console.error(`Error: ${javascriptTestFile} is not a valid file`);
    process.exit(1);
  }

  const cCoverage = findExports();
  const javascriptCoverage = findJavaScriptExports();

  captureRuntimeCoverage(testExecutable, cCoverage);
  scanJavaScriptTestFile(javascriptTestFile, javascriptCoverage);
  printCoverage(cCoverage, javascriptCoverage);
}

if(require.main === module) {
  main();
}
