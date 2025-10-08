import { open } from 'fs/promises';
import { Emoji } from './emoji';
import { log } from './log';
import { EmojiProperties, EmojiPropertiesStrings } from './types';
import { Character } from './character';

export async function generateEmojiProperties(characters: Character[], path = './UCD/emoji/emoji-data.txt') {
  log('GENERATE EMOJI PROPERTIES');
  const file = await open(path);
  const emojiMap: { [key: string]: Emoji } = {};
  const characterMap: { [key: string]: Character } = {};

  for(const char of characters) {
    characterMap['' + char.codepoint] = char;
  }

  for await (const line of file.readLines()) {
    if(line.length === 0 || line.startsWith('#')) {
      continue;
    }

    const split = line.split(';');

    if(split.length < 2) continue;

    const codepoint = split[0].trim();
    const emoji = split[1].trim().split('#')[0].trim();
    let codepointStart = 0;
    let codepointEnd = 0;

    if(codepoint.includes('..')) {
      const codepoints = codepoint.split('..');

      if(codepoints.length === 2) {
        codepointStart = parseInt(codepoints[0], 16);
        codepointEnd = parseInt(codepoints[1], 16);
      }
    } else {
      codepointStart = parseInt(codepoint, 16);
      codepointEnd = codepointStart;
    }

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
