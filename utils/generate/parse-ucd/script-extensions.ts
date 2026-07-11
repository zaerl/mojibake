/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { Property } from './properties';
import { parsePropertyFile, ucdCodepointRange } from './utils';

export type ScriptExtension = {
  start: number;
  end: number;
  scripts: number[];
};

export async function readScriptExtensions(properties: Property[]): Promise<ScriptExtension[]> {
  const script = properties.find(property => property.shortName === 'sc');

  if(script === undefined) {
    throw new Error('Script property metadata is missing');
  }

  const rows: ScriptExtension[] = [];

  for await (const fields of parsePropertyFile('./unicode-data/UCD/ScriptExtensions.txt')) {
    const range = ucdCodepointRange(fields[0]);
    const scripts = fields[1].split(/\s+/).map(name => {
      const value = script.values[name];

      if(typeof value !== 'number') {
        throw new Error(`Unknown Script_Extensions value: ${name}`);
      }

      return value;
    });

    rows.push({ start: range.codepointStart, end: range.codepointEnd, scripts });
  }

  return rows;
}
