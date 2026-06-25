/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { existsSync } from 'node:fs';
import { downloadFile } from '../utils';
import { CLDR_COMMON_URL } from './utils';

const CLDR_DATA_DIR = './locales/cldr';

const localeIdDataFiles = [
  // Locale ID validity for parser validation.
  'validity/language.xml',
  'validity/script.xml',
  'validity/region.xml',
  'validity/variant.xml',
  'validity/subdivision.xml',

  // Unicode locale extension keys and types.
  'bcp47/calendar.xml',
  'bcp47/collation.xml',
  'bcp47/currency.xml',
  'bcp47/measure.xml',
  'bcp47/number.xml',
  'bcp47/segmentation.xml',
  'bcp47/timezone.xml',
  'bcp47/transform.xml',
  'bcp47/transform-destination.xml',
  'bcp47/transform_hybrid.xml',
  'bcp47/transform_ime.xml',
  'bcp47/transform_keyboard.xml',
  'bcp47/transform_mt.xml',
  'bcp47/transform_private_use.xml',
  'bcp47/variant.xml',

  // Canonical aliases for language, script, territory, variant, and subdivisions.
  'supplemental/supplementalMetadata.xml',
];

export async function downloadLocaleIdData() {
  for(const file of localeIdDataFiles) {
    const url = `${CLDR_COMMON_URL}/${file}`;
    const dest = `${CLDR_DATA_DIR}/${file}`;

    if(existsSync(dest)) {
      continue;
    }

    console.log(`Downloading locale ID data: ${file}`);
    await downloadFile(url, dest);
  }
}
