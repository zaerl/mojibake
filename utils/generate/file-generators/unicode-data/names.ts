/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { iLog } from '../../log';
import {
  codepointPageBitsets, codepointPages, formatBitsetPages, formatBytes, formatCompactIntegers,
  indexedPages,
} from '../../utils';
import { NameRow } from '../types';

// Token stream bytes. Words are lexicon ids (one byte, or a lead byte plus one byte) or spelled
// inline after a token carrying their length; a space is implied between two consecutive words.
const TOKEN_END = 0x00;
const TOKEN_HYPHEN = 0x01;
const TOKEN_SPACE = 0x02;
const TOKEN_SPELL_FIRST = 0x03;
const MAX_WORD_LENGTH = 24;
const TOKEN_WORD_FIRST = TOKEN_SPELL_FIRST + MAX_WORD_LENGTH;
const TOKEN_LEAD_FIRST = 0xF0;
const ONE_BYTE_WORDS = TOKEN_LEAD_FIRST - TOKEN_WORD_FIRST;
const TWO_BYTE_WORDS = (0x100 - TOKEN_LEAD_FIRST) * 0x100;
const MAX_PAGE_PREFIXES = 0x7F;
const MAX_NAME_LENGTH = 127;
// Decoders copy words in fixed 8-byte pieces, so word tables end with this much padding.
const WORD_PADDING = MAX_WORD_LENGTH;

type Token = string; // A word, '-', or ' '

// Splits a name into words and explicit separators. A single space between two words is implied
// by the encoding, so only other separator characters become tokens.
function tokenize(name: string): Token[] {
  const tokens: Token[] = [];
  const parts = name.match(/[^ -]+|[ -]+/g) ?? [];

  for(const part of parts) {
    if(part[0] !== ' ' && part[0] !== '-') {
      tokens.push(part);
    } else if(part !== ' ') {
      tokens.push(...part.split(''));
    }
  }

  return tokens;
}

// Words never contain '|', so it separates tokens in map keys.
function sequenceKey(tokens: Token[]) {
  return tokens.join('|');
}

