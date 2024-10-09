/**
 * The mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "test.h"

/**
 * Get codepoints from a string
 * Example: "0044 0307", gives 2 codepoints
 */
size_t get_codepoints(char *buffer, char *codepoints, size_t size) {
    char *token, *string, *tofree;
    tofree = string = strdup(buffer);
    unsigned int index = 0;

    while((token = strsep(&string, " ")) != NULL) {
        mjb_codepoint codepoint = strtoul((const char*)token, NULL, 16);
        // ++index;
        index += mjb_codepoint_encode(codepoint, codepoints + index, size - index, MJB_ENCODING_UTF_8);
    }

    codepoints[++index] = '\0';
    free(tofree);

    return index;
}

/**
 *
 * source, NFC, NFD, NFKC, NFKD
 * source, c1, c2, c3, c4
 *
 * NFC
 * c2 == toNFC(c1) == toNFC(c2) == toNFC(c3)
 * c4 == toNFC(c4) == toNFC(c5)
 *
 * NFD
 * c3 == toNFD(c1) == toNFD(c2) == toNFD(c3)
 * c5 == toNFD(c4) == toNFD(c5)
 *
 * NFKC
 * c4 == toNFKC(c1) == toNFKC(c2) == toNFKC(c3) == toNFKC(c4) == toNFKC(c5)
 *
 * NFKD
 * c5 == toNFKD(c1) == toNFKD(c2) == toNFKD(c3) == toNFKD(c4) == toNFKD(c5)
 *
 * [see: NormalizationTest.txt]
 * Example for LATIN CAPITAL LETTER D WITH DOT ABOVE
 * From UnicodeData.txt#L6883
 * 1E00 ...;0044 0307;...
 *
 * From utils/UCD/NormalizationTest.txt#L46
 * 1E0A;1E0A;0044 0307;1E0A;0044 0307; # (Ḋ; Ḋ; D◌̇; Ḋ; D◌̇; ) LATIN CAPITAL LETTER D WITH DOT ABOVE
 * c1 Source 1E0A (Ḋ)
 * c2 NFC    1E0A (Ḋ)
 * c3 NFD    0044 0307 (D◌̇)
 * c4 NFKC   1E0A (Ḋ)
 * c5 NFKD   0044 0307 (D◌̇)
 */
unsigned int check_normalization(char *source, size_t source_size, char *normalized, size_t normalized_size, mjb_normalization form) {
    size_t normalized_size_res;
    char *normalized_res = mjb_normalize(source, source_size, &normalized_size_res, MJB_ENCODING_UTF_8, form);
    int ret = 0; // OK

    if(normalized_res == NULL) {
        return 1; // Normalization failed
    }

    if(normalized_size_res != normalized_size) {
        mjb_free(normalized_res);

        return 2; // Size mismatch
    }

    for(size_t i = 0; i < normalized_size; ++i) {
        if(normalized_res[i] != normalized[i]) {
            ret = 3; // Codepoint mismatch
            break;
        }
    }

    if(normalized_res != NULL) {
        mjb_free(normalized_res);
    }

    return ret;
}

/**
 * Run utils/UCD/NormalizationTest.txt tests
 */
void *test_normalization(void *arg) {
    char line[512];
    char test_name[128];
    unsigned int limit = 0;
    unsigned int current_line = 1;
    // unsigned int index = 0;

    // 128 characters is enough for any test.
    const char source[128] = { 0 };
    const char nfc[128] = { 0 };
    const char nfd[128] = { 0 };
    const char nfkc[128] = { 0 };
    const char nfkd[128] = { 0 };

    size_t source_size = 0;
    size_t nfc_size = 0;
    size_t nfd_size = 0;
    size_t nfkc_size = 0;
    size_t nfkd_size = 0;

    FILE *file = fopen("./utils/UCD/NormalizationTest.txt", "r");

    if(file == NULL) {
        ATT_ASSERT("Not opened", "Opened file", "Valid normalization test file")

        return NULL;
    }

    // Parse the file
    while(fgets(line, 512, file)) {
        // puts("Normalization test");
        if(line[0] == '#' || line[0] == '@' || strnlen(line, 512) == 0) {
            ++current_line;

            continue;
        }

        // Reset everything
        source_size = 0;
        nfc_size = 0;
        nfd_size = 0;
        nfkc_size = 0;
        nfkd_size = 0;

        char *token, *string, *tofree;
        tofree = string = strdup(line);
        unsigned int field = 0;

        while((token = strsep(&string, ";")) != NULL) {
            switch(field) {
                case 0: // Source
                    source_size = get_codepoints(token, (char*)source, 128);
                    break;

                case 1: // NFC
                    nfc_size = get_codepoints(token, (char*)nfc, 128);
                    break;

                case 2: // NFD
                    nfd_size = get_codepoints(token, (char*)nfd, 128);
                    break;

                case 3: // NFKC
                    nfkc_size = get_codepoints(token, (char*)nfkc, 128);
                    break;

                case 4: // NFKD
                    nfkd_size = get_codepoints(token, (char*)nfkd, 128);
                    break;
            }

            // Skip trailing comments
            if(++field == 5) {
                break;
            }
        }

        free(tofree);

        char *valids[4] = { "OK",  "Normalization failed", "Size mismatch", "Codepoint mismatch" };
        // unsigned int valid0 = check_normalization((char*)source, source_size, (char*)nfc, nfc_size, MJB_NORMALIZATION_NFC);
        unsigned int valid1 = check_normalization((char*)source, source_size, (char*)nfd, nfd_size, MJB_NORMALIZATION_NFD);
        // unsigned int valid2 = check_normalization((char*)source, source_size, (char*)nfkc, nfkc_size, MJB_NORMALIZATION_NFKC);
        // unsigned int valid3 = check_normalization((char*)source, source_size, (char*)nfkd, nfkd_size, MJB_NORMALIZATION_NFKD);

        // snprintf(test_name, 128, "#%u NFC", current_line);
        // ATT_ASSERT(valids[valid0], valids[0], test_name)

        snprintf(test_name, 128, "#%u NFD", current_line);
        ATT_ASSERT(valids[valid1], valids[0], test_name)

        // snprintf(test_name, 128, "#%u NFKC", current_line);
        // ATT_ASSERT(valids[valid2], valids[2], test_name)

        // snprintf(test_name, 128, "#%u NFKD", current_line);
        // ATT_ASSERT(valids[valid3], valids[3], test_name)

        memset((void*)source, 0, 128);
        memset((void*)nfc, 0, 128);
        memset((void*)nfd, 0, 128);
        memset((void*)nfkc, 0, 128);
        memset((void*)nfkd, 0, 128);

        ++current_line;

        if(++limit == 1) {
            break;
        }
    }

    fclose(file);

    return NULL;
}
