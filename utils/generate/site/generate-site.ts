/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { copyFileSync, createReadStream, existsSync, mkdirSync, readdirSync, readFileSync, statSync, watch, writeFileSync } from 'fs';
import http from 'http';
import markdownit from 'markdown-it';
import { basename, extname, join, relative } from 'path';
import { cfns } from '../html-function';
import { Section } from '../functions';
import { getVersion, substituteBlock, substituteText } from '../utils';
import hljs from 'highlight.js/lib/core';

const SOURCE_DIR = '../../src/site';
const BUILD_DIR = '../../build-wasm/src';
const API_DIST_DIR = '../../src/api/dist';
const API_ROUTE = '/api/';
const SERVE_PORT = 6251;
const HIGHLIGHT_THEMES = {
  'highlight-light.css': require.resolve('highlight.js/styles/github.css'),
  'highlight-dark.css': require.resolve('highlight.js/styles/github-dark.css'),
};

const API_SECTIONS = [
  {
    section: Section.TextTransformation,
    id: 'text-transformation',
    title: 'Text transformation',
    description: 'Normalize, case-convert, filter, and convert Unicode text.'
  },
  {
    section: Section.TextAnalysis,
    id: 'text-analysis',
    title: 'Text analysis',
    description: 'Inspect codepoints, properties, boundaries, width, emoji, and bidirectional text.'
  },
  {
    section: Section.Segmentation,
    id: 'segmentation',
    title: 'Segmentation',
    description: 'Segment text into grapheme clusters, words, and sentences.'
  },
  {
    section: Section.SortingComparison,
    id: 'sorting-comparison',
    title: 'Sorting and comparison',
    description: 'Compare and sort strings with the Unicode Collation Algorithm.'
  },
  {
    section: Section.Security,
    id: 'security',
    title: 'Security',
    description: 'Detect visually confusable Unicode text.'
  },
  {
    section: Section.Utility,
    id: 'utility',
    title: 'Utilities',
    description: 'Use locales, memory hooks, version details, and low-level string helpers.'
  }
] as const;

const MIME_TYPES: Record<string, string> = {
  '.html': 'text/html; charset=utf-8',
  '.css': 'text/css',
  '.js': 'application/javascript',
  '.wasm': 'application/wasm',
  '.json': 'application/json',
  '.png': 'image/png',
  '.ico': 'image/x-icon',
  '.webmanifest': 'application/manifest+json',
};

function getFunctions() {
  const functs = cfns();

  return functs.filter(fn => /*fn.isWASM() && */!fn.isInternal());
}

function escapeHTML(value: string): string {
  return value.replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&#39;');
}

function formatAPINavigation(functs: ReturnType<typeof getFunctions>): string {
  return API_SECTIONS.map(section => {
    const sectionFunctions = functs.filter(fn => fn.getSection() === section.section);
    const links = sectionFunctions.map(fn =>
      `          <li><a href="#${fn.getName()}" data-function-link ` +
      `data-search="${escapeHTML(`${fn.getName()} ${fn.comment}`.toLowerCase())}">` +
      `<code>${fn.getName()}</code></a></li>`
    ).join('\n');

    return `      <details class="sidebar-group" data-section-nav open>
        <summary>
          <span>${section.title}</span>
          <span class="sidebar-count" aria-label="${sectionFunctions.length} functions">` +
      `${sectionFunctions.length}</span>
        </summary>
        <ul>
${links}
        </ul>
      </details>`;
  }).join('\n');
}

function formatAPISections(
  functs: ReturnType<typeof getFunctions>,
  functionNames: Set<string>
): string {
  return API_SECTIONS.map(section => {
    const sectionFunctions = functs.filter(fn => fn.getSection() === section.section);
    const functionsHTML = sectionFunctions.map(fn =>
      fn.formatHTML(functionNames)
    ).join('\n');

    return `    <section class="api-section" id="api-${section.id}" data-api-section>
      <header class="api-section-header">
        <div>
          <p class="eyebrow">API section</p>
          <h2>${section.title}</h2>
          <p>${section.description}</p>
        </div>
        <span class="api-section-count">${sectionFunctions.length} functions</span>
      </header>
${functionsHTML}
    </section>`;
  }).join('\n');
}

