/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <string.h>

#include "mojibake-internal.h"
#include "utf.h"

typedef struct mjb_caseless_value {
    const char *buffer;
    size_t byte_length;
    mjb_encoding encoding;
    bool owned;
} mjb_caseless_value;

static bool mjb_caseless_mode_is_valid(mjb_caseless_mode mode) {
    return mode == MJB_CASELESS_CANONICAL || mode == MJB_CASELESS_UNNORMALIZED ||
        mode == MJB_CASELESS_COMPATIBILITY || mode == MJB_CASELESS_IDENTIFIER;
}

static void mjb_caseless_value_free(mjb_caseless_value *value) {
    if(value->owned) {
        mjb_free((void *)value->buffer);
    }

    value->buffer = NULL;
    value->byte_length = 0;
    value->owned = false;
}

static void mjb_caseless_value_replace(mjb_caseless_value *value, const mjb_result *next) {
    const char *previous = value->buffer;
    bool previous_owned = value->owned;

    if(previous_owned && next->output != previous) {
        mjb_free((void *)previous);
    }

    value->buffer = next->output;
    value->byte_length = next->output_size;
    value->encoding = MJB_ENC_UTF_8;
    value->owned = next->owned || (previous_owned && next->output == previous);
}

static mjb_status mjb_caseless_normalize(mjb_caseless_value *value, mjb_normalization form) {
    mjb_result next;
    mjb_text text = mjb_text_make(value->buffer, value->byte_length, value->encoding);
    mjb_status status = mjb_normalize(&text, MJB_MALFORMED_STOP, form, MJB_ENC_UTF_8, &next, NULL);

    if(status == MJB_STATUS_OK) {
        mjb_caseless_value_replace(value, &next);
    }

    return status;
}

static mjb_status mjb_caseless_casefold(mjb_caseless_value *value) {
    mjb_result next;
    mjb_status status = mjb_casefold_default(value->buffer, value->byte_length, value->encoding,
        MJB_ENC_UTF_8, &next);

    if(status == MJB_STATUS_OK) {
        mjb_caseless_value_replace(value, &next);
    }

    return status;
}

static mjb_status mjb_caseless_nfkc_casefold(mjb_caseless_value *value) {
    mjb_result next;
    mjb_text text = mjb_text_make(value->buffer, value->byte_length, value->encoding);
    mjb_status status = mjb_nfkc_casefold(&text, MJB_MALFORMED_STOP, MJB_ENC_UTF_8, &next, NULL);

    if(status == MJB_STATUS_OK) {
        mjb_caseless_value_replace(value, &next);
    }

    return status;
}

static mjb_status mjb_caseless_transform(mjb_caseless_value *value, mjb_caseless_mode mode) {
    mjb_status status;

    switch(mode) {
        case MJB_CASELESS_UNNORMALIZED:
            return mjb_caseless_casefold(value);

        case MJB_CASELESS_CANONICAL:
            status = mjb_caseless_normalize(value, MJB_NORMALIZATION_NFD);

            if(status == MJB_STATUS_OK) {
                status = mjb_caseless_casefold(value);
            }

            if(status == MJB_STATUS_OK) {
                status = mjb_caseless_normalize(value, MJB_NORMALIZATION_NFD);
            }

            return status;

        case MJB_CASELESS_COMPATIBILITY:
            status = mjb_caseless_normalize(value, MJB_NORMALIZATION_NFD);

            if(status == MJB_STATUS_OK) {
                status = mjb_caseless_casefold(value);
            }

            if(status == MJB_STATUS_OK) {
                status = mjb_caseless_normalize(value, MJB_NORMALIZATION_NFKD);
            }

            if(status == MJB_STATUS_OK) {
                status = mjb_caseless_casefold(value);
            }

            if(status == MJB_STATUS_OK) {
                status = mjb_caseless_normalize(value, MJB_NORMALIZATION_NFKD);
            }

            return status;

        case MJB_CASELESS_IDENTIFIER:
            status = mjb_caseless_normalize(value, MJB_NORMALIZATION_NFD);

            if(status == MJB_STATUS_OK) {
                status = mjb_caseless_nfkc_casefold(value);
            }

            return status;
    }

    return MJB_STATUS_INVALID_ARGUMENT;
}

