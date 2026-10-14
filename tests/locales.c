/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"

ATT_TEST(locales) {
    mjb_locale_id locale;

    ATT_ASSERT_STATUS(mjb_locale_parse(NULL, &locale), MJB_STATUS_INVALID_ARGUMENT,
        "Parse locale rejects NULL text");

    mjb_text text = mjb_text_utf8(NULL, 1);
    ATT_ASSERT_STATUS(mjb_locale_parse(&text, &locale),
        MJB_STATUS_INVALID_ARGUMENT, "Parse locale rejects NULL id");

    text = mjb_text_utf8("en", 2);
    ATT_ASSERT_STATUS(mjb_locale_parse(&text, NULL),
        MJB_STATUS_INVALID_ARGUMENT, "Parse locale rejects NULL locale output");

    // \xC3\xA9 -> é
    text = mjb_text_utf8("\xC3\xA9", 2);
    ATT_ASSERT_STATUS(mjb_locale_parse(&text, &locale),
        MJB_STATUS_INVALID_ARGUMENT, "Parse locale rejects non-ASCII UTF-8");

    text = mjb_text_make("en", 2, MJB_ENC_UTF_16LE);
    ATT_ASSERT_STATUS(mjb_locale_parse(&text, &locale),
        MJB_STATUS_INVALID_ARGUMENT, "Parse locale rejects input with wrong encoding");

    text = mjb_text_utf8("sr-Latn-RS", 10);
    ATT_ASSERT_STATUS(mjb_locale_parse(&text, &locale),
        MJB_STATUS_OK, "Parse language-script-region");
    ATT_ASSERT(locale.language, "sr", "Parse language-script-region language");
    ATT_ASSERT(locale.script, "Latn", "Parse language-script-region script");
    ATT_ASSERT(locale.region, "RS", "Parse language-script-region region");
    ATT_ASSERT((unsigned int)locale.locale, (unsigned int)MJB_LOCALE_SR,
        "Parse language-script-region locale");
    ATT_ASSERT(locale.code, "sr", "Parse language-script-region code");

    text = mjb_text_utf8("zh-cmn-Hans-CN", 14);
    ATT_ASSERT_STATUS(mjb_locale_parse(&text, &locale),
        MJB_STATUS_OK, "Parse extlang");
    ATT_ASSERT(locale.language, "zh", "Parse extlang language");
    ATT_ASSERT(locale.extlang, "cmn", "Parse extlang value");
    ATT_ASSERT(locale.script, "Hans", "Parse extlang script");
    ATT_ASSERT(locale.region, "CN", "Parse extlang region");

    text = mjb_text_utf8("de-CH-1901", 10);
    ATT_ASSERT_STATUS(mjb_locale_parse(&text, &locale),
        MJB_STATUS_OK, "Parse variant");
    ATT_ASSERT(locale.language, "de", "Parse variant language");
    ATT_ASSERT(locale.region, "CH", "Parse variant region");
    ATT_ASSERT(locale.variant, "1901", "Parse variant value");

    text = mjb_text_utf8("en-US-u-islamcal", 16);
    ATT_ASSERT_STATUS(mjb_locale_parse(&text, &locale),
        MJB_STATUS_OK, "Parse extension");
    ATT_ASSERT(locale.language, "en", "Parse extension language");
    ATT_ASSERT(locale.region, "US", "Parse extension region");
    ATT_ASSERT(locale.extensions, "u-islamcal", "Parse extension value");

    text = mjb_text_utf8("de-CH-x-phonebk", 15);
    ATT_ASSERT_STATUS(mjb_locale_parse(&text, &locale),
        MJB_STATUS_OK, "Parse private use");
    ATT_ASSERT(locale.language, "de", "Parse private use language");
    ATT_ASSERT(locale.region, "CH", "Parse private use region");
    ATT_ASSERT(locale.private_use, "x-phonebk", "Parse private use value");

    text = mjb_text_utf8("i-enochian", 10);
    ATT_ASSERT_STATUS(mjb_locale_parse(&text, &locale),
        MJB_STATUS_OK, "Parse grandfathered tag");
    ATT_ASSERT(locale.grandfathered, "i-enochian", "Parse grandfathered value");

    text = mjb_text_utf8("x-whatever", 10);
    ATT_ASSERT_STATUS(mjb_locale_parse(&text, &locale),
        MJB_STATUS_OK, "Parse private use only");
    ATT_ASSERT(locale.private_use, "x-whatever", "Parse private use only value");
    ATT_ASSERT((unsigned int)locale.locale, (unsigned int)MJB_LOCALE_NOT_SET,
        "Parse private use only has no locale");
    ATT_ASSERT(locale.code, "", "Parse private use only has no code");

    text = mjb_text_utf8("cmn-Hans-CN", 11);
    ATT_ASSERT_STATUS(mjb_locale_parse(&text, &locale), MJB_STATUS_OK,
        "Parse three-letter language");
    ATT_ASSERT((unsigned int)locale.locale, (unsigned int)MJB_LOCALE_NOT_SET,
        "Parse three-letter language has no locale");
    ATT_ASSERT(locale.code, "", "Parse three-letter language has no code");

    text = mjb_text_utf8("TR-tr", 5);
    ATT_ASSERT_STATUS(mjb_locale_parse(&text, &locale), MJB_STATUS_OK,
        "Parse uppercase language");
    ATT_ASSERT((unsigned int)locale.locale, (unsigned int)MJB_LOCALE_TR,
        "Parse uppercase language locale");
    ATT_ASSERT(locale.code, "tr", "Parse uppercase language code");

    const char utf16le_locale[] = { 'e', '\0', 'n', '\0', '-', '\0', 'U', '\0', 'S', '\0' };
    text = mjb_text_make(utf16le_locale, sizeof(utf16le_locale), MJB_ENC_UTF_16LE);
    ATT_ASSERT_STATUS(mjb_locale_parse(&text, &locale),
        MJB_STATUS_OK, "Parse UTF-16LE locale");
    ATT_ASSERT(locale.language, "en", "Parse UTF-16LE locale language");
    ATT_ASSERT(locale.region, "US", "Parse UTF-16LE locale region");

    // Language tags listed in RFC 5646 Appendix A:
    // https://datatracker.ietf.org/doc/html/rfc5646#appendix-A
    static const char *rfc5646_appendix_a_valid_tags[] = { // Simple language subtag:
        "de",                                              // German
        "fr",                                              // French
        "ja",                                              // Japanese
        "i-enochian",                                      // (example of a grandfathered tag

        // Language subtag plus Script subtag:
        "zh-Hant", // Chinese written using the Traditional Chinese script
        "zh-Hans", // Chinese written using the Simplified Chinese script
        "sr-Cyrl", // Serbian written using the Cyrillic script
        "sr-Latn", // Serbian written using the Latin script

        // Extended language subtags and their primary language subtag counterparts:
        "zh-cmn-Hans-CN", // Chinese, Mandarin, Simplified script, as used in China
        "cmn-Hans-CN",    // Mandarin Chinese, Simplified script, as used in China
        "zh-yue-HK",      // Chinese, Cantonese, as used in Hong Kong SAR
        "yue-HK",         // Cantonese Chinese, as used in Hong Kong SAR

        // Language-Script-Region:
        "zh-Hans-CN", // Chinese written using the Simplified script as used in mainland China
        "sr-Latn-RS", // Serbian written using the Latin script as used in Serbia

        // Language-Variant:
        "sl-rozaj",       // Resian dialect of Slovenian
        "sl-rozaj-biske", // San Giorgio dialect of Resian dialect of Slovenian
        "sl-nedis",       // Nadiza dialect of Slovenian

        // Language-Region-Variant:
        "de-CH-1901",  // German as used in Switzerland using the 1901 variant [orthography]
        "sl-IT-nedis", // lovenian as used in Italy, Nadiza dialect

        // Language-Script-Region-Variant:
        "hy-Latn-IT-arevela", // Eastern Armenian written in Latin script, as used in Italy

        // Language-Region:
        "de-DE",  // German for Germany
        "en-US",  // English as used in the United States
        "es-419", // Spanish appropriate for the Latin America and Caribbean region using the UN
        // region code

        // Private use subtags:
        "de-CH-x-phonebk", "az-Arab-x-AZE-derbend",

        // Private use registry values:
        "x-whatever",             // private use using the singleton 'x'
        "qaa-Qaaa-QM-x-southern", // all private tags
        "de-Qaaa",                // German, with a private script
        "sr-Latn-QM",             // Serbian, Latin script, private region
        "sr-Qaaa-RS",             // Serbian, private script, for Serbia

        // Tags that use extensions (examples ONLY -- extensions MUST be defined by revision or
        // update to this document, or by RFC):
        "en-US-u-islamcal", "zh-CN-a-myext-x-private", "en-a-myext-b-another"
    };

    for(size_t i = 0;
        i < sizeof(rfc5646_appendix_a_valid_tags) / sizeof(rfc5646_appendix_a_valid_tags[0]); ++i) {
        char description[128];
        const char *tag = rfc5646_appendix_a_valid_tags[i];

        snprintf(description, sizeof(description), "Parse RFC 5646 Appendix A valid tag %s", tag);
        text = mjb_text_utf8(tag, strlen(tag));
        ATT_ASSERT_STATUS(mjb_locale_parse(&text, &locale),
            MJB_STATUS_OK, description);
    }

    // Invalid language tags listed in RFC 5646 Appendix A:
    // https://datatracker.ietf.org/doc/html/rfc5646#appendix-A
    static const char *rfc5646_appendix_a_invalid_tags[] = {
        "de-419-DE", // two region tags
        "a-DE", // use of a single-character subtag in primary position; note that there are a few
        // grandfathered tags that start with "i-" that are valid
        "ar-a-aaa-b-bbb-a-ccc", // two extensions with same single-letter prefix
    };

    for(size_t i = 0;
        i < sizeof(rfc5646_appendix_a_invalid_tags) / sizeof(rfc5646_appendix_a_invalid_tags[0]);
        ++i) {
        char description[128];
        const char *tag = rfc5646_appendix_a_invalid_tags[i];

        snprintf(description, sizeof(description), "Parse RFC 5646 Appendix A invalid tag %s", tag);
        text = mjb_text_utf8(tag, strlen(tag));
        ATT_ASSERT_STATUS(mjb_locale_parse(&text, &locale),
            MJB_STATUS_INVALID_ARGUMENT, description);
    }

    text = mjb_text_utf8("de-419-DE", 9);
    ATT_ASSERT_STATUS(mjb_locale_parse(&text, &locale),
        MJB_STATUS_INVALID_ARGUMENT, "Parse locale rejects duplicate region");

    text = mjb_text_utf8("a-DE", 4);
    ATT_ASSERT_STATUS(mjb_locale_parse(&text, &locale),
        MJB_STATUS_INVALID_ARGUMENT, "Parse locale rejects one-character language");

    text = mjb_text_utf8("ar-a-aaa-b-bbb-a-ccc", 20);
    ATT_ASSERT_STATUS(mjb_locale_parse(&text, &locale),
        MJB_STATUS_INVALID_ARGUMENT, "Parse locale rejects duplicate extension");

    text = mjb_text_utf8("de-1901-1901", 12);
    ATT_ASSERT_STATUS(mjb_locale_parse(&text, &locale),
        MJB_STATUS_INVALID_ARGUMENT, "Parse locale rejects duplicate variant");

    text = mjb_text_utf8("en-abcdefghi", 12);
    ATT_ASSERT_STATUS(mjb_locale_parse(&text, &locale),
        MJB_STATUS_INVALID_ARGUMENT, "Parse locale rejects long subtag");

    ATT_ASSERT_STATUS(mjb_set_locale(MJB_LOCALE_IT), MJB_STATUS_OK, "Set locale it_IT");
    ATT_ASSERT((unsigned int)mjb_get_locale(), (unsigned int)MJB_LOCALE_IT, "Get locale it_IT");
    ATT_ASSERT_STATUS(mjb_set_locale((mjb_locale)MJB_LOCALE_NUM), MJB_STATUS_INVALID_ARGUMENT,
        "Set locale to unknown value");
    ATT_ASSERT_STATUS(mjb_set_locale(MJB_LOCALE_NOT_SET), MJB_STATUS_INVALID_ARGUMENT,
        "Set locale to not set");
    ATT_ASSERT((unsigned int)mjb_get_locale(), (unsigned int)MJB_LOCALE_IT,
        "Not set locale does not change current locale");
    ATT_ASSERT((unsigned int)mjb_get_locale(), (unsigned int)MJB_LOCALE_IT,
        "Invalid locale does not change current locale");
    ATT_ASSERT_STATUS(mjb_set_locale((mjb_locale)-1), MJB_STATUS_INVALID_ARGUMENT,
        "Set locale to negative value");
    ATT_ASSERT((unsigned int)mjb_get_locale(), (unsigned int)MJB_LOCALE_IT,
        "Negative locale does not change current locale");
    ATT_ASSERT((mjb_reset_locale(), true), true, "Reset locale");
    ATT_ASSERT((unsigned int)mjb_get_locale(), (unsigned int)MJB_LOCALE_EN,
        "Reset locale restores English");
    ATT_ASSERT((mjb_reset_locale(), true), true, "Reset locale is idempotent");

    ATT_ASSERT(mjb_locale_code(MJB_LOCALE_TR), "tr", "Locale code tr");
    ATT_ASSERT(mjb_locale_code(MJB_LOCALE_ZU), "zu", "Locale code zu");
    ATT_ASSERT(mjb_locale_code(MJB_LOCALE_NOT_SET), "", "Locale code not set");
    ATT_ASSERT(mjb_locale_code((mjb_locale)MJB_LOCALE_NUM) == NULL, true, "Locale code invalid");
    ATT_ASSERT(mjb_locale_code((mjb_locale)-1) == NULL, true, "Locale code negative");

    ATT_ASSERT(mjb_script_code(MJB_SC_LATN), "Latn", "Script code Latn");
    ATT_ASSERT(mjb_script_code(MJB_SC_KORE), "Kore", "Script code Kore");
    ATT_ASSERT(mjb_script_code(MJB_SC_NOT_SET), "", "Script code not set");
    ATT_ASSERT(mjb_script_code((mjb_script)MJB_SC_COUNT) == NULL, true, "Script code invalid");
}
