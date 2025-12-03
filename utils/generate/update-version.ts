import { readFileSync, writeFileSync } from 'fs';
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
}
