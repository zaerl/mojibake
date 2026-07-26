/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <string.h>

#include "mojibake-internal.h"
#include "punycode.h"
#include "unicode-tables.h"
#include "utf.h"

#if MJB_FEATURE_IDNA
typedef struct mjb_idna_codepoints {
    mjb_codepoint *values;
    size_t count;
} mjb_idna_codepoints;

typedef enum mjb_idna_joining_type {
    MJB_IDNA_JOIN_CAUSING = 1,
    MJB_IDNA_DUAL_JOINING,
    MJB_IDNA_LEFT_JOINING,
    MJB_IDNA_RIGHT_JOINING,
    MJB_IDNA_TRANSPARENT
} mjb_idna_joining_type;

static void mjb_idna_codepoints_free(mjb_idna_codepoints *codepoints) {
    mjb_free(codepoints->values);
    codepoints->values = NULL;
    codepoints->count = 0;
}

static mjb_status mjb_idna_decode_utf8(const char *buffer, size_t byte_length,
    mjb_idna_codepoints *codepoints) {
    codepoints->values = NULL;
    codepoints->count = 0;

    if(byte_length == 0) {
        return MJB_STATUS_OK;
    }

    if(byte_length > SIZE_MAX / sizeof(mjb_codepoint)) {
        return MJB_STATUS_OVERFLOW;
    }

    codepoints->values = (mjb_codepoint *)mjb_alloc(byte_length * sizeof(mjb_codepoint));

    if(codepoints->values == NULL) {
        return MJB_STATUS_NO_MEMORY;
    }

    uint8_t state = MJB_UTF_ACCEPT;
    bool in_error = false;

    for(size_t index = 0; index < byte_length;) {
        mjb_codepoint codepoint;
        mjb_decode_result decoded = mjb_next_codepoint(buffer, byte_length, &state, &index,
            MJB_ENC_UTF_8, &codepoint, &in_error);

        if(decoded == MJB_DECODE_END) {
            break;
        }

        if(decoded == MJB_DECODE_ERROR) {
            mjb_idna_codepoints_free(codepoints);
            return MJB_STATUS_MALFORMED_INPUT;
        }

        if(decoded == MJB_DECODE_OK) {
            codepoints->values[codepoints->count++] = codepoint;
        }
    }

    return MJB_STATUS_OK;
}

static mjb_status mjb_idna_new_output(size_t capacity, mjb_output *output) {
    if(capacity == 0) {
        capacity = 1;
    }

    char *buffer = (char *)mjb_alloc(capacity);

    if(buffer == NULL) {
        return MJB_STATUS_NO_MEMORY;
    }

    buffer[0] = '\0';
    mjb_output_init_dynamic(output, buffer, capacity);

    return MJB_STATUS_OK;
}

static mjb_status mjb_idna_map(const char *buffer, size_t byte_length, mjb_encoding encoding,
    mjb_output *mapped) {
    mjb_status status = mjb_idna_new_output(byte_length, mapped);

    if(status != MJB_STATUS_OK) {
        return status;
    }

    uint8_t state = MJB_UTF_ACCEPT;
    bool in_error = false;

    for(size_t index = 0; index < byte_length;) {
        mjb_codepoint codepoint;
        mjb_decode_result decoded = mjb_next_codepoint(buffer, byte_length, &state, &index,
            encoding, &codepoint, &in_error);

        if(decoded == MJB_DECODE_END) {
            break;
        }

        if(decoded == MJB_DECODE_INCOMPLETE) {
            continue;
        }

        if(decoded == MJB_DECODE_ERROR) {
            status = MJB_STATUS_MALFORMED_INPUT;
            goto fail;
        }

        mjb_unicode_idna_status mapping_status;
        const mjb_codepoint *mapping;
        uint8_t mapping_length;

        if(!mjb_unicode_idna_lookup(codepoint, &mapping_status, &mapping, &mapping_length)) {
            status = MJB_STATUS_INVALID_CODEPOINT;
            goto fail;
        }

        if(mapping_status == MJB_UNICODE_IDNA_IGNORED) {
            continue;
        }

        if(mapping_status == MJB_UNICODE_IDNA_MAPPED) {
            for(uint8_t i = 0; i < mapping_length; ++i) {
                status = mjb_output_codepoint(mapped, mapping[i], MJB_ENC_UTF_8);

                if(status != MJB_STATUS_OK) {
                    goto fail;
                }
            }
        } else {
            status = mjb_output_codepoint(mapped, codepoint, MJB_ENC_UTF_8);

            if(status != MJB_STATUS_OK) {
                goto fail;
            }
        }
    }

    return MJB_STATUS_OK;

fail:
    mjb_free(mapped->buffer);
    mapped->buffer = NULL;
    return status;
}

