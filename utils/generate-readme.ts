import { readFileSync, writeFileSync } from 'fs';
import { cfns } from './cfunction';
import { substituteText } from './utils';

export function generateReadme() {
  let fileContent = readFileSync('../README.md', 'utf-8');

  fileContent = substituteText(fileContent, "## API\n\n", null, cfns.map(value => value.formatMD()).join('\n\n'));

  writeFileSync('../README.md', fileContent);
}
