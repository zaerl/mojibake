/**
 * The mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

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
void *test_codepoint(void *arg);
void *test_encoding(void *arg);
void *test_hangul(void *arg);
void *test_mojibake(void *arg);
void *test_normalization(void *arg);
void *test_plane(void *arg);
void *test_sort(void *arg);
void *test_utf8(void *arg);
void *test_version(void *arg);

#ifdef __cplusplus
}
#endif

#endif // MJB_TEST_H