static bool mjb_idna_starts_with_ace(const char *label, size_t byte_length) {
    return byte_length >= 4 && label[0] == 'x' && label[1] == 'n' && label[2] == '-' &&
        label[3] == '-';
}

static mjb_status mjb_idna_decode_label(const char *label, size_t byte_length, mjb_output *unicode,
    mjb_idna_info *info) {
    if(!mjb_idna_starts_with_ace(label, byte_length)) {
        return mjb_output_write(unicode, label, byte_length);
    }

    mjb_codepoint *decoded = NULL;
    size_t count = 0;
    mjb_status status = mjb_punycode_decode(label + 4, byte_length - 4, &decoded, &count);

    if(status == MJB_STATUS_NO_MEMORY || status == MJB_STATUS_OVERFLOW) {
        return status;
    }

    if(status != MJB_STATUS_OK) {
        info->errors |= MJB_IDNA_ERROR_PUNYCODE;
        mjb_free(decoded);
        return mjb_output_write(unicode, label, byte_length);
    }

    if(count == 0) {
        info->errors |= MJB_IDNA_ERROR_PUNYCODE;
        mjb_free(decoded);

        if(byte_length == 4) {
            return MJB_STATUS_OK;
        }

        return mjb_output_write(unicode, label, byte_length);
    }

    bool ascii_only = true;

    for(size_t i = 0; i < count; ++i) {
        ascii_only = ascii_only && decoded[i] < 0x80;
        status = mjb_output_codepoint(unicode, decoded[i], MJB_ENC_UTF_8);

        if(status != MJB_STATUS_OK) {
            mjb_free(decoded);
            return status;
        }
    }

    if(ascii_only) {
        info->errors |= MJB_IDNA_ERROR_PUNYCODE;
    }

    mjb_free(decoded);
    return MJB_STATUS_OK;
}

static mjb_status mjb_idna_decode_domain(const char *buffer, size_t byte_length,
    mjb_output *unicode, mjb_idna_info *info) {
    mjb_status status = mjb_idna_new_output(byte_length, unicode);

    if(status != MJB_STATUS_OK) {
        return status;
    }

    size_t label_start = 0;

    for(size_t i = 0; i <= byte_length; ++i) {
        if(i != byte_length && buffer[i] != '.') {
            continue;
        }

        status = mjb_idna_decode_label(buffer + label_start, i - label_start, unicode, info);

        if(status != MJB_STATUS_OK) {
            mjb_free(unicode->buffer);
            unicode->buffer = NULL;
            return status;
        }

        if(i != byte_length) {
            status = mjb_output_write(unicode, ".", 1);

            if(status != MJB_STATUS_OK) {
                mjb_free(unicode->buffer);
                unicode->buffer = NULL;
                return status;
            }
        }

        label_start = i + 1;
    }

    return MJB_STATUS_OK;
}

static mjb_bidi_class mjb_idna_bidi_class(mjb_codepoint codepoint) {
    mjb_bidi_class bidi = MJB_PR_BIDI_CLASS_L;
    bool mirrored = false;
    mjb_unicode_bidi_lookup(codepoint, &bidi, &mirrored);

    return bidi;
}

