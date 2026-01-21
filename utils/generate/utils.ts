/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { readFileSync } from 'fs';

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

const EGYPTIAN_H_START = 0x13000;
const EGYPTIAN_H_FORMAT_EXT_START = 0x13460;
const EGYPTIAN_H_EXT_END = 0x143FF;

const TANGUT_COMPONENT_START = 0x18800;
const TANGUT_COMPONENT_END = 0x18AFF;

const TANGUT_COMPONENT_SUPPLEMENT_START = 0x18D80;
const TANGUT_COMPONENT_SUPPLEMENT_END = 0x18DF2;

const KHITAN_SMALL_SCRIPT_CHARACTER_START = 0x18B00;
const KHITAN_SMALL_SCRIPT_CHARACTER_END = 0x18CFF;

export interface CodepointsRange {
  range: boolean;
  name: string;
  rangeStart: number;
  rangeEnd: number;
}

export type CodepointsRangeMap = { [name: string]: CodepointsRange };

// Filter away characters that will not be included in the database
export function isCodepointOnRanges(codepoint: number, name: string): CodepointsRange | null {
  const ret: CodepointsRange = {
    range: false,
    name: name,
    rangeStart: 0,
    rangeEnd: 0,
  };

  if(name.startsWith('<') && name !== '<control>') {
    const split = name.split(',');
    ret.name = split[0].substring(1);
    ret.range = true;

    return ret;
  }

  if(codepoint >= EGYPTIAN_H_FORMAT_EXT_START &&
    codepoint <= EGYPTIAN_H_EXT_END) {
    ret.range = true;
    ret.name = 'EGYPTIAN HIEROGLYPH';
    ret.rangeStart = EGYPTIAN_H_FORMAT_EXT_START;
    ret.rangeEnd = EGYPTIAN_H_EXT_END;

    return ret;
  }

  if(codepoint >= TANGUT_COMPONENT_START &&
    codepoint <= TANGUT_COMPONENT_END) {
    ret.range = true;
    ret.name = 'TANGUT COMPONENT';
    ret.rangeStart = TANGUT_COMPONENT_START;
    ret.rangeEnd = TANGUT_COMPONENT_END;

    return ret;
  }

  if(codepoint >= TANGUT_COMPONENT_SUPPLEMENT_START &&
    codepoint <= TANGUT_COMPONENT_SUPPLEMENT_END) {
    ret.range = true;
    ret.name = 'TANGUT COMPONENT SUPPLEMENT';
    ret.rangeStart = TANGUT_COMPONENT_SUPPLEMENT_START;
    ret.rangeEnd = TANGUT_COMPONENT_SUPPLEMENT_END;

    return ret;
  }

  if(codepoint >= KHITAN_SMALL_SCRIPT_CHARACTER_START &&
    codepoint <= KHITAN_SMALL_SCRIPT_CHARACTER_END) {
    ret.range = true;
    ret.name = 'KHITAN SMALL SCRIPT CHARACTER';
    ret.rangeStart = KHITAN_SMALL_SCRIPT_CHARACTER_START;
    ret.rangeEnd = KHITAN_SMALL_SCRIPT_CHARACTER_END;

    return ret;
  }

  return ret;
}

export function compressName(codepoint: number, name: string): string | null {
  // Strip away egyptian names
  // EGYPTIAN HIEROGLYPH A001 -> EGYPTIAN HIEROGLYPH-143FA -> U+143FF
  if(codepoint >= EGYPTIAN_H_START && codepoint <= EGYPTIAN_H_EXT_END) {
    return name.replace('EGYPTIAN HIEROGLYPH ', '');
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
