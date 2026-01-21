/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { Character } from '../character';
import { log } from '../log';
import { CodepointsRangeMap } from '../utils';
import { parsePropertyFile } from './parse-property-file';

// Sorted by most used properties first
export const properties = {
  'Grapheme_Base': { flag: 0x1, count: 0 },
  'ID_Continue': { flag: 0x2, count: 0 },
  'XID_Continue': { flag: 0x4, count: 0 },
  'Alphabetic': { flag: 0x8, count: 0 },
  'ID_Start': { flag: 0x10, count: 0 },
  'XID_Start': { flag: 0x20, count: 0 },
  'Cased': { flag: 0x40, count: 0 },
  'InCB': { flag: 0x80, count: 0 },
  'Changes_When_Casemapped': { flag: 0x100, count: 0 },
  'Case_Ignorable': { flag: 0x200, count: 0 },
  'Lowercase': { flag: 0x400, count: 0 },
  'Math': { flag: 0x800, count: 0 },
  'Grapheme_Extend': { flag: 0x1000, count: 0 },
  'Extend': { flag: 0x2000, count: 0 }, // Indic_Conjunct_Break=Extend
  'Uppercase': { flag: 0x4000, count: 0 },
  'Changes_When_Uppercased': { flag: 0x8000, count: 0 },
  'Changes_When_Casefolded': { flag: 0x10000, count: 0 },
  'Changes_When_Titlecased': { flag: 0x20000, count: 0 },
  'Changes_When_Lowercased': { flag: 0x40000, count: 0 },
  'Consonant': { flag: 0x80000, count: 0 }, // Indic_Conjunct_Break=Consonant
  'Default_Ignorable_Code_Point': { flag: 0x100000, count: 0 },
  'Grapheme_Link': { flag: 0x200000, count: 0 },
  'Linker': { flag: 0x400000, count: 0 }, // Indic_Conjunct_Break=Linker
};

export async function generateDerivedCoreProperties(characters: Character[], ranges: CodepointsRangeMap, path = './UCD/DerivedCoreProperties.txt') {
  log('GENERATE DERIVED CORE PROPERTIES');

  const characterMap: { [key: string]: Character } = {};

  for(const char of characters) {
    characterMap['' + char.codepoint] = char;
  }

  for await (const split of parsePropertyFile(path)) {
    if(split.length < 2) {
      continue;
    }

    const codepoint = split[0];
    const property = split[1];
    const additionalProperty = split.length > 2 ? split[2] : null;
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

    for(let cp = codepointStart; cp <= codepointEnd; ++cp) {
      const index = '' + cp;

      if(characterMap[index]) {

        if(properties[property as keyof typeof properties] === undefined) {
          console.log(`Unknown property: ${property}`);
          continue;
        }

        if(additionalProperty && properties[additionalProperty as keyof typeof properties] === undefined) {
          console.log(`Unknown additional property: ${additionalProperty}`);
          continue;
        }

        properties[property as keyof typeof properties].count++;

        if(characterMap[index].derivedCoreProperties === null) {
          characterMap[index].derivedCoreProperties = properties[property as keyof typeof properties].flag;
        } else {
          characterMap[index].derivedCoreProperties |= properties[property as keyof typeof properties].flag;

          if(additionalProperty) {
            properties[additionalProperty as keyof typeof properties].count++;

            if(characterMap[index].derivedCoreProperties === null) {
              characterMap[index].derivedCoreProperties = properties[additionalProperty as keyof typeof properties].flag;
            } else {
              characterMap[index].derivedCoreProperties |= properties[additionalProperty as keyof typeof properties].flag;
            }
          }
        }

        continue;
      } else if(characterMap[index] === undefined) {
        let found = false;

        for(const rangeName in ranges) {
          const range = ranges[rangeName];

          if(cp >= range.rangeStart && cp <= range.rangeEnd) {
            // Check if the property is in one of the UnicodeData.txt ranges
            found = true;
            break;
          }
        }

        if(!found) {
          // TODO: the character is not found. Default: Default_Ignorable_Code_Point
        }
      }
    }
  }

  /*const sortedProperties = [];

  for(const name in properties) {
    const property = properties[name as keyof typeof properties];
    sortedProperties.push({
      name: name,
      count: property.count,
      flag: property.flag
    });
  }

  sortedProperties.sort((a, b) => b.count - a.count);

  console.log(sortedProperties);*/
}
