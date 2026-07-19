/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { spawn } from 'node:child_process';
import { readFile } from 'node:fs/promises';
import { createServer } from 'node:http';
import { dirname, extname, resolve, sep } from 'node:path';
import { fileURLToPath } from 'node:url';

const apiRoot = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const contentTypes = new Map([
  ['.html', 'text/html; charset=utf-8'],
  ['.js', 'text/javascript; charset=utf-8'],
  ['.wasm', 'application/wasm']
]);

const server = createServer(async (request, response) => {
  try {
    const url = new URL(request.url ?? '/', 'http://127.0.0.1');
    const pathname = url.pathname === '/' ? '/tests/browser.html' : url.pathname;
    const file = resolve(apiRoot, `.${decodeURIComponent(pathname)}`);

    if(!file.startsWith(`${apiRoot}${sep}`)) {
      response.writeHead(403).end('Forbidden');
      return;
    }

    const body = await readFile(file);
    response.writeHead(200, {
      'Content-Type': contentTypes.get(extname(file)) ?? 'application/octet-stream'
    });

    response.end(body);
  } catch(error) {
    response.writeHead(404).end(error instanceof Error ? error.message : String(error));
  }
});

await new Promise((resolveListen, rejectListen) => {
  server.once('error', rejectListen);
  server.listen(0, '127.0.0.1', resolveListen);
});

const address = server.address();

if(address === null || typeof address === 'string') {
  server.close();

  throw new Error('Could not determine browser test server address');
}

const url = `http://127.0.0.1:${address.port}/tests/browser.html`;

// Let's try the best.
const defaultBrowserCandidates = [
  'google-chrome',
  'chromium',
  'chromium-browser',
  'chrome',
  'msedge',
  '/Applications/Google Chrome.app/Contents/MacOS/Google Chrome'
];

if(process.platform === 'win32') {
  const windowsRoots = [
    process.env.PROGRAMFILES,
    process.env['PROGRAMFILES(X86)'],
    process.env.LOCALAPPDATA
  ].filter(root => root !== undefined);

  for(const root of windowsRoots) {
    defaultBrowserCandidates.push(resolve(root, 'Google/Chrome/Application/chrome.exe'));
    defaultBrowserCandidates.push(resolve(root, 'Microsoft/Edge/Application/msedge.exe'));
  }
}

const browserCandidates = process.env.MJB_BROWSER ? [process.env.MJB_BROWSER] :
  defaultBrowserCandidates;

async function runBrowser(command) {
  return await new Promise((resolveRun, rejectRun) => {
    const child = spawn(command, [
      '--headless',
      '--disable-dev-shm-usage',
      '--disable-gpu',
      '--no-sandbox',
      '--virtual-time-budget=15000',
      '--dump-dom',
      url
    ]);
    let stdout = '';
    let stderr = '';
    let settled = false;

    const finish = callback => value => {
      if(settled) {
        return;
      }

      settled = true;
      clearTimeout(timeout);
      callback(value);
    };
    const timeout = setTimeout(() => {
      child.kill('SIGKILL');
      finish(rejectRun)(new Error(`Browser runtime test timed out with ${command}`));
    }, 30000);

    child.stdout.setEncoding('utf8');
    child.stderr.setEncoding('utf8');
    child.stdout.on('data', chunk => stdout += chunk);
    child.stderr.on('data', chunk => stderr += chunk);
    child.once('error', finish(rejectRun));
    child.once('close', code => finish(resolveRun)({ code, stdout, stderr }));
  });
}

let browserResult = null;
let browserCommand = '';

try {
  for(const candidate of browserCandidates) {
    try {
      browserResult = await runBrowser(candidate);
      browserCommand = candidate;
      break;
    } catch(error) {
      if(!(error instanceof Error) || !('code' in error) || error.code !== 'ENOENT') {
        throw error;
      }
    }
  }
} finally {
  await new Promise(resolveClose => server.close(resolveClose));
}

if(browserResult === null) {
  throw new Error(`No supported browser found (tried ${browserCandidates.join(', ')})`);
}

if(browserResult.code !== 0 || !browserResult.stdout.includes('id="result">PASS<')) {
  console.error(browserResult.stdout);
  console.error(browserResult.stderr);
  throw new Error(`Browser runtime test failed with ${browserCommand}`);
}

console.log(`Browser runtime test passed with ${browserCommand}`);
