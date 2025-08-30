/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>

#include "test.h"

void *test_segmentation(void *arg) {
    ATT_ASSERT(mjb_segmentation("", 0, MJB_ENCODING_UTF_8), true, "Empty string")

    return NULL;
}
