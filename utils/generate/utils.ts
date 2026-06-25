/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { createWriteStream, mkdirSync, readFileSync } from 'fs';
import { get } from 'https';
import { dirname } from 'path';
import { pipeline } from 'stream';
import { promisify } from 'util';

const DOWNLOAD_OPTIONS = {
  headers: {
    'User-Agent': 'mojibake-locale-generator',
  },
};

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

// Filter away characters that will not be included in generated tables.
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

// Used to generate the unicode tables

export function formatHex(value: number, width = 0) {
  const hex = value.toString(16).toUpperCase().padStart(width, '0');
  return `0x${hex}`;
}

export function formatHex64(value: bigint, width = 0) {
  const hex = value.toString(16).toUpperCase().padStart(width, '0');
  return `0x${hex}ULL`;
}

export function formatBytes(values: number[], columns = 12) {
  const lines = [];

  for(let i = 0; i < values.length; i += columns) {
    lines.push(`    ${values.slice(i, i + columns).map((value) => formatHex(value, 2)).join(', ')},`);
  }

  return lines.join('\n');
}

export function formatCodepoints(values: number[]) {
  const lines = [];

  for(let i = 0; i < values.length; i += 8) {
    lines.push(`    ${values.slice(i, i + 8).map((value) => formatHex(value, 4)).join(', ')},`);
  }

  return lines.join('\n');
}

export function formatHalfwords(values: number[], columns = 8) {
  const lines = [];

  for(let i = 0; i < values.length; i += columns) {
    lines.push(`    ${values.slice(i, i + columns).map((value) => formatHex(value, 4)).join(', ')},`);
  }

  return lines.join('\n');
}

export function formatWords(values: number[], columns = 6) {
  const lines = [];

  for(let i = 0; i < values.length; i += columns) {
    lines.push(`    ${values.slice(i, i + columns).map((value) => formatHex(value, 8)).join(', ')},`);
  }

  return lines.join('\n');
}

export function formatLongWords(values: bigint[], width = 14) {
  const lines = [];

  for(let i = 0; i < values.length; i += 4) {
    lines.push(`    ${values.slice(i, i + 4).map((value) => formatHex64(value, width)).join(', ')},`);
  }

  return lines.join('\n');
}

// Appends a NUL-terminated string to a shared byte table and returns its offset.
export function addStringData(data: number[], offsets: Map<string, number>, value: string) {
  const existing = offsets.get(value);

  if(existing !== undefined) {
    return existing;
  }

  const offset = data.length;

  for(let i = 0; i < value.length; ++i) {
    data.push(value.charCodeAt(i));
  }

  data.push(0);
  offsets.set(value, offset);

  return offset;
}

// Formats generated mjb_unicode_page initializers.
export function formatPages(pages: { starts: number[]; counts: number[] }) {
  return pages.starts.map((start, index) => `    { ${start}, ${pages.counts[index]} },`).join('\n');
}

// Formats strings as adjacent escaped C string literals with NUL terminators.
export function cStringData(values: string[]) {
  return values.map((value) =>
    `    "${value.replace(/\\/g, '\\\\').replace(/"/g, '\\"')}\\0"`
  ).join('\n');
}

// Converts sparse page metadata into a compact page table plus page index.
export function indexedPages(pages: { starts: number[]; counts: number[] }) {
  const index = new Array(pages.starts.length).fill(0xFFFF);
  const starts: number[] = [];
  const counts: number[] = [];

  for(let page = 0; page < pages.starts.length; ++page) {
    if(pages.counts[page] === 0) {
      continue;
    }

    if(starts.length >= 0xFFFF) {
      throw new Error('Indexed page table is too large to pack');
    }

    index[page] = starts.length;
    starts.push(pages.starts[page]);
    counts.push(pages.counts[page]);
  }

  return { index, pages: { starts, counts } };
}

// Packs boolean values into little-endian bitset bytes.
export function bitset(values: boolean[]) {
  const data = new Array(Math.ceil(values.length / 8)).fill(0);

  for(let i = 0; i < values.length; ++i) {
    if(values[i]) {
      data[i >> 3] |= 1 << (i & 7);
    }
  }

  return data;
}

// Builds per-256-codepoint page start/count metadata for codepoint rows.
export function codepointPages(rows: { codepoint: number }[]) {
  const pageCount = rows.length === 0 ? 0 : (rows[rows.length - 1].codepoint >> 8) + 1;
  const starts: number[] = [];
  const counts: number[] = [];
  let index = 0;

  for(let page = 0; page < pageCount; ++page) {
    const start = index;

    while(index < rows.length && (rows[index].codepoint >> 8) === page) {
      ++index;
    }

    starts.push(start);
    counts.push(index - start);
  }

  return { starts, counts };
}

// Packs codepoint sequences by reusing duplicate and substring payloads.
export function packCodepointSequences(sequences: number[][]) {
  const data: number[] = [];
  const dataOffsets = new Map<string, { offset: number; length: number }>();
  const uniqueSequences = [...new Map(sequences.map((values) => [values.join(','), values])).values()]
    .sort((a, b) => b.length - a.length);

  // Finds an existing payload offset for a codepoint sequence.
  const findDataOffset = (values: number[]) => {
    for(let offset = 0; offset <= data.length - values.length; ++offset) {
      let matches = true;

      for(let i = 0; i < values.length; ++i) {
        if(data[offset + i] !== values[i]) {
          matches = false;
          break;
        }
      }

      if(matches) {
        return offset;
      }
    }

    return -1;
  };

  for(const values of uniqueSequences) {
    const key = values.join(',');
    let offset = findDataOffset(values);

    if(offset < 0) {
      offset = data.length;
      data.push(...values);
    }

    dataOffsets.set(key, { offset, length: values.length });
  }

  return {
    data,
    entries: sequences.map((values) => {
      const entry = dataOffsets.get(values.join(','));

      if(entry === undefined) {
        throw new Error('Missing packed codepoint sequence');
      }

      return entry;
    }),
  };
}

// Compares byte arrays lexicographically for stable packing order.
export function compareBytes(a: number[], b: number[]) {
  const length = Math.min(a.length, b.length);

  for(let i = 0; i < length; ++i) {
    if(a[i] !== b[i]) {
      return a[i] - b[i];
    }
  }

  return a.length - b.length;
}

export async function downloadFile(url: string, dest: string) {
  const streamPipeline = promisify(pipeline);
  mkdirSync(dirname(dest), { recursive: true });

  return new Promise((resolve, reject) => {
    get(url, DOWNLOAD_OPTIONS, async (res) => {
      if(res.statusCode !== 200) {
        reject(new Error(`Failed to get '${url}' (${res.statusCode})`));
        res.resume();

        return;
      }

      const fileStream = createWriteStream(dest);

      try {
        await streamPipeline(res, fileStream);
        resolve(undefined);
      } catch(err) {
        reject(err);
      }
    }).on('error', reject);
  });
}

export async function downloadText(url: string): Promise<string> {
  return new Promise((resolve, reject) => {
    get(url, DOWNLOAD_OPTIONS, (res) => {
      if(res.statusCode !== 200) {
        reject(new Error(`Failed to get '${url}' (${res.statusCode})`));
        res.resume();

        return;
      }

      res.setEncoding('utf-8');
      let data = '';

      res.on('data', chunk => {
        data += chunk;
      });

      res.on('end', () => {
        resolve(data);
      });
    }).on('error', reject);
  });
}
