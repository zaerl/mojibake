/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <string.h>

#include "mojibake-internal.h"

extern mojibake mjb_global;

typedef struct mjb_locale_subtag {
    const char *start;
    size_t length;
} mjb_locale_subtag;

typedef enum mjb_locale_case {
    MJB_LOCALE_CASE_LOWER,
    MJB_LOCALE_CASE_UPPER,
    MJB_LOCALE_CASE_TITLE
} mjb_locale_case;

#define MJB_LOCALE_SUBTAG_MATCHES(subtag, literal) \
    mjb_locale_subtag_matches((subtag), (literal), sizeof(literal) - 1)

static char mjb_locale_ascii_lower(char c) {
    if(c >= 'A' && c <= 'Z') {
        return (char)(c + ('a' - 'A'));
    }

    return c;
}

static char mjb_locale_ascii_upper(char c) {
    if(c >= 'a' && c <= 'z') {
        return (char)(c - ('a' - 'A'));
    }

    return c;
}

static bool mjb_locale_ascii_is_alpha(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static bool mjb_locale_ascii_is_digit(char c) {
    return c >= '0' && c <= '9';
}

static bool mjb_locale_subtag_is_alpha(const mjb_locale_subtag *subtag) {
    for(size_t i = 0; i < subtag->length; ++i) {
        if(!mjb_locale_ascii_is_alpha(subtag->start[i])) {
            return false;
        }
    }

    return true;
}

static bool mjb_locale_subtag_is_digit(const mjb_locale_subtag *subtag) {
    for(size_t i = 0; i < subtag->length; ++i) {
        if(!mjb_locale_ascii_is_digit(subtag->start[i])) {
            return false;
        }
    }

    return true;
}

static bool mjb_locale_subtag_is_variant(const mjb_locale_subtag *subtag) {
    if(subtag->length >= 5 && subtag->length <= 8) {
        return true;
    }

    return subtag->length == 4 && mjb_locale_ascii_is_digit(subtag->start[0]);
}

static bool mjb_locale_subtag_is_singleton(const mjb_locale_subtag *subtag) {
    // See https://datatracker.ietf.org/doc/html/rfc5646 "singleton" section.
    return subtag->length == 1 && mjb_locale_ascii_lower(subtag->start[0]) != 'x';
}

static unsigned int mjb_locale_singleton_index(const mjb_locale_subtag *subtag) {
    char c = mjb_locale_ascii_lower(subtag->start[0]);

    if(mjb_locale_ascii_is_digit(c)) {
        return (unsigned int)(c - '0');
    }

    return (unsigned int)(10 + c - 'a');
}

static bool mjb_locale_subtag_matches(const mjb_locale_subtag *subtag, const char *literal,
    size_t literal_size) {
    if(subtag->length != literal_size) {
        return false;
    }

    for(size_t i = 0; i < literal_size; ++i) {
        if(mjb_locale_ascii_lower(subtag->start[i]) != literal[i]) {
            return false;
        }
    }

    return true;
}

static bool mjb_locale_subtag_equal(const mjb_locale_subtag *a, const mjb_locale_subtag *b) {
    if(a->length != b->length) {
        return false;
    }

    for(size_t i = 0; i < a->length; ++i) {
        if(mjb_locale_ascii_lower(a->start[i]) != mjb_locale_ascii_lower(b->start[i])) {
            return false;
        }
    }

    return true;
}

static char mjb_locale_apply_case(char c, mjb_locale_case letter_case, size_t index) {
    if(letter_case == MJB_LOCALE_CASE_UPPER ||
        (letter_case == MJB_LOCALE_CASE_TITLE && index == 0)) {
        return mjb_locale_ascii_upper(c);
    }

    if(letter_case == MJB_LOCALE_CASE_LOWER || letter_case == MJB_LOCALE_CASE_TITLE) {
        return mjb_locale_ascii_lower(c);
    }

    return c;
}

static bool mjb_locale_append_subtag(char *output, size_t output_size,
    const mjb_locale_subtag *subtag, mjb_locale_case letter_case) {
    size_t output_index = 0;

    while(output_index < output_size && output[output_index] != '\0') {
        ++output_index;
    }

    if(output_index >= output_size) {
        return false;
    }

    size_t separator = output_index == 0 ? 0 : 1;

    if(output_index + separator + subtag->length >= output_size) {
        return false;
    }

    if(separator != 0) {
        output[output_index++] = '-';
    }

    for(size_t i = 0; i < subtag->length; ++i) {
        output[output_index++] = mjb_locale_apply_case(subtag->start[i], letter_case, i);
    }

    output[output_index] = '\0';

    return true;
}

static bool mjb_locale_copy_subtag(char *output, size_t output_size,
    const mjb_locale_subtag *subtag, mjb_locale_case letter_case) {
    if(output_size == 0 || subtag->length >= output_size) {
        return false;
    }

    output[0] = '\0';

    return mjb_locale_append_subtag(output, output_size, subtag, letter_case);
}

static bool mjb_locale_copy_subtags(char *output, size_t output_size,
    const mjb_locale_subtag *subtags, size_t start, size_t end, mjb_locale_case letter_case) {
    if(output_size == 0) {
        return false;
    }

    output[0] = '\0';

    for(size_t i = start; i < end; ++i) {
        if(!mjb_locale_append_subtag(output, output_size, &subtags[i], letter_case)) {
            return false;
        }
    }

    return true;
}

// Match grandfathered tags made of a fixed prefix and one suffix.
static bool mjb_locale_is_grandfathered(const mjb_locale_subtag *subtags, size_t count) {
    if(count == 2) {
        // See https://datatracker.ietf.org/doc/html/rfc5646 "grandfathered" section.
        return (
                MJB_LOCALE_SUBTAG_MATCHES(&subtags[0], "i") &&
                (
                    MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "ami") ||
                    MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "bnn") ||
                    MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "default") ||
                    MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "enochian") ||
                    MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "hak") ||
                    MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "klingon") ||
                    MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "lux") ||
                    MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "mingo") ||
                    MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "navajo") ||
                    MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "pwn") ||
                    MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "tao") ||
                    MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "tay") ||
                    MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "tsu")
                )
            ) ||
            (
                MJB_LOCALE_SUBTAG_MATCHES(&subtags[0], "art") &&
                MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "lojban")
            ) ||
            (
                MJB_LOCALE_SUBTAG_MATCHES(&subtags[0], "cel") &&
                MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "gaulish")
            ) ||
            (
                MJB_LOCALE_SUBTAG_MATCHES(&subtags[0], "no") &&
                (
                    MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "bok") ||
                    MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "nyn")
                )
            ) ||
            (
                MJB_LOCALE_SUBTAG_MATCHES(&subtags[0], "zh") &&
                (
                    MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "guoyu") ||
                    MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "hakka") ||
                    MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "min") ||
                    MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "xiang")
                )
            );
    }

    if(count == 3) {
        // Match grandfathered tags that require all three subtags.
        return (
            MJB_LOCALE_SUBTAG_MATCHES(&subtags[0], "en") &&
            MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "gb") &&
            MJB_LOCALE_SUBTAG_MATCHES(&subtags[2], "oed")
        ) ||
        (
            MJB_LOCALE_SUBTAG_MATCHES(&subtags[0], "sgn") &&
            (
                (
                    MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "be") &&
                    (
                        MJB_LOCALE_SUBTAG_MATCHES(&subtags[2], "fr") ||
                        MJB_LOCALE_SUBTAG_MATCHES(&subtags[2], "nl")
                    )
                ) ||
                (
                    MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "ch") &&
                    MJB_LOCALE_SUBTAG_MATCHES(&subtags[2], "de")
                )
            )
        ) ||
        (
            MJB_LOCALE_SUBTAG_MATCHES(&subtags[0], "zh") &&
            MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "min") &&
            MJB_LOCALE_SUBTAG_MATCHES(&subtags[2], "nan")
        );
    }

    return false;
}

