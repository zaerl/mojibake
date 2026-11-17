/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { writeFileSync } from 'fs';
import { iLog } from '../log';
import { Property } from '../parse-ucd/properties';
import { Block, Categories, characterDecompositionMapping } from '../types';

// Generate src/elixir/lib/mojibake/unicode_data.ex: the tables that turn the integers of the C
// enums (mjb_category, mjb_decomposition, mjb_script, mjb_block, mjb_property and the values of
// the stored properties) into Elixir atoms. They come from the same data as src/unicode.h, so the
// two cannot drift apart.

const OUTPUT = '../../src/elixir/lib/mojibake/unicode_data.ex';

// The mjb_script values after the Script property values, in the order of header.ts. They are
// ISO 15924 codes for script combinations, with no Script property value of their own.
const EXTRA_SCRIPTS: [string, string][] = [
  ['Hanb', 'Han_With_Bopomofo'],
  ['Jpan', 'Japanese'],
  ['Kore', 'Korean'],
];

// "Latin-1 Supplement" -> "latin_1_supplement", "ExtendNumLet" -> "extend_num_let",
// "ALetter" -> "a_letter". Unicode matches property values loosely, ignoring case, spaces,
// hyphens and underscores (UAX #44 LM3), so the atoms stay faithful to the UCD names.
function snakeCase(name: string): string {
  return name
    .replace(/([a-z0-9])([A-Z])/g, '$1_$2')
    .replace(/([A-Z]+)([A-Z][a-z])/g, '$1_$2')
    .replace(/[^A-Za-z0-9]+/g, '_')
    .replace(/^_+|_+$/g, '')
    .toLowerCase();
}

function atom(name: string): string {
  return /^[a-z_][a-z0-9_]*$/.test(name) ? `:${name}` : `:"${name}"`;
}

function string(value: string): string {
  return `"${value.replace(/\\/g, '\\\\').replace(/"/g, '\\"')}"`;
}

function hex(value: number): string {
  return '0x' + value.toString(16).toUpperCase().padStart(4, '0');
}

// Throw if two entries of a table map to the same atom.
function checkUnique(table: string, names: string[]) {
  const seen = new Set<string>();

  for(const name of names) {
    if(seen.has(name)) {
      throw new Error(`Elixir table ${table}: duplicate atom ${name}`);
    }

    seen.add(name);
  }
}

// The short and long name of each value of a property, indexed by the value ID that the library
// stores. PropertyValueAliases.txt lists the short name first; a value whose short and long names
// are the same has a single key.
function valueNames(property: Property): { short: string, long: string }[] {
  const names: { short: string, long: string }[] = [];

  for(const [key, id] of Object.entries(property.values)) {
    if(names[id] === undefined) {
      names[id] = { short: key, long: key };
    } else {
      names[id].long = key;
    }
  }

  return names;
}

function findProperty(properties: Property[], shortName: string): Property {
  const property = properties.find(value => value.shortName === shortName);

  if(property === undefined) {
    throw new Error(`Elixir tables: no ${shortName} property`);
  }

  return property;
}

function tuple(name: string, comment: string[], entries: string[]): string {
  const lines = comment.map(line => `  # ${line}`);

  lines.push(`  @${name} {`);
  entries.forEach((entry, index) => lines.push(`    ${entry}${index === entries.length - 1 ? '' : ','}`));
  lines.push('  }');

  return lines.join('\n');
}

function categoriesTable(properties: Property[]): string {
  const names = valueNames(findProperty(properties, 'gc'));
  const general = findProperty(properties, 'gc');
  const entries: string[] = [];
  const atoms: string[] = [];

  for(let id = 0; Categories[id] !== undefined; ++id) {
    const abbreviation = Categories[id];
    const value = general.values[abbreviation];

    if(value === undefined) {
      throw new Error(`Elixir tables: unknown General_Category ${abbreviation}`);
    }

    const name = snakeCase(names[value].long);
    atoms.push(name);
    entries.push(`{${atom(name)}, ${string(abbreviation)}}`);
  }

  checkUnique('categories', atoms);

  return tuple('categories', [
    'mjb_category -> {atom, abbreviation}',
  ], entries);
}

