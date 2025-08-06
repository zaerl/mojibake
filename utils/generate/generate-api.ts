import { writeFileSync } from 'fs';
import { cfns } from './cfunction';

export function generateAPI() {
  let fileContent = "## API\n\n";
  fileContent += cfns().map(value => value.formatMD()).join('\n\n');

  writeFileSync('../../API.md', fileContent);
}
