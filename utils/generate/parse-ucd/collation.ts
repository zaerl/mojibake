/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { createReadStream } from 'fs';
import { createInterface } from 'readline';

export interface CollationElement {
  primary: number;   // 0x0000–0xFFFF
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
 * Encode CollationElement array as a four-byte BLOB.
 *
 * Bytes 0–1 contain the little-endian primary weight
 * Byte 2 contains secondary bits 0–7
 * Byte 3 contains secondary bit 8, five tertiary bits, and the variable flag.
 * The DUCET field limits therefore fit in 31 bits without changing their values.
 */
export function encodeCollationWeights(elements: CollationElement[]): Buffer {
  const buf = Buffer.allocUnsafe(elements.length * 4);

  for(let i = 0; i < elements.length; i++) {
    const element = elements[i];

    if(element.primary > 0xFFFF || element.secondary > 0x1FF || element.tertiary > 0x1F) {
      throw new Error(
        `Collation element is too large to pack: ${element.primary}.` +
        `${element.secondary}.${element.tertiary}`
      );
    }

    buf.writeUInt16LE(element.primary, i * 4);
    buf[i * 4 + 2] = element.secondary & 0xFF;
    buf[i * 4 + 3] = (element.secondary >> 8) | (element.tertiary << 1) |
      (element.variable ? 0x40 : 0);
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
