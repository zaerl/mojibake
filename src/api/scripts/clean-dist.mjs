/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { rm } from 'node:fs/promises';

await rm('dist', { recursive: true, force: true });
