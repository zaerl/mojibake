/**
 * The mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#ifndef MB_TEST_H
#define MB_TEST_H

#include "../src/mojibake.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MB_DB_PATH "../src/mojibake.db"

typedef void (*mb_test)(void);

void mb_assert(char *message, bool test);
void mb_print_character(mb_character* character, mb_codepoint codepoint);
void mb_run_test(char *name, mb_test test);
unsigned int mb_valid_count();
unsigned int mb_total_count();

/* Tests */

void mb_version_test();
void mb_version_number_test();
void mb_unicode_version_test();

void mb_codepoint_character_test();
void mb_codepoint_block_test();
void mb_codepoint_is_test();
void mb_codepoint_is_graphic_test();
void mb_codepoint_is_valid_test();
void mb_codepoint_lc_uc_tc_test();
void mb_codepoint_normalize_test();

void mb_plane_is_valid_test();
void mb_plane_name_test();

void mb_string_encoding_test();
void mb_string_is_ascii_test();
void mb_string_is_utf8_test();

#ifdef __cplusplus
}
#endif

#endif /* MB_TEST_H */
