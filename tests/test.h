/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#pragma once

#ifndef MJB_TEST_H
#define MJB_TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/mojibake.h"
#include "./attractor/attractor.h"

#ifdef __cplusplus
extern "C" {
#endif

// Start tests declarations.
int test_bidi(void *arg);
int test_bidi_class(void *arg);
int test_break_line(void *arg);
int test_break_sentence(void *arg);
int test_break_word(void *arg);
int test_case(void *arg);
int test_cjk(void *arg);
int test_codepoint(void *arg);
int test_collation(void *arg);
int test_display(void *arg);
int test_east_asian_width(void *arg);
int test_embedded_null(void *arg);
int test_emoji(void *arg);
int test_encoding(void *arg);
int test_example(void *arg);
int test_filter(void *arg);
int test_hangul_composition(void *arg);
int test_hangul(void *arg);
int test_identifier(void *arg);
int test_locales(void *arg);
int test_mojibake(void *arg);
int test_next(void *arg);
int test_normalization(void *arg);
int test_plane(void *arg);
int test_properties(void *arg);
int test_quick_check(void *arg);
int test_security(void *arg);
int test_segmentation(void *arg);
int test_special_case(void *arg);
int test_string(void *arg);
int test_utf(void *arg);
int test_version(void *arg);

#ifdef __cplusplus
int test_cpp_break(void *arg);
int test_cpp_locales(void *arg);
int test_cpp_mojibake(void *arg);
int test_cpp_normalization(void *arg);
#endif

void set_error_callback(att_test_callback callback);
bool is_exit_on_error(void);

void mjb_test_coverage_clear(void);
void mjb_test_coverage_set(const char *name);

// Set the coverage for those ATT_ASSERT calls that not directly call a mjb_* function.
#define MJB_TEST_COVERAGE(NAME) mjb_test_coverage_set(#NAME)

#define ATT_ASSERT_STATUS(VALUE, EXPECTED, MESSAGE) \
    ATT_ASSERT((unsigned int)(VALUE), (unsigned int)(EXPECTED), MESSAGE)

// Utils
typedef void (*test_file_callback)(const char *buffer, size_t byte_length,
    unsigned int current_line, mjb_break_type *expected_types);

size_t get_string_from_codepoints(char *buffer, size_t byte_length, char *codepoints);
char *run_mjb_map_case(const char *buffer, size_t byte_length, mjb_map_case_type type,
    mjb_encoding encoding);
void read_test_file(const char *filename, test_file_callback callback);

char *mjb_test_strsep(char **stringp, const char *delim);

#ifdef __cplusplus
}
#endif

#endif // MJB_TEST_H
