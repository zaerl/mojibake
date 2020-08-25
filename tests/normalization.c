#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "test.h"

size_t get_codepoints(char *buffer, mjb_codepoint *codepoints, size_t size) {
    char *token = strtok(buffer, " ");
    unsigned int index = 0;

    while(token != NULL) {
        codepoints[index] = strtoul((const char*)token, NULL, 16);
        token = strtok(NULL, " ");

        ++index;
    }

    return index;
}

bool check_normalization(void *source, size_t source_size, mjb_codepoint *normalized, size_t normalized_size, mjb_normalization normalization) {
    void *normalized_res = mjb_normalize(source, source_size, MJB_ENCODING_UTF_32, normalization);
    bool ret = true;

    for(unsigned int i = 0; i < source_size; ++i) {
        if(((mjb_codepoint*)normalized_res)[i] != ((mjb_codepoint*)source)[i]) {
            ret = false;
            break;
        }
    }

    if(normalized_res != NULL) {
        mbj_release(normalized_res);
    }

    return ret;
}

MJB_EXPORT void mjb_codepoint_normalize_test() {
    mjb_initialize(MJB_DB_PATH);

    char line[512];
    unsigned int index = 0;
    char *token;

    mjb_codepoint source[16];
    mjb_codepoint nfc[16];
    mjb_codepoint nfd[16];
    mjb_codepoint nfkc[16];
    mjb_codepoint nfkd[16];

    size_t source_count = 0;
    size_t nfc_count = 0;
    size_t nfd_count = 0;
    size_t nfkc_count = 0;
    size_t nfkd_count = 0;

    FILE *file = fopen("../utils/UCD/NormalizationTest.txt", "r");

    if(file == NULL) {
        mjb_assert("Valid normalization test file", false);
        mjb_close();

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
                    source_count += get_codepoints(token, source, 16);
                    break;

                case 1: /* NFC */
                    nfc_count += get_codepoints(token, nfc, 16);
                    break;

                case 2: /* NFD */
                    nfd_count += get_codepoints(token, nfd, 16);
                    break;

                case 3: /* NFKC */
                    nfkc_count += get_codepoints(token, nfkc, 16);
                    break;

                case 4: /* NFKD */
                    nfkd_count += get_codepoints(token, nfkd, 16);
                    break;

                /* default: * comment
                    puts(&line[token - line]);
                    printf("%s", &token[i == 5 ? 3 : 0]); */
            }

            token = strtok(NULL, ";");
            ++field;
        }

        /* mjb_normalize(source, 16, MJB_NORMALIZATION_NFC) */
        snprintf(line, 512, "Normalization #%u", index);

        bool valid = check_normalization(source, source_count, nfc, nfc_count, MJB_NORMALIZATION_NFC);

        if(valid) {
            valid = check_normalization(source, source_count, nfd, nfd_count, MJB_NORMALIZATION_NFD);
        }

        if(valid) {
            valid = check_normalization(source, source_count, nfkc, nfkc_count, MJB_NORMALIZATION_NFKC);
        }

        if(valid) {
            valid = check_normalization(source, source_count, nfkd, nfkd_count, MJB_NORMALIZATION_NFKD);
        }

        mjb_assert(line, valid);

        source_count = 0;
        nfc_count = 0;
        nfd_count = 0;
        nfkc_count = 0;
        nfkd_count = 0;

        memset(&source, 0, 16 * sizeof(mjb_codepoint));
        memset(&nfc, 0, 16 * sizeof(mjb_codepoint));
        memset(&nfd, 0, 16 * sizeof(mjb_codepoint));
        memset(&nfkc, 0, 16 * sizeof(mjb_codepoint));
        memset(&nfkd, 0, 16 * sizeof(mjb_codepoint));

        field = 0;
        ++index;
    }

    fclose(file);
    mjb_close();
}
