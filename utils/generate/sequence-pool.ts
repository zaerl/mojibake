/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

// Finds how many values at the end of data overlap the start of values.
export function suffixPrefixOverlap(data: number[], values: number[]) {
  const max = Math.min(data.length, values.length - 1);

  outer:
  for(let length = max; length > 0; --length) {
    const start = data.length - length;

    for(let i = 0; i < length; ++i) {
      if(data[start + i] !== values[i]) {
        continue outer;
      }
    }

    return length;
  }

  return 0;
}

type SuffixState = {
  length: number;
  link: number;
  transitions: Map<number, number>;
  earliestEnd: number;
};

// Incrementally indexes every substring of a value stream. Keeping the earliest end position on
// each state makes find() return the first occurrence without rescanning the packed data.
export class SuffixAutomaton {
  private states: SuffixState[] = [{
    length: 0,
    link: -1,
    transitions: new Map(),
    earliestEnd: -1,
  }];
  private last = 0;
  private dataLength = 0;

  find(values: number[]) {
    let state = 0;

    for(const value of values) {
      const next = this.states[state].transitions.get(value);

      if(next === undefined) {
        return -1;
      }

      state = next;
    }

    return this.states[state].earliestEnd - values.length + 1;
  }

  append(values: number[]) {
    for(const value of values) {
      this.appendValue(value);
    }
  }

  private appendValue(value: number) {
    const end = this.dataLength++;
    const current = this.states.length;
    this.states.push({
      length: this.states[this.last].length + 1,
      link: 0,
      transitions: new Map(),
      earliestEnd: end,
    });

    let state = this.last;

    while(state >= 0 && !this.states[state].transitions.has(value)) {
      this.states[state].transitions.set(value, current);
      state = this.states[state].link;
    }

    if(state < 0) {
      this.states[current].link = 0;
    } else {
      const next = this.states[state].transitions.get(value)!;

      if(this.states[state].length + 1 === this.states[next].length) {
        this.states[current].link = next;
      } else {
        const clone = this.states.length;
        this.states.push({
          length: this.states[state].length + 1,
          link: this.states[next].link,
          transitions: new Map(this.states[next].transitions),
          earliestEnd: this.states[next].earliestEnd,
        });

        while(state >= 0 && this.states[state].transitions.get(value) === next) {
          this.states[state].transitions.set(value, clone);
          state = this.states[state].link;
        }

        this.states[next].link = clone;
        this.states[current].link = clone;
      }
    }

    this.last = current;
  }
}

// Longest codepoint sequence the runtime decodes into a caller buffer. Mirrors
// MJB_UNICODE_SEQUENCE_MAX in src/unicode-tables.h.
export const SEQUENCE_MAX_CODEPOINTS = 18;

// Encodes codepoints as 16-bit units: BMP codepoints directly, others as surrogate pairs.
export function encodeSequenceUnits(codepoints: number[]) {
  const units: number[] = [];

  for(const codepoint of codepoints) {
    if(codepoint < 0 || codepoint > 0x10FFFF || (codepoint >= 0xD800 && codepoint <= 0xDFFF)) {
      throw new Error(`Codepoint cannot be stored as sequence units: ${codepoint}`);
    }

    if(codepoint < 0x10000) {
      units.push(codepoint);
    } else {
      const offset = codepoint - 0x10000;
      units.push(0xD800 | (offset >> 10), 0xDC00 | (offset & 0x3FF));
    }
  }

  return units;
}

// A pool of 16-bit sequence units packed with substring and overlap sharing. An extension pool
// chains to a frozen base pool: sequences found in the base keep their base offsets, so an
// optional table can reuse core payload while its own units compile out with the feature.
export class SequencePool {
  readonly units: number[] = [];
  private readonly automaton = new SuffixAutomaton();
  private readonly base: SequencePool | null;
  private frozen = false;

  constructor(base: SequencePool | null = null) {
    this.base = base;

    if(base !== null) {
      base.frozen = true;
    }
  }

  // The first virtual offset of this pool's own units.
  get baseSize(): number {
    return this.base === null ? 0 : this.base.baseSize + this.base.units.length;
  }

  // Finds an already stored sequence and returns its virtual offset, or -1.
  find(units: number[]): number {
    if(this.base !== null) {
      const offset = this.base.find(units);

      if(offset >= 0) {
        return offset;
      }
    }

    const offset = this.automaton.find(units);

    return offset < 0 ? -1 : this.baseSize + offset;
  }

  // Stores a sequence, reusing existing units when possible, and returns its virtual offset.
  add(units: number[]): number {
    if(units.length === 0) {
      throw new Error('Cannot pool an empty sequence');
    }

    const existing = this.find(units);

    if(existing >= 0) {
      return existing;
    }

    if(this.frozen) {
      throw new Error('Cannot add sequences to a frozen base pool');
    }

    const overlap = suffixPrefixOverlap(this.units, units);
    const offset = this.units.length - overlap;
    const appended = units.slice(overlap);
    this.units.push(...appended);
    this.automaton.append(appended);

    return this.baseSize + offset;
  }
}

// Adds every sequence to a pool, longest first for better sharing, and returns the virtual offset
// and codepoint count of each input sequence.
export function poolSequences(pool: SequencePool, sequences: number[][]) {
  const unique = new Map<string, number[]>();

  for(const codepoints of sequences) {
    if(codepoints.length > SEQUENCE_MAX_CODEPOINTS) {
      throw new Error(`Sequence is too long to pool: ${codepoints.length}`);
    }

    unique.set(codepoints.join(','), codepoints);
  }

  const offsets = new Map<string, number>();
  const ordered = [...unique.entries()].sort(([, a], [, b]) => {
    const unitsA = encodeSequenceUnits(a);
    const unitsB = encodeSequenceUnits(b);

    if(unitsA.length !== unitsB.length) {
      return unitsB.length - unitsA.length;
    }

    for(let i = 0; i < unitsA.length; ++i) {
      if(unitsA[i] !== unitsB[i]) {
        return unitsA[i] - unitsB[i];
      }
    }

    return 0;
  });

  for(const [key, codepoints] of ordered) {
    offsets.set(key, pool.add(encodeSequenceUnits(codepoints)));
  }

  return sequences.map((codepoints) => {
    const offset = offsets.get(codepoints.join(','));

    if(offset === undefined) {
      throw new Error('Missing pooled sequence');
    }

    return { offset, length: codepoints.length };
  });
}
