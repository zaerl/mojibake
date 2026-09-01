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
#include "attractor.h"

#ifdef __cplusplus
extern "C" {
#endif

void set_error_callback(att_test_callback callback);
bool is_exit_on_error(void);

void mjb_test_coverage_clear(void);
void mjb_test_coverage_set(const char *name);

bool mjb_test_allocator_initialize(void);
mjb_status mjb_test_allocator_invalid_status(void);
mjb_status mjb_test_allocator_install_status(void);
void mjb_test_allocator_reset(void);
void mjb_test_allocator_fail_after(size_t fail_after);
size_t mjb_test_allocator_call_count(void);

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
void mjb_test_free(void *output);
void read_test_file(const char *filename, test_file_callback callback);

char *mjb_test_strsep(char **stringp, const char *delim);

#ifdef __cplusplus
}
#endif

#endif // MJB_TEST_H
