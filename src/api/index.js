/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import http from 'http';
import { promises as fs } from 'fs';
import mojibakeModule from './mojibake.js';
import { CStruct } from './cstruct.js';
import functionsDefault from './functions.js';

let functions = {};
let mojibake = null;
let nextCharacterBuffer = null;

const encodingTypes = {
  'UTF-8': 0x2,
  'UTF-16BE': 0x8,
  'UTF-16LE': 0x10,
  'UTF-32BE': 0x40,
  'UTF-32LE': 0x80,
};

const normalizations = {
  'NFC': 0x0,
  'NFD': 0x1,
  'NFKC': 0x2,
  'NFKD': 0x3,
};

const caseTypes = {
  'upper': 0x1,
  'lower': 0x2,
  'title': 0x3,
  'casefold': 0x4,
};

const planeTypes = {
  'BMP': 0,
  'SMP': 1,
  'SIP': 2,
  'TIP': 3,
  'SSP': 4,
  'PUA_A': 5,
  'PUA_B': 16,
};

async function loadFunctions() {
  for(const func of functionsDefault) {
    if(!func.wasm) {
      continue;
    }

    functions[`mjb_${func.name}`] = func;
  }
}

function getenum(value, enumMap, defaultValue) {
  if(value === undefined || value === null) {
    if(defaultValue === undefined || defaultValue === null) {
      return undefined;
    }

    return enumMap[defaultValue];
  } else if(typeof value === 'number') {
    return value;
  } else {
    const transformed = value.toUpperCase();

    if(enumMap[transformed]) {
      return enumMap[transformed];
    }
  }
}

function readCodepoint(value) {
  // Handle different codepoint formats: decimal, hex, 0x hex, U+FFFF
  value = value.trim().toUpperCase();
  let codepoint = null;

  if(value.startsWith('U+')) {
    // U+FFFF format
    codepoint = parseInt(value.substring(2), 16);
  } else if(value.startsWith('0X')) {
    // 0xFFFF format
    codepoint = parseInt(value.substring(2), 16);
  } else {
    // Plain hex format
    codepoint = parseInt(value, 16);
  }

  if(isNaN(codepoint) || codepoint < 0) {
    throw new Error(`Invalid codepoint. Must be a 16-bit positive integer in decimal, hex (0xFFFF), or U+FFFF format.`);
  }

  return codepoint;
}

function getArgValue(type, value) {
  switch(type) {
    case 'mjb_encoding':
      return getenum(value, encodingTypes, 'UTF-8');
    case 'mjb_normalization':
      return getenum(value, normalizations, 'NFC');
    case 'mjb_case_type':
      return getenum(value, caseTypes, null);
    case 'mjb_plane':
      return getenum(value, planeTypes, null);
    case 'mjb_codepoint':
      if(typeof value === 'string' && value.trim() !== '') {
        return readCodepoint(value);
      } else {
        return null;
      }
    default:
      return value;
  }
}

function pointerToCharacter(ptr) {
  const struct = new CStruct(ptr);

  const char = struct.toObject({
    codepoint: 'u32',
    name: 'str128',
    category: 'u32',
    combining: 'u32',
    bidirectional: 'u16',
    decomposition: 'u32',
    decimal: 'i32',
    digit: 'i32',
    numeric: 'str16',
    mirrored: 'u8',
    uppercase: 'u32',
    lowercase: 'u32',
    titlecase: 'u32',
  }, mojibake.HEAPU8);

  if(char.decimal === -1) {
    char.decimal = null;
  }

  if(char.digit === -1) {
    char.digit = null;
  }

  char.uppercase = char.uppercase === 0 ? null : char.uppercase;
  char.lowercase = char.lowercase === 0 ? null : char.lowercase;
  char.titlecase = char.titlecase === 0 ? null : char.titlecase;
  char.mirrored = char.mirrored === 1;

  return char;
}

function utf8StringToHex(ptr, length) {
  const hex = [];
  let i = 0;

  while(mojibake.HEAPU8[ptr] !== 0 || (i < length)) {
    hex.push(mojibake.HEAPU8[ptr].toString(16).toUpperCase().padStart(2, '0'));
    ++ptr;
    ++i;
  }

  return hex;
}

function decodeString(buffer, size, encoding) {
  if(encoding == encodingTypes['UTF-8']) {
    return mojibake.UTF8ToString(buffer, size);
  } else {
    let retHex = [];

    for(let i = 0; i < size; ++i) {
      retHex.push(mojibake.HEAPU8[buffer + i].toString(16).toUpperCase().padStart(2, '0'));
    }

    return retHex.join(' ');
  }
}

function codepointToString(codepoint, all = true) {
  if(!all && (codepoint === 0 || codepoint === null)) {
    return 'N/A';
  }

  return 'U+' + codepoint.toString(16).toUpperCase().padStart(4, '0');
}

