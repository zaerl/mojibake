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
void *test_breaking(void *arg);
void *test_case(void *arg);
void *test_cjk(void *arg);
void *test_codepoint(void *arg);
void *test_emoji(void *arg);
void *test_encoding(void *arg);
void *test_east_asian_width(void *arg);
void *test_hangul_composition(void *arg);
void *test_hangul(void *arg);
void *test_mojibake(void *arg);
void *test_next(void *arg);
void *test_normalization(void *arg);
void *test_plane(void *arg);
void *test_quick_check(void *arg);
void *test_segmentation(void *arg);
void *test_special_case(void *arg);
void *test_string(void *arg);
void *test_version(void *arg);

#ifdef __cplusplus
void *test_cpp_mojibake(void *arg);
void *test_cpp_normalization(void *arg);
#endif

void set_error_callback(att_test_callback callback);
bool is_exit_on_error(void);

// Utils
size_t get_string_from_codepoints(char *buffer, size_t size, char *codepoints);

#ifdef _WIN32
// Windows-compatible strsep declaration
static char *strsep(char **stringp, const char *delim);
#endif

#ifdef __cplusplus
}
#endif

#endif // MJB_TEST_H
