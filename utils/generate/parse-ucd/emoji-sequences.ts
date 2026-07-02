/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { log } from '../log';
import { parsePropertyFile, ucdCodepointRange } from './utils';

export const enum EmojiSequenceType {
  None,
  Basic,
  Keycap,
  Flag,
  Tag,
  Modifier,
  ZWJ,
}

export const enum EmojiQualification {
  None,
  Component,
  FullyQualified,
  MinimallyQualified,
  Unqualified,
}

export type EmojiSequence = {
  codepoints: number[];
  type: EmojiSequenceType;
  qualification: EmojiQualification;
};

const sequenceTypes: Record<string, EmojiSequenceType> = {
  Basic_Emoji: EmojiSequenceType.Basic,
  Emoji_Keycap_Sequence: EmojiSequenceType.Keycap,
  RGI_Emoji_Flag_Sequence: EmojiSequenceType.Flag,
  RGI_Emoji_Tag_Sequence: EmojiSequenceType.Tag,
  RGI_Emoji_Modifier_Sequence: EmojiSequenceType.Modifier,
  RGI_Emoji_ZWJ_Sequence: EmojiSequenceType.ZWJ,
};

const qualifications: Record<string, EmojiQualification> = {
  component: EmojiQualification.Component,
  'fully-qualified': EmojiQualification.FullyQualified,
  'minimally-qualified': EmojiQualification.MinimallyQualified,
  unqualified: EmojiQualification.Unqualified,
};

function sequenceKey(codepoints: number[]) {
  return codepoints.join(' ');
}

function compareSequences(a: EmojiSequence, b: EmojiSequence) {
  const min = Math.min(a.codepoints.length, b.codepoints.length);

  for(let i = 0; i < min; ++i) {
    const diff = a.codepoints[i] - b.codepoints[i];

    if(diff !== 0) {
      return diff;
    }
  }

  return a.codepoints.length - b.codepoints.length;
}

function parseCodepointSequences(field: string) {
  const tokens = field.trim().split(/\s+/).filter(Boolean);
  const sequences: number[][] = [[]];

  for(const token of tokens) {
    const { codepointStart, codepointEnd } = ucdCodepointRange(token);
    const next: number[][] = [];

    for(const sequence of sequences) {
      for(let cp = codepointStart; cp <= codepointEnd; ++cp) {
        next.push([...sequence, cp]);
      }
    }

    sequences.splice(0, sequences.length, ...next);
  }

  return sequences;
}

function setType(rows: Map<string, EmojiSequence>, codepoints: number[], type: EmojiSequenceType) {
  const key = sequenceKey(codepoints);
  const row = rows.get(key);

  if(row) {
    row.type = type;
  } else {
    rows.set(key, {
      codepoints,
      type,
      qualification: EmojiQualification.None,
    });
  }
}

function setQualification(rows: Map<string, EmojiSequence>, codepoints: number[],
    qualification: EmojiQualification) {
  const key = sequenceKey(codepoints);
  const row = rows.get(key);

  if(row) {
    row.qualification = qualification;
  } else {
    rows.set(key, {
      codepoints,
      type: EmojiSequenceType.None,
      qualification,
    });
  }
}

async function readEmojiSequenceTypes(rows: Map<string, EmojiSequence>, path: string) {
  for await (const split of parsePropertyFile(path)) {
    if(split.length < 2) {
      continue;
    }

    const type = sequenceTypes[split[1]];

    if(type === undefined) {
      console.log(`Unknown emoji sequence type: ${split[1]}`);
      continue;
    }

    for(const codepoints of parseCodepointSequences(split[0])) {
      setType(rows, codepoints, type);
    }
  }
}

async function readEmojiQualifications(rows: Map<string, EmojiSequence>, path: string) {
  for await (const split of parsePropertyFile(path)) {
    if(split.length < 2) {
      continue;
    }

    const qualification = qualifications[split[1]];

    if(qualification === undefined) {
      console.log(`Unknown emoji qualification: ${split[1]}`);
      continue;
    }

    for(const codepoints of parseCodepointSequences(split[0])) {
      setQualification(rows, codepoints, qualification);
    }
  }
}

export async function generateEmojiSequences() {
  log('GENERATE EMOJI SEQUENCES');

  const rows = new Map<string, EmojiSequence>();

  await readEmojiSequenceTypes(rows, './unicode-data/emoji/emoji-sequences.txt');
  await readEmojiSequenceTypes(rows, './unicode-data/emoji/emoji-zwj-sequences.txt');
  await readEmojiQualifications(rows, './unicode-data/emoji/emoji-test.txt');

  return [...rows.values()].sort(compareSequences);
}
