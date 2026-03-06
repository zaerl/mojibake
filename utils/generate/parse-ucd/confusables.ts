/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { createReadStream } from 'fs';
import { createInterface } from 'readline';

export interface ConfusableEntry {
    codepoint: number;
    skeleton: number[];
}

/**
 * Parse confusables.txt from Unicode Security Mechanisms (UTS#39).
 * Format: source_cp ; skeleton_cp1 [skeleton_cp2 ...] ; type # comment
 */
export async function parseConfusables(path: string): Promise<ConfusableEntry[]> {
    const entries: ConfusableEntry[] = [];

    const rl = createInterface({
        input: createReadStream(path),
        crlfDelay: Infinity,
    });

    for await (const rawLine of rl) {
        const line = rawLine.trim();

        if(line === '' || line.startsWith('#')) {
            continue;
        }

        const fields = line.split(';');

        if(fields.length < 2) {
            continue;
        }

        // Strip inline comment from field 0 and parse the source codepoint
        const sourceHex = fields[0].split('#')[0].trim();
        const codepoint = parseInt(sourceHex, 16);

        if(isNaN(codepoint)) {
            continue;
        }

        // Strip inline comment from field 1 and parse the skeleton codepoints
        const skeletonField = fields[1].split('#')[0].trim();
        const skeleton = skeletonField
            .split(/\s+/)
            .filter(s => s.length > 0)
            .map(s => parseInt(s, 16))
            .filter(n => !isNaN(n));

        if(skeleton.length === 0) {
            continue;
        }

        entries.push({ codepoint, skeleton });
    }

    return entries;
}
