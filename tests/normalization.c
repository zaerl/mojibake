/**
 * The mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "test.h"

#if 0
#define DEBUG_PRINTF(fmt, ...) printf(fmt, __VA_ARGS__)
#define DEBUG_SNPRINTF(buffer, size, fmt, ...) snprintf(buffer, size, fmt, __VA_ARGS__)
#define DEBUG_PUTS(str) puts(str)
#else
#define DEBUG_PRINTF(fmt, ...)
#define DEBUG_SNPRINTF(buffer, size, fmt, ...)
#define DEBUG_PUTS(str)
#endif

void mjb_test_string_to_hex(const char *input) {
    for(unsigned i = 0; i < strlen(input); ++i) {
        DEBUG_PRINTF("%02X ", (unsigned char)input[i]);
    }

    DEBUG_PUTS("");
}

/**
 * Get codepoints from a string
 * Example: "0044 0307", gives 2 codepoints
 */
size_t get_utf8_string(char *buffer, char *codepoints, size_t size, char *type) {
    char *token, *string, *tofree;
    tofree = string = strdup(buffer);
    unsigned int index = 0;

    DEBUG_PRINTF("get_utf8_string %s (%s)\n", type, buffer);

    while((token = strsep(&string, " ")) != NULL) {
        mjb_codepoint codepoint = strtoul((const char*)token, NULL, 16);
        // DEBUG_PRINTF("(%s) %02X ", token, codepoint);
        // ++index;
        index += mjb_codepoint_encode(codepoint, codepoints + index, size - index, MJB_ENCODING_UTF_8);
    }

    codepoints[++index] = '\0';

    DEBUG_PUTS("\nTo hex");
    mjb_test_string_to_hex(codepoints);
    DEBUG_PUTS("End to hex");

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
int check_normalization(char *source, size_t source_size, char *normalized, size_t normalized_size, mjb_normalization form, unsigned int current_line) {
    size_t normalized_size_res;
    char test_name[128];
    char *names[4] = { "NFC",  "NFD", "NFKC", "NFKD" };

    char *normalized_res = mjb_normalize(source, source_size, &normalized_size_res, MJB_ENCODING_UTF_8, form);

    if(normalized_res == NULL) {
        snprintf(test_name, 128, "#%u mjb_normalize %s", current_line, names[form]);
        ATT_ASSERT(true, false, test_name)

        if(normalized_res != NULL) {
            mjb_free(normalized_res);
        }

        return 0;
    }

    /*if(normalized_size_res != normalized_size) {
        DEBUG_SNPRINTF(test_name, 128, "#%u size %s", current_line, names[form]);
        ATT_ASSERT(normalized_size_res, normalized_size, test_name)

        if(normalized_res != NULL) {
            mjb_free(normalized_res);
        }

        return;
    }*/

    char *normalized_hex = (char*)mjb_alloc(normalized_size * 3);

    for(size_t i = 0; i < normalized_size - 1; ++i) {
        snprintf(normalized_hex + i * 3, 4, "%02X%c", (unsigned char)normalized[i], i == normalized_size - 2 ? '\0' : ' ');
    }

    char *normalized_res_hex = (char*)mjb_alloc(normalized_size_res * 3);

    for(size_t i = 0; i < normalized_size_res; ++i) {
        snprintf(normalized_res_hex + i * 3, 4, "%02X%c", (unsigned char)normalized_res[i], i == normalized_size_res - 1 ? '\0' : ' ');
    }

    snprintf(test_name, 128, "#%u %s", current_line, names[form]);
    int ret = ATT_ASSERT(normalized_res_hex, normalized_hex, test_name)

    if(normalized_res != NULL) {
        mjb_free(normalized_res);
    }

    mjb_free(normalized_res_hex);
    mjb_free(normalized_hex);

    return ret;
}

/**
 * Run utils/UCD/NormalizationTest.txt tests
 */
void run_normalization_tests(int limit) {
    char line[1024];
    unsigned int current_line = 1;
    unsigned int count = 0;
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

    FILE *file = fopen("./utils/UCD/NormalizationTest.txt", "r");

    if(file == NULL) {
        ATT_ASSERT("Not opened", "Opened file", "Valid normalization test file")

        return;
    }

    if(limit == 0) {
        return;
    }

    // Parse the file
    while(fgets(line, 1024, file)) {
        // DEBUG_PUTS("Normalization test");
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
                    source_size = get_utf8_string(token, (char*)source, 128, "Source");
                    break;

                case 1: // NFC
                    nfc_size = get_utf8_string(token, (char*)nfc, 128, "NFC");
                    break;

                case 2: // NFD
                    nfd_size = get_utf8_string(token, (char*)nfd, 128, "NFD");
                    break;

                case 3: // NFKC
                    nfkc_size = get_utf8_string(token, (char*)nfkc, 128, "NFKC");
                    break;

                case 4: // NFKD
                    nfkd_size = get_utf8_string(token, (char*)nfkd, 128, "NFKD");
                    break;
            }

            // Skip trailing comments
            if(++field == 5) {
                break;
            }
        }

        free(tofree);

        // check_normalization((char*)source, source_size, (char*)nfc, nfc_size, MJB_NORMALIZATION_NFC, current_line);
        int ret = check_normalization((char*)source, source_size, (char*)nfd, nfd_size, MJB_NORMALIZATION_NFD, current_line);
        // unsigned int valid2 = check_normalization((char*)source, source_size, (char*)nfkc, nfkc_size, MJB_NORMALIZATION_NFKC);
        // unsigned int valid3 = check_normalization((char*)source, source_size, (char*)nfkd, nfkd_size, MJB_NORMALIZATION_NFKD);

        memset((void*)source, 0, 256);
        memset((void*)nfc, 0, 256);
        memset((void*)nfd, 0, 256);
        memset((void*)nfkc, 0, 256);
        memset((void*)nfkd, 0, 256);

        ++current_line;

        if(!ret) {
            // break;
        }

        if(limit == -1) {
            continue;
        }

        DEBUG_PUTS("--------------------");

        if(++count == limit) {
            break;
        }
    }

    fclose(file);
}

void *test_normalization(void *arg) {
    // 0041 LATIN CAPITAL LETTER A
    // 0300 COMBINING GRAVE ACCENT
    // 41 CC 80
    // ATT_ASSERT(mjb_normalize("\xC3\x80", 1, &normalized_size_res, MJB_ENCODING_UTF_8, MJB_NORMALIZATION_NFD), "A\xCC\x80", "LATIN CAPITAL LETTER A WITH GRAVE");
    // ATT_ASSERT(normalized_size_res, 3, "Normalized size A/B/C")

    run_normalization_tests(-1);

    return NULL;
}