function stringToCodepointList(string) {
  const codepoints = [];

  for(let i = 0; i < string.length; i++) {
    const codepoint = string.codePointAt(i);

    if(codepoint) {
      codepoints.push(codepointToString(codepoint));

      // Skip the second surrogate pair code unit
      if(codepoint > 0xFFFF) {
        ++i;
      }
    }
  }

  return codepoints;
}

function ccallCodepointCharacter(codepoint) {
  // Allocate memory for mjb_character structure
  const structSize = 512;
  const ptr = mojibake._malloc(structSize);

  // Initialize memory to zero
  for(let i = 0; i < structSize; ++i) {
    mojibake.HEAPU8[ptr + i] = 0;
  }

  // Call mjb_codepoint_character
  const result = mojibake._mjb_codepoint_character(codepoint, ptr);

  if(result) {
    const char = pointerToCharacter(ptr, mojibake.HEAPU8);
    // Free allocated memory
    mojibake._free(ptr);

    return char;
  } else {
    // Free allocated memory
    mojibake._free(ptr);

    throw new Error('Failed to get character data for codepoint');
  }
}

function ccallNormalize(argTypes, buffer, size, encoding, form) {
  // See mjb_result on mojibake.h
  const structSize = 24;
  const ptr = mojibake._malloc(structSize);

  // Initialize memory to zero
  for(let i = 0; i < structSize; ++i) {
    mojibake.HEAPU8[ptr + i] = 0;
  }

  const result = mojibake.ccall('mjb_normalize', 'boolean', argTypes, [buffer, size, encoding, form, ptr]);

  if(result) {
    const struct = new CStruct(ptr);
    const result = struct.toObject({
      output: 'u32',
      output_size: 'u32',
      transformed: 'u8'
    }, mojibake.HEAPU8);

    const output = mojibake.UTF8ToString(result.output);
    const ret = {
      output: output,
      utf8: utf8StringToHex(result.output, result.output_size).join(' '),
      codepoints: stringToCodepointList(output).join(' '),
      output_size: result.output_size,
      transformed: result.transformed === 1
    };

    // Free allocated memory
    mojibake._free(ptr);

    return ret;
  } else {
    // Free allocated memory
    mojibake._free(ptr);

    throw new Error('Failed to get normalization data');
  }
}

function ccallCodepointEncode(codepoint, encoding) {
  const buffer = mojibake._malloc(5);

  for(let i = 0; i < 5; ++i) {
    mojibake.HEAPU8[buffer + i] = 0;
  }

  const result = mojibake._mjb_codepoint_encode(codepoint, buffer, 5, encoding);

  if(result) {
    mojibake._free(buffer);

    return decodeString(buffer, result, encoding);
  } else {
    mojibake._free(buffer);

    throw new Error('Failed to encode codepoint');
  }
}

function ccallEmoji(codepoint) {
  const structSize = 10;
  const ptr = mojibake._malloc(structSize);

  for(let i = 0; i < structSize; ++i) {
      mojibake.HEAPU8[ptr + i] = 0;
  }

  const result = mojibake._mjb_codepoint_emoji(codepoint, ptr);

  if(result) {
    const struct = new CStruct(ptr);
    const result = struct.toObject({
      codepoint: 'u32',
      emoji: 'u8',
      presentation: 'u8',
      modifier: 'u8',
      modifier_base: 'u8',
      component: 'u8',
      extended_pictographic: 'u8'
    }, mojibake.HEAPU8);

    const ret = {
      codepoint: result.codepoint === 0 ? null : result.codepoint,
      emoji: result.emoji === 1,
      presentation: result.presentation === 1,
      modifier: result.modifier === 1,
      modifier_base: result.modifier_base === 1,
      component: result.component === 1,
      extended_pictographic: result.extended_pictographic === 1
    };

    // Free allocated memory
    mojibake._free(ptr);

    return ret;
  } else {
    // Free allocated memory
    mojibake._free(ptr);

    throw new Error('Failed to get emoji data for codepoint');
  }
}

function ccCall(functionName, ret, argTypes, args) {
  if(functionName === 'mjb_codepoint_character') {
    return ccallCodepointCharacter(args[0]);
  } else if(functionName === 'mjb_normalize') {
    return ccallNormalize(argTypes, args[0], args[1], args[2], args[3]);
  } else if(functionName === 'mjb_codepoint_encode') {
    return ccallCodepointEncode(args[0], args[3]);
  } else if(functionName === 'mjb_codepoint_emoji') {
    return ccallEmoji(args[0]);
  } else {
    return mojibake.ccall(functionName, ret, argTypes, args);
  }
}

function typeMap(type) {
  if(type === 'const char *') {
    return 'string';
  } else if(type === 'char *') {
    return 'string';
  } else if(type === 'bool') {
    return 'boolean';
  }

  return 'number';
}

