/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { Character } from './character';
import { Emoji } from './emoji';
import { CaseFoldEntry, SimpleCaseFoldEntry } from './parse-ucd/casefold';
import {
  CollationEntry, encodeCodepointSequence, encodeCollationWeights, ImplicitWeightRange,
} from './parse-ucd/collation';
import { ConfusableEntry } from './parse-ucd/confusables';
import { EmojiSequence } from './parse-ucd/emoji-sequences';
import { PropertyRange } from './parse-ucd/properties';
import { NewCases } from './parse-ucd/special-casing';
import { Prefix } from './prefix-compressor';
import {
  BlockRow, CaseFoldRow, CaseFoldSimpleRow, CollationContractionRow, CollationEntryRow,
  CollationImplicitRangeRow, CompositionRow, ConfusableRow, DecompositionRow, EmojiRow,
  EmojiSequenceRow, NameRow, NCharacterRow, NumericRow, PrefixRow, PropertyRangeRow, SimpleCaseRow,
  SpecialCaseRow,
  ScriptExtensionRow,
} from './file-generators/types';
import { Block, CalculatedDecomposition, CaseType, Composition } from './types';

export type UnicodeTableData = {
  blocks: BlockRow[];
  prefixes: PrefixRow[];
  names: NameRow[];
  emoji: EmojiRow[];
  properties: PropertyRangeRow[];
  scriptExtensions: ScriptExtensionRow[];
  nCharacters: NCharacterRow[];
  decompositions: DecompositionRow[];
  compatibilityDecompositions: DecompositionRow[];
  compositions: CompositionRow[];
  numericValues: NumericRow[];
  simpleCaseMappings: SimpleCaseRow[];
  specialCaseMappings: SpecialCaseRow[];
  caseFoldMappings: CaseFoldRow[];
  caseFoldSimpleMappings: CaseFoldSimpleRow[];
  confusables: ConfusableRow[];
  collationEntries: CollationEntryRow[];
  collationImplicitRanges: CollationImplicitRangeRow[];
  collationContractions: CollationContractionRow[];
  emojiSequences: EmojiSequenceRow[];
};

function emptyUnicodeTableData(): UnicodeTableData {
  return {
    blocks: [],
    prefixes: [],
    names: [],
    emoji: [],
    properties: [],
    scriptExtensions: [],
    nCharacters: [],
    decompositions: [],
    compatibilityDecompositions: [],
    compositions: [],
    numericValues: [],
    simpleCaseMappings: [],
    specialCaseMappings: [],
    caseFoldMappings: [],
    caseFoldSimpleMappings: [],
    confusables: [],
    collationEntries: [],
    collationImplicitRanges: [],
    collationContractions: [],
    emojiSequences: [],
  };
}

let unicodeTableData = emptyUnicodeTableData();

const byCodepoint = <T extends { codepoint: number }>(a: T, b: T) => a.codepoint - b.codepoint;

function sortDecompositions(rows: DecompositionRow[]) {
  return [...rows].sort((a, b) => a.id - b.id);
}

function sortPropertyRanges(rows: PropertyRangeRow[]) {
  return [...rows].sort((a, b) =>
    a.start_codepoint - b.start_codepoint ||
    (a.end_codepoint ?? a.start_codepoint) - (b.end_codepoint ?? b.start_codepoint)
  );
}

// Reset all generated Unicode rows before starting a new generation pass.
export function resetUnicodeTableData() {
  unicodeTableData = emptyUnicodeTableData();
}

