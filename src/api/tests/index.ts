/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

// This is a temporary file for testing the Mojibake API. Do not edit.

import createMojibake from '../index.js';

// Temporary file, do not edit.
const mojibake = await createMojibake({
  locateFile: (path, prefix) => `${prefix}${path}`
});

console.log(mojibake.unicodeVersion());