// Used for mjb_next_character callback.
function _mjbNextCharacterCallback(character, type) {
  const char = pointerToCharacter(character);
  nextCharacterBuffer.push(char);

  return true;
}

// Make the callback globally accessible for Emscripten
global._mjbNextCharacterCallback = _mjbNextCharacterCallback;

async function initializeMojibake() {
  const buf = await fs.readFile('./mojibake.db');
  const dbBytes = new Uint8Array(buf.buffer, buf.byteOffset, buf.byteLength);
  const dbPtr = mojibake._malloc(dbBytes.length);
  mojibake.HEAPU8.set(dbBytes, dbPtr);

  // Initialize the library
  const initialized = mojibake._mjb_initialize_v2(0, 0, 0, dbPtr, dbBytes.length);
  mojibake._free(dbPtr);

  if(!initialized) {
    throw new Error('Failed to initialize Mojibake');
  }
}

async function parseRequest(req) {
  const url = new URL(req.url, `http://${req.headers.host}`);
  const functionName = url.searchParams.get('function');

  if(functionName === null) {
    return functionsDefault;
  }

  if(!functions[functionName]) {
    throw new Error(`Function ${functionName} not found`);
  }

  const functionArgs = functions[functionName].args;

  // A function with no arguments returns the returned value
  if(functionArgs.length === 0 || (functionArgs.length === 1 && functionArgs[0].type === 'void')) {
    const result = mojibake.ccall(
      functionName,
      functions[functionName].ret === 'const char *' ? 'string' : 'number', []
    );

    return {
      [functionName.replace(/^mjb_/, '')]: result
    };
  }

  const args = [];
  const argTypes = [];
  const ret = typeMap(functions[functionName].ret);
  let hasBuffer = null;

  // Get all parameters from the function declaration
  for(let i = 0; i < functionArgs.length; ++i) {
    const arg = functionArgs[i];
    const rawType = arg.type;
    const mappedType = typeMap(rawType);
    let value = null;
    console.log(arg, rawType, mappedType);

    argTypes.push(mappedType);

    if(arg.wasm_generated) {
      if(rawType === 'size_t' && (
        arg === 'size' ||
        arg === 'max_length' ||
        arg === 's1_length' ||
        arg === 's2_length') &&
        hasBuffer !== null) {
        // Calculate the size of the buffer in bytes (UTF-8)
        const encoder = new TextEncoder().encode(hasBuffer);
        value = encoder.length;
      }
    } else {
      value = getArgValue(rawType, url.searchParams.get(arg.name));
      console.log(rawType, url.searchParams.get(arg.name));

      if(value === undefined || value === null) {
        throw new Error(`Missing parameter: ${arg.name}`);
      }

      if(mappedType === 'number') {
        value = parseInt(value, 10);
      } else if(mappedType === 'boolean') {
        value = value === '1';
      }

      if(hasBuffer === null && rawType === 'const char *' &&
        (arg.name === 'buffer' || arg.name === 's1' || arg.name === 's2')) {
        hasBuffer = value;
      }
    }

    args.push(value);
  }

  nextCharacterBuffer = [];
  let result = ccCall(functionName, ret, argTypes, args);

  if(functionName === 'mjb_next_character') {
    return {
      characters: nextCharacterBuffer,
    };
  }

  nextCharacterBuffer = null;

  return result;
}

const server = http.createServer(async (req, res) => {
  let httpStatus = 200;
  let response = null;

  res.setHeader('Access-Control-Allow-Origin', '*');
  res.setHeader('Access-Control-Allow-Methods', 'GET,POST,OPTIONS');
  res.setHeader('Access-Control-Allow-Headers', 'Content-Type, Authorization');

  if(req.method === 'OPTIONS') {
    res.writeHead(204);
    res.end();

    return;
  }

  try {
    response = await parseRequest(req);
  } catch(error) {
    const clientIp = req.headers['x-real-ip'] || req.headers['x-forwarded-for'] || req.socket.remoteAddress;
    console.error(`[${new Date().toISOString()}] ${clientIp} - Error:`, error.message);
    console.error(error);

    httpStatus = 400;
    response = {
      error: error.name,
      message: error.message
    };
  }

  res.writeHead(httpStatus, { 'Content-Type': 'application/json' });
  res.end(JSON.stringify(response));
});

server.listen(3000, '0.0.0.0', async () => {
  await loadFunctions();
  mojibake = await mojibakeModule();
  await initializeMojibake();
  console.log('Server running on http://0.0.0.0:3000');
});

// Graceful shutdown
process.on('SIGTERM', () => {
  server.close(() => {
    console.log('Server closed');
    process.exit(0);
  });
});

process.on('SIGINT', () => {
  server.close(() => {
    console.log('Server closed');
    process.exit(0);
  });
});