// Return sorted copies.
export function getUnicodeTableData(): UnicodeTableData {
  return {
    blocks: [...unicodeTableData.blocks].sort((a, b) => a.start - b.start),
    prefixes: [...unicodeTableData.prefixes].sort((a, b) => a.id - b.id),
    names: [...unicodeTableData.names].sort(byCodepoint),
    emoji: [...unicodeTableData.emoji].sort(byCodepoint),
    properties: sortPropertyRanges(unicodeTableData.properties),
    scriptExtensions: [...unicodeTableData.scriptExtensions].sort((a, b) => a.start - b.start),
    nCharacters: [...unicodeTableData.nCharacters].sort(byCodepoint),
    decompositions: sortDecompositions(unicodeTableData.decompositions),
    compatibilityDecompositions:
      sortDecompositions(unicodeTableData.compatibilityDecompositions),
    compositions: [...unicodeTableData.compositions].sort((a, b) =>
      a.starter_codepoint - b.starter_codepoint ||
      a.combining_codepoint - b.combining_codepoint
    ),
    numericValues: [...unicodeTableData.numericValues].sort(byCodepoint),
    simpleCaseMappings: [...unicodeTableData.simpleCaseMappings].sort(byCodepoint),
    specialCaseMappings: [...unicodeTableData.specialCaseMappings].sort((a, b) =>
      a.codepoint - b.codepoint || a.case_type - b.case_type
    ),
    caseFoldMappings: [...unicodeTableData.caseFoldMappings].sort(byCodepoint),
    caseFoldSimpleMappings: [...unicodeTableData.caseFoldSimpleMappings].sort(byCodepoint),
    confusables: [...unicodeTableData.confusables].sort(byCodepoint),
    collationEntries: [...unicodeTableData.collationEntries].sort(byCodepoint),
    collationImplicitRanges: [...unicodeTableData.collationImplicitRanges].sort((a, b) =>
      a.start - b.start
    ),
    collationContractions: [...unicodeTableData.collationContractions].sort((a, b) =>
      a.first_codepoint - b.first_codepoint
    ),
    emojiSequences: [...unicodeTableData.emojiSequences].sort((a, b) => {
      const min = Math.min(a.codepoints.length, b.codepoints.length);

      for(let i = 0; i < min; ++i) {
        const diff = a.codepoints[i] - b.codepoints[i];

        if(diff !== 0) {
          return diff;
        }
      }

      return a.codepoints.length - b.codepoints.length;
    }),
  };
}

// Return a compact row-count summary for verbose generator logs.
export function unicodeTableDataSummary() {
  return {
    blocks: unicodeTableData.blocks.length,
    characters: unicodeTableData.nCharacters.length,
    decompositions: unicodeTableData.decompositions.length,
    compatibilityDecompositions: unicodeTableData.compatibilityDecompositions.length,
    compositions: unicodeTableData.compositions.length,
    propertyRanges: unicodeTableData.properties.length,
    collationEntries: unicodeTableData.collationEntries.length,
    collationImplicitRanges: unicodeTableData.collationImplicitRanges.length,
    collationContractions: unicodeTableData.collationContractions.length,
    confusables: unicodeTableData.confusables.length,
    emojiSequences: unicodeTableData.emojiSequences.length,
  };
}

// Add one Unicode block row.
export function addBlock(index: number, block: Block) {
  unicodeTableData.blocks.push({
    id: index,
    start: block.start,
    end: block.end,
    name: block.name,
  });
}

// Add core character rows and derived name, numeric, and simple-case indexes.
export function addCharacters(characters: Character[], prefixes: Prefix[]) {
  for(let i = 1; i < prefixes.length; ++i) {
    unicodeTableData.prefixes.push({
      id: prefixes[i].id,
      name: prefixes[i].prefix,
    });
  }

  for(const char of characters) {
    unicodeTableData.nCharacters.push({
      codepoint: char.codepoint,
      category: char.category,
      combining: char.combining,
      bidirectional: char.bidirectional ?? 0,
      decomposition: char.decomposition,
      quick_check: char.quickCheck,
      mirrored: char.mirrored ? 1 : 0,
    });

    if(char.name !== null) {
      unicodeTableData.names.push({
        codepoint: char.codepoint,
        name: char.name,
        prefix: char.prefix,
      });
    }

    if(char.decimal !== null || char.digit !== null || char.numeric !== null) {
      unicodeTableData.numericValues.push({
        codepoint: char.codepoint,
        decimal: char.decimal,
        digit: char.digit,
        numeric: char.numeric,
      });
    }

    if(char.uppercase !== null || char.lowercase !== null || char.titlecase !== null) {
      unicodeTableData.simpleCaseMappings.push({
        codepoint: char.codepoint,
        uppercase: char.uppercase,
        lowercase: char.lowercase,
        titlecase: char.titlecase,
      });
    }
  }
}

// Add canonical or compatibility decomposition rows.
export function addDecompositions(decompositions: CalculatedDecomposition[], compat = false) {
  const rows = compat ?
    unicodeTableData.compatibilityDecompositions :
    unicodeTableData.decompositions;

  for(const value of decompositions) {
    rows.push({
      id: value.codepoint,
      value: value.value,
    });
  }
}

// Add canonical composition rows.
export function addCompositions(compositions: Composition[]) {
  for(const comp of compositions) {
    unicodeTableData.compositions.push({
      starter_codepoint: comp.starter_codepoint,
      combining_codepoint: comp.combining_codepoint,
      composite_codepoint: comp.composite_codepoint,
    });
  }
}

