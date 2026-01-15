/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { readFileSync, writeFileSync, copyFileSync, mkdirSync, watch, readdirSync, statSync } from 'fs';
import { join, basename, relative } from 'path';
import { cfns } from '../function';
import { getVersion, substituteBlock, substituteText } from '../utils';

const SOURCE_DIR = '../../src/site';
const BUILD_DIR = '../../build-wasm/src';

function getFunctions() {
  const functs = cfns();

  return functs.filter(fn => fn.isWASM() && !fn.isInternal());
}

function processIndexHtml() {
  console.log('Processing index.html...');

  let fileContent = readFileSync(`${SOURCE_DIR}/index.html`, 'utf-8');
  const functs = getFunctions();

  fileContent = substituteBlock(fileContent,
    'const functions = {',
    '};',
    functs.map(fn => `"${fn.getName()}": ${fn.formatJSON()}`).join(',\n'));

  fileContent = substituteBlock(fileContent,
    '<section id="functions" class="loading">',
    '</section>',
    functs.map(fn => '    ' + fn.formatHTML()).join('\n'));

  const version = getVersion();
  const fileName = `mojibake-amalgamation-${version.major}${version.minor}${version.revision}.zip`;
  const embeddedFileName = `mojibake-embedded-amalgamation-${version.major}${version.minor}${version.revision}.zip`;
  const wasmFileName = `mojibake-wasm-${version.major}${version.minor}${version.revision}.zip`;

  fileContent = substituteText(fileContent, '[AM_HREF]', fileName);
  fileContent = substituteText(fileContent, '[AM_NAME]', fileName);
  fileContent = substituteText(fileContent, '[EMBEDDED_HREF]', embeddedFileName);
  fileContent = substituteText(fileContent, '[EMBEDDED_NAME]', embeddedFileName);
  fileContent = substituteText(fileContent, '[WASM_HREF]', wasmFileName);
  fileContent = substituteText(fileContent, '[WASM_NAME]', wasmFileName);
  fileContent = substituteText(fileContent, '[VERSION]', version.version);

  writeFileSync(`${BUILD_DIR}/index.html`, fileContent);
  console.log('index.html processed successfully');
}

function copyFile(filePath: string) {
  const relativePath = relative(SOURCE_DIR, filePath);
  const destPath = join(BUILD_DIR, relativePath);

  console.log(`Copying ${basename(filePath)}...`);

  // Ensure destination directory exists
  const destDir = join(destPath, '..');
  mkdirSync(destDir, { recursive: true });

  copyFileSync(filePath, destPath);
  console.log(`${basename(filePath)} copied successfully`);
}

function handleFileChange(filePath: string) {
  const fileName = basename(filePath);

  if(fileName === 'index.html') {
    processIndexHtml();
  } else if(!fileName.startsWith('.')) {
    // Skip hidden files like .DS_Store
    copyFile(filePath);
  }
}

function watchDirectory(dir: string) {
  console.log(`Watching ${dir}...`);

  try {
    watch(dir, { recursive: true }, (eventType, filename) => {
      if(filename && !filename.startsWith('.')) {
        const fullPath = join(dir, filename);

        // Check if file exists (it might have been deleted)
        try {
          const stats = statSync(fullPath);
          if(stats.isFile()) {
            handleFileChange(fullPath);
          }
        } catch (err) {
          // File was deleted or doesn't exist, ignore
        }
      }
    });
  } catch (err) {
    console.error('Error setting up watcher:', err);
    process.exit(1);
  }
}

// Check if watch mode is enabled
const watchMode = process.argv.includes('--watch');

// Ensure build directory exists
mkdirSync(BUILD_DIR, { recursive: true });

// Process all files initially
console.log('Initial build...');
processIndexHtml();

// Copy all non-index.html files initially
try {
  const files = readdirSync(SOURCE_DIR);
  files.forEach(file => {
    if(file !== 'index.html' && !file.startsWith('.')) {
      const fullPath = join(SOURCE_DIR, file);
      const stats = statSync(fullPath);

      if(stats.isFile()) {
        copyFile(fullPath);
      } else if(stats.isDirectory()) {
        // Copy directories recursively
        const copyDir = (src: string, dest: string) => {
          mkdirSync(dest, { recursive: true });
          const entries = readdirSync(src);
          entries.forEach(entry => {
            const srcPath = join(src, entry);
            const destPath = join(dest, entry);
            const stats = statSync(srcPath);

            if(stats.isFile() && !entry.startsWith('.')) {
              copyFileSync(srcPath, destPath);
              console.log(`Copied ${relative(SOURCE_DIR, srcPath)}`);
            } else if(stats.isDirectory()) {
              copyDir(srcPath, destPath);
            }
          });
        };

        copyDir(fullPath, join(BUILD_DIR, file));
      }
    }
  });
} catch (err) {
  console.error('Error during initial file copy:', err);
}

if(watchMode) {
  // Start watching
  watchDirectory(SOURCE_DIR);
  console.log('Ready for changes. Press Ctrl+C to stop.');
} else {
  console.log('Build complete.');
}
