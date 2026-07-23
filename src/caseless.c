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
    value->owned = next->transformed || (previous_owned && next->output == previous);
}

static mjb_status mjb_caseless_normalize(mjb_caseless_value *value, mjb_normalization form) {
    mjb_result next;
    mjb_status status = mjb_normalize(value->buffer, value->byte_length, value->encoding, form,
        MJB_ENC_UTF_8, &next);

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
    mjb_status status = mjb_nfkc_casefold(value->buffer, value->byte_length, value->encoding,
        MJB_ENC_UTF_8, &next);

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

MJB_EXPORT mjb_status mjb_caseless_match(const char *s1, size_t s1_byte_length,
    mjb_encoding s1_encoding, const char *s2, size_t s2_byte_length, mjb_encoding s2_encoding,
    mjb_caseless_mode mode, bool *matches) {
    if(matches == NULL) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    *matches = false;

    if(!mjb_caseless_mode_is_valid(mode) || (s1 == NULL && s1_byte_length > 0) ||
        (s2 == NULL && s2_byte_length > 0)) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    // First string.
    mjb_status status = mjb_resolve_input_byte_length(s1, &s1_byte_length, s1_encoding);

    if(status != MJB_STATUS_OK) {
        return status;
    }

    // Second string.
    status = mjb_resolve_input_byte_length(s2, &s2_byte_length, s2_encoding);

    if(status != MJB_STATUS_OK) {
        return status;
    }

    if(s1_byte_length > 0) {
        status = mjb_validate_code_unit_sequence(s1, s1_byte_length, s1_encoding);

        if(status != MJB_STATUS_OK) {
            return status;
        }
    }

    if(s2_byte_length > 0) {
        status = mjb_validate_code_unit_sequence(s2, s2_byte_length, s2_encoding);

        if(status != MJB_STATUS_OK) {
            return status;
        }
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
