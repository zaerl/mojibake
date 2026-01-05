/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { readFileSync } from "fs";

export function substituteBlock(fileContent: string, start: string, end: string | null, replacement: string) {
  const startIndex = fileContent.indexOf(start) + start.length;
  const endIndex = end === null ? null : fileContent.indexOf(end, startIndex);

  const ret = fileContent.slice(0, startIndex) + replacement;

  if(endIndex !== null) {
    return ret + fileContent.slice(endIndex);
  } else {
    return ret + "\n";
  }
}

export function substituteText(fileContent: string, block: string, replacement: string) {
  return fileContent.split(block).join(replacement);
}

export function commonPrefix(str1: string, str2: string): string {
  let prefix = '';
  let specialCases = [
    'ANATOLIAN HIEROGLYPH ',
    'BAMUM LETTER PHASE',
    'BRAILLE PATTERN DOTS-',
    'CJK COMPATIBILITY IDEOGRAPH',
    'EGYPTIAN HIEROGLYPH',
    'MATHEMATICAL BOLD',
    'MATHEMATICAL SANS-SERIF',
    'NUSHU CHARACTER-',
    'VARIATION SELECTOR-',
  ];

  for(let specialCase of specialCases) {
    if(str1.startsWith(specialCase) && str2.startsWith(specialCase)) {
      return specialCase;
    }
  }

  const minLength = Math.min(str1.length, str2.length);

  for(let i = 0; i < minLength; i++) {
    if(str1[i] === str2[i]) {
      prefix += str1[i];
    } else {
      break;
    }
  }

  return prefix;
}

export function getVersion() {
  const version = readFileSync('../../VERSION', 'utf-8').trim().split('.');
  const major = parseInt(version[0]);
  const minor = parseInt(version[1]);
  const revision = parseInt(version[2]);

  return {
    major,
    minor,
    revision,
    version: `${major}.${minor}.${revision}`,
  }
}

export function compressName(codepoint: number, name: string): string | null {
  // Strip away egyptian names
  if(codepoint >= 0x13000 && codepoint <= 0x143FF) {
    if(codepoint >= 0x13460) {
      return null;
    } else {
      return name.replace('EGYPTIAN HIEROGLYPH ', '');
    }
  } else if(codepoint >= 0xF900 && codepoint <= 0xFAD9) {
    // CJK Compatibility Ideographs
    return null;
  } else if(codepoint >= 0x2F800 && codepoint <= 0x2FA1F) {
    // CJK Compatibility Ideographs Supplement
    return null;
  } else if(codepoint >= 0x14400 && codepoint <= 0x1467F) {
    // Anatolian Hieroglyphs
    return name.replace('ANATOLIAN HIEROGLYPH A', '');
  } else if((codepoint >= 0x12000 && codepoint <= 0x12399) ||
    (codepoint >= 0x12480 && codepoint <= 0x12543)) {
    // Cuneiform signs
    return name.replace('CUNEIFORM SIGN ', '');
  }

  return name;
}
