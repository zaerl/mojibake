import { open } from 'fs/promises';
import { Character } from './character';
import { log } from './log';
import { writeFileSync } from 'fs';

enum LineBreakingClass {
  // Non-tailorable Line Breaking Classes
  BK, // Mandatory Break
  CR, // Carriage Return
  LF, // Line Feed
  CM, // Combining Mark
  NL, // Next Line
  SG, // Surrogate
  WJ, // Word Joiner
  ZW, // Zero Width Space
  GL, // Non-breaking
  SP, // Space
  ZWJ, // Zero Width Joiner

  // Break Opportunities
  B2, // Break Opportunity Before and After
  BA, // Break After
  BB, // Break Before
  HY, // Hyphen
  CB, // Contingent Break Opportunity

  // Characters Prohibiting Certain Breaks
  CL, // Close Punctuation
  CP, // Close Parenthesis
  EX, // Exclamation / Interrogation
  IN, // Inseparable
  NS, // Nonstarter
  OP, // Open Punctuation
  QU, // Quotation

  // Numeric Context
  IS, // Infix Numeric Separator
  NU, // Numeric
  PO, // Postfix Numeric
  PR, // Prefix Numeric
  SY, // Symbols Allowing Break After

  // Other Characters
  AI, // Ambiguous (Alphabetic or Ideographic)
  AK, // Aksara
  AL, // Alphabetic
  AP, // Aksara Pre-Base
  AS, // Aksara Start
  CJ, // Conditional Japanese Starter
  EB, // Emoji Base
  EM, // Emoji Modifier
  H2, // Hangul LV Syllable
  H3, // Hangul LVT Syllable
  HL, // Hebrew Letter
  ID, // Ideographic
  JL, // Hangul L Jamo
  JV, // Hangul V Jamo
  JT, // Hangul T Jamo
  RI, // Regional Indicator
  SA, // Complex Context Dependent (South East Asian)
  VF, // Virama Final
  VI, // Virama
  XX, // Unknown
};

export async function generateBreaks(characters: Character[], path = './UCD/LineBreak.txt') {
  log('GENERATE BREAKS');
  const file = await open(path);
  const characterMap: { [key: string]: Character } = {};

  for(const char of characters) {
    characterMap['' + char.codepoint] = char;
  }

  for await (const line of file.readLines()) {
    if(line.length === 0 || line.startsWith('#') || line.startsWith('F0000') ||
      line.startsWith('100000')) {
      continue;
    }

    const split = line.split(';');

    if(split.length < 2) continue;

    const codepoint = split[0].trim();
    const breakClass = split[1].trim().split('#')[0].trim();
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

    if(LineBreakingClass[breakClass as keyof typeof LineBreakingClass] === undefined) {
      console.log(`Unknown line breaking class: ${breakClass}`);
      continue;
    } else {
      for(let cp = codepointStart; cp <= codepointEnd; ++cp) {
        const index = '' + cp;

        if(characterMap[index]) {
          characterMap[index].lineBreakingClass = LineBreakingClass[breakClass as keyof typeof LineBreakingClass];
        }
      }
    }
  }
}

export async function generateLineBreaksTest(path = './UCD/auxiliary/LineBreakTest.txt') {
  log('GENERATE LINE BREAKS TEST');
  const file = await open(path);
  let max = 0;
  let output: string[] = [];

  for await (const line of file.readLines()) {
    if(line.length === 0 || line.startsWith('#')) {
      continue;
    }

    const split = line.split('#');
    if(split.length < 2) continue;

    const rule = split[0].trim();
    const withSlash = rule.replace(/÷/g, '+');
    const final = withSlash.replace(/×/g, 'x');

    max = Math.max(max, final.length);
    output.push(final);
  }

  writeFileSync('./UCD/auxiliary/LineBreakTestModified.txt', output.join('\n'));
}
