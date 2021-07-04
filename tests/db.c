/**
 * The mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"

MJB_EXPORT void mjb_ready_test() {
    mojibake *mjb;

    mjb_assert("Not ready", !mjb_ready(NULL));

    bool result = mjb_initialize(NULL);
    mjb_assert("Not valid initialize call", !result && mjb != NULL);
    mjb_assert("Not valid initialize", !mjb_ready(mjb));

    result = mjb_initialize(&mjb);
    mjb_assert("Valid initialize call", result);
    mjb_assert("Valid initialize", mjb_ready(mjb));
}
