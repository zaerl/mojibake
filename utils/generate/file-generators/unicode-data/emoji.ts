/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { iLog } from '../../log';
import { formatCodepoints, formatHalfwords, formatLongWords, formatWords } from '../../utils';
import { EmojiRow, EmojiSequenceRow } from '../types';

const enum EmojiFlag {
  Emoji = 1 << 0,
  Presentation = 1 << 1,
  Modifier = 1 << 2,
  ModifierBase = 1 << 3,
  Component = 1 << 4,
  ExtendedPictographic = 1 << 5,
}

const enum EmojiSequenceEntry {
  EdgeStartBits = 13,
  EdgeCountBits = 11,
  TypeBits = 4,
  QualificationBits = 3,
  EdgeCountShift = 13,
  TypeShift = 24,
  QualificationShift = 28,
}

type EmojiTrieNode = {
  children: Map<number, EmojiTrieNode>;
  index: number;
  canonicalId: number;
  edgeStart: number;
  type: number;
  qualification: number;
};

// Encodes emoji boolean columns into the generated emoji flag bitset.
function emojiFlags(row: EmojiRow) {
  let flags = 0;

  if(row.emoji) {
    flags |= EmojiFlag.Emoji;
  }

  if(row.emoji_presentation) {
    flags |= EmojiFlag.Presentation;
  }

  if(row.emoji_modifier) {
    flags |= EmojiFlag.Modifier;
  }

  if(row.emoji_modifier_base) {
    flags |= EmojiFlag.ModifierBase;
  }

  if(row.emoji_component) {
    flags |= EmojiFlag.Component;
  }

  if(row.extended_pictographic) {
    flags |= EmojiFlag.ExtendedPictographic;
  }

  return flags;
}

function generateEmojiSequenceData(rows: EmojiSequenceRow[]) {
  const root: EmojiTrieNode = {
    children: new Map(),
    index: -1,
    canonicalId: -1,
    edgeStart: 0,
    type: 0,
    qualification: 0,
  };

  for(const row of rows) {
    let node = root;

    for(const codepoint of row.codepoints) {
      let child = node.children.get(codepoint);

      if(child === undefined) {
        child = {
          children: new Map(),
          index: -1,
          canonicalId: -1,
          edgeStart: 0,
          type: 0,
          qualification: 0,
        };
        node.children.set(codepoint, child);
      }

      node = child;
    }

    node.type = row.type;
    node.qualification = row.qualification;
  }

  const canonicalNodes: EmojiTrieNode[] = [];
  const canonicalByKey = new Map<string, EmojiTrieNode>();

  function canonicalize(node: EmojiTrieNode): EmojiTrieNode {
    const children = [...node.children.entries()]
      .sort((a, b) => a[0] - b[0])
      .map(([codepoint, child]) => [codepoint, canonicalize(child)] as const);
    const key = JSON.stringify([
      node.type,
      node.qualification,
      children.map(([codepoint, child]) => [codepoint, child.canonicalId]),
    ]);
    const existing = canonicalByKey.get(key);

    if(existing !== undefined) {
      return existing;
    }

    const canonical: EmojiTrieNode = {
      children: new Map(children),
      index: -1,
      canonicalId: canonicalNodes.length,
      edgeStart: 0,
      type: node.type,
      qualification: node.qualification,
    };

    canonicalNodes.push(canonical);
    canonicalByKey.set(key, canonical);

    return canonical;
  }

  const canonicalRoot = canonicalize(root);

  const nodes: EmojiTrieNode[] = [];
  const edgeCodepoints: number[] = [];
  const edgeChildren: number[] = [];

  function addNode(node: EmojiTrieNode) {
    if(node.index >= 0) {
      return;
    }

    node.index = nodes.length;
    nodes.push(node);

    const edges = [...node.children.entries()].sort((a, b) => a[0] - b[0]);
    node.edgeStart = edgeCodepoints.length;
    const edgeStart = edgeCodepoints.length;

    for(const [codepoint, child] of edges) {
      edgeCodepoints.push(codepoint);
      edgeChildren.push(0);
    }

    for(let i = 0; i < edges.length; ++i) {
      const child = edges[i][1];
      addNode(child);
      edgeChildren[edgeStart + i] = child.index;
    }
  }

  addNode(canonicalRoot);

  const entries = nodes.map((node) => {
    const edgeCount = node.children.size;

    if(node.edgeStart >= (1 << EmojiSequenceEntry.EdgeStartBits) ||
      edgeCount >= (1 << EmojiSequenceEntry.EdgeCountBits) ||
      node.type >= (1 << EmojiSequenceEntry.TypeBits) ||
      node.qualification >= (1 << EmojiSequenceEntry.QualificationBits)) {
      throw new Error(
        `Emoji trie node out of bounds: edgeStart=${node.edgeStart}, ` +
        `edgeCount=${edgeCount}, type=${node.type}, qualification=${node.qualification}`
      );
    }

    return node.edgeStart |
      (edgeCount << EmojiSequenceEntry.EdgeCountShift) |
      (node.type << EmojiSequenceEntry.TypeShift) |
      (node.qualification << EmojiSequenceEntry.QualificationShift);
  });

  if(edgeCodepoints.length !== edgeChildren.length) {
    throw new Error('Emoji trie edge arrays are mismatched');
  }

  if(nodes.length >= 0x10000) {
    throw new Error(`Emoji trie has too many nodes: ${nodes.length}`);
  }

  for(const child of edgeChildren) {
    if(child >= 0x10000) {
      throw new Error(`Emoji trie child index out of bounds: ${child}`);
    }
  }

  for(const row of rows) {
    if(row.type >= (1 << EmojiSequenceEntry.TypeBits) ||
      row.qualification >= (1 << EmojiSequenceEntry.QualificationBits)) {
      throw new Error(
        `Emoji sequence out of bounds: length=${row.codepoints.length}, ` +
        `type=${row.type}, qualification=${row.qualification}`
      );
    }
  }

  return `typedef uint32_t mjb_unicode_emoji_sequence_node;

enum {
    MJB_UNICODE_EMOJI_SEQUENCE_EDGE_START_MASK = ${(1 << EmojiSequenceEntry.EdgeStartBits) - 1},
    MJB_UNICODE_EMOJI_SEQUENCE_EDGE_COUNT_SHIFT = ${EmojiSequenceEntry.EdgeCountShift},
    MJB_UNICODE_EMOJI_SEQUENCE_EDGE_COUNT_MASK = ${(1 << EmojiSequenceEntry.EdgeCountBits) - 1},
    MJB_UNICODE_EMOJI_SEQUENCE_TYPE_SHIFT = ${EmojiSequenceEntry.TypeShift},
    MJB_UNICODE_EMOJI_SEQUENCE_TYPE_MASK = ${(1 << EmojiSequenceEntry.TypeBits) - 1},
    MJB_UNICODE_EMOJI_SEQUENCE_QUALIFICATION_SHIFT = ${EmojiSequenceEntry.QualificationShift},
    MJB_UNICODE_EMOJI_SEQUENCE_QUALIFICATION_MASK = ${(1 << EmojiSequenceEntry.QualificationBits) - 1}
};

static const mjb_unicode_emoji_sequence_node mjb_unicode_emoji_sequence_nodes[] = {
${formatWords(entries)}
};

static const mjb_codepoint mjb_unicode_emoji_sequence_codepoints[] = {
${formatCodepoints(edgeCodepoints)}
};

static const uint16_t mjb_unicode_emoji_sequence_children[] = {
${formatHalfwords(edgeChildren)}
};
`;
}

