/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "mojibake-internal.h"

extern mojibake mjb_global;

MJB_EXPORT bool mjb_locale_set(unsigned int locale) {
    if(locale >= MJB_LOCALE_NUM) {
        return false;
    }

    if(!mjb_initialize()) {
        return false;
    }

    mjb_global.locale = (mjb_locale)locale;

    return true;
}
