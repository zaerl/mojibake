/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { open } from 'fs/promises';

export function ucdInt(field: string, base: number = 16) {
  if(field === '') {
    return null;
  }

  return parseInt(field, base);
}

export function ucdString(field: string) {
  if(field === '') {
    return null;
  }

  return field;
}

export function ucdBool(field: string) {
  if(field === '') {
    return false;
  }

  return field === 'Y';
}

export function ucdCodepointRange(field: string) {
  let codepointStart = 0;
  let codepointEnd = 0;

  if(field.includes('..')) {
    const codepoints = field.split('..');

    if(codepoints.length === 2) {
      codepointStart = ucdInt(codepoints[0]) as number;
      codepointEnd = ucdInt(codepoints[1]) as number;
    }
  } else {
    codepointStart = ucdInt(field) as number;
    codepointEnd = codepointStart;
  }

  return {
    codepointStart,
    codepointEnd
  };
}

// Parse files like DerivedNormalizationProps.txt, SpecialCasing.txt, etc.
export async function* parsePropertyFile(path: string, starts: string[] = [], divider: string = ';', filterEmpty: boolean = true) {
  const file = await open(path);

  for await (let line of file.readLines()) {
    line = line.trim();

    // Comment or empty line
    if(!line || line === '' || line.startsWith('#') || starts.some(start => line.startsWith(start))) {
      continue
    }

    const split = line.split(divider).map(s => s.trim());

    if(split.length) {
      // If the last element contains a '#', return the left part
      const last = split[split.length - 1];

      if(last.includes('#')) {
        split[split.length - 1] = last.split('#')[0].trim();
      }
    }

    yield filterEmpty ? split.filter(s => s !== '') : split;
  }

  file.close();
}

export function ucdNameToEnumName(prefix: string, name: string): string {
  return `${prefix}_${name.toUpperCase().replace(/[ \-]/g, '_')}`;
}