// Add multi-codepoint special casing rows.
export function addSpecialCasing(newCases: NewCases) {
  const firstMapping = (values: (number | null)[]) => {
    if(values[0] === null) {
      throw new Error('Special casing row is missing its first mapping');
    }

    return values[0];
  };

  for(let i = 0; i < newCases.length; ++i) {
    if(newCases[i].hasLowercase > 1) {
      unicodeTableData.specialCaseMappings.push({
        codepoint: newCases[i].codepoint,
        case_type: CaseType.LowerCase,
        new_case_1: firstMapping(newCases[i].lowercase),
        new_case_2: newCases[i].lowercase[1] ?? null,
        new_case_3: newCases[i].lowercase[2] ?? null,
      });
    }

    if(newCases[i].hasTitlecase > 1) {
      unicodeTableData.specialCaseMappings.push({
        codepoint: newCases[i].codepoint,
        case_type: CaseType.TitleCase,
        new_case_1: firstMapping(newCases[i].titlecase),
        new_case_2: newCases[i].titlecase[1] ?? null,
        new_case_3: newCases[i].titlecase[2] ?? null,
      });
    }

    if(newCases[i].hasUppercase > 1) {
      unicodeTableData.specialCaseMappings.push({
        codepoint: newCases[i].codepoint,
        case_type: CaseType.UpperCase,
        new_case_1: firstMapping(newCases[i].uppercase),
        new_case_2: newCases[i].uppercase[1] ?? null,
        new_case_3: newCases[i].uppercase[2] ?? null,
      });
    }
  }
}

// Add full case-folding rows.
export function addCaseFolding(entries: CaseFoldEntry[]) {
  for(const entry of entries) {
    unicodeTableData.caseFoldMappings.push({
      codepoint: entry.codepoint,
      new_case_1: entry.mapping[0],
      new_case_2: entry.mapping[1] ?? null,
      new_case_3: entry.mapping[2] ?? null,
    });
  }
}

// Add simple (S) case-folding rows.
export function addSimpleCaseFolding(entries: SimpleCaseFoldEntry[]) {
  for(const entry of entries) {
    unicodeTableData.caseFoldSimpleMappings.push({
      codepoint: entry.codepoint,
      mapping: entry.mapping,
    });
  }
}

// Add emoji property rows.
export function addEmojiProperties(emojiProperties: Emoji[]) {
  for(const emoji of emojiProperties) {
    unicodeTableData.emoji.push({
      codepoint: emoji.codepoint,
      emoji: emoji.emoji ? 1 : 0,
      emoji_presentation: emoji.emoji_presentation ? 1 : 0,
      emoji_modifier: emoji.emoji_modifier ? 1 : 0,
      emoji_modifier_base: emoji.emoji_modifier_base ? 1 : 0,
      emoji_component: emoji.emoji_component ? 1 : 0,
      extended_pictographic: emoji.extended_pictographic ? 1 : 0,
    });
  }
}

// Add emoji string sequence rows.
export function addEmojiSequences(sequences: EmojiSequence[]) {
  for(const sequence of sequences) {
    unicodeTableData.emojiSequences.push({
      codepoints: sequence.codepoints,
      type: sequence.type,
      qualification: sequence.qualification,
    });
  }
}

// Add pre-encoded property range rows.
export function addPropertyRanges(propertyRanges: PropertyRange[]) {
  for(const pr of propertyRanges) {
    unicodeTableData.properties.push({
      start_codepoint: pr.start,
      end_codepoint: pr.start === pr.end ? null : pr.end,
      properties: Buffer.from(pr.properties),
    });
  }
}

export function addScriptExtensions(rows: ScriptExtensionRow[]) {
  unicodeTableData.scriptExtensions.push(...rows);
}

// Add DUCET collation entries and contractions.
export function addCollation(entries: CollationEntry[], implicitRanges: ImplicitWeightRange[]) {
  for(const entry of entries) {
    const weights = encodeCollationWeights(entry.elements);

    if(entry.codepoints.length === 1) {
      unicodeTableData.collationEntries.push({
        codepoint: entry.codepoints[0],
        weights,
      });
    } else {
      unicodeTableData.collationContractions.push({
        first_codepoint: entry.codepoints[0],
        sequence: encodeCodepointSequence(entry.codepoints),
        weights,
      });
    }
  }

  const offsets = new Map<number, number>();

  for(const range of implicitRanges) {
    offsets.set(range.base, Math.min(offsets.get(range.base) ?? range.start, range.start));
  }

  for(const range of implicitRanges) {
    unicodeTableData.collationImplicitRanges.push({
      ...range,
      offset: offsets.get(range.base)!,
    });
  }
}

// Add UTS#39 confusable skeleton rows.
export function addConfusables(entries: ConfusableEntry[]) {
  for(const entry of entries) {
    unicodeTableData.confusables.push({
      codepoint: entry.codepoint,
      skeleton: encodeCodepointSequence(entry.skeleton),
    });
  }
}
