/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "../../../src/cpp/mojibake.hpp"
#include "../../test.h"

ATT_TEST(cpp_locales) {
    const auto locale = mjb::parse_locale("sr-Latn-RS");
    ATT_ASSERT(std::string(locale.language()), std::string("sr"), "parse_locale language");
    ATT_ASSERT(std::string(locale.script()), std::string("Latn"), "parse_locale script");
    ATT_ASSERT(std::string(locale.region()), std::string("RS"), "parse_locale region");

    bool locale_error = false;

    try {
        (void)mjb::parse_locale("not_a_locale");
    } catch(const mjb::LocaleError &error) {
        locale_error = error.status() == MJB_STATUS_INVALID_ARGUMENT;
    }

    ATT_ASSERT(locale_error, true, "LocaleError preserves status");
    ATT_ASSERT(mjb::uppercase("istanbul", MJB_LOCALE_TR), std::string("\xC4\xB0STANBUL"),
        "uppercase applies the Turkish locale");
    ATT_ASSERT(mjb::uppercase("istanbul"), std::string("ISTANBUL"),
        "uppercase defaults to locale-independent mappings");
    ATT_ASSERT(mjb::casefold("I", MJB_LOCALE_AZ), std::string("\xC4\xB1"),
        "casefold applies Turkic folding for Azerbaijani");
    ATT_ASSERT(mjb::lowercase("I\xCC\x80", MJB_LOCALE_LT), std::string("i\xCC\x87\xCC\x80"),
        "lowercase applies the Lithuanian locale");

    bool invalid_locale = false;

    try {
        (void)mjb::uppercase("i", (mjb_locale)MJB_LOCALE_NUM);
    } catch(const mjb::LibraryError &error) {
        invalid_locale = error.status() == MJB_STATUS_INVALID_ARGUMENT;
    }

    ATT_ASSERT(invalid_locale, true, "uppercase rejects an invalid locale");
}