static bool mjb_locale_parse_ascii(const char *ascii_id, size_t ascii_size, mjb_locale_id *locale,
    mjb_error *error) {
    mjb_locale_subtag *subtags = NULL;

#define MJB_LOCALE_PARSE_RETURN(value, error_type) \
    do { \
        if(error != NULL) { \
            *error = (error_type); \
        } \
        if(subtags != NULL) { \
            mjb_free(subtags); \
        } \
        return (value); \
    } while(0)

    if(ascii_size == 0) {
        MJB_LOCALE_PARSE_RETURN(false, MJB_ERROR_INVALID_ARGUMENT);
    }

    size_t subtag_capacity = ascii_size / 2 + 1;
    subtags = (mjb_locale_subtag*)mjb_alloc(subtag_capacity * sizeof(*subtags));

    if(subtags == NULL) {
        MJB_LOCALE_PARSE_RETURN(false, MJB_ERROR_INVALID_ARGUMENT);
    }

    const char *subtag_start = NULL;
    size_t subtag_length = 0;
    size_t subtag_count = 0;

    // Split the ASCII tag into hyphen-separated alphanumeric subtags.
    for(size_t i = 0; i < ascii_size; ++i) {
        char c = ascii_id[i];

        if(mjb_locale_ascii_is_alpha(c) || mjb_locale_ascii_is_digit(c)) {
            if(subtag_length == 0) {
                subtag_start = ascii_id + i;
            }

            ++subtag_length;

            if(subtag_length > 8) {
                MJB_LOCALE_PARSE_RETURN(false, MJB_ERROR_INVALID_ARGUMENT);
            }

            continue;
        }

        if(c == '-') {
            if(subtag_length == 0 || subtag_count >= subtag_capacity) {
                MJB_LOCALE_PARSE_RETURN(false, MJB_ERROR_INVALID_ARGUMENT);
            }

            subtags[subtag_count].start = subtag_start;
            subtags[subtag_count].length = subtag_length;
            ++subtag_count;
            subtag_start = NULL;
            subtag_length = 0;
            continue;
        }

        MJB_LOCALE_PARSE_RETURN(false, MJB_ERROR_INVALID_ARGUMENT);
    }

    if(subtag_length == 0 || subtag_count >= subtag_capacity) {
        MJB_LOCALE_PARSE_RETURN(false, MJB_ERROR_INVALID_ARGUMENT);
    }

    subtags[subtag_count].start = subtag_start;
    subtags[subtag_count].length = subtag_length;
    ++subtag_count;

    // Clear the output only after the input has passed lexical validation.
    memset(locale, 0, sizeof(*locale));

    // i-klingon
    // ^ grandfathered tag
    if(mjb_locale_is_grandfathered(subtags, subtag_count)) {
        if(!mjb_locale_copy_subtags(locale->grandfathered, sizeof(locale->grandfathered),
            subtags, 0, subtag_count, MJB_LOCALE_CASE_LOWER)) {
            MJB_LOCALE_PARSE_RETURN(false, MJB_ERROR_INVALID_ARGUMENT);
        }

        MJB_LOCALE_PARSE_RETURN(true, MJB_ERROR_NONE);
    }

    // x-whatever
    // ^ private-use-only tag
    if(MJB_LOCALE_SUBTAG_MATCHES(&subtags[0], "x")) {
        // See https://datatracker.ietf.org/doc/html/rfc5646 "privateuse" section.
        if(subtag_count < 2 || !mjb_locale_copy_subtags(locale->privateuse,
            sizeof(locale->privateuse), subtags, 0, subtag_count, MJB_LOCALE_CASE_LOWER)) {
            MJB_LOCALE_PARSE_RETURN(false, MJB_ERROR_INVALID_ARGUMENT);
        }

        MJB_LOCALE_PARSE_RETURN(true, MJB_ERROR_NONE);
    }

    size_t index = 0;
    const mjb_locale_subtag *language = &subtags[index];

    if(!mjb_locale_subtag_is_alpha(language)) {
        MJB_LOCALE_PARSE_RETURN(false, MJB_ERROR_INVALID_ARGUMENT);
    }

    // language[-script][-region][-variant...][-extension...][-x-private...]
    // ^ primary language, with up to three optional extlang subtags
    if(language->length >= 2 && language->length <= 3) {
        if(!mjb_locale_copy_subtag(locale->language, sizeof(locale->language), language,
            MJB_LOCALE_CASE_LOWER)) {
            MJB_LOCALE_PARSE_RETURN(false, MJB_ERROR_INVALID_ARGUMENT);
        }

        ++index;

        // See https://datatracker.ietf.org/doc/html/rfc5646 "extlang" section.
        for(
            size_t extlang_count = 0;
            extlang_count < 3 && index < subtag_count && subtags[index].length == 3 &&
            mjb_locale_subtag_is_alpha(&subtags[index]);
            ++extlang_count, ++index
        ) {
            if(!mjb_locale_append_subtag(locale->extlang, sizeof(locale->extlang),
                &subtags[index], MJB_LOCALE_CASE_LOWER)) {
                MJB_LOCALE_PARSE_RETURN(false, MJB_ERROR_INVALID_ARGUMENT);
            }
        }
    } else if(language->length >= 4 && language->length <= 8) {
        if(!mjb_locale_copy_subtag(locale->language, sizeof(locale->language), language,
            MJB_LOCALE_CASE_LOWER)) {
            MJB_LOCALE_PARSE_RETURN(false, MJB_ERROR_INVALID_ARGUMENT);
        }

        ++index;
    } else {
        MJB_LOCALE_PARSE_RETURN(false, MJB_ERROR_INVALID_ARGUMENT);
    }

    // language[-script][-region][-variant...][-extension...][-x-private...]
    //          ^ optional script
    if(index < subtag_count && subtags[index].length == 4 &&
        mjb_locale_subtag_is_alpha(&subtags[index])) {
        if(!mjb_locale_copy_subtag(locale->script, sizeof(locale->script), &subtags[index],
            MJB_LOCALE_CASE_TITLE)) {
            MJB_LOCALE_PARSE_RETURN(false, MJB_ERROR_INVALID_ARGUMENT);
        }

        ++index;
    }

    // language[-script][-region][-variant...][-extension...][-x-private...]
    //                   ^ optional region
    if(index < subtag_count &&
        ((subtags[index].length == 2 && mjb_locale_subtag_is_alpha(&subtags[index])) ||
         (subtags[index].length == 3 && mjb_locale_subtag_is_digit(&subtags[index])))) {
        // See https://datatracker.ietf.org/doc/html/rfc5646 "region" section.
        if(!mjb_locale_copy_subtag(locale->region, sizeof(locale->region), &subtags[index],
            MJB_LOCALE_CASE_UPPER)) {
            MJB_LOCALE_PARSE_RETURN(false, MJB_ERROR_INVALID_ARGUMENT);
        }

        ++index;
    }

    size_t variant_start = index;

    // language[-script][-region][-variant...][-extension...][-x-private...]
    //                            ^ optional variants
    while(index < subtag_count && mjb_locale_subtag_is_variant(&subtags[index])) {
        for(size_t i = variant_start; i < index; ++i) {
            if(mjb_locale_subtag_equal(&subtags[i], &subtags[index])) {
                MJB_LOCALE_PARSE_RETURN(false, MJB_ERROR_INVALID_ARGUMENT);
            }
        }

        if(!mjb_locale_append_subtag(locale->variant, sizeof(locale->variant), &subtags[index],
            MJB_LOCALE_CASE_LOWER)) {
            MJB_LOCALE_PARSE_RETURN(false, MJB_ERROR_INVALID_ARGUMENT);
        }

        ++index;
    }

    bool seen_singletons[36] = {false};

    // language[-script][-region][-variant...][-extension...][-x-private...]
    //                                         ^ optional extensions
    while(index < subtag_count && mjb_locale_subtag_is_singleton(&subtags[index])) {
        unsigned int singleton = mjb_locale_singleton_index(&subtags[index]);

        if(seen_singletons[singleton]) {
            MJB_LOCALE_PARSE_RETURN(false, MJB_ERROR_INVALID_ARGUMENT);
        }

        seen_singletons[singleton] = true;

        if(!mjb_locale_append_subtag(locale->extensions, sizeof(locale->extensions),
            &subtags[index], MJB_LOCALE_CASE_LOWER)) {
            MJB_LOCALE_PARSE_RETURN(false, MJB_ERROR_INVALID_ARGUMENT);
        }

        ++index;

        if(index >= subtag_count || subtags[index].length < 2) {
            MJB_LOCALE_PARSE_RETURN(false, MJB_ERROR_INVALID_ARGUMENT);
        }

        while(index < subtag_count && subtags[index].length >= 2) {
            if(!mjb_locale_append_subtag(locale->extensions, sizeof(locale->extensions),
                &subtags[index], MJB_LOCALE_CASE_LOWER)) {
                MJB_LOCALE_PARSE_RETURN(false, MJB_ERROR_INVALID_ARGUMENT);
            }

            ++index;
        }
    }

    // language[-script][-region][-variant...][-extension...][-x-private...]
    //                                                        ^ optional private-use tail
    if(index < subtag_count && MJB_LOCALE_SUBTAG_MATCHES(&subtags[index], "x")) {
        if(index + 1 >= subtag_count ||
            !mjb_locale_copy_subtags(locale->privateuse, sizeof(locale->privateuse), subtags,
                index, subtag_count, MJB_LOCALE_CASE_LOWER)) {
            MJB_LOCALE_PARSE_RETURN(false, MJB_ERROR_INVALID_ARGUMENT);
        }

        index = subtag_count;
    }

    if(index != subtag_count) {
        MJB_LOCALE_PARSE_RETURN(false, MJB_ERROR_INVALID_ARGUMENT);
    }

    MJB_LOCALE_PARSE_RETURN(true, MJB_ERROR_NONE);

#undef MJB_LOCALE_PARSE_RETURN
}

