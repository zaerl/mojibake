import { readFileSync, writeFileSync } from 'fs';
import path from 'path';
import { substituteText } from './utils';

function getFunctions() {
  const jsonPath = path.join(__dirname, 'functions.json');
  const jsonData = readFileSync(jsonPath, 'utf8');
  const data: any[] = JSON.parse(jsonData);

  return JSON.stringify(data).slice(1, -1);
}

export async function generateSite() {
  let fileContent = readFileSync('../../build-wasm/src/index.html', 'utf-8');

  fileContent = substituteText(fileContent,
    "const functions = [",
    "];",
    getFunctions());

  writeFileSync('../../build-wasm/src/index.html', fileContent);
}
