/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */
import { copyFile, mkdir } from 'node:fs/promises';

await mkdir('dist', { recursive: true });

await Promise.all([
  copyFile('mojibake.js', 'dist/mojibake.js'),
  copyFile('mojibake.wasm', 'dist/mojibake.wasm'),
  copyFile('mojibake.d.ts', 'dist/mojibake.d.ts')
]);