// Parses a BCP 47 language tag and returns the corresponding locale ID.
// language[-script][-region][-variant...][-extension...][-x-private...]
MJB_EXPORT bool mjb_locale_parse(const char *id, size_t size, mjb_encoding encoding,
    mjb_locale_id *locale, mjb_error *error) {
    if(error != NULL) {
        *error = MJB_ERROR_NONE;
    }

    if((id == NULL && size > 0) || locale == NULL) {
        if(error != NULL) {
            *error = MJB_ERROR_INVALID_ARGUMENT;
        }

        return false;
    }

    const char *ascii_id = id;
    size_t ascii_size = size;
    mjb_result converted = {NULL, 0, false};

    // Parse byte-oriented ASCII after validating or converting the input encoding.
    if(encoding == MJB_ENCODING_ASCII) {
        if(!mjb_string_is_ascii(id, size)) {
            if(error != NULL) {
                *error = MJB_ERROR_INVALID_ARGUMENT;
            }

            return false;
        }
    } else if(encoding == MJB_ENCODING_UTF_8 && mjb_string_is_ascii(id, size)) {
        // Already suitable for the byte-oriented locale parser.
    } else if(!mjb_string_convert_encoding(id, size, encoding, MJB_ENCODING_ASCII, &converted)) {
        if(error != NULL) {
            *error = MJB_ERROR_INVALID_ARGUMENT;
        }

        return false;
    } else {
        ascii_id = converted.output;
        ascii_size = converted.output_size;
    }

    bool parsed = mjb_locale_parse_ascii(ascii_id, ascii_size, locale, error);

    if(converted.transformed) {
        mjb_free(converted.output);
    }

    return parsed;
}

MJB_EXPORT bool mjb_locale_canonicalize(const char *id, size_t size, mjb_result *result,
    mjb_error *error) {
    (void)id;
    (void)size;
    (void)result;

    if(error != NULL) {
        *error = MJB_ERROR_UNSUPPORTED;
    }

    return false;
}

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
