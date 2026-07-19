/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { iLog } from '../../log';
import {
  codepointPageBitsets, codepointPages, formatBytes, formatCodepoints, formatCompactIntegers,
  formatHalfwords, formatLongWords, formatWords, indexedPages,
} from '../../utils';
import { CompositionRow, DecompositionRow } from '../types';

// Groups ordered decomposition rows into one sequence per source codepoint.
function collectDecompositionGroups(rows: DecompositionRow[]) {
  const groups: { codepoint: number; values: number[] }[] = [];
  let currentId: number | null = null;
  let currentValues: number[] = [];

  // Appends the current decomposition group when one is open.
  const flush = () => {
    if(currentId === null) {
      return;
    }

    groups.push({ codepoint: currentId, values: currentValues });
  };

  for(const row of rows) {
    if(currentId !== row.id) {
      flush();
      currentId = row.id;
      currentValues = [];
    }

    currentValues.push(row.value);
  }

  flush();

  return groups;
}

// Emits shared decomposition data, packed decomposition entries, and composition pairs.
export function generateDecompositionAndCompositionTables(
  canonicalRows: DecompositionRow[],
  compatibilityRows: DecompositionRow[],
  compositionRows: CompositionRow[],
) {
  iLog('Decomposition and composition');

  const canonicalGroups = collectDecompositionGroups(canonicalRows);
  const compatibilityGroups = collectDecompositionGroups(compatibilityRows);
  const data: number[] = [];
  const dataOffsets = new Map<string, number>();

  // Interns a decomposition sequence and returns its shared data offset.
  const addSequence = (values: number[]) => {
    const key = values.join(',');
    let offset = dataOffsets.get(key);

    if(offset === undefined) {
      offset = data.length;
      data.push(...values);
      dataOffsets.set(key, offset);
    }

    if(offset > 0xFFFF) {
      throw new Error(`Decomposition data offset is too large to pack: ${offset}`);
    }

    return offset;
  };

  // Emits one packed decomposition table for canonical or compatibility mappings.
  const emitTable = (name: string, groups: typeof canonicalGroups) => {
    const pages = indexedPages(codepointPages(groups));
    const pageBitsets = codepointPageBitsets(groups, pages.pages);
    const mappings: number[] = [];
    const exceptionIndices: number[] = [];
    const exceptionLengths: number[] = [];

    groups.forEach((group, index) => {
      if(group.codepoint > 0x1FFFFF) {
        throw new Error(`${name} decomposition codepoint is too large to pack: ${group.codepoint}`);
      }

      if(group.values.length > 0x1F) {
        throw new Error(
          `${name} decomposition length is too large to pack: ${group.values.length}`
        );
      }

      const offset = addSequence(group.values);

      if(offset > 0x1FFF) {
        throw new Error(`${name} decomposition data offset is too large to pack: ${offset}`);
      }

      // Lengths 1..8 fit in the upper three bits. Longer mappings are extremely rare and use a
      // tiny exception table, keeping every hot-path mapping at two bytes.
      const encodedLength = Math.min(group.values.length, 8) - 1;
      mappings.push(offset | (encodedLength << 13));

      if(group.values.length > 8) {
        if(index > 0xFFFF || group.values.length > 0xFF) {
          throw new Error(`${name} decomposition exception is too large to pack: ${index}`);
        }

        exceptionIndices.push(index);
        exceptionLengths.push(group.values.length);
      }
    });

    const emittedExceptionIndices = exceptionIndices.length === 0 ? [0] : exceptionIndices;
    const emittedExceptionLengths = exceptionLengths.length === 0 ? [0] : exceptionLengths;
    const exceptionCountName = `MJB_UNICODE_${name.toUpperCase()}_DECOMPOSITION_EXCEPTION_COUNT`;

    return `enum { ${exceptionCountName} = ${exceptionIndices.length} };

static const uint8_t mjb_unicode_${name}_decomposition_page_index[] = {
${formatBytes(pages.index)}
};

static const uint16_t mjb_unicode_${name}_decomposition_page_starts[] = {
${formatHalfwords(pages.pages.starts)}
};

static const uint64_t mjb_unicode_${name}_decomposition_page_bits[] = {
${formatLongWords(pageBitsets.data, 16)}
};

static const uint32_t mjb_unicode_${name}_decomposition_page_ranks[] = {
${formatWords(pageBitsets.ranks)}
};

static const uint16_t mjb_unicode_${name}_decompositions[] = {
${formatCompactIntegers(mappings, 16)}
};

static const uint16_t mjb_unicode_${name}_decomposition_exception_indices[] = {
${formatHalfwords(emittedExceptionIndices)}
};

static const uint8_t mjb_unicode_${name}_decomposition_exception_lengths[] = {
${formatBytes(emittedExceptionLengths)}
};
`;
  };

  const compositionEntries = compositionRows.map((row) => {
    if(row.starter_codepoint > 0x1FFFFF ||
      row.combining_codepoint > 0x1FFFFF ||
      row.composite_codepoint > 0x1FFFFF) {
      throw new Error(`Composition codepoint is too large to pack: ${row.composite_codepoint}`);
    }

    return BigInt(row.starter_codepoint) |
      (BigInt(row.combining_codepoint) << 21n) |
      (BigInt(row.composite_codepoint) << 42n);
  });

  const canonicalTable = emitTable('canonical', canonicalGroups);
  const compatibilityTable = emitTable('compatibility', compatibilityGroups);

  return `typedef uint64_t mjb_unicode_composition_entry;

static const mjb_codepoint mjb_unicode_decomposition_data[] = {
${formatCodepoints(data)}
};

${canonicalTable}
${compatibilityTable}
static const mjb_unicode_composition_entry mjb_unicode_compositions[] = {
${formatLongWords(compositionEntries)}
};
`;
}
