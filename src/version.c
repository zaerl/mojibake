/**
 * The mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "mojibake.h"

/* Output the current library version (MJB_VERSION) */
MJB_EXPORT char *mjb_version(void) {
    return MJB_VERSION;
}

/* Output the current library version number (MJB_VERSION_NUMBER) */
MJB_EXPORT unsigned int mjb_version_number(void) {
    return MJB_VERSION_NUMBER;
}

/* Output the current supported unicode version (MJB_UNICODE_VERSION) */
MJB_EXPORT char *mjb_unicode_version(void) {
    return MJB_UNICODE_VERSION;
}
