/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>

#ifdef MJB_USE_EMBEDDED_DB
#include "../build-embedded-amalgamation/mojibake.h"
#else
#include "../build-amalgamation/mojibake.h"
#endif

// This is a simple example of how to use the Mojibake library.
int main(int argc, char * const argv[]) {
#ifndef MJB_USE_EMBEDDED_DB
    // We initialize the library. No need to call this. It's only needed if you want to specify
    // where the database is located.
    if(!mjb_initialize_v2(NULL, NULL, NULL, "../mojibake.db", 0)) {
        fprintf(stderr, "Error: Failed to initialize Mojibake\n");

        return 1;
    }
#endif

    printf("This is an example of Mojibake v%s\n", mjb_version());
    printf("Unicode version: %s\n", mjb_unicode_version());

    mjb_character character;
    mjb_codepoint_character(0x022A, &character);

    // This will print: "U+022A: LATIN CAPITAL LETTER O WITH DIAERESIS AND MACRON"
    printf("U+%04X: %s\n", character.codepoint, character.name);

    return 0;
}
