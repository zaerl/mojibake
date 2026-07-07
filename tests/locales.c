/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"

int test_locales(void *arg) {
    mjb_locale_id locale;
    mjb_error error = MJB_ERROR_NONE;

    ATT_ASSERT_STATUS(mjb_locale_parse(NULL, 1, MJB_ENC_UTF_8, &locale, &error),
        MJB_STATUS_INVALID_ARGUMENT, "Parse locale rejects NULL id")
    ATT_ASSERT((unsigned int)error, (unsigned int)MJB_ERROR_INVALID_ARGUMENT,
        "Parse locale NULL id error")

    ATT_ASSERT_STATUS(mjb_locale_parse(NULL, 1, MJB_ENC_UTF_8, &locale, NULL),
        MJB_STATUS_INVALID_ARGUMENT, "Parse locale handles NULL error output")

    error = MJB_ERROR_NONE;
    ATT_ASSERT_STATUS(mjb_locale_parse("en", 2, MJB_ENC_UTF_8, NULL, &error),
        MJB_STATUS_INVALID_ARGUMENT, "Parse locale rejects NULL locale output")
    ATT_ASSERT((unsigned int)error, (unsigned int)MJB_ERROR_INVALID_ARGUMENT,
        "Parse locale NULL locale output error")

    error = MJB_ERROR_NONE;
    // \xC3\xA9 -> é
    ATT_ASSERT_STATUS(mjb_locale_parse("\xC3\xA9", 2, MJB_ENC_UTF_8, &locale, &error),
        MJB_STATUS_INVALID_ARGUMENT, "Parse locale rejects non-ASCII UTF-8")
    ATT_ASSERT((unsigned int)error, (unsigned int)MJB_ERROR_INVALID_ARGUMENT,
        "Parse locale non-ASCII UTF-8 error")

    error = MJB_ERROR_NONE;
    ATT_ASSERT_STATUS(mjb_locale_parse("en", 2, MJB_ENC_UTF_16LE, &locale, &error),
        MJB_STATUS_INVALID_ARGUMENT, "Parse locale rejects input with wrong encoding")
    ATT_ASSERT((unsigned int)error, (unsigned int)MJB_ERROR_INVALID_ARGUMENT,
        "Parse locale wrong encoding error")

    error = MJB_ERROR_INVALID_ARGUMENT;
    ATT_ASSERT_STATUS(mjb_locale_parse("sr-Latn-RS", 10, MJB_ENC_UTF_8, &locale, &error),
        MJB_STATUS_OK, "Parse language-script-region")
    ATT_ASSERT((unsigned int)error, (unsigned int)MJB_ERROR_NONE,
        "Parse language-script-region error")
    ATT_ASSERT(locale.language, "sr", "Parse language-script-region language")
    ATT_ASSERT(locale.script, "Latn", "Parse language-script-region script")
    ATT_ASSERT(locale.region, "RS", "Parse language-script-region region")

    error = MJB_ERROR_INVALID_ARGUMENT;
    ATT_ASSERT_STATUS(mjb_locale_parse("zh-cmn-Hans-CN", 14, MJB_ENC_UTF_8,
        &locale, &error), MJB_STATUS_OK, "Parse extlang")
    ATT_ASSERT(locale.language, "zh", "Parse extlang language")
    ATT_ASSERT(locale.extlang, "cmn", "Parse extlang value")
    ATT_ASSERT(locale.script, "Hans", "Parse extlang script")
    ATT_ASSERT(locale.region, "CN", "Parse extlang region")

    error = MJB_ERROR_INVALID_ARGUMENT;
    ATT_ASSERT_STATUS(mjb_locale_parse("de-CH-1901", 10, MJB_ENC_UTF_8, &locale,
        &error), MJB_STATUS_OK, "Parse variant")
    ATT_ASSERT(locale.language, "de", "Parse variant language")
    ATT_ASSERT(locale.region, "CH", "Parse variant region")
    ATT_ASSERT(locale.variant, "1901", "Parse variant value")

    error = MJB_ERROR_INVALID_ARGUMENT;
    ATT_ASSERT_STATUS(mjb_locale_parse("en-US-u-islamcal", 16, MJB_ENC_UTF_8,
        &locale, &error), MJB_STATUS_OK, "Parse extension")
    ATT_ASSERT(locale.language, "en", "Parse extension language")
    ATT_ASSERT(locale.region, "US", "Parse extension region")
    ATT_ASSERT(locale.extensions, "u-islamcal", "Parse extension value")

    error = MJB_ERROR_INVALID_ARGUMENT;
    ATT_ASSERT_STATUS(mjb_locale_parse("de-CH-x-phonebk", 15, MJB_ENC_UTF_8,
        &locale, &error), MJB_STATUS_OK, "Parse private use")
    ATT_ASSERT(locale.language, "de", "Parse private use language")
    ATT_ASSERT(locale.region, "CH", "Parse private use region")
    ATT_ASSERT(locale.private_use, "x-phonebk", "Parse private use value")

    error = MJB_ERROR_INVALID_ARGUMENT;
    ATT_ASSERT_STATUS(mjb_locale_parse("i-enochian", 10, MJB_ENC_UTF_8, &locale,
        &error), MJB_STATUS_OK, "Parse grandfathered tag")
    ATT_ASSERT(locale.grandfathered, "i-enochian", "Parse grandfathered value")

    error = MJB_ERROR_INVALID_ARGUMENT;
    ATT_ASSERT_STATUS(mjb_locale_parse("x-whatever", 10, MJB_ENC_UTF_8, &locale,
        &error), MJB_STATUS_OK, "Parse private use only")
    ATT_ASSERT(locale.private_use, "x-whatever", "Parse private use only value")

    const char utf16le_locale[] = { 'e', '\0', 'n', '\0', '-', '\0', 'U', '\0', 'S', '\0' };
    error = MJB_ERROR_INVALID_ARGUMENT;
    ATT_ASSERT_STATUS(mjb_locale_parse(utf16le_locale, sizeof(utf16le_locale),
        MJB_ENC_UTF_16LE, &locale, &error), MJB_STATUS_OK, "Parse UTF-16LE locale")
    ATT_ASSERT(locale.language, "en", "Parse UTF-16LE locale language")
    ATT_ASSERT(locale.region, "US", "Parse UTF-16LE locale region")

    // Language tags listed in RFC 5646 Appendix A:
    // https://datatracker.ietf.org/doc/html/rfc5646#appendix-A
    static const char *rfc5646_appendix_a_valid_tags[] = {
        // Simple language subtag:
        "de", // German
        "fr", // French
        "ja", // Japanese
        "i-enochian", // (example of a grandfathered tag

        // Language subtag plus Script subtag:
        "zh-Hant", // Chinese written using the Traditional Chinese script
        "zh-Hans", // Chinese written using the Simplified Chinese script
        "sr-Cyrl", // Serbian written using the Cyrillic script
        "sr-Latn", // Serbian written using the Latin script

        // Extended language subtags and their primary language subtag counterparts:
        "zh-cmn-Hans-CN", // Chinese, Mandarin, Simplified script, as used in China
        "cmn-Hans-CN", // Mandarin Chinese, Simplified script, as used in China
        "zh-yue-HK", // Chinese, Cantonese, as used in Hong Kong SAR
        "yue-HK", // Cantonese Chinese, as used in Hong Kong SAR

        // Language-Script-Region:
        "zh-Hans-CN", // Chinese written using the Simplified script as used in mainland China
        "sr-Latn-RS", // Serbian written using the Latin script as used in Serbia

        // Language-Variant:
        "sl-rozaj", // Resian dialect of Slovenian
        "sl-rozaj-biske", // San Giorgio dialect of Resian dialect of Slovenian
        "sl-nedis", // Nadiza dialect of Slovenian

        // Language-Region-Variant:
        "de-CH-1901", // German as used in Switzerland using the 1901 variant [orthography]
        "sl-IT-nedis", // lovenian as used in Italy, Nadiza dialect

        // Language-Script-Region-Variant:
        "hy-Latn-IT-arevela", // Eastern Armenian written in Latin script, as used in Italy

        // Language-Region:
        "de-DE", // German for Germany
        "en-US", // English as used in the United States
        "es-419", // Spanish appropriate for the Latin America and Caribbean region using the UN
        // region code

        // Private use subtags:
        "de-CH-x-phonebk",
        "az-Arab-x-AZE-derbend",

        // Private use registry values:
        "x-whatever", // private use using the singleton 'x'
        "qaa-Qaaa-QM-x-southern", // all private tags
        "de-Qaaa", // German, with a private script
        "sr-Latn-QM", // Serbian, Latin script, private region
        "sr-Qaaa-RS", // Serbian, private script, for Serbia

        // Tags that use extensions (examples ONLY -- extensions MUST be defined by revision or
        // update to this document, or by RFC):
        "en-US-u-islamcal",
        "zh-CN-a-myext-x-private",
        "en-a-myext-b-another"
    };

    for(
        size_t i = 0;
        i < sizeof(rfc5646_appendix_a_valid_tags) / sizeof(rfc5646_appendix_a_valid_tags[0]);
        ++i
    ) {
        char description[128];
        const char *tag = rfc5646_appendix_a_valid_tags[i];

        error = MJB_ERROR_INVALID_ARGUMENT;
        snprintf(description, sizeof(description), "Parse RFC 5646 Appendix A valid tag %s", tag);
        ATT_ASSERT_STATUS(mjb_locale_parse(tag, strlen(tag), MJB_ENC_UTF_8, &locale,
            &error), MJB_STATUS_OK, description)

        snprintf(description, sizeof(description), "Parse RFC 5646 Appendix A valid tag %s error",
            tag);
        ATT_ASSERT((unsigned int)error, (unsigned int)MJB_ERROR_NONE, description)
    }

    // Invalid language tags listed in RFC 5646 Appendix A:
    // https://datatracker.ietf.org/doc/html/rfc5646#appendix-A
    static const char *rfc5646_appendix_a_invalid_tags[] = {
        "de-419-DE", // two region tags
        "a-DE", // use of a single-character subtag in primary position; note that there are a few
        // grandfathered tags that start with "i-" that are valid
        "ar-a-aaa-b-bbb-a-ccc", // two extensions with same single-letter prefix
    };

    for(
        size_t i = 0;
        i < sizeof(rfc5646_appendix_a_invalid_tags) / sizeof(rfc5646_appendix_a_invalid_tags[0]);
        ++i
    ) {
        char description[128];
        const char *tag = rfc5646_appendix_a_invalid_tags[i];

        error = MJB_ERROR_NONE;
        snprintf(description, sizeof(description), "Parse RFC 5646 Appendix A invalid tag %s",
            tag);
        ATT_ASSERT_STATUS(mjb_locale_parse(tag, strlen(tag), MJB_ENC_UTF_8, &locale,
            &error), MJB_STATUS_INVALID_ARGUMENT, description)

        snprintf(description, sizeof(description), "Parse RFC 5646 Appendix A invalid tag %s error",
            tag);
        ATT_ASSERT((unsigned int)error, (unsigned int)MJB_ERROR_INVALID_ARGUMENT, description)
    }

    error = MJB_ERROR_NONE;
    ATT_ASSERT_STATUS(mjb_locale_parse("de-419-DE", 9, MJB_ENC_UTF_8, &locale,
        &error), MJB_STATUS_INVALID_ARGUMENT, "Parse locale rejects duplicate region")
    ATT_ASSERT((unsigned int)error, (unsigned int)MJB_ERROR_INVALID_ARGUMENT,
        "Parse locale duplicate region error")

    error = MJB_ERROR_NONE;
    ATT_ASSERT_STATUS(mjb_locale_parse("a-DE", 4, MJB_ENC_UTF_8, &locale, &error),
        MJB_STATUS_INVALID_ARGUMENT, "Parse locale rejects one-character language")
    ATT_ASSERT((unsigned int)error, (unsigned int)MJB_ERROR_INVALID_ARGUMENT,
        "Parse locale one-character language error")

    error = MJB_ERROR_NONE;
    ATT_ASSERT_STATUS(mjb_locale_parse("ar-a-aaa-b-bbb-a-ccc", 20, MJB_ENC_UTF_8, &locale,
        &error), MJB_STATUS_INVALID_ARGUMENT, "Parse locale rejects duplicate extension")
    ATT_ASSERT((unsigned int)error, (unsigned int)MJB_ERROR_INVALID_ARGUMENT,
        "Parse locale duplicate extension error")

    error = MJB_ERROR_NONE;
    ATT_ASSERT_STATUS(mjb_locale_parse("de-1901-1901", 12, MJB_ENC_UTF_8, &locale,
        &error), MJB_STATUS_INVALID_ARGUMENT, "Parse locale rejects duplicate variant")
    ATT_ASSERT((unsigned int)error, (unsigned int)MJB_ERROR_INVALID_ARGUMENT,
        "Parse locale duplicate variant error")

    error = MJB_ERROR_NONE;
    ATT_ASSERT_STATUS(mjb_locale_parse("en-abcdefghi", 12, MJB_ENC_UTF_8, &locale,
        &error), MJB_STATUS_INVALID_ARGUMENT, "Parse locale rejects long subtag")
    ATT_ASSERT((unsigned int)error, (unsigned int)MJB_ERROR_INVALID_ARGUMENT,
        "Parse locale long subtag error")

    ATT_ASSERT_STATUS(mjb_locale_set(MJB_LOCALE_IT), MJB_STATUS_OK, "Set locale it_IT")
    ATT_ASSERT_STATUS(mjb_locale_set(MJB_LOCALE_NUM), MJB_STATUS_INVALID_ARGUMENT,
        "Set locale to unknown value")

    return 0;
}
