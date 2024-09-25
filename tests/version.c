#include "../src/mojibake.h"
#include "test.h"

void *test_version(void *arg) {
    ATT_ASSERT(strcmp(mjb_version(), MJB_VERSION), 0, "Valid version");
    ATT_ASSERT(mjb_version_number(), MJB_VERSION_NUMBER, "Valid version number");
    ATT_ASSERT(mjb_version_number(), MJB_VERSION_NUMBER, "Valid version number");
    ATT_ASSERT(strcmp(mjb_unicode_version(), MJB_UNICODE_VERSION), 0, "Valid unicode version");

    return NULL;
}
