/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

// This is a temporary file for testing the Mojibake API. Do not edit.

import createMojibake from '../index.js';
import { ATT_ASSERT, att_get_total_tests, att_get_valid_tests, att_set_verbose } from './attractor.js';

// Temporary file, do not edit.
const mojibake = await createMojibake({
  locateFile: (path, prefix) => `${prefix}${path}`
});

let showColors = true;
let verbosity = 0;

att_set_verbose(verbosity);

ATT_ASSERT(mojibake.unicodeVersion(), '17.0.0', 'Unicode version should be 17.0.0');

const valid = att_get_valid_tests();
const total = att_get_total_tests();
const isValid = valid === total;

const colorCode = showColors ? (isValid ? "\x1B[32m" : "\x1B[31m") : "";

console.log(
  `${verbosity >= 1 ? "\n" : ""}Tests valid/run: ${colorCode}${valid}/${total}${showColors ? "\x1B[0m" : ""}`
);
