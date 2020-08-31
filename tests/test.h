/**
 * The mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#ifndef MJB_TEST_H
#define MJB_TEST_H

#include <time.h>

#include "../src/mojibake.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MJB_DB_PATH "../src/mojibake.db"
#define SECTIONS_COUNT 7

typedef void (*mjb_test)(void);

void mjb_assert(char *message, bool test);
void mjb_run_test(char *name, mjb_test test);
void mjb_select_section(int section);
unsigned int mjb_valid_count();
unsigned int mjb_total_count();
const char *mjb_section_name(unsigned int section);
unsigned int mjb_section_valid_count(unsigned int section);
unsigned int mjb_section_total_count(unsigned int section);
clock_t mjb_section_delta(unsigned int section);
void mjb_print_character(mjb_character *character, mjb_codepoint codepoint);

/* Codepoint */
void mjb_codepoint_character_test();
void mjb_codepoint_block_test();
void mjb_codepoint_is_test();
void mjb_codepoint_is_graphic_test();
void mjb_codepoint_is_valid_test();
void mjb_codepoint_lc_uc_tc_test();

/* db */
void mjb_ready_test();

/* Encoding */
void mjb_string_encoding_test();
void mjb_string_is_ascii_test();
void mjb_string_is_utf8_test();

/* Memory */
void mjb_memory_test();

/* Normalization */
void mjb_codepoint_normalize_test();

/* Plane */
void mjb_plane_is_valid_test();
void mjb_plane_name_test();

/* Version */
void mjb_version_test();
void mjb_version_number_test();
void mjb_unicode_version_test();

#ifdef __cplusplus
}
#endif

#endif /* MJB_TEST_H */