static mjb_status mjb_caseless_match_texts(const mjb_text *first, const mjb_text *second,
    mjb_caseless_mode mode, bool *matches) {
    if(matches == NULL) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    *matches = false;

    if(!mjb_caseless_mode_is_valid(mode) || first == NULL || second == NULL ||
        (first->buffer == NULL && first->byte_length > 0) ||
        (second->buffer == NULL && second->byte_length > 0)) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    const char *s1 = first->buffer;
    size_t s1_byte_length = first->byte_length;
    mjb_encoding s1_encoding = first->encoding;
    const char *s2 = second->buffer;
    size_t s2_byte_length = second->byte_length;
    mjb_encoding s2_encoding = second->encoding;

    // First string.
    mjb_status status = mjb_resolve_input_byte_length(s1, &s1_byte_length, s1_encoding);

    if(status != MJB_STATUS_OK) {
        return status;
    }

    status = s1_byte_length == 0 ?
        MJB_STATUS_OK :
        mjb_check_input_encoding_byte_order(s1, s1_byte_length, s1_encoding);

    if(status != MJB_STATUS_OK) {
        return status;
    }

    // Second string.
    status = mjb_resolve_input_byte_length(s2, &s2_byte_length, s2_encoding);

    if(status != MJB_STATUS_OK) {
        return status;
    }

    status = s2_byte_length == 0 ?
        MJB_STATUS_OK :
        mjb_check_input_encoding_byte_order(s2, s2_byte_length, s2_encoding);

    if(status != MJB_STATUS_OK) {
        return status;
    }

    mjb_caseless_value left = { s1, s1_byte_length, s1_encoding, false };
    mjb_caseless_value right = { s2, s2_byte_length, s2_encoding, false };
    status = mjb_caseless_transform(&left, mode);

    if(status == MJB_STATUS_OK) {
        status = mjb_caseless_transform(&right, mode);
    }

    if(status == MJB_STATUS_OK) {
        *matches = left.byte_length == right.byte_length &&
            (left.byte_length == 0 || memcmp(left.buffer, right.buffer, left.byte_length) == 0);
    }

    mjb_caseless_value_free(&right);
    mjb_caseless_value_free(&left);

    return status;
}

MJB_EXPORT mjb_status mjb_caseless_match(const mjb_text *first, const mjb_text *second,
    mjb_malformed_policy malformed_policy, mjb_caseless_mode mode, bool *matches,
    mjb_diagnostic *first_diagnostic, mjb_diagnostic *second_diagnostic) {
    mjb_diagnostic_reset(first_diagnostic);
    mjb_diagnostic_reset(second_diagnostic);

    if(!mjb_malformed_policy_is_valid(malformed_policy)) {
        if(matches != NULL) {
            *matches = false;
        }

        return MJB_STATUS_INVALID_ARGUMENT;
    }

    // Well-formed input is compared directly, malformed input is validated and repaired only then.
    mjb_status status = mjb_caseless_match_texts(first, second, mode, matches);

    if(status != MJB_STATUS_MALFORMED_INPUT) {
        return status;
    }

    mjb_text first_prepared;
    mjb_text second_prepared;
    mjb_result first_sanitized = { NULL, 0, false };
    mjb_result second_sanitized = { NULL, 0, false };
    status = mjb_prepare_text_input(first, malformed_policy, first_diagnostic, &first_prepared,
        &first_sanitized);

    if(status == MJB_STATUS_OK) {
        status = mjb_prepare_text_input(second, malformed_policy, second_diagnostic,
            &second_prepared, &second_sanitized);
    }

    if(status == MJB_STATUS_OK) {
        status = mjb_caseless_match_texts(&first_prepared, &second_prepared, mode, matches);
    }

    mjb_result_free(&first_sanitized);
    mjb_result_free(&second_sanitized);

    return status;
}
