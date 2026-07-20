/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { readdirSync, readFileSync, statSync, writeFileSync } from 'fs';
import path from 'path';
import { getVersion, substituteBlock } from '../utils';

export async function updateVersion() {
  let fileContent = readFileSync('../../src/mojibake.h', 'utf-8');
  const v = getVersion();
  const versionNumber = v.major << 8 | v.minor << 4 | v.revision;
  const hexNumber = '0x' + versionNumber.toString(16).toUpperCase();

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

  const zip = `mojibake-amalgamation-${v.major}${v.minor}${v.revision}`;
  fileContent = readFileSync('../../README.md', 'utf-8');
  fileContent = substituteBlock(fileContent, 'Download it here [', '.zip)',
    `${zip}.zip](https://github.com/zaerl/mojibake/releases/download/v${v.version}/${zip}`);

  writeFileSync('../../README.md', fileContent);

  // Find all SKILL.md files recursively under a directory
  function updateSkillMdFiles(dir: string) {
    const entries = readdirSync(dir);

    for(const entry of entries) {
      const full = path.join(dir, entry);
      const stat = statSync(full);

      if(stat.isDirectory()) {
        updateSkillMdFiles(full);
      } else if(entry === 'SKILL.md') {
        fileContent = readFileSync(full, 'utf-8');
        fileContent = substituteBlock(fileContent, 'version: ', '\n', v.version);
        writeFileSync(full, fileContent);
      }
    }
  }

  // Update .claude/skills/*/SKILL.md version fields
  updateSkillMdFiles(path.resolve(__dirname, '../../../.claude/skills'));

  console.log(`\nVersion updated to ${v.version}`);
  console.log('make wasm');
  console.log('make amalgamation');
  console.log('npm i in utils/generate');
  console.log('npm i in src/api');
  console.log(`Update CHANGELOG.md: add a [${v.version}] section`);
  console.log(`git commit --signoff -am "Update version to ${v.version}"`);
  console.log(`git tag v${v.version}`);
  console.log(`git push && git push origin --tags`);
  console.log('Deploy');
}
