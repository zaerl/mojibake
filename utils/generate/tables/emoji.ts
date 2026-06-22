/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { formatLongWords } from '../utils';
import { EmojiRow } from './types';

const enum EmojiFlag {
  Emoji = 1 << 0,
  Presentation = 1 << 1,
  Modifier = 1 << 2,
  ModifierBase = 1 << 3,
  Component = 1 << 4,
  ExtendedPictographic = 1 << 5,
}

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

// Emits compact emoji property ranges with packed flags.
export function generateEmoji(rows: EmojiRow[]) {
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
`;
}