static bool mjb_idna_is_bidi_domain(const mjb_idna_codepoints *domain) {
    for(size_t i = 0; i < domain->count; ++i) {
        mjb_bidi_class bidi = mjb_idna_bidi_class(domain->values[i]);

        if(bidi == MJB_PR_BIDI_CLASS_R || bidi == MJB_PR_BIDI_CLASS_AL ||
            bidi == MJB_PR_BIDI_CLASS_AN) {
            return true;
        }
    }

    return false;
}

static bool mjb_idna_bidi_allowed_ltr(mjb_bidi_class bidi) {
    return bidi == MJB_PR_BIDI_CLASS_L || bidi == MJB_PR_BIDI_CLASS_EN ||
        bidi == MJB_PR_BIDI_CLASS_ES || bidi == MJB_PR_BIDI_CLASS_CS ||
        bidi == MJB_PR_BIDI_CLASS_ET || bidi == MJB_PR_BIDI_CLASS_ON ||
        bidi == MJB_PR_BIDI_CLASS_BN || bidi == MJB_PR_BIDI_CLASS_NSM;
}

static bool mjb_idna_bidi_allowed_rtl(mjb_bidi_class bidi) {
    return bidi == MJB_PR_BIDI_CLASS_R || bidi == MJB_PR_BIDI_CLASS_AL ||
        bidi == MJB_PR_BIDI_CLASS_AN || bidi == MJB_PR_BIDI_CLASS_EN ||
        bidi == MJB_PR_BIDI_CLASS_ES || bidi == MJB_PR_BIDI_CLASS_CS ||
        bidi == MJB_PR_BIDI_CLASS_ET || bidi == MJB_PR_BIDI_CLASS_ON ||
        bidi == MJB_PR_BIDI_CLASS_BN || bidi == MJB_PR_BIDI_CLASS_NSM;
}

static bool mjb_idna_valid_bidi_label(const mjb_idna_codepoints *label) {
    mjb_bidi_class first = mjb_idna_bidi_class(label->values[0]);
    bool rtl = first == MJB_PR_BIDI_CLASS_R || first == MJB_PR_BIDI_CLASS_AL;

    if(!rtl && first != MJB_PR_BIDI_CLASS_L) {
        return false;
    }

    bool has_en = false;
    bool has_an = false;

    for(size_t i = 0; i < label->count; ++i) {
        mjb_bidi_class bidi = mjb_idna_bidi_class(label->values[i]);

        if((rtl && !mjb_idna_bidi_allowed_rtl(bidi)) ||
            (!rtl && !mjb_idna_bidi_allowed_ltr(bidi))) {
            return false;
        }

        has_en = has_en || bidi == MJB_PR_BIDI_CLASS_EN;
        has_an = has_an || bidi == MJB_PR_BIDI_CLASS_AN;
    }

    if(rtl && has_en && has_an) {
        return false;
    }

    size_t last = label->count;

    do {
        --last;
    } while(last > 0 && mjb_idna_bidi_class(label->values[last]) == MJB_PR_BIDI_CLASS_NSM);

    mjb_bidi_class final = mjb_idna_bidi_class(label->values[last]);

    if(rtl) {
        return final == MJB_PR_BIDI_CLASS_R || final == MJB_PR_BIDI_CLASS_AL ||
            final == MJB_PR_BIDI_CLASS_EN || final == MJB_PR_BIDI_CLASS_AN;
    }

    return final == MJB_PR_BIDI_CLASS_L || final == MJB_PR_BIDI_CLASS_EN;
}

static uint8_t mjb_idna_get_joining_type(mjb_codepoint codepoint) {
    uint8_t joining_type = 0;
    mjb_unicode_has_property(codepoint, MJB_PR_JOINING_TYPE, &joining_type);

    return joining_type;
}

