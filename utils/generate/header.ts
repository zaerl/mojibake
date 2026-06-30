/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { readFileSync, writeFileSync } from 'fs';
import { cfns } from './html-function';
import { Property } from './parse-ucd/properties';
import { BidiBracket, BidiMirroringPair, Block, Categories, characterDecompositionMapping } from './types';
import { substituteBlock } from './utils';

function getBlockEnumNames(blocks: Block[], wasm = false) {
  return blocks.map((value: Block, index: number) => `  ${wasm ? value.wasmEnumName : '  ' + value.enumName}`).join(',\n');
}

function getCategoryEnumNames(categories: string[], wasm = false) {
  const categoryEnums: string[] = [];

  for(let i = 0; i < categories.length; ++i) {
    const comment = ` // ${i} (${Categories[i]}) ${categories[i]}`;
    categoryEnums.push(`${wasm ? '  ' : '    MJB_CATEGORY_'}${Categories[i].toUpperCase()}${ i === categories.length - 1 ? ' ' : ','}${comment}`);
  }

  return categoryEnums.join('\n');
}

function getDecompositionEnumNames(wasm = false) {
  const prefix = wasm ? '  ' : '    MJB_DECOMPOSITION_';

  return Object.keys(characterDecompositionMapping).map((value: string, index: number) => `${prefix}${value.toUpperCase().replace(/[<>]/g, '')}`).join(',\n');
}

function getPropertyEnumNames(properties: Property[], wasm = false) {
  const propertyEnums: string[] = [];
  const prefix = wasm ? '  ' : '    MJB_PR_';

  for(let i = 0; i < properties.length; ++i) {
    const name = properties[i].name.toUpperCase().replace(/[<>]/g, '');
    propertyEnums.push(`${prefix}${name}${ i === properties.length - 1 ? '' : ','}${properties[i].bool ? '' : ' // enumerated'}`);
  }

  return propertyEnums.join('\n');
}

function getPropertyNames(properties: Property[]) {
  return properties.map((value: Property, index: number) => {
    return `    "${value.name}"${index === properties.length - 1 ? '' : ','}${value.bool ? '' : ' // enumerated'}`;
  }).join('\n');
}

function getScriptEnumNames(properties: { [key: string]: number }, wasm = false) {
  const propertyEnums: string[] = [];

  /*
    {
      Adlm: 1,
      Adlam: 1,
      ...
    }
  */
  let previousValue = 0;

  for(const key in properties) {
    if(properties[key] === previousValue) {
      continue;
    }

    propertyEnums.push(`${wasm ? '  ' : '    MJB_SC_'}${key.toUpperCase()}`);
    previousValue = properties[key];
  }

  return propertyEnums;
}

function getFunctions() {
  return cfns().map(value => value.formatC()).join("\n\n") + "\n";
}

function getBidiBracketInfo(bidiBrackets: BidiBracket[]) {
  let ret = '';
  let i = 0;

  for(const bidiBracket of bidiBrackets) {
    let prefix = i === 0 ? '    ' : ', ';
    let suffix = i === 2 ? ',\n' : '';

    if(++i === 3) {
      i = 0;
    }

    ret += `${prefix}{ 0x${bidiBracket.cp.toString(16).padStart(4, '0').toUpperCase()}, `
    ret += `0x${bidiBracket.pair.toString(16).padStart(4, '0').toUpperCase()}, ${bidiBracket.isOpen ? 'true' : 'false' } }${suffix}`;
  }

  return ret;
}

function getBidiMirroringInfo(pairs: BidiMirroringPair[]) {
  let ret = '';
  let i = 0;

  for(const pair of pairs) {
    let prefix = i === 0 ? '    ' : ', ';
    let suffix = i === 2 ? ',\n' : '';

    if(++i === 3) {
      i = 0;
    }

    ret += `${prefix}{ 0x${pair.cp.toString(16).padStart(4, '0').toUpperCase()}, `
    ret += `0x${pair.mirror.toString(16).padStart(4, '0').toUpperCase()} }${suffix}`;
  }

  return ret;
}

