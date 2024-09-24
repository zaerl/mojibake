import { readFileSync, writeFileSync } from 'fs';
import { cfns } from './cfunction';
import { substituteText } from './utils';
import { Block, Categories, characterDecompositionMapping } from './types';

export function generateHeader(blocks: Block[], categories: string[]) {
  const categoryEnums: string[] = [];

  for(let i = 0; i < categories.length; ++i) {
    categoryEnums.push(`    MJB_CATEGORY_${Categories[i].toUpperCase()} = 0x${(1 << i).toString(16).padStart(8, '0')}${ i === categories.length - 1 ? ' ' : ','} // ${i} (${Categories[i]}) ${categories[i]}`);
  }

  let fileContent = readFileSync('../src/mojibake.h', 'utf-8');

  fileContent = substituteText(fileContent, "typedef enum mjb_block {\n", "\n} mjb_block;", blocks.map((value: Block, index: number) => `    ${value.enumName} = ${index}`).join(',\n'));
  fileContent = substituteText(fileContent, "typedef enum mjb_category {\n", "\n} mjb_category;", categoryEnums.join('\n'));
  fileContent = substituteText(fileContent, '#define MJB_CATEGORY_COUNT ', "\n", '' + categoryEnums.length);
  fileContent = substituteText(fileContent, "typedef enum mjb_decomposition {\n", "\n} mjb_decomposition;", Object.keys(characterDecompositionMapping).map((value: string, index: number) => `    MJB_DECOMPOSITION_${value.toUpperCase().replace(/[<>]/g, '')} = ${index}`).join(',\n'));
  fileContent = substituteText(fileContent, "} mjb_character;\n\n", "\n#ifdef __cplusplus", cfns.map(value => value.formatC()).join("\n\n") + "\n");

  writeFileSync('../src/mojibake.h', fileContent);
}
