/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { Character } from '../character';
import { Emoji } from '../emoji';
import { log } from '../log';
import { EmojiProperties, EmojiPropertiesStrings } from '../types';
import { parsePropertyFile, ucdCodepointRange } from './utils';

export async function generateEmojiProperties(characters: Character[]) {
  log('GENERATE EMOJI PROPERTIES');

  const path = './UCD/emoji/emoji-data.txt';
  const emojiMap: { [key: string]: Emoji } = {};
  const characterMap: { [key: string]: Character } = {};

  for(const char of characters) {
    characterMap['' + char.codepoint] = char;
  }

  for await (const split of parsePropertyFile(path, ['F0000', '100000'])) {
    if(split.length < 2) {
      continue;
    }

    const codepoint = split[0];
    const emoji = split[1];

    let { codepointStart, codepointEnd } = ucdCodepointRange(codepoint);

    if(EmojiProperties[emoji as EmojiPropertiesStrings] === undefined) {
      console.log(`Unknown emoji property: ${emoji}`);
      continue;
    } else {
      for(let cp = codepointStart; cp <= codepointEnd; ++cp) {
        const index = '' + cp;

        if(!emojiMap[index]) {
          emojiMap[index] = new Emoji(cp, false, false, false, false, false, false);
        }

        const field = emoji.toLowerCase();
        (emojiMap[index] as any)[field as keyof Emoji] = true;

        if(characterMap[index]) {
          characterMap[index].extendedPictographic = EmojiProperties[emoji as EmojiPropertiesStrings] ===
            EmojiProperties.Extended_Pictographic;
        }
      }
    }
  }

  return Object.values(emojiMap).sort((a, b) => a.codepoint - b.codepoint);
}
