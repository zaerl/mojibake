/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>

#include "../build-amalgamation/mojibake.h"

// This is a simple example of how to use the Mojibake library.
int main(int argc, char *const argv[]) {
    printf("This is an example of Mojibake v%s\n", mjb_version());
    printf("Unicode version: %s\n", mjb_unicode_version());

    mjb_character character;

    if(mjb_codepoint_character(0x022A, &character) != MJB_STATUS_OK) {
        fprintf(stderr, "Failed to read character information\n");
        return 1;
    }

    // This will print: "U+022A: LATIN CAPITAL LETTER O WITH DIAERESIS AND MACRON"
    printf("U+%04X: %s\n", character.codepoint, character.name);

    return 0;
}
