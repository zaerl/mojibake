/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "../src/mojibake.h"
#include "test.h"

void *test_version(void *arg) {
    ATT_ASSERT(mjb_version(), (const char*)MJB_VERSION, "Valid version");
    ATT_ASSERT(mjb_version_number(), MJB_VERSION_NUMBER, "Valid version number");
    ATT_ASSERT(mjb_unicode_version(), (const char*)MJB_UNICODE_VERSION, "Valid unicode version");

    return NULL;
}
