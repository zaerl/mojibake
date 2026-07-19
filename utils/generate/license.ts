/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

const mojibakeFileLicense = `/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */`;

export function getFileLicense(description: string[] = []) {
  if(description.length) {
    return mojibakeFileLicense.replace('*/', `*\n * ${description.join('\n * ')}\n */`);
  }

  return mojibakeFileLicense;
}
