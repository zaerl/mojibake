/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { writeFileSync } from 'fs';
import { cfns } from './function';

export function generateAPI() {
  let fileContent = "## API\n\n";
  fileContent += cfns().map(value => value.formatMD()).join('\n\n');

  writeFileSync('../../API.md', fileContent);
}
