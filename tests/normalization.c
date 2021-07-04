/**
 * The mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "test.h"

size_t get_codepoints(char *buffer, mjb_codepoint *codepoints, size_t size) {
    char *copy = (char*)malloc(512);
    char *save_ptr;
    strcpy(copy, buffer);

    char *token = strtok_r(copy, " ", &save_ptr);
    unsigned int index = 0;

    while(token != NULL) {
        codepoints[index] = strtoul((const char*)token, NULL, 16);
        token = strtok_r(NULL, " ", &save_ptr);

        ++index;
    }

    free(copy);

    return index;
}

/*
 source, NFC, NFD, NFKC, NFKD
 source, c1, c2, c3, c4

 NFC
 c2 ==  toNFC(c1) ==  toNFC(c2) ==  toNFC(c3)
 c4 ==  toNFC(c4) ==  toNFC(c5)

 NFD
 c3 ==  toNFD(c1) ==  toNFD(c2) ==  toNFD(c3)
 c5 ==  toNFD(c4) ==  toNFD(c5)

 NFKC
 c4 == toNFKC(c1) == toNFKC(c2) == toNFKC(c3) == toNFKC(c4) == toNFKC(c5)

 NFKD
 c5 == toNFKD(c1) == toNFKD(c2) == toNFKD(c3) == toNFKD(c4) == toNFKD(c5)

 [see: NormalizationTest.txt]
*/
unsigned int check_normalization(mojibake *mjb, void *source, size_t source_size, mjb_codepoint *normalized, size_t normalized_size, mjb_normalization form) {
    size_t normalized_size_res;
    void *normalized_res = mjb_normalize(mjb, source, source_size, &normalized_size_res, MJB_ENCODING_UTF_32, form);
    int ret = 0;

    if(normalized_size_res != normalized_size) {
        mjb_free(mjb, normalized_res);

        return 1;
    }

    for(unsigned int i = 0; i < normalized_size; ++i) {
        if(((mjb_codepoint*)normalized)[i] != ((mjb_codepoint*)normalized_res)[i]) {
            ret = 2;
            break;
        }
    }

    if(normalized_res != NULL) {
        mjb_free(mjb, normalized_res);
    }

    return ret;
}

MJB_EXPORT void mjb_codepoint_normalize_test() {
    mojibake *mjb;
    mjb_initialize(&mjb);

    char line[512];
    unsigned int index = 0;
    char *token;

    mjb_codepoint source[16];
    mjb_codepoint nfc[16];
    mjb_codepoint nfd[16];
    mjb_codepoint nfkc[16];
    mjb_codepoint nfkd[16];

    size_t source_size = 0;
    size_t nfc_size = 0;
    size_t nfd_size = 0;
    size_t nfkc_size = 0;
    size_t nfkd_size = 0;

    FILE *file = fopen("../utils/UCD/NormalizationTest.txt", "r");

    if(file == NULL) {
        mjb_assert("Valid normalization test file", false);

        return;
    }

    while(fgets(line, 512, file)) {
        if(line[0] == '#' || line[0] == '@' || strnlen(line, 512) == 0) {
            continue;
        }

        char *token = strtok(line, ";");
        char *comment = NULL;
        unsigned int field = 0;

        while(token != NULL) {
            switch(field) {
                case 0: /* Source */
                    source_size += get_codepoints(token, source, 16);
                    break;

                case 1: /* NFC */
                    nfc_size += get_codepoints(token, nfc, 16);
                    break;

                case 2: /* NFD */
                    nfd_size += get_codepoints(token, nfd, 16);
                    break;

                case 3: /* NFKC */
                    nfkc_size += get_codepoints(token, nfkc, 16);
                    break;

                case 4: /* NFKD */
                    nfkd_size += get_codepoints(token, nfkd, 16);
                    break;

                /* default: * comment
                    puts(&line[token - line]);
                    printf("%s", &token[i == 5 ? 3 : 0]); */
            }

            token = strtok(NULL, ";");
            ++field;
        }

        char *valids[3] = { "OK", "SIZE", "CODE" };
        unsigned valid1 = check_normalization(mjb, source, source_size, nfc, nfc_size, MJB_NORMALIZATION_NFC);
        unsigned valid2 = check_normalization(mjb, source, source_size, nfd, nfd_size, MJB_NORMALIZATION_NFD);
        unsigned valid3 = check_normalization(mjb, source, source_size, nfkc, nfkc_size, MJB_NORMALIZATION_NFKC);
        unsigned valid4 = check_normalization(mjb, source, source_size, nfkd, nfkd_size, MJB_NORMALIZATION_NFKD);

        /* mjb_normalize(source, 16, MJB_NORMALIZATION_NFC) */
        snprintf(line, 512, "Normalization %u %s/%s/%s/%s", index, valids[valid1], valids[valid2], valids[valid3], valids[valid4]);
        mjb_assert(line, valid1 == 0 && valid2 == 0 && valid3 == 0 && valid4 == 0);

        source_size = 0;
        nfc_size = 0;
        nfd_size = 0;
        nfkc_size = 0;
        nfkd_size = 0;

        memset(&source, 0, 16 * sizeof(mjb_codepoint));
        memset(&nfc, 0, 16 * sizeof(mjb_codepoint));
        memset(&nfd, 0, 16 * sizeof(mjb_codepoint));
        memset(&nfkc, 0, 16 * sizeof(mjb_codepoint));
        memset(&nfkd, 0, 16 * sizeof(mjb_codepoint));

        field = 0;
        ++index;
    }

    fclose(file);
}