// Emits compact emoji property ranges with packed flags.
export function generateEmoji(rows: EmojiRow[], sequenceRows: EmojiSequenceRow[]) {
  iLog('Emoji');

  const ranges: { start: number; end: number; flags: number }[] = [];

  for(const row of rows) {
    const flags = emojiFlags(row);
    const previous = ranges[ranges.length - 1];

    if(previous && previous.end + 1 === row.codepoint && previous.flags === flags) {
      previous.end = row.codepoint;
    } else {
      ranges.push({ start: row.codepoint, end: row.codepoint, flags });
    }
  }

  const entries = ranges.map((range) => {
    const delta = range.end - range.start;

    if(range.start > 0x1FFFFF || delta > 0xFFFF || range.flags > 0x3F) {
      throw new Error(
        `Emoji range out of bounds: start=${range.start}, delta=${delta}, flags=${range.flags}`
      );
    }

    return BigInt(range.start) | (BigInt(delta) << 21n) | (BigInt(range.flags) << 37n);
  });

  return `typedef uint64_t mjb_unicode_emoji_entry;

enum {
    MJB_UNICODE_EMOJI_FLAG_EMOJI = ${EmojiFlag.Emoji},
    MJB_UNICODE_EMOJI_FLAG_PRESENTATION = ${EmojiFlag.Presentation},
    MJB_UNICODE_EMOJI_FLAG_MODIFIER = ${EmojiFlag.Modifier},
    MJB_UNICODE_EMOJI_FLAG_MODIFIER_BASE = ${EmojiFlag.ModifierBase},
    MJB_UNICODE_EMOJI_FLAG_COMPONENT = ${EmojiFlag.Component},
    MJB_UNICODE_EMOJI_FLAG_EXTENDED_PICTOGRAPHIC = ${EmojiFlag.ExtendedPictographic}
};

static const mjb_unicode_emoji_entry mjb_unicode_emoji[] = {
${formatLongWords(entries)}
};

${generateEmojiSequenceData(sequenceRows)}
`;
}
