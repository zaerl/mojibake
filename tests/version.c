#include <string.h>

#include "test.h"

MB_EXPORT void mb_version_test() {
    char *version = mb_version();
    size_t size = sizeof(MB_VERSION);
    int result = strncmp(version, MB_VERSION, size);

    mb_assert("Valid version", result == 0);
}

MB_EXPORT void mb_version_number_test() {
    unsigned int version_number = mb_version_number();

    mb_assert("Valid version number", version_number == MB_VERSION_NUMBER);
}

MB_EXPORT void mb_unicode_version_test() {
    char *version = mb_unicode_version();
    size_t size = sizeof(MB_UNICODE_VERSION);
    int result = strncmp(version, MB_UNICODE_VERSION, size);

    mb_assert("Valid unicode version", result == 0);
}
