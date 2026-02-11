/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { readdirSync, readFileSync, statSync, writeFileSync } from 'fs';
import path from 'path';
import { getVersion, substituteBlock } from './utils';

export async function updateVersion() {
  let fileContent = readFileSync('../../src/mojibake.h', 'utf-8');
  const v = getVersion();
  const versionNumber = v.major << 8 | v.minor << 4 | v.revision;

  fileContent = substituteBlock(fileContent, '#define MJB_VERSION_NUMBER', '\n',
    `   0x${versionNumber.toString(16)} // MAJOR << 8 | MINOR << 4 | REVISION`);
  fileContent = substituteBlock(fileContent, '#define MJB_VERSION_MAJOR', '\n', '    ' + v.major.toString());
  fileContent = substituteBlock(fileContent, '#define MJB_VERSION_MINOR', '\n', '    ' + v.minor.toString());
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
  updateSkillMdFiles(path.resolve(__dirname, '../../.claude/skills'));
}
