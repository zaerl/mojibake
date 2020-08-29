/**
 * The mojibake library tests
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>

#include "test.h"
#include "../src/mojibake.h"

static const char *db_name = "../src/mojibake.db";

int main(int argc, const char *argv[]) {
    printf("\x1b[36mMojibake %s test\x1B[0m\n\n", mjb_version());

    bool codepoint = true;
    bool db = true;
    bool encoding = true;
    bool normalization = true;
    bool plane = true;
    bool version = true;

    if(argc > 1 && argv[1][0] == '-') {
        size_t length = strlen(argv[1]);
        codepoint = false;
        db = false;
        encoding = false;
        normalization = false;
        plane = false;
        version = false;

        for(int i = 1; i < length; ++i) {
            switch(argv[1][i]) {
                case 'c':
                    codepoint = true;
                    break;

                case 'd':
                    db = true;
                    break;

                case 'e':
                    encoding = true;
                    break;

                case 'n':
                    normalization = true;
                    break;

                case 'p':
                    plane = true;
                    break;

                case 'v':
                    version = true;
                    break;
            }
        }
    }

    /* Codepoint */
    if(codepoint) {
        mjb_select_section(0);
        mjb_run_test("Codepoint character", mjb_codepoint_character_test);
        mjb_run_test("Codepoint block", mjb_codepoint_block_test);
        mjb_run_test("Codepoint is", mjb_codepoint_is_test);
        mjb_run_test("Codepoint is graphic", mjb_codepoint_is_graphic_test);
        mjb_run_test("Codepoint is valid", mjb_codepoint_is_valid_test);
        mjb_run_test("Codepoint is LC/UC/TC", mjb_codepoint_lc_uc_tc_test);
    }

    /* db */
    if(db) {
        mjb_select_section(1);
        mjb_run_test("Ready", mjb_ready_test);
    }

    /* Encoding */
    if(encoding) {
        mjb_select_section(2);
        mjb_run_test("String encoding", mjb_string_encoding_test);
        mjb_run_test("String is ASCII", mjb_string_is_ascii_test);
        mjb_run_test("String is UTF-8", mjb_string_is_utf8_test);
    }

    /* Normalization */
    if(normalization) {
        mjb_select_section(3);
        mjb_run_test("Normalize NFD/NFC/NFKD/NFKC", mjb_codepoint_normalize_test);
    }

    /* Plane */
    if(plane) {
        mjb_select_section(4);
        mjb_run_test("Codespace plane is valid", mjb_plane_is_valid_test);
        mjb_run_test("Codespace plane name", mjb_plane_name_test);
    }

    /* Version */
    if(version) {
        mjb_select_section(5);
        mjb_run_test("Get version", mjb_version_test);
        mjb_run_test("Get version number", mjb_version_number_test);
        mjb_run_test("Get unicode version", mjb_unicode_version_test);
    }

    unsigned int i = 0;
    unsigned int valid;
    unsigned int total;

    for(; i < 6; ++i) {
        valid = mjb_section_valid_count(i);
        total = mjb_section_total_count(i);

        /* Green if valid and red if not */
        const char *colorCode = valid == total ? "\x1B[32m" : "\x1B[31m";

        printf("%s tests valid/run: %s%d/%d\n\x1B[0m", mjb_section_name(i), colorCode, valid, total);
    }

    valid = mjb_valid_count();
    total = mjb_total_count();

    /* Green if valid and red if not */
    const char *colorCode = valid == total ? "\x1B[32m" : "\x1B[31m";

    printf("\nTests valid/run: %s%d/%d\n\x1B[0m", colorCode, valid, total);

    return total == valid ? 0 : -1;
}
