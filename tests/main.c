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

static void mb_ready_test() {
    mb_assert("Not ready", !mb_ready());

    bool result = mb_initialize("null.db");
    mb_assert("Not valid DB call", !result);
    mb_assert("Not valid DB", !mb_ready());

    result = mb_close();
    mb_assert("Not valid DB close call", !result);
    mb_assert("DB closed", !mb_ready());

    result = mb_initialize(db_name);
    mb_assert("Valid DB call", result);
    mb_assert("Valid DB", mb_ready());

    result = mb_close();
    mb_assert("Valid DB close call", result);
    mb_assert("DB closed", !mb_ready());
}

int main(int argc, const char * argv[]) {
    printf("\x1b[36mMojibake %s test\x1B[0m\n\n", mb_version());

    mb_run_test("Get version", mb_version_test);
    mb_run_test("Get version number", mb_version_number_test);
    mb_run_test("Get unicode version", mb_unicode_version_test);
    mb_run_test("Codepoint is valid", mb_codepoint_is_valid_test);
    mb_run_test("Codespace plane is valid", mb_plane_is_valid_test);
    mb_run_test("Codespace plane name", mb_plane_name_test);
    mb_run_test("String encoding", mb_string_encoding_test);
    mb_run_test("String is ASCII", mb_string_is_ascii_test);
    mb_run_test("String is UTF-8", mb_string_is_utf8_test);

    /* Init tests */
    mb_run_test("Ready", mb_ready_test);
    mb_run_test("Codepoint character", mb_codepoint_character_test);
    mb_run_test("Codepoint block", mb_codepoint_block_test);
    mb_run_test("Codepoint is", mb_codepoint_is_test);
    mb_run_test("Codepoint is graphic", mb_codepoint_is_graphic_test);
    mb_run_test("Codepoint is LC/UC/TC", mb_codepoint_lc_uc_tc_test);
    mb_run_test("Normalize NFD/NFC/NFKD/NFKC", mb_codepoint_normalize_test);

    unsigned int valid = mb_valid_count();
    unsigned int total = mb_total_count();

    /* Green if valid and red if not */
    const char* colorCode = valid == total ? "\x1B[32m" : "\x1B[31m";

    printf("%sTests valid/run: %d/%d\n\x1B[0m", colorCode, valid, total);

    return total == valid ? 0 : -1;
}
