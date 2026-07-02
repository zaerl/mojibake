/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

// This is a temporary file for testing the Mojibake API. Do not edit.

import createMojibake from '../index.js';
import { ATT_ASSERT, att_set_verbose } from './attraction.js';

// Temporary file, do not edit.
const mojibake = await createMojibake({
  locateFile: (path, prefix) => `${prefix}${path}`
});

att_set_verbose(2);

ATT_ASSERT(mojibake.unicodeVersion(), '17.0.0', 'Unicode version should be 17.0.0');
