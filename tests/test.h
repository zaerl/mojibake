/**
 * The mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#ifndef MJB_TEST_H
#define MJB_TEST_H

#include "../src/mojibake.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MJB_DB_PATH "../src/mojibake.db"

typedef void (*mjb_test)(void);

void mjb_assert(char *message, bool test);
void mjb_print_character(mjb_character *character, mjb_codepoint codepoint);
void mjb_run_test(char *name, mjb_test test);
unsigned int mjb_valid_count();
unsigned int mjb_total_count();

/* Tests */

void mjb_version_test();
void mjb_version_number_test();
void mjb_unicode_version_test();

void mjb_codepoint_character_test();
void mjb_codepoint_block_test();
void mjb_codepoint_is_test();
void mjb_codepoint_is_graphic_test();
void mjb_codepoint_is_valid_test();
void mjb_codepoint_lc_uc_tc_test();
void mjb_codepoint_normalize_test();

void mjb_plane_is_valid_test();
void mjb_plane_name_test();

void mjb_string_encoding_test();
void mjb_string_is_ascii_test();
void mjb_string_is_utf8_test();

#ifdef __cplusplus
}
#endif

#endif /* MJB_TEST_H */
