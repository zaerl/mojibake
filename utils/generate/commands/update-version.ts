/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { readdirSync, readFileSync, writeFileSync } from 'fs';
import path from 'path';
import { getVersion, substituteBlock } from '../utils';

export async function updateVersion() {
  let fileContent = readFileSync('../../src/mojibake.h', 'utf-8');
  const v = getVersion();
  const hexNumber = '0x' + v.number.toString(16).toUpperCase();

  fileContent = substituteBlock(fileContent, '#define MJB_VERSION_NUMBER', '\n',
    ` ${hexNumber} // MAJOR << 8 | MINOR << 4 | REVISION`);
  fileContent = substituteBlock(fileContent, '#define MJB_VERSION_MAJOR', '\n', ' ' + v.major.toString());
  fileContent = substituteBlock(fileContent, '#define MJB_VERSION_MINOR', '\n', ' ' + v.minor.toString());
  fileContent = substituteBlock(fileContent, '#define MJB_VERSION_REVISION', '\n', ' ' + v.revision.toString());

  fileContent = substituteBlock(fileContent, '#define MJB_VERSION "', '-WASM"\n', v.version);
  fileContent = substituteBlock(fileContent, '#else\n    #define MJB_VERSION "', '"\n', v.version);

  writeFileSync('../../src/mojibake.h', fileContent);

  fileContent = readFileSync('../../CMakeLists.txt', 'utf-8');
  fileContent = substituteBlock(fileContent, '    VERSION ', '\n', v.version);
  writeFileSync('../../CMakeLists.txt', fileContent);

  fileContent = readFileSync('package.json', 'utf-8');
  fileContent = substituteBlock(fileContent, '"version": "', '",\n', v.version);
  writeFileSync('package.json', fileContent);

  fileContent = readFileSync('../../src/api/package.json', 'utf-8');
  fileContent = substituteBlock(fileContent, '"version": "', '",\n', v.version);
  writeFileSync('../../src/api/package.json', fileContent);

  fileContent = readFileSync('../../src/api/tests/index.ts', 'utf-8');
  fileContent = substituteBlock(fileContent, 'mojibake.version(), \'', '-WASM', v.version);
  fileContent = substituteBlock(fileContent, 'versionNumber(), ', ',', hexNumber);
  writeFileSync('../../src/api/tests/index.ts', fileContent);

  fileContent = readFileSync('../../src/api/tests/browser.html', 'utf-8');
  fileContent = substituteBlock(fileContent, 'mojibake.version(), \'', '-WASM', v.version);
  writeFileSync('../../src/api/tests/browser.html', fileContent);

  // Rust crates: the workspace version, the mojibake-sys dependency, and both lockfile entries
  fileContent = readFileSync('../../src/rust/Cargo.toml', 'utf-8');
  fileContent = substituteBlock(fileContent, '[workspace.package]\nversion = "', '"', v.version);
  writeFileSync('../../src/rust/Cargo.toml', fileContent);

  fileContent = readFileSync('../../src/rust/mojibake/Cargo.toml', 'utf-8');
  fileContent = substituteBlock(fileContent, 'path = "../mojibake-sys", version = "', '"', v.version);
  writeFileSync('../../src/rust/mojibake/Cargo.toml', fileContent);

  fileContent = readFileSync('../../src/rust/README.md', 'utf-8');
  fileContent = substituteBlock(fileContent, 'mojibake-rs = "', '"', v.versionShort);
  writeFileSync('../../src/rust/README.md', fileContent);

  fileContent = readFileSync('../../src/rust/Cargo.lock', 'utf-8');
  fileContent = substituteBlock(fileContent, 'name = "mojibake-rs"\nversion = "', '"', v.version);
  fileContent = substituteBlock(fileContent, 'name = "mojibake-sys"\nversion = "', '"', v.version);
  writeFileSync('../../src/rust/Cargo.lock', fileContent);

  const zip = `mojibake-amalgamation-${v.major}${v.minor}${v.revision}`;
  fileContent = readFileSync('../../README.md', 'utf-8');
  fileContent = substituteBlock(fileContent, 'Download it here [', '.zip)',
    `${zip}.zip](https://github.com/zaerl/mojibake/releases/download/v${v.version}/${zip}`);

  writeFileSync('../../README.md', fileContent);

  // Find all SKILL.md files recursively under a directory
  function updateSkillMdFiles(dir: string) {
    const entries = readdirSync(dir, { withFileTypes: true });

    for(const entry of entries) {
      const full = path.join(dir, entry.name);

      if(entry.isDirectory()) {
        updateSkillMdFiles(full);
      } else if(entry.name === 'SKILL.md') {
        fileContent = readFileSync(full, 'utf-8');
        fileContent = substituteBlock(fileContent, 'version: ', '\n', v.version);
        writeFileSync(full, fileContent);
      }
    }
  }

  // Update .claude/skills/*/SKILL.md version fields
  updateSkillMdFiles(path.resolve(__dirname, '../../../.claude/skills'));

  const steps = [
    'make wasm',
    'make amalgamation',
    'npm i in utils/generate',
    'npm i in src/api',
    `Update CHANGELOG.md: add a [${v.version}] section`,
    `git commit --signoff -am "Update version to ${v.version}"`,
    // package-rust refuses to run while src/ has uncommitted changes, so it follows the commit
    'make test-rust && make package-rust',
    `git tag v${v.version}`,
    'git push && git push origin --tags',
    'Deploy',
  ];

  console.log(`\nVersion updated to ${v.version}\n`);
  steps.forEach((step, i) => console.log(`${i + 1}. ${step}`));

  console.log(`\nPublish the Rust crates ${v.version} to crates.io from the clean, tagged checkout`);
  console.log('(requires `cargo login` with a crates.io token that owns mojibake-sys and mojibake-rs).');
  console.log('Cargo uploads mojibake-sys first, then mojibake-rs:\n');
  console.log('make vendor-rust');
  console.log('(cd src/rust && cargo publish --workspace --locked --allow-dirty)');
  console.log('\nPublished versions cannot be overwritten; check them at:');
  console.log(`https://crates.io/crates/mojibake-sys/${v.version}`);
  console.log(`https://crates.io/crates/mojibake-rs/${v.version}`);
}