// Emits page-prefixed character names as shared token streams over a word lexicon.
export function generateNames(rows: NameRow[]) {
  iLog('Names');

  const names = rows.map((row) => row.name ?? '');
  const tokenized = names.map(tokenize);
  const pages = indexedPages(codepointPages(rows));
  const pageBitsets = codepointPageBitsets(rows, pages.pages);

  for(const name of names) {
    if(name.length > MAX_NAME_LENGTH) {
      throw new Error(`Character name is too long for the runtime buffer: ${name}`);
    }

    for(let i = 0; i < name.length; ++i) {
      const code = name.charCodeAt(i);

      if(code < 0x20 || code >= 0x80 || name[i] === '|') {
        throw new Error(`Character name has an unsupported character: ${name}`);
      }
    }

    for(const token of tokenize(name)) {
      if(token.length > MAX_WORD_LENGTH) {
        throw new Error(`Character name word is too long to copy: ${token}`);
      }
    }
  }

  // Lexicon: the most frequent words get one byte, the next ones two bytes, the rest are spelled.
  let lexicon: string[] = [];
  let wordIds = new Map<string, number>();

  const buildLexicon = (sequences: Token[][]) => {
    const counts = new Map<string, number>();

    for(const tokens of sequences) {
      for(const token of tokens) {
        if(token !== '-' && token !== ' ') {
          counts.set(token, (counts.get(token) ?? 0) + 1);
        }
      }
    }

    lexicon = [...counts.entries()]
      .filter(([word, count]) => count >= 2 && word.length >= 2)
      .sort(([a, countA], [b, countB]) => countB - countA || (a < b ? -1 : a > b ? 1 : 0))
      .slice(0, ONE_BYTE_WORDS + TWO_BYTE_WORDS)
      .map(([word]) => word);
    wordIds = new Map(lexicon.map((word, id) => [word, id]));
  };

  const tokenBytes = (token: Token) => {
    if(token === '-' || token === ' ') {
      return 1;
    }

    const id = wordIds.get(token);

    if(id === undefined) {
      return 1 + token.length;
    }

    return id < ONE_BYTE_WORDS ? 1 : 2;
  };

  const sequenceBytes = (tokens: Token[]) =>
    tokens.reduce((sum, token) => sum + tokenBytes(token), 0);

  const encode = (tokens: Token[], terminate: boolean) => {
    const bytes: number[] = [];

    for(const token of tokens) {
      if(token === '-') {
        bytes.push(TOKEN_HYPHEN);
        continue;
      }

      if(token === ' ') {
        bytes.push(TOKEN_SPACE);
        continue;
      }

      const id = wordIds.get(token);

      if(id === undefined) {
        bytes.push(TOKEN_SPELL_FIRST + token.length - 1);

        for(let i = 0; i < token.length; ++i) {
          bytes.push(token.charCodeAt(i));
        }
      } else if(id < ONE_BYTE_WORDS) {
        bytes.push(TOKEN_WORD_FIRST + id);
      } else {
        const rest = id - ONE_BYTE_WORDS;
        bytes.push(TOKEN_LEAD_FIRST + (rest >> 8), rest & 0xFF);
      }
    }

    if(terminate) {
      bytes.push(TOKEN_END);
    }

    return bytes;
  };

  buildLexicon(tokenized);

  // Greedy page prefixes in token space: a prefix pays for its own bytes and one page-table
  // halfword per page that uses it, and each name takes its longest selected prefix.
  const prefixNames = new Map<string, { tokens: Token[]; names: number[] }>();

  tokenized.forEach((tokens, index) => {
    for(let length = 1; length <= tokens.length; ++length) {
      const prefix = tokens.slice(0, length);
      const key = sequenceKey(prefix);
      let entry = prefixNames.get(key);

      if(entry === undefined) {
        entry = { tokens: prefix, names: [] };
        prefixNames.set(key, entry);
      }

      entry.names.push(index);
    }
  });

  const pagesOf = (indexes: number[]) =>
    new Set(indexes.map((index) => rows[index].codepoint >> 8)).size;
  const candidates = [...prefixNames.values()]
    .filter((entry) => entry.names.length >= 2)
    .map((entry) => ({
      ...entry,
      bytes: sequenceBytes(entry.tokens),
      savings: (entry.names.length - 1) * sequenceBytes(entry.tokens) - pagesOf(entry.names) * 2,
    }))
    .sort((a, b) => b.savings - a.savings || b.tokens.length - a.tokens.length ||
      (sequenceKey(a.tokens) < sequenceKey(b.tokens) ? -1 : 1));
  const bestPrefix = new Array<Token[]>(rows.length).fill([]);

  for(const candidate of candidates) {
    let additional = 0;

    for(const index of candidate.names) {
      if(candidate.tokens.length > bestPrefix[index].length) {
        additional += candidate.bytes - sequenceBytes(bestPrefix[index]);
      }
    }

    if(additional - (candidate.bytes + 1 + pagesOf(candidate.names) * 2) <= 0) {
      continue;
    }

    for(const index of candidate.names) {
      if(candidate.tokens.length > bestPrefix[index].length) {
        bestPrefix[index] = candidate.tokens;
      }
    }
  }

  const remainders = tokenized.map((tokens, index) => tokens.slice(bestPrefix[index].length));

  // Rebuild the lexicon from what is actually stored: shared remainder roots and used prefixes.
  const uniqueRemainders = new Map<string, Token[]>();
  const uniquePrefixes = new Map<string, Token[]>();
  remainders.forEach((tokens) => uniqueRemainders.set(sequenceKey(tokens), tokens));
  bestPrefix.forEach((tokens) => uniquePrefixes.set(sequenceKey(tokens), tokens));
  buildLexicon([...uniqueRemainders.values(), ...uniquePrefixes.values()]);

  // Packs token sequences with suffix sharing and returns the byte offset of every sequence.
  const packSequences = (sequences: Token[][]) => {
    const data: number[] = [];
    const offsets = new Map<string, number>([['', 0]]);
    const covered = new Set<string>(['']);
    const longestFirst = [...new Map(sequences.map((tokens) => [sequenceKey(tokens), tokens]))
      .values()]
      .sort((a, b) => b.length - a.length || (sequenceKey(a) < sequenceKey(b) ? -1 : 1));

    // Offset zero holds the empty sequence.
    data.push(TOKEN_END);

    for(const tokens of longestFirst) {
      if(covered.has(sequenceKey(tokens))) {
        continue;
      }

      const start = data.length;
      data.push(...encode(tokens, true));

      let offset = start;

      for(let i = 0; i < tokens.length; ++i) {
        const suffix = sequenceKey(tokens.slice(i));

        if(!covered.has(suffix)) {
          covered.add(suffix);
          offsets.set(suffix, offset);
        }

        offset += tokenBytes(tokens[i]);
      }
    }

    return { data, offsets };
  };

  const packedRemainders = packSequences(remainders);
  const packedPrefixes = packSequences([...uniquePrefixes.values()]);

  if(packedRemainders.data.length > 0x1FFFF) {
    throw new Error(`Name data is too large to index: ${packedRemainders.data.length}`);
  }

  if(packedPrefixes.data.length > 0xFFFF) {
    throw new Error(`Name prefix data is too large to index: ${packedPrefixes.data.length}`);
  }

  // Per page: local prefix table, prefix runs with a block index, and entry offsets.
  const pagePrefixOffsets: number[] = [];
  const pagePrefixStarts: number[] = [];
  const prefixRuns: number[] = [];
  const pageRunStarts: number[] = [];
  const runBlockStarts: number[] = [];
  const nameOffsets: number[] = [];
  const nameOffsetHighBits: number[] = [];

  pages.pages.starts.forEach((start, page) => {
    const localPrefixes = new Map<string, number>();
    const end = start + pages.pages.counts[page];
    const pageRuns: { start: number; prefix: number }[] = [];

    pagePrefixStarts.push(pagePrefixOffsets.length);
    pageRunStarts.push(prefixRuns.length / 2);

    for(let index = start; index < end; ++index) {
      const key = sequenceKey(bestPrefix[index]);
      let local = localPrefixes.get(key);

      if(local === undefined) {
        local = localPrefixes.size;

        if(local >= MAX_PAGE_PREFIXES) {
          throw new Error(`Page ${page} has too many name prefixes: ${local + 1}`);
        }

        localPrefixes.set(key, local);
        pagePrefixOffsets.push(packedPrefixes.offsets.get(key)!);
      }

      const last = pageRuns[pageRuns.length - 1];

      if(last === undefined || last.prefix !== local) {
        pageRuns.push({ start: index - start, prefix: local });
      }

      const offset = packedRemainders.offsets.get(sequenceKey(remainders[index]))!;
      nameOffsets.push(offset & 0xFFFF);

      if((index & 7) === 0) {
        nameOffsetHighBits.push(0);
      }

      nameOffsetHighBits[nameOffsetHighBits.length - 1] |= (offset >> 16) << (index & 7);
    }

    // Each 16-entry block starts at the last run beginning at or before its first entry.
    let run = 0;

    for(let block = 0; block < 16; ++block) {
      while(run + 1 < pageRuns.length && pageRuns[run + 1].start <= block * 16) {
        ++run;
      }

      runBlockStarts.push(run);
    }

    for(const pageRun of pageRuns) {
      prefixRuns.push(pageRun.start, pageRun.prefix);
    }
  });

  pageRunStarts.push(prefixRuns.length / 2);

  if(pagePrefixOffsets.length > 0xFFFF || prefixRuns.length > 0xFFFF) {
    throw new Error('Name page tables are too large to index');
  }

  // Lexicon words are stored as a length byte followed by their characters.
  const lexiconData: number[] = [];
  const lexiconOffsets: number[] = [];

  for(const word of lexicon) {
    if(lexiconData.length > 0xFFFF) {
      throw new Error('Name lexicon is too large to index');
    }

    lexiconOffsets.push(lexiconData.length);
    lexiconData.push(word.length);

    for(let i = 0; i < word.length; ++i) {
      lexiconData.push(word.charCodeAt(i));
    }
  }

  const padding = new Array<number>(WORD_PADDING).fill(0);

  return `typedef struct mjb_unicode_page {
    uint16_t start;
    uint16_t count;
} mjb_unicode_page;

#if MJB_FEATURE_CHARACTER_NAMES
enum {
    MJB_UNICODE_NAME_TOKEN_END = ${TOKEN_END},
    MJB_UNICODE_NAME_TOKEN_HYPHEN = ${TOKEN_HYPHEN},
    MJB_UNICODE_NAME_TOKEN_SPACE = ${TOKEN_SPACE},
    MJB_UNICODE_NAME_TOKEN_SPELL_FIRST = ${TOKEN_SPELL_FIRST},
    MJB_UNICODE_NAME_TOKEN_WORD_FIRST = ${TOKEN_WORD_FIRST},
    MJB_UNICODE_NAME_TOKEN_LEAD_FIRST = ${TOKEN_LEAD_FIRST},
    MJB_UNICODE_NAME_WORD_MAX = ${MAX_WORD_LENGTH}
};

// Word tables end with MJB_UNICODE_NAME_WORD_MAX padding bytes for fixed-size copies.
static const uint8_t mjb_unicode_name_lexicon_data[] = {
${formatBytes([...lexiconData, ...padding], 24)}
};

static const uint16_t mjb_unicode_name_lexicon_offsets[] = {
${formatCompactIntegers(lexiconOffsets, 16)}
};

static const uint8_t mjb_unicode_name_prefix_data[] = {
${formatBytes([...packedPrefixes.data, ...padding], 24)}
};

static const uint16_t mjb_unicode_name_page_prefix_offsets[] = {
${formatCompactIntegers(pagePrefixOffsets, 16)}
};

// Pairs of (first page-local entry, local prefix id) for each run of identical prefixes.
static const uint8_t mjb_unicode_name_prefix_runs[] = {
${formatCompactIntegers(prefixRuns, 32)}
};

static const uint16_t mjb_unicode_name_page_run_starts[] = {
${formatCompactIntegers(pageRunStarts, 16)}
};

// Per page and 16-entry block, the page-relative index of the run covering the block start.
static const uint8_t mjb_unicode_name_run_block_starts[] = {
${formatCompactIntegers(runBlockStarts, 32)}
};

static const uint8_t mjb_unicode_name_data[] = {
${formatBytes([...packedRemainders.data, ...padding], 24)}
};

static const uint8_t mjb_unicode_name_page_index[] = {
${formatBytes(pages.index)}
};

// The extra halfword of each page is the start of its prefix table.
static const mjb_unicode_bitset_page mjb_unicode_name_pages[] = {
${formatBitsetPages(pages.pages, pageBitsets, pagePrefixStarts)}
};

static const uint16_t mjb_unicode_name_offsets[] = {
${formatCompactIntegers(nameOffsets, 16)}
};

static const uint8_t mjb_unicode_name_offset_high_bits[] = {
${formatCompactIntegers(nameOffsetHighBits, 32)}
};
#endif
`;
}
