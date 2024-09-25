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
void *test_encoding(void *arg);
void *test_plane(void *arg);

#ifdef __cplusplus
}
#endif

#endif /* MJB_TEST_H */
