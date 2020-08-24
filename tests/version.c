#include <string.h>

#include "test.h"

MJB_EXPORT void mjb_version_test() {
    char *version = mjb_version();
    size_t size = sizeof(MJB_VERSION);
    int result = strncmp(version, MJB_VERSION, size);

    mjb_assert("Valid version", result == 0);
}

MJB_EXPORT void mjb_version_number_test() {
    unsigned int version_number = mjb_version_number();

    mjb_assert("Valid version number", version_number == MJB_VERSION_NUMBER);
}

MJB_EXPORT void mjb_unicode_version_test() {
    char *version = mjb_unicode_version();
    size_t size = sizeof(MJB_UNICODE_VERSION);
    int result = strncmp(version, MJB_UNICODE_VERSION, size);

    mjb_assert("Valid unicode version", result == 0);
}