static bool mjb_idna_preceded_by_virama(const mjb_idna_codepoints *label, size_t index) {
    if(index == 0) {
        return false;
    }

    mjb_n_character previous;

    return mjb_n_codepoint_character(label->values[index - 1], &previous) &&
        previous.combining == MJB_CCC_VIRAMA;
}

static bool mjb_idna_valid_zwnj(const mjb_idna_codepoints *label, size_t index) {
    if(mjb_idna_preceded_by_virama(label, index)) {
        return true;
    }

    size_t before = index;

    while(before > 0) {
        uint8_t joining_type = mjb_idna_get_joining_type(label->values[before - 1]);

        if(joining_type != MJB_IDNA_TRANSPARENT) {
            if(joining_type != MJB_IDNA_LEFT_JOINING && joining_type != MJB_IDNA_DUAL_JOINING) {
                return false;
            }

            break;
        }

        --before;
    }

    if(before == 0) {
        return false;
    }

    size_t after = index + 1;

    while(after < label->count &&
        mjb_idna_get_joining_type(label->values[after]) == MJB_IDNA_TRANSPARENT) {
        ++after;
    }

    if(after >= label->count) {
        return false;
    }

    uint8_t joining_type = mjb_idna_get_joining_type(label->values[after]);

    return joining_type == MJB_IDNA_RIGHT_JOINING || joining_type == MJB_IDNA_DUAL_JOINING;
}

static bool mjb_idna_valid_contextj(const mjb_idna_codepoints *label) {
    for(size_t i = 0; i < label->count; ++i) {
        if(label->values[i] == 0x200C && !mjb_idna_valid_zwnj(label, i)) {
            return false;
        }

        if(label->values[i] == 0x200D && !mjb_idna_preceded_by_virama(label, i)) {
            return false;
        }
    }

    return true;
}

static mjb_status mjb_idna_label_is_nfc(const char *label, size_t byte_length, bool *is_nfc) {
    mjb_result normalized;
    mjb_status status = mjb_normalize(label, byte_length, MJB_ENC_UTF_8, MJB_NORMALIZATION_NFC,
        MJB_ENC_UTF_8, &normalized);

    if(status != MJB_STATUS_OK) {
        return status;
    }

    *is_nfc = normalized.output_size == byte_length &&
        (byte_length == 0 || memcmp(normalized.output, label, byte_length) == 0);
    mjb_result_free(&normalized);

    return MJB_STATUS_OK;
}

static bool mjb_idna_is_std3_ascii(mjb_codepoint codepoint) {
    return (codepoint >= 'a' && codepoint <= 'z') || (codepoint >= '0' && codepoint <= '9') ||
        codepoint == '-';
}

static mjb_status mjb_idna_validate_label(const char *label_buffer, size_t byte_length,
    const mjb_idna_codepoints *label, bool bidi_domain, mjb_idna_info *info) {
    if(label->count == 0) {
        info->errors |= MJB_IDNA_ERROR_EMPTY_LABEL;
        return MJB_STATUS_OK;
    }

    if(label->values[0] == '-' || label->values[label->count - 1] == '-' ||
        (label->count >= 4 && label->values[2] == '-' && label->values[3] == '-')) {
        info->errors |= MJB_IDNA_ERROR_HYPHEN;
    }

    bool is_nfc;
    mjb_status status = mjb_idna_label_is_nfc(label_buffer, byte_length, &is_nfc);

    if(status != MJB_STATUS_OK) {
        return status;
    }

    if(!is_nfc) {
        info->errors |= MJB_IDNA_ERROR_NOT_NFC;
    }

    mjb_category category;

    if(mjb_unicode_category_lookup(label->values[0], &category) && category >= MJB_CATEGORY_MN &&
        category <= MJB_CATEGORY_ME) {
        info->errors |= MJB_IDNA_ERROR_LEADING_MARK;
    }

    for(size_t i = 0; i < label->count; ++i) {
        mjb_unicode_idna_status mapping_status;
        const mjb_codepoint *mapping;
        uint8_t mapping_length;

        if(!mjb_unicode_idna_lookup(label->values[i], &mapping_status, &mapping, &mapping_length) ||
            (mapping_status != MJB_UNICODE_IDNA_VALID &&
                mapping_status != MJB_UNICODE_IDNA_DEVIATION)) {
            info->errors |= MJB_IDNA_ERROR_DISALLOWED;
        }

        if(label->values[i] < 0x80 && !mjb_idna_is_std3_ascii(label->values[i])) {
            info->errors |= MJB_IDNA_ERROR_STD3;
        }
    }

    if(!mjb_idna_valid_contextj(label)) {
        info->errors |= MJB_IDNA_ERROR_CONTEXTJ;
    }

    if(bidi_domain && !mjb_idna_valid_bidi_label(label)) {
        info->errors |= MJB_IDNA_ERROR_BIDI;
    }

    return MJB_STATUS_OK;
}

