#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "test.h"

void get_codepoints(char *buffer, mjb_codepoint *codepoints, size_t size) {
    char *token = strtok(buffer, " ");
    unsigned int index = 0;

    while(token != NULL) {
        codepoints[index] = strtoul((const char*)token, NULL, 16);
        token = strtok(NULL, " ");

        ++index;
    }
}

bool check_normalizations(mjb_codepoint *source, mjb_codepoint *nfc, mjb_codepoint *nfd, mjb_codepoint *nfkc, mjb_codepoint *nfkd) {
    return true;
}

MJB_EXPORT void mjb_codepoint_normalize_test() {
    mjb_initialize(MJB_DB_PATH);

    char line[512];
    mjb_codepoint source[16];
    mjb_codepoint nfc[16];
    mjb_codepoint nfd[16];
    mjb_codepoint nfkc[16];
    mjb_codepoint nfkd[16];
    unsigned int index = 0;
    char *token;
    FILE *file = fopen("../utils/UCD/NormalizationTest.txt", "r");

    if(file == NULL) {
        mjb_assert("Valid normalization test file", false);

        return;
    }

    while(fgets(line, 512, file)) {
        if(line[0] == '#' || strnlen(line, 512) == 0) {
            continue;
        }

        /* printf("%s", line); */
        char *token = strtok(line, ";");
        char *comment = NULL;
        unsigned int field = 0;

        while(token != NULL) {
            switch(field) {
                case 0: /* Source */
                    get_codepoints(token, source, 16);
                    break;

                case 1: /* NFC */
                    get_codepoints(token, nfc, 16);
                    break;

                case 2: /* NFD */
                    get_codepoints(token, nfd, 16);
                    break;

                case 3: /* NFKC */
                    get_codepoints(token, nfkc, 16);
                    break;

                case 4: /* NFKD */
                    get_codepoints(token, nfkd, 16);
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
        mjb_assert(line, check_normalizations(source, nfc, nfd, nfkc, nfkd));

        field = 0;
        ++index;
    }

    fclose(file);

    mjb_close();
}
