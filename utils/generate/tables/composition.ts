/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { iLog } from '../log';
import { formatCodepoints, formatLongWords } from '../utils';
import { CompositionRow, DecompositionRow } from './types';

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
    const entries: bigint[] = [];

    for(const group of groups) {
      if(group.codepoint > 0x1FFFFF) {
        throw new Error(`${name} decomposition codepoint is too large to pack: ${group.codepoint}`);
      }

      if(group.values.length > 0x1F) {
        throw new Error(`${name} decomposition length is too large to pack: ${group.values.length}`);
      }

      const offset = addSequence(group.values);

      entries.push(BigInt(group.codepoint) |
        (BigInt(group.values.length) << 21n) |
        (BigInt(offset) << 26n));
    }

    return `static const mjb_unicode_decomposition_entry mjb_unicode_${name}_decompositions[] = {
${formatLongWords(entries)}
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

  return `typedef uint64_t mjb_unicode_decomposition_entry;
typedef uint64_t mjb_unicode_composition_entry;

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