static mjb_status mjb_idna_validate_domain(const char *buffer, size_t byte_length,
    mjb_idna_info *info) {
    mjb_idna_codepoints domain;
    mjb_status status = mjb_idna_decode_utf8(buffer, byte_length, &domain);

    if(status != MJB_STATUS_OK) {
        return status;
    }

    bool bidi_domain = mjb_idna_is_bidi_domain(&domain);
    mjb_idna_codepoints_free(&domain);
    size_t label_start = 0;

    for(size_t i = 0; i <= byte_length; ++i) {
        if(i != byte_length && buffer[i] != '.') {
            continue;
        }

        mjb_idna_codepoints label;
        status = mjb_idna_decode_utf8(buffer + label_start, i - label_start, &label);

        if(status != MJB_STATUS_OK) {
            return status;
        }

        bool trailing_root = i == byte_length && i > 0 && buffer[i - 1] == '.';

        if(trailing_root) {
            status = MJB_STATUS_OK;
        } else {
            status = mjb_idna_validate_label(buffer + label_start, i - label_start, &label,
                bidi_domain, info);
        }
        mjb_idna_codepoints_free(&label);

        if(status != MJB_STATUS_OK) {
            return status;
        }

        label_start = i + 1;
    }

    return MJB_STATUS_OK;
}

static mjb_status mjb_idna_encode_label(const char *label_buffer, size_t byte_length,
    const mjb_idna_codepoints *label, mjb_output *ascii) {
    bool ascii_only = true;

    for(size_t i = 0; i < label->count; ++i) {
        if(label->values[i] >= 0x80) {
            ascii_only = false;
            break;
        }
    }

    if(ascii_only) {
        return mjb_output_write(ascii, label_buffer, byte_length);
    }

    mjb_status status = mjb_output_write(ascii, "xn--", 4);

    if(status != MJB_STATUS_OK) {
        return status;
    }

    return mjb_punycode_encode(label->values, label->count, ascii);
}

static mjb_status mjb_idna_encode_domain(const char *buffer, size_t byte_length, mjb_output *ascii,
    mjb_idna_info *info) {
    mjb_status status = mjb_idna_new_output(byte_length, ascii);

    if(status != MJB_STATUS_OK) {
        return status;
    }

    size_t label_start = 0;

    for(size_t i = 0; i <= byte_length; ++i) {
        if(i != byte_length && buffer[i] != '.') {
            continue;
        }

        mjb_idna_codepoints label;
        status = mjb_idna_decode_utf8(buffer + label_start, i - label_start, &label);

        if(status != MJB_STATUS_OK) {
            goto fail;
        }

        size_t output_start = ascii->size;
        status = mjb_idna_encode_label(buffer + label_start, i - label_start, &label, ascii);
        mjb_idna_codepoints_free(&label);

        if(status != MJB_STATUS_OK) {
            goto fail;
        }

        size_t output_length = ascii->size - output_start;

        if(output_length == 0 || output_length > 63) {
            info->errors |= MJB_IDNA_ERROR_LABEL_LENGTH;
        }

        if(i != byte_length) {
            status = mjb_output_write(ascii, ".", 1);

            if(status != MJB_STATUS_OK) {
                goto fail;
            }
        }

        label_start = i + 1;
    }

    if(ascii->size == 0 || ascii->size > 253) {
        info->errors |= MJB_IDNA_ERROR_DOMAIN_LENGTH;
    }

    return MJB_STATUS_OK;

fail:
    mjb_free(ascii->buffer);
    ascii->buffer = NULL;
    return status;
}

