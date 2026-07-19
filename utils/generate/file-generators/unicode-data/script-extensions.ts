/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { iLog } from '../../log';
import { formatBytes, formatLongWords } from '../../utils';
import { ScriptExtensionRow } from '../types';

export function generateScriptExtensions(rows: ScriptExtensionRow[]) {
  iLog('Script extensions');

  const data: number[] = [];
  const offsets = new Map<string, number>();
  const entries: bigint[] = [];

  for(const row of rows) {
    const key = row.scripts.join(',');
    let offset = offsets.get(key);

    if(offset === undefined) {
      offset = data.length;
      offsets.set(key, offset);
      data.push(...row.scripts);
    }

    const delta = row.end - row.start;

    if(row.start > 0x1FFFFF || delta > 0x1FFFFF || offset > 0xFFFF ||
      row.scripts.length === 0 || row.scripts.length > 64) {
      throw new Error(`Script_Extensions entry out of bounds at U+${row.start.toString(16)}`);
    }

    entries.push(BigInt(row.start) | (BigInt(delta) << 21n) | (BigInt(offset) << 42n) |
      (BigInt(row.scripts.length - 1) << 58n));
  }

  return `typedef uint64_t mjb_unicode_script_extension_entry;

static const uint8_t mjb_unicode_script_extension_data[] = {
${formatBytes(data, 24)}
};

static const mjb_unicode_script_extension_entry mjb_unicode_script_extensions[] = {
${formatLongWords(entries)}
};
`;
}