function decompositionTable(): string {
  const entries = Object.keys(characterDecompositionMapping).map((key, index) => {
    if(index === 0) {
      return 'nil';
    }

    return atom(snakeCase(key.replace(/[<>]/g, '')));
  });

  checkUnique('decomposition_types', entries);

  return tuple('decomposition_types', [
    'mjb_decomposition -> atom, nil for MJB_DECOMPOSITION_NONE',
  ], entries);
}

function scriptsTable(properties: Property[]): string {
  const names = valueNames(findProperty(properties, 'sc'));
  const entries = ['nil'];
  const atoms: string[] = [];

  for(let id = 1; id < names.length; ++id) {
    const name = snakeCase(names[id].long);
    atoms.push(name);
    entries.push(`{${atom(name)}, ${string(names[id].short)}}`);
  }

  for(const [code, longName] of EXTRA_SCRIPTS) {
    const name = snakeCase(longName);
    atoms.push(name);
    entries.push(`{${atom(name)}, ${string(code)}}`);
  }

  checkUnique('scripts', atoms);

  return tuple('scripts', [
    'mjb_script -> {atom, ISO 15924 code}, nil for MJB_SC_NOT_SET',
  ], entries);
}

function blocksTable(blocks: Block[]): string {
  const atoms = blocks.map(block => snakeCase(block.name));
  checkUnique('blocks', atoms);

  return tuple('blocks', [
    'mjb_block -> {atom, name, first codepoint, last codepoint}',
  ], blocks.map((block, index) =>
    `{${atom(atoms[index])}, ${string(block.name)}, ${hex(block.start)}, ${hex(block.end)}}`));
}

function propertiesTable(properties: Property[]): string {
  const lines = [
    '  # Properties with stored per-codepoint values: atom -> {mjb_property, values}. values is',
    '  # :binary, or a tuple of value atoms indexed by the stored value, nil for "no value".',
    '  @properties %{',
  ];
  const stored = properties.filter(property => property.stored);
  const atoms = stored.map(property => snakeCase(property.name));

  checkUnique('properties', atoms);

  stored.forEach((property, index) => {
    const comma = index === stored.length - 1 ? '' : ',';
    const key = atoms[index];
    const keyword = /^[a-z_][a-z0-9_]*$/.test(key) ? `${key}:` : `"${key}":`;

    if(property.bool) {
      lines.push(`    ${keyword} {${property.id}, :binary}${comma}`);
      return;
    }

    const names = valueNames(property);
    const values = ['nil'];

    for(let id = 1; id < names.length; ++id) {
      values.push(atom(snakeCase(names[id].long)));
    }

    checkUnique(`${key} values`, values);
    lines.push(`    ${keyword}`);
    lines.push(`      {${property.id},`);
    lines.push('       {');
    values.forEach((value, valueIndex) =>
      lines.push(`         ${value}${valueIndex === values.length - 1 ? '' : ','}`));
    lines.push(`       }}${comma}`);
  });

  lines.push('  }');

  return lines.join('\n');
}

export function generateElixirUnicodeData(blocks: Block[], properties: Property[]) {
  iLog('Generate Elixir Unicode tables');

  const content = `# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

# This file is automatically generated by utils/generate/file-generators/elixir-unicode-data.ts
# from the Unicode Character Database, like the enums of src/unicode.h. Do not edit.

defmodule Mojibake.UnicodeData do
  @moduledoc false

${categoriesTable(properties)}

${decompositionTable()}

${scriptsTable(properties)}

${blocksTable(blocks)}

${propertiesTable(properties)}

  def categories, do: @categories
  def decomposition_types, do: @decomposition_types
  def scripts, do: @scripts
  def blocks, do: @blocks
  def properties, do: @properties
end
`;

  writeFileSync(OUTPUT, content);
}