static mjb_status mjb_idna_finish_output(mjb_output *output, mjb_encoding output_encoding,
    mjb_result *result) {
    if(output_encoding == MJB_ENC_UTF_8) {
        result->output = output->buffer;
        result->output_size = output->size;
        result->transformed = true;
        output->buffer = NULL;
        return MJB_STATUS_OK;
    }

    if(output->size == 0) {
        mjb_free(output->buffer);
        output->buffer = NULL;
        result->output = NULL;
        result->output_size = 0;
        result->transformed = false;
        return MJB_STATUS_OK;
    }

    mjb_status status = mjb_convert_encoding(output->buffer, output->size, MJB_ENC_UTF_8,
        output_encoding, result);
    mjb_free(output->buffer);
    output->buffer = NULL;

    return status;
}

static mjb_status mjb_idna_process(const char *buffer, size_t byte_length, mjb_encoding encoding,
    mjb_encoding output_encoding, bool to_ascii, mjb_idna_info *info, mjb_result *result) {
    if(info == NULL || result == NULL || (buffer == NULL && byte_length > 0)) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    info->errors = MJB_IDNA_ERROR_NONE;
    result->output = NULL;
    result->output_size = 0;
    result->transformed = false;

    if(!mjb_encoding_is_valid_input(encoding) || !mjb_encoding_is_valid_output(output_encoding)) {
        return MJB_STATUS_INVALID_ENCODING;
    }

    mjb_status status = mjb_resolve_input_byte_length(buffer, &byte_length, encoding);

    if(status != MJB_STATUS_OK) {
        return status;
    }

    if(encoding == MJB_ENC_ASCII) {
        for(size_t i = 0; i < byte_length; ++i) {
            if(((uint8_t)buffer[i] & 0x80) != 0) {
                return MJB_STATUS_MALFORMED_INPUT;
            }
        }
    }

    status = mjb_validate_code_unit_sequence(buffer, byte_length, encoding);

    if(status != MJB_STATUS_OK) {
        return status;
    }

    mjb_output mapped;
    status = mjb_idna_map(buffer, byte_length, encoding, &mapped);

    if(status != MJB_STATUS_OK) {
        return status;
    }

    mjb_result normalized;
    status = mjb_normalize(mapped.buffer, mapped.size, MJB_ENC_UTF_8, MJB_NORMALIZATION_NFC,
        MJB_ENC_UTF_8, &normalized);

    if(status != MJB_STATUS_OK) {
        mjb_free(mapped.buffer);
        return status;
    }

    const char *normalized_buffer = normalized.output;
    size_t normalized_size = normalized.output_size;
    mjb_output unicode;
    status = mjb_idna_decode_domain(normalized_buffer, normalized_size, &unicode, info);
    mjb_result_free(&normalized);
    mjb_free(mapped.buffer);

    if(status != MJB_STATUS_OK) {
        return status;
    }

    status = mjb_idna_validate_domain(unicode.buffer, unicode.size, info);

    if(status != MJB_STATUS_OK) {
        mjb_free(unicode.buffer);
        return status;
    }

    if(!to_ascii) {
        status = mjb_idna_finish_output(&unicode, output_encoding, result);
        mjb_free(unicode.buffer);
        return status;
    }

    mjb_output ascii;
    status = mjb_idna_encode_domain(unicode.buffer, unicode.size, &ascii, info);
    mjb_free(unicode.buffer);

    if(status != MJB_STATUS_OK) {
        return status;
    }

    status = mjb_idna_finish_output(&ascii, output_encoding, result);
    mjb_free(ascii.buffer);
    return status;
}
#endif // MJB_FEATURE_IDNA

