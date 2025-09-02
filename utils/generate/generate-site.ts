import { readFileSync, writeFileSync } from 'fs';
import { cfns } from './function';
import { substituteText } from './utils';

function getFunctions() {
  const functs = cfns();

  return functs.map(value => value.formatJSON()).join(',');
}

function formatFunction(funct: any[]) {
  // return `<div><div>${funct.comment}</div><div>${funct.ret} ${funct.name}(${funct.args.length ? funct.args.join(', ') : 'void'})</div></div>`;
}

export async function generateSite() {
  let fileContent = readFileSync('../../build-wasm/src/index.html', 'utf-8');
  const functs = getFunctions();

  fileContent = substituteText(fileContent,
    "const functions = [",
    "];",
    functs);

  writeFileSync('../../build-wasm/src/index.html', fileContent);
}
