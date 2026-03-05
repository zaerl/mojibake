/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { createReadStream } from 'fs';
import { createInterface } from 'readline';

export interface CollationElement {
  primary: number;   // 0x0000–0x73C2
  secondary: number; // 0x0000–0x0127
  tertiary: number;  // 0x0000–0x001F
  variable: boolean; // true if marked with * in DUCET
}

export interface CollationEntry {
  codepoints: number[];         // 1–3 codepoints (contraction key)
  elements: CollationElement[]; // 1–18 collation elements
}

export interface ImplicitWeightRange {
  start: number;
  end: number;
  base: number;
}

/**
 * Parse the DUCET allkeys.txt file.
 * Returns single-codepoint entries, contractions, and @implicitweights ranges.
 */
export async function parseCollationAllKeys(path: string): Promise<{
  entries: CollationEntry[];
  implicitRanges: ImplicitWeightRange[];
}> {
  const entries: CollationEntry[] = [];
  const implicitRanges: ImplicitWeightRange[] = [];

  const rl = createInterface({
    input: createReadStream(path),
    crlfDelay: Infinity,
  });

  for await (const rawLine of rl) {
    const line = rawLine.trim();

    if(line === '' || line.startsWith('#')) {
      continue;
    }

    if(line.startsWith('@implicitweights')) {
      const m = line.match(/@implicitweights\s+([0-9A-Fa-f]+)\.\.([0-9A-Fa-f]+);\s*([0-9A-Fa-f]+)/);

      if(m) {
        implicitRanges.push({
          start: parseInt(m[1], 16),
          end:   parseInt(m[2], 16),
          base:  parseInt(m[3], 16),
        });
      }

      continue;
    }

    if(line.startsWith('@')) {
      continue;
    }

    // Format: HHHH [HHHH ...] ; [.*PPPP.SSSS.TTTT] ... # comment
    const semicolonIdx = line.indexOf(';');

    if(semicolonIdx < 0) {
      continue;
    }

    const cpPart = line.substring(0, semicolonIdx).trim();
    const rest   = line.substring(semicolonIdx + 1);

    const codepoints = cpPart
      .split(/\s+/)
      .filter(s => s.length > 0)
      .map(s => parseInt(s, 16));

    if(codepoints.length === 0) {
      continue;
    }

    const elements: CollationElement[] = [];
    const weightRegex = /\[([.*])([0-9A-Fa-f]{4})\.([0-9A-Fa-f]{4})\.([0-9A-Fa-f]{4})\]/g;
    let m: RegExpExecArray | null;

    while ((m = weightRegex.exec(rest)) !== null) {
      elements.push({
        variable:  m[1] === '*',
        primary:   parseInt(m[2], 16),
        secondary: parseInt(m[3], 16),
        tertiary:  parseInt(m[4], 16),
      });
    }

    if(elements.length === 0) {
      continue;
    }

    entries.push({ codepoints, elements });
  }

  return { entries, implicitRanges };
}

/**
 * Encode CollationElement array as a BLOB.
 * Each element = 6 bytes: [P_hi P_lo S_hi S_lo T_hi T_lo]
 * Bit 15 of TERTIARY is set if the element is variable.
 * (Primary can reach 0xFBC2 for CJK implicit ranges, so bit 15 of primary
 *  cannot safely be used as a flag. Tertiary max is 0x001F, leaving bits 5–15 free.)
 */
export function encodeCollationWeights(elements: CollationElement[]): Buffer {
  const buf = Buffer.allocUnsafe(elements.length * 6);

  for(let i = 0; i < elements.length; i++) {
    let tertiary = elements[i].tertiary;

    if(elements[i].variable) tertiary |= 0x8000;

    buf.writeUInt16BE(elements[i].primary,   i * 6);
    buf.writeUInt16BE(elements[i].secondary, i * 6 + 2);
    buf.writeUInt16BE(tertiary,              i * 6 + 4);
  }

  return buf;
}

/**
 * Encode a codepoint sequence as a BLOB: N × 4-byte big-endian uint32_t.
 */
export function encodeCodepointSequence(codepoints: number[]): Buffer {
  const buf = Buffer.allocUnsafe(codepoints.length * 4);

  for(let i = 0; i < codepoints.length; i++) {
    buf.writeUInt32BE(codepoints[i], i * 4);
  }

  return buf;
}
