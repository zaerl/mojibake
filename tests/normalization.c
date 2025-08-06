/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "test.h"

bool next_character(mjb_character *character, mjb_next_character_type type) {
    printf(" \x1B[31mU+%04X\x1B[0m", (unsigned int)character->codepoint);

    return true;
}

/**
 * Get codepoints from a string
 * Example: "0044 0307", gives 2 codepoints
 */
size_t get_utf8_string(char *buffer, char *codepoints, size_t size) {
    char *token, *string, *tofree;
    tofree = string = strdup(buffer);
    unsigned int index = 0;

    while((token = strsep(&string, " ")) != NULL) {
        mjb_codepoint codepoint = strtoul((const char*)token, NULL, 16);
        index += mjb_codepoint_encode(codepoint, codepoints + index, size - index, MJB_ENCODING_UTF_8);
    }

    codepoints[++index] = '\0';
    free(tofree);

    return index;
}

int check_normalization(char *source, size_t source_size, char *normalized, size_t normalized_size,
    mjb_normalization form, unsigned int current_line, unsigned int step) {
    size_t normalized_size_res;
    char test_name[128];
    char *names[4] = { "NFC",  "NFD", "NFKC", "NFKD" };

    // CURRENT_ASSERT mjb_normalize
    // CURRENT_COUNT 199650
    char *normalized_res = mjb_normalize(source, source_size, &normalized_size_res, MJB_ENCODING_UTF_8, form);

    if(normalized_res == NULL) {
        snprintf(test_name, 128, "#%u mjb_normalize %s %u", current_line, names[form], step);
        ATT_ASSERT(true, false, test_name)

        if(normalized_res != NULL) {
            mjb_free(normalized_res);
        }

        return 0;
    }

    snprintf(test_name, 128, "#%u %s %u", current_line, names[form], step);

    if(is_exit_on_error()) {
        if(strcmp(normalized_res, normalized) != 0) {
            printf("\n%s: source:", test_name);
            mjb_next_character(source, source_size, MJB_ENCODING_UTF_8, next_character);
            printf("\nExpected: ");
            mjb_next_character(normalized, normalized_size, MJB_ENCODING_UTF_8, next_character);
            printf("\nGot: ");
            mjb_next_character(normalized_res, normalized_size_res, MJB_ENCODING_UTF_8, next_character);
            puts("");
        }
    }

    int ret = ATT_ASSERT(normalized_res, normalized, test_name)

    if(normalized_res != NULL) {
        mjb_free(normalized_res);
    }

    return ret;
}

/**
 * Run utils/generate/UCD/NormalizationTest.txt tests
 */
