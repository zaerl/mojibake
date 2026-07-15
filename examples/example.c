/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>

#include "../build-amalgamation/mojibake.h"

void print_string(const char *input, size_t length);

// This is a simple example of how to use the Mojibake library.
int main(int argc, char *const argv[]) {
    const char *input = "Cafe\xCC\x81";
    size_t length = strlen(input);
    mjb_result result;

    // Normalize example: in NFC e + ◌́ -> é (U+00E9)
    if(mjb_normalize(input, length, MJB_ENC_UTF_8, MJB_NORMALIZATION_NFC, MJB_ENC_UTF_8, &result) !=
        MJB_STATUS_OK) {
        return 1;
    }

    // Cafe + ◌́ (U+0301, COMBINING ACUTE ACCENT) -> Café
    print_string(input, length);

    // Caf + é (U+00E9, LATIN SMALL LETTER E WITH ACUTE) -> Café
    print_string(result.output, result.output_size);

    const char *mojibake = "文字化け";
    length = strlen(mojibake);

    // String length example: mjb_string_length counts the number of characters in a string, not the
    // number of bytes.
    printf("\"%s\" encoded in UTF-8 is %zu bytes long, but instead is %zu characters long\n",
        mojibake, length, mjb_string_length(mojibake, length, MJB_ENC_UTF_8));

    mjb_result_free(&result);

    const char *case_input = "Straße";

    // NFKC casefold example: in NFKC casefold, ß -> ss
    if(mjb_nfkc_casefold(case_input, strlen(case_input), MJB_ENC_UTF_8, MJB_ENC_UTF_8, &result) !=
        MJB_STATUS_OK) {
        return 1;
    }

    printf("%s -> %.*s\n", case_input, (int)result.output_size, result.output);
    mjb_result_free(&result);

    return 0;
}

void print_string(const char *input, size_t length) {
    for(size_t i = 0; i < length; ++i) {
        unsigned char byte = (unsigned char)input[i];

        if(byte >= 0x21 && byte <= 0x7E) {
            printf("%c", byte);
        } else {
            printf("<%02X>", byte);
        }
    }

    printf("\n");
}