MJB_EXPORT mjb_status mjb_idna_to_ascii(const char *buffer, size_t byte_length,
    mjb_encoding encoding, mjb_encoding output_encoding, mjb_idna_info *info, mjb_result *result) {
#if MJB_FEATURE_IDNA
    return mjb_idna_process(buffer, byte_length, encoding, output_encoding, true, info, result);
#else
    (void)buffer;
    (void)byte_length;
    (void)encoding;
    (void)output_encoding;
    (void)info;
    (void)result;
    return MJB_STATUS_FEATURE_NOT_ENABLED;
#endif
}

MJB_EXPORT mjb_status mjb_idna_to_unicode(const char *buffer, size_t byte_length,
    mjb_encoding encoding, mjb_encoding output_encoding, mjb_idna_info *info, mjb_result *result) {
#if MJB_FEATURE_IDNA
    return mjb_idna_process(buffer, byte_length, encoding, output_encoding, false, info, result);
#else
    (void)buffer;
    (void)byte_length;
    (void)encoding;
    (void)output_encoding;
    (void)info;
    (void)result;
    return MJB_STATUS_FEATURE_NOT_ENABLED;
#endif
}

#if MJB_FEATURE_IDNA
static mjb_status mjb_idna_into(const char *buffer, size_t byte_length, mjb_encoding encoding,
    mjb_encoding output_encoding, bool to_ascii, mjb_idna_info *info, void *output,
    size_t *output_size) {
    if(output_size == NULL) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    size_t capacity = output == NULL ? 0 : *output_size;
    *output_size = 0;
    mjb_result result;
    mjb_status status = mjb_idna_process(buffer, byte_length, encoding, output_encoding, to_ascii,
        info, &result);

    if(status != MJB_STATUS_OK) {
        return status;
    }

    *output_size = result.output_size;

    if(output != NULL && capacity < result.output_size) {
        mjb_result_free(&result);
        return MJB_STATUS_OUTPUT_TOO_SMALL;
    }

    if(output != NULL && result.output_size > 0) {
        memcpy(output, result.output, result.output_size);
    }

    mjb_result_free(&result);
    return MJB_STATUS_OK;
}
#endif // MJB_FEATURE_IDNA

MJB_EXPORT mjb_status mjb_idna_to_ascii_into(const char *buffer, size_t byte_length,
    mjb_encoding encoding, mjb_encoding output_encoding, mjb_idna_info *info, void *output,
    size_t *output_size) {
#if MJB_FEATURE_IDNA
    return mjb_idna_into(buffer, byte_length, encoding, output_encoding, true, info, output,
        output_size);
#else
    (void)buffer;
    (void)byte_length;
    (void)encoding;
    (void)output_encoding;
    (void)info;
    (void)output;
    (void)output_size;
    return MJB_STATUS_FEATURE_NOT_ENABLED;
#endif
}

MJB_EXPORT mjb_status mjb_idna_to_unicode_into(const char *buffer, size_t byte_length,
    mjb_encoding encoding, mjb_encoding output_encoding, mjb_idna_info *info, void *output,
    size_t *output_size) {
#if MJB_FEATURE_IDNA
    return mjb_idna_into(buffer, byte_length, encoding, output_encoding, false, info, output,
        output_size);
#else
    (void)buffer;
    (void)byte_length;
    (void)encoding;
    (void)output_encoding;
    (void)info;
    (void)output;
    (void)output_size;
    return MJB_STATUS_FEATURE_NOT_ENABLED;
#endif
}
