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

#define SECTIONS_COUNT 8

typedef void (*mjb_test)(void);

void mjb_assert(char *message, bool test);
void mjb_run_test(char *name, mjb_test test);
void mjb_select_section(int section);
unsigned int mjb_valid_count(void);
unsigned int mjb_total_count(void);
const char *mjb_section_name(unsigned int section);
unsigned int mjb_section_valid_count(unsigned int section);
unsigned int mjb_section_total_count(unsigned int section);
clock_t mjb_section_delta(unsigned int section);
void mjb_print_character(mjb_character *character, mjb_codepoint codepoint);

/* Array */
void mjb_array_test(void);

/* Codepoint */
void mjb_codepoint_character_test(void);
void mjb_codepoint_block_test(void);
void mjb_codepoint_is_test(void);
void mjb_codepoint_is_graphic_test(void);
void mjb_codepoint_is_valid_test(void);
void mjb_codepoint_lc_uc_tc_test(void);

/* db */
void mjb_ready_test(void);

/* Encoding */
void mjb_string_encoding_test(void);
void mjb_string_is_ascii_test(void);
void mjb_string_is_utf8_test(void);

/* Memory */
void mjb_memory_test(void);

/* Normalization */
void mjb_codepoint_normalize_test(void);

/* Plane */
void mjb_plane_is_valid_test(void);
void mjb_plane_name_test(void);

/* Version */
void mjb_version_test(void);
void mjb_version_number_test(void);
void mjb_unicode_version_test(void);

#ifdef __cplusplus
}
#endif

#endif /* MJB_TEST_H */
