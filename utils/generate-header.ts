import { readFileSync, writeFileSync } from 'fs';
import { cfns } from './cfunction';
import { Block, Categories, characterDecompositionMapping } from './types';
import { substituteText } from './utils';

function getBlockEnumNames(blocks: Block[]) {
  return blocks.map((value: Block, index: number) => `    ${value.enumName} = ${index}`).join(',\n');
}

function getCategoryEnumNames(categories: string[]) {
  const categoryEnums: string[] = [];

  for(let i = 0; i < categories.length; ++i) {
    categoryEnums.push(`    MJB_CATEGORY_${Categories[i].toUpperCase()} = 0x${(1 << i).toString(16).padStart(8, '0')}${ i === categories.length - 1 ? ' ' : ','} // ${i} (${Categories[i]}) ${categories[i]}`);
  }

  return categoryEnums.join('\n');
}

function getDecompositionEnumNames() {
  return Object.keys(characterDecompositionMapping).map((value: string, index: number) => `    MJB_DECOMPOSITION_${value.toUpperCase().replace(/[<>]/g, '')} = ${index}`).join(',\n')
}

function getFunctions() {
  return cfns.map(value => value.formatC()).join("\n\n") + "\n";
}

export function generateHeader(blocks: Block[], categories: string[]) {
  let fileContent = readFileSync('../src/mojibake.h', 'utf-8');

  fileContent = substituteText(fileContent, "typedef enum mjb_block {\n", "\n} mjb_block;", getBlockEnumNames(blocks));
  fileContent = substituteText(fileContent, '#define MJB_BLOCK_NUM ', "\n", '' + blocks.length);
  fileContent = substituteText(fileContent, "typedef enum mjb_category {\n", "\n} mjb_category;", getCategoryEnumNames(categories));
  fileContent = substituteText(fileContent, '#define MJB_CATEGORY_COUNT ', "\n", '' + categories.length);
  fileContent = substituteText(fileContent, "typedef enum mjb_decomposition {\n", "\n} mjb_decomposition;", getDecompositionEnumNames());
  fileContent = substituteText(fileContent, "} mjb_character;\n\n", "\n#ifdef __cplusplus", getFunctions());

  writeFileSync('../src/mojibake.h', fileContent);
}
