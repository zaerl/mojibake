/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { readFileSync, writeFileSync } from 'fs';
import { cfns } from './function';
import { properties } from './parse-ucd/derived-core-properties';
import { Block, Categories, characterDecompositionMapping } from './types';
import { substituteBlock } from './utils';

function getBlockEnumNames(blocks: Block[]) {
  return blocks.map((value: Block, index: number) => `    ${value.enumName}`).join(',\n');
}

function getCategoryEnumNames(categories: string[]) {
  const categoryEnums: string[] = [];

  for(let i = 0; i < categories.length; ++i) {
    categoryEnums.push(`    MJB_CATEGORY_${Categories[i].toUpperCase()}${ i === categories.length - 1 ? ' ' : ','} // ${i} (${Categories[i]}) ${categories[i]}`);
  }

  return categoryEnums.join('\n');
}

function getDecompositionEnumNames() {
  return Object.keys(characterDecompositionMapping).map((value: string, index: number) => `    MJB_DECOMPOSITION_${value.toUpperCase().replace(/[<>]/g, '')}`).join(',\n')
}

function getDerivedCorePropertiesEnumNames() {
  return Object.keys(properties).map((value: string, index: number) => {
    const hexValue = properties[value as keyof typeof properties].flag.toString(16);
    const enumName = `MJB_DCP_${value.toUpperCase()}`;

    return `    ${enumName} = 0x${hexValue}`;
  }).join(',\n');
}

function getFunctions() {
  return cfns().map(value => value.formatC()).join("\n\n") + "\n";
}

export function generateHeader(blocks: Block[], categories: string[]) {
  let fileContent = readFileSync('../../src/unicode.h', 'utf-8');

  fileContent = substituteBlock(fileContent, "typedef enum mjb_block {\n", "\n} mjb_block;", getBlockEnumNames(blocks));
  fileContent = substituteBlock(fileContent, '#define MJB_BLOCK_NUM ', "\n", '' + blocks.length);
  fileContent = substituteBlock(fileContent, "typedef enum mjb_category {\n", "\n} mjb_category;", getCategoryEnumNames(categories));
  fileContent = substituteBlock(fileContent, '#define MJB_CATEGORY_COUNT ', "\n", '' + categories.length);
  fileContent = substituteBlock(fileContent, "typedef enum mjb_decomposition {\n", "\n} mjb_decomposition;", getDecompositionEnumNames());
  fileContent = substituteBlock(fileContent, "typedef enum mjb_derived_core_property {\n", "\n} mjb_derived_core_property;", getDerivedCorePropertiesEnumNames());
  fileContent = substituteBlock(fileContent, '#define MJB_DERIVED_CORE_PROPERTY_COUNT ', "\n", '' + Object.keys(properties).length);

  writeFileSync('../../src/unicode.h', fileContent);

  fileContent = readFileSync('../../src/mojibake.h', 'utf-8');
  fileContent = substituteBlock(fileContent, "// This functions list is automatically generated. Do not edit.\n\n", "\n#ifdef __cplusplus", getFunctions());

  writeFileSync('../../src/mojibake.h', fileContent);
}
