/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"

void *test_locales(void *arg) {
    ATT_ASSERT(mjb_locale_set(MJB_LOCALE_IT), true, "Set locale it_IT")
    ATT_ASSERT(mjb_locale_set(MJB_LOCALE_NUM), false, "Set locale to unknown value")

    return NULL;
}