export function generateHeader(blocks: Block[], categories: string[], properties: Property[], bidiBrackets: BidiBracket[], bidiMirroring: BidiMirroringPair[]) {
  let fileContent = readFileSync('../../src/unicode.h', 'utf-8');
  let fileWASMContent = readFileSync('../../src/api/unicode.ts', 'utf-8');

  // Fill unicode.h mjb_block.
  fileContent = substituteBlock(fileContent, "typedef enum mjb_block {\n", "\n} mjb_block;", getBlockEnumNames(blocks));
  fileContent = substituteBlock(fileContent, '#define MJB_BLOCK_NUM ', "\n", '' + blocks.length);

  // Fill unicode.ts Block.
  fileWASMContent = substituteBlock(fileWASMContent, "export enum Block {\n", "\n};", getBlockEnumNames(blocks, true));

  // Fill unicode.h mjb_category.
  fileContent = substituteBlock(fileContent, "typedef enum mjb_category {\n", "\n} mjb_category;", getCategoryEnumNames(categories));
  fileContent = substituteBlock(fileContent, '#define MJB_CATEGORY_COUNT ', "\n", '' + categories.length);

  // Fill unicode.ts Category.
  fileWASMContent = substituteBlock(fileWASMContent, "export enum Category {\n", "\n};", getCategoryEnumNames(categories, true));

  // Fill unicode.h mjb_decomposition and mjb_property.
  fileContent = substituteBlock(fileContent, "typedef enum mjb_decomposition {\n", "\n} mjb_decomposition;", getDecompositionEnumNames());
  fileContent = substituteBlock(fileContent, "typedef enum mjb_property {\n", "\n} mjb_property;", getPropertyEnumNames(properties));
  fileContent = substituteBlock(fileContent, '#define MJB_PR_COUNT ', "\n", '' + properties.length);

  // Fill unicode.ts Decomposition and Property.
  fileWASMContent = substituteBlock(fileWASMContent, "export enum Decomposition {\n", "\n};", getDecompositionEnumNames(true));
  fileWASMContent = substituteBlock(fileWASMContent, "export enum Property {\n", "\n};", getPropertyEnumNames(properties, true));

  const scriptProperty = properties.find(property => property.name === 'Script');

  if(scriptProperty) {
    // Fill unicode.h mjb_script.
    let scriptEnumNames = getScriptEnumNames(scriptProperty.values);
    fileContent = substituteBlock(fileContent, "  MJB_SC_NOT_SET, // 0 is \"no value\"\n", "\n} mjb_script;", scriptEnumNames.join(',\n'));
    fileContent = substituteBlock(fileContent, '#define MJB_SC_COUNT ', "\n", '' + (scriptEnumNames.length + 1));

    // Fill unicode.ts Script.
    scriptEnumNames = getScriptEnumNames(scriptProperty.values, true);
    fileWASMContent = substituteBlock(fileWASMContent, "  NOT_SET, // 0 is \"no value\"\n", "\n};", scriptEnumNames.join(',\n'));
  }

  let boolCount = properties.reduce((previousValue, currentValue) => previousValue + (currentValue.bool ? 1 : 0), 0);
  let enumCount = properties.length - boolCount;
  fileContent = substituteBlock(fileContent, '#define MJB_PR_ENUM_COUNT ', "\n", '' + enumCount);
  fileContent = substituteBlock(fileContent, '#define MJB_PR_BOOL_COUNT ', "\n", '' + boolCount);
  fileContent = substituteBlock(fileContent, '#define MJB_PR_BUFFER_SIZE ', "\n", '' + (boolCount + enumCount * 2));

  writeFileSync('../../src/unicode.h', fileContent);

  fileContent = readFileSync('../../src/mojibake.h', 'utf-8');

  // Add list of functions to mojibake.h
  fileContent = substituteBlock(fileContent, "// This functions list is automatically generated. Do not edit.\n\n", "\n#ifdef __cplusplus", getFunctions());

  writeFileSync('../../src/mojibake.h', fileContent);

  fileContent = readFileSync('../../src/properties.c', 'utf-8');

  // Add list of property names to shell maps.c
  fileContent = substituteBlock(fileContent, "static const char *mjb_property_names[] = {\n", "\n};", getPropertyNames(properties));

  writeFileSync('../../src/properties.c', fileContent);

  fileContent = readFileSync('../../src/bidi.c', 'utf-8');

  // Fill bidi.c static arrays with bidi bracket and mirroring information.
  fileContent = substituteBlock(fileContent, "#define MJB_BIDI_BRACKET_COUNT", "\n", ' ' + bidiBrackets.length);
  fileContent = substituteBlock(fileContent, "static const mjb_bidi_bracket_info mjb_bidi_brackets[] = {\n", "\n};", getBidiBracketInfo(bidiBrackets));
  fileContent = substituteBlock(fileContent, "#define MJB_BIDI_MIRRORING_COUNT", "\n", ' ' + bidiMirroring.length);
  fileContent = substituteBlock(fileContent, "static const mjb_bidi_mirroring_pair mjb_bidi_mirroring[] = {\n", "\n};", getBidiMirroringInfo(bidiMirroring));

  writeFileSync('../../src/bidi.c', fileContent);
  writeFileSync('../../src/api/unicode.ts', fileWASMContent);
}
