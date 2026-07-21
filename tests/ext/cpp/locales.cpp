/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "../../../src/cpp/mojibake.hpp"
#include "../../test.h"

int test_cpp_locales(void *arg) {
    const auto locale = mjb::parse_locale("sr-Latn-RS");
    ATT_ASSERT(std::string(locale.language()), std::string("sr"), "parse_locale language")
    ATT_ASSERT(std::string(locale.script()), std::string("Latn"), "parse_locale script")
    ATT_ASSERT(std::string(locale.region()), std::string("RS"), "parse_locale region")

    bool locale_error = false;

    try {
        (void)mjb::parse_locale("not_a_locale");
    } catch(const mjb::LocaleError &error) {
        locale_error = error.status() == MJB_STATUS_INVALID_ARGUMENT &&
            error.error() == MJB_ERROR_INVALID_ARGUMENT;
    }

    ATT_ASSERT(locale_error, true, "LocaleError preserves locale error")
    mjb::set_locale(MJB_LOCALE_IT);
    ATT_ASSERT((unsigned int)mjb::get_locale(), (unsigned int)MJB_LOCALE_IT,
        "get_locale returns selected locale")
    mjb::set_locale(MJB_LOCALE_EN);

    return 0;
}