function processIndexHtml() {
  console.log('Processing index.html...');

  let fileContent = readFileSync(`${SOURCE_DIR}/index.html`, 'utf-8');
  const functs = getFunctions();
  const functionNames = new Set(functs.map(fn => fn.getName()));

  fileContent = substituteBlock(fileContent,
    '<nav id="api-navigation" aria-label="API reference">',
    '</nav>',
    '\n' + formatAPINavigation(functs) + '\n    ');

  fileContent = substituteBlock(fileContent,
    'const functions = {',
    '};',
    functs.map(fn => `"${fn.getName()}": ${fn.formatJSON()}`).join(',\n'));

  fileContent = substituteBlock(fileContent,
    '<div id="functions">',
    '</div>',
    '\n' + formatAPISections(functs, functionNames) + '\n        ');

  fileContent = substituteBlock(fileContent,
    "// On click events\n",
    "    </script>",
    functs.filter(fn => fn.isWASM()).map(fn => '        ' + fn.formatEventListener()).join('\n') + "\n"
  );

  const version = getVersion();
  const fileName = `mojibake-amalgamation-${version.major}${version.minor}${version.revision}.zip`;
  const wasmFileName = `mojibake-wasm-${version.major}${version.minor}${version.revision}.zip`;
  const baseURL = `https://github.com/zaerl/mojibake/releases/download/v${version.version}/`;

  fileContent = substituteText(fileContent, '[AM_HREF]', baseURL + fileName);
  fileContent = substituteText(fileContent, '[AM_NAME]', fileName);
  fileContent = substituteText(fileContent, '[WASM_HREF]', baseURL + wasmFileName);
  fileContent = substituteText(fileContent, '[WASM_NAME]', wasmFileName);
  fileContent = substituteText(fileContent, '[VERSION]', version.version);

  let readme = readFileSync('../../README.md', 'utf-8');
  readme = substituteBlock(readme, 'CONFORMANCE-REQUIREMENTS.md)\n', '## Feature highlights', '');
  readme = substituteBlock(readme, 'CONFORMANCE-REQUIREMENTS.md)\n', '## Feature highlights', '');

  const md = markdownit({
    highlight: function (str, lang) {
      if(lang && hljs.getLanguage(lang)) {

        try {
          return hljs.highlight(str, { language: lang }).value;
        } catch (__) {}
      }

      return ''; // use external default escaping
    }
  }).use(require('markdown-it-footnote'));

  const header = md.render(readme.slice(readme.indexOf('**Mojibake'), readme.indexOf('### Thanks')));
  fileContent = substituteText(fileContent, '[HEADER_HERE]', header);

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

function copyHighlightThemes() {
  for(const [fileName, sourcePath] of Object.entries(HIGHLIGHT_THEMES)) {
    copyFileSync(sourcePath, join(BUILD_DIR, fileName));
    console.log(`${fileName} copied successfully`);
  }
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
copyHighlightThemes();

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

function serveStatic(port = SERVE_PORT) {
  const server = http.createServer((req, res) => {
    let urlPath = req.url === '/' ? '/index.html' : req.url!;
    urlPath = urlPath.split('?')[0];

    if(urlPath === '/api' || urlPath === API_ROUTE) {
      urlPath = `${API_ROUTE}index.js`;
    }

    const filePath = urlPath.startsWith(API_ROUTE) ?
      join(API_DIST_DIR, urlPath.slice(API_ROUTE.length)) :
      join(BUILD_DIR, urlPath);

    if(!existsSync(filePath) || !statSync(filePath).isFile()) {
      res.writeHead(404);
      res.end('Not found');
      return;
    }

    const mimeType = MIME_TYPES[extname(filePath)] || 'application/octet-stream';
    res.writeHead(200, { 'Content-Type': mimeType });
    createReadStream(filePath).pipe(res);
  });

  server.on('error', (err: NodeJS.ErrnoException) => {
    if(err.code === 'EADDRINUSE') {
      console.log(`Port ${port} in use, trying ${port + 1}...`);
      server.close();
      serveStatic(port + 1);
    } else {
      console.error('Server error:', err.message);
      process.exit(1);
    }
  });

  server.listen(port, '127.0.0.1', () => {
    console.log(`Site served at http://localhost:${port}`);
    console.log(`API library served at http://localhost:${port}${API_ROUTE}`);
  });
}

if(watchMode) {
  // Start watching and serving
  watchDirectory(SOURCE_DIR);
  serveStatic();
  console.log('Ready for changes. Press Ctrl+C to stop.');
} else {
  console.log('Build complete.');
}
