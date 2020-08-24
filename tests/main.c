/**
 * The mojibake library tests
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>

#include "test.h"
#include "../src/mojibake.h"

static const char* db_name = "../src/mojibake.db";

static void mjb_ready_test() {
    mjb_assert("Not ready", !mjb_ready());

    bool result = mjb_initialize("null.db");
    mjb_assert("Not valid DB call", !result);
    mjb_assert("Not valid DB", !mjb_ready());

    result = mjb_close();
    mjb_assert("Not valid DB close call", !result);
    mjb_assert("DB closed", !mjb_ready());

    result = mjb_initialize(db_name);
    mjb_assert("Valid DB call", result);
    mjb_assert("Valid DB", mjb_ready());

    result = mjb_close();
    mjb_assert("Valid DB close call", result);
    mjb_assert("DB closed", !mjb_ready());
}

int main(int argc, const char * argv[]) {
    printf("\x1b[36mMojibake %s test\x1B[0m\n\n", mjb_version());

    mjb_run_test("Get version", mjb_version_test);
    mjb_run_test("Get version number", mjb_version_number_test);
    mjb_run_test("Get unicode version", mjb_unicode_version_test);
    mjb_run_test("Codepoint is valid", mjb_codepoint_is_valid_test);
    mjb_run_test("Codespace plane is valid", mjb_plane_is_valid_test);
    mjb_run_test("Codespace plane name", mjb_plane_name_test);
    mjb_run_test("String encoding", mjb_string_encoding_test);
    mjb_run_test("String is ASCII", mjb_string_is_ascii_test);
    mjb_run_test("String is UTF-8", mjb_string_is_utf8_test);

    /* Init tests */
    mjb_run_test("Ready", mjb_ready_test);
    mjb_run_test("Codepoint character", mjb_codepoint_character_test);
    mjb_run_test("Codepoint block", mjb_codepoint_block_test);
    mjb_run_test("Codepoint is", mjb_codepoint_is_test);
    mjb_run_test("Codepoint is graphic", mjb_codepoint_is_graphic_test);
    mjb_run_test("Codepoint is LC/UC/TC", mjb_codepoint_lc_uc_tc_test);
    mjb_run_test("Normalize NFD/NFC/NFKD/NFKC", mjb_codepoint_normalize_test);

    unsigned int valid = mjb_valid_count();
    unsigned int total = mjb_total_count();

    /* Green if valid and red if not */
    const char* colorCode = valid == total ? "\x1B[32m" : "\x1B[31m";

    printf("%sTests valid/run: %d/%d\n\x1B[0m", colorCode, valid, total);

    return total == valid ? 0 : -1;
}
