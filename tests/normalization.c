/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "test.h"
#include "../src/utf8.h"

static bool next_character(mjb_character *character, mjb_next_character_type type) {
    printf(" \x1B[31mU+%04X\x1B[0m", (unsigned int)character->codepoint);

    return true;
}

/**
 * Get codepoints from a string
 * Example: "0044 0307", gives 2 codepoints
 */
static size_t get_utf8_string(char *buffer, char *codepoints, size_t size) {
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

static bool has_only_latin1(char *source, size_t source_size) {
    uint8_t state = MJB_UTF_ACCEPT;
    mjb_codepoint current_codepoint = 0;

    for(size_t i = 0; i < source_size && source[i]; ++i) {
        // Find next codepoint.
        state = mjb_utf8_decode_step(state, source[i], &current_codepoint);

        if(state == MJB_UTF_REJECT) {
            return false;
        }

        if(state != MJB_UTF_ACCEPT) {
            continue;
        }

        if(current_codepoint >= 0x100) {
            return false;
        }
    }

    return true;
}

static int check_normalization(char *source, size_t source_size, char *normalized, size_t normalized_size,
    mjb_normalization form, unsigned int current_line, const char *step) {
    mjb_normalization_result result;
    char test_name[128];

    // CURRENT_ASSERT mjb_normalize
    // CURRENT_COUNT 400680
    bool ret = mjb_normalize(source, source_size, MJB_ENCODING_UTF_8, form, &result);

    if(!ret) {
        snprintf(test_name, 128, "#%u %s", current_line, step);
        ATT_ASSERT(true, false, test_name)

        if(result.output != NULL && result.output != source) {
            mjb_free(result.output);
        }

        return 0;
    }

    snprintf(test_name, 128, "#%u %s", current_line, step);

    if(is_exit_on_error()) {
        if(strcmp(result.output, normalized) != 0) {
            printf("\n%s: source:", test_name);
            mjb_next_character(source, source_size, MJB_ENCODING_UTF_8, next_character);
            printf("\nExpected: ");
            mjb_next_character(normalized, normalized_size, MJB_ENCODING_UTF_8, next_character);
            printf("\nGot: ");
            mjb_next_character(result.output, result.output_size, MJB_ENCODING_UTF_8, next_character);
            puts("");
        }
    }

    int test_ret = ATT_ASSERT(result.output, normalized, test_name)

    if(result.output != NULL && result.output != source) {
        mjb_free(result.output);
    }

    if(!test_ret) {
        return test_ret;
    }

    bool is_ascii = mjb_string_is_ascii(source, source_size);

    // An ASCII string should not be normalized.
    if(is_ascii) {
        snprintf(test_name, 128, "#%u %s is ASCII and not normalized", current_line, step);
        test_ret = ATT_ASSERT(result.normalized, false, test_name)
    }

    if(form == MJB_NORMALIZATION_NFC && has_only_latin1(source, source_size)) {
        snprintf(test_name, 128, "#%u %s \"%s\" is Latin-1 and not normalized", current_line, step, source);
        test_ret = ATT_ASSERT(result.normalized, false, test_name)
    }

    return test_ret;
}

/**
 * Run utils/generate/UCD/NormalizationTest.txt tests
 */
static void run_normalization_tests(int limit) {
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
        if(line[0] == '#' || line[0] == '@' || strnlen(line, 512) <= 1) {
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
        check_normalization((char*)source, source_size, (char*)nfd, nfd_size, MJB_NORMALIZATION_NFD, current_line, "nfd == toNFD(source)");
        check_normalization((char*)nfc, nfc_size, (char*)nfd, nfd_size, MJB_NORMALIZATION_NFD, current_line, "nfd == toNFD(nfc)");
        check_normalization((char*)nfd, nfd_size, (char*)nfd, nfd_size, MJB_NORMALIZATION_NFD, current_line, "nfd == toNFD(nfd)");
        check_normalization((char*)nfkc, nfkc_size, (char*)nfkd, nfkd_size, MJB_NORMALIZATION_NFD, current_line, "nfd == toNFD(nfkc)");
        check_normalization((char*)nfkd, nfkd_size, (char*)nfkd, nfkd_size, MJB_NORMALIZATION_NFD, current_line, "nfd == toNFD(nfkd)");

        /**
         * NFKD
         * nfkd == toNFKD(source) == toNFKD(nfc) == toNFKD(nfd) == toNFKD(nfkc) == toNFKD(nfkd)
         */
        check_normalization((char*)source, source_size, (char*)nfkd, nfkd_size, MJB_NORMALIZATION_NFKD, current_line, "nfkd == toNFKD(source)");
        check_normalization((char*)nfc, nfc_size, (char*)nfkd, nfkd_size, MJB_NORMALIZATION_NFKD, current_line, "nfkd == toNFKD(nfc)");
        check_normalization((char*)nfd, nfd_size, (char*)nfkd, nfkd_size, MJB_NORMALIZATION_NFKD, current_line, "nfkd == toNFKD(nfd)");
        check_normalization((char*)nfkc, nfkc_size, (char*)nfkd, nfkd_size, MJB_NORMALIZATION_NFKD, current_line, "nfkd == toNFKD(nfkc)");
        check_normalization((char*)nfkd, nfkd_size, (char*)nfkd, nfkd_size, MJB_NORMALIZATION_NFKD, current_line, "nfkd == toNFKD(nfkd)");

        /**
         * NFC
         * nfc == toNFC(source) == toNFC(nfc) == toNFC(nfd)
         * nfkc == toNFC(nfkc) == toNFC(nfkd)
         */
        check_normalization((char*)source, source_size, (char*)nfc, nfc_size, MJB_NORMALIZATION_NFC, current_line, "nfc == toNFC(source)");
        check_normalization((char*)nfc, nfc_size, (char*)nfc, nfc_size, MJB_NORMALIZATION_NFC, current_line, "nfc == toNFC(nfc)");
        check_normalization((char*)nfd, nfd_size, (char*)nfc, nfc_size, MJB_NORMALIZATION_NFC, current_line, "nfc == toNFC(nfd)");
        check_normalization((char*)nfkc, nfkc_size, (char*)nfkc, nfkc_size, MJB_NORMALIZATION_NFC, current_line, "nfc == toNFC(nfkc)");
        check_normalization((char*)nfkd, nfkd_size, (char*)nfkc, nfkc_size, MJB_NORMALIZATION_NFC, current_line, "nfc == toNFC(nfkd)");

        /**
         * NFKC
         * nfkc == toNFKC(source) == toNFKC(nfc) == toNFKC(nfd) == toNFKC(nfkc) == toNFKC(nfkd)
         */
        check_normalization((char*)source, source_size, (char*)nfkc, nfkc_size, MJB_NORMALIZATION_NFKC, current_line, "nfkc == toNFKC(source)");
        check_normalization((char*)nfc, nfc_size, (char*)nfkc, nfkc_size, MJB_NORMALIZATION_NFKC, current_line, "nfkc == toNFKC(nfc)");
        check_normalization((char*)nfd, nfd_size, (char*)nfkc, nfkc_size, MJB_NORMALIZATION_NFKC, current_line, "nfkc == toNFKC(nfd)");
        check_normalization((char*)nfkc, nfkc_size, (char*)nfkc, nfkc_size, MJB_NORMALIZATION_NFKC, current_line, "nfkc == toNFKC(nfkc)");
        check_normalization((char*)nfkd, nfkd_size, (char*)nfkc, nfkc_size, MJB_NORMALIZATION_NFKC, current_line, "nfkc == toNFKC(nfkd)");

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