void run_normalization_tests(int limit) {
    char line[1024];
    unsigned int current_line = 1;
    int count = 0;
    // unsigned int index = 0;

    // 256 characters is enough for any test.
    const char source[256] = { 0 };
    const char nfc[256] = { 0 };
    const char nfd[256] = { 0 };
    const char nfkc[256] = { 0 };
    const char nfkd[256] = { 0 };

    size_t source_size = 0;
    size_t nfc_size = 0;
    size_t nfd_size = 0;
    size_t nfkc_size = 0;
    size_t nfkd_size = 0;

    FILE *file = fopen("./utils/generate/UCD/NormalizationTest.txt", "r");

    if(file == NULL) {
        ATT_ASSERT("Not opened", "Opened file", "Valid normalization test file")

        return;
    }

    if(limit == 0) {
        return;
    }

    // Parse the file
    while(fgets(line, 1024, file)) {
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
                    source_size = get_utf8_string(token, (char*)source, 256);
                    break;

                case 1: // NFC
                    nfc_size = get_utf8_string(token, (char*)nfc, 256);
                    break;

                case 2: // NFD
                    nfd_size = get_utf8_string(token, (char*)nfd, 256);
                    break;

                case 3: // NFKC
                    nfkc_size = get_utf8_string(token, (char*)nfkc, 256);
                    break;

                case 4: // NFKD
                    nfkd_size = get_utf8_string(token, (char*)nfkd, 256);
                    break;
            }

            // Skip trailing comments
            if(++field == 5) {
                break;
            }
        }

        free(tofree);

        /**
         * NFD
         * nfd == toNFD(source) == toNFD(nfc) == toNFD(nfd)
         * nfkd == toNFD(nfkc) == toNFD(nfkd)
         */
        check_normalization((char*)source, source_size, (char*)nfd, nfd_size, MJB_NORMALIZATION_NFD, current_line, 1);
        check_normalization((char*)nfc, nfc_size, (char*)nfd, nfd_size, MJB_NORMALIZATION_NFD, current_line, 2);
        check_normalization((char*)nfd, nfd_size, (char*)nfd, nfd_size, MJB_NORMALIZATION_NFD, current_line, 3);
        check_normalization((char*)nfkc, nfkc_size, (char*)nfkd, nfkd_size, MJB_NORMALIZATION_NFD, current_line, 4);
        check_normalization((char*)nfkd, nfkd_size, (char*)nfkd, nfkd_size, MJB_NORMALIZATION_NFD, current_line, 5);

        /**
         * NFKD
         * nfkd == toNFKD(source) == toNFKD(nfc) == toNFKD(nfd) == toNFKD(nfkc) == toNFKD(nfkd)
         */
        check_normalization((char*)source, source_size, (char*)nfkd, nfkd_size, MJB_NORMALIZATION_NFKD, current_line, 1);
        check_normalization((char*)nfc, nfc_size, (char*)nfkd, nfkd_size, MJB_NORMALIZATION_NFKD, current_line, 2);
        check_normalization((char*)nfd, nfd_size, (char*)nfkd, nfkd_size, MJB_NORMALIZATION_NFKD, current_line, 3);
        check_normalization((char*)nfkc, nfkc_size, (char*)nfkd, nfkd_size, MJB_NORMALIZATION_NFKD, current_line, 4);
        check_normalization((char*)nfkd, nfkd_size, (char*)nfkd, nfkd_size, MJB_NORMALIZATION_NFKD, current_line, 5);

        /**
         * TODO: add NFC normalization
         * nfc == toNFC(source) == toNFC(nfc) == toNFC(nfd)
         * nfkc == toNFC(nfkc) == toNFC(nfkd)
         */
        check_normalization((char*)source, source_size, (char*)nfc, nfc_size, MJB_NORMALIZATION_NFC, current_line, 1);
        check_normalization((char*)nfc, nfc_size, (char*)nfc, nfc_size, MJB_NORMALIZATION_NFC, current_line, 2);
        check_normalization((char*)nfd, nfd_size, (char*)nfc, nfc_size, MJB_NORMALIZATION_NFC, current_line, 3);
        check_normalization((char*)nfkc, nfkc_size, (char*)nfkc, nfkc_size, MJB_NORMALIZATION_NFC, current_line, 4);
        check_normalization((char*)nfkd, nfkd_size, (char*)nfkc, nfkc_size, MJB_NORMALIZATION_NFC, current_line, 5);

        /**
         * TODO: add NFKC normalization
         * nfkc == toNFKC(source) == toNFKC(nfc) == toNFKC(nfd) == toNFKC(nfkc) == toNFKC(nfkd)
         */
        check_normalization((char*)source, source_size, (char*)nfkc, nfkc_size, MJB_NORMALIZATION_NFKC, current_line, 1);
        check_normalization((char*)nfc, nfc_size, (char*)nfkc, nfkc_size, MJB_NORMALIZATION_NFKC, current_line, 2);
        check_normalization((char*)nfd, nfd_size, (char*)nfkc, nfkc_size, MJB_NORMALIZATION_NFKC, current_line, 3);
        check_normalization((char*)nfkc, nfkc_size, (char*)nfkc, nfkc_size, MJB_NORMALIZATION_NFKC, current_line, 4);
        check_normalization((char*)nfkd, nfkd_size, (char*)nfkc, nfkc_size, MJB_NORMALIZATION_NFKC, current_line, 5);

        memset((void*)source, 0, 256);
        memset((void*)nfc, 0, 256);
        memset((void*)nfd, 0, 256);
        memset((void*)nfkc, 0, 256);
        memset((void*)nfkd, 0, 256);

        ++current_line;

        if(limit == -1) {
            continue;
        }

        if(++count == limit) {
            break;
        }
    }

    fclose(file);
}

void *test_normalization(void *arg) {
    run_normalization_tests(-1);

    return NULL;
}
