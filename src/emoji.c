/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "mojibake-internal.h"
#include "unicode-tables.h"
#include "utf.h"

// An emoji sequence can have a maximum of 32 codepoints, according to the Unicode Standard.

#define MJB_EMOJI_SEQUENCE_MAX_CODEPOINTS 32

MJB_EXPORT mjb_status mjb_codepoint_emoji_properties(mjb_codepoint codepoint,
    mjb_emoji_properties *emoji) {
    if(emoji == NULL || !mjb_codepoint_is_valid(codepoint)) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    if(!mjb_unicode_emoji_lookup(codepoint, emoji)) {
        return MJB_STATUS_NOT_FOUND;
    }

    return MJB_STATUS_OK;
}

MJB_EXPORT bool mjb_codepoint_is_emoji(mjb_codepoint codepoint) {
    mjb_emoji_properties emoji;

    return mjb_codepoint_emoji_properties(codepoint, &emoji) == MJB_STATUS_OK && emoji.emoji;
}

MJB_EXPORT bool mjb_codepoint_is_emoji_presentation(mjb_codepoint codepoint) {
    mjb_emoji_properties emoji;

    return mjb_codepoint_emoji_properties(codepoint, &emoji) == MJB_STATUS_OK && emoji.presentation;
}

MJB_EXPORT bool mjb_codepoint_is_emoji_modifier(mjb_codepoint codepoint) {
    mjb_emoji_properties emoji;

    return mjb_codepoint_emoji_properties(codepoint, &emoji) == MJB_STATUS_OK && emoji.modifier;
}

MJB_EXPORT bool mjb_codepoint_is_emoji_modifier_base(mjb_codepoint codepoint) {
    mjb_emoji_properties emoji;

    return mjb_codepoint_emoji_properties(codepoint, &emoji) == MJB_STATUS_OK &&
        emoji.modifier_base;
}

MJB_EXPORT bool mjb_codepoint_is_emoji_component(mjb_codepoint codepoint) {
    mjb_emoji_properties emoji;

    return mjb_codepoint_emoji_properties(codepoint, &emoji) == MJB_STATUS_OK && emoji.component;
}

MJB_EXPORT bool mjb_codepoint_is_extended_pictographic(mjb_codepoint codepoint) {
    mjb_emoji_properties emoji;

    return mjb_codepoint_emoji_properties(codepoint, &emoji) == MJB_STATUS_OK &&
        emoji.extended_pictographic;
}

static mjb_status mjb_emoji_decode_sequence(const char *buffer, size_t byte_length,
    mjb_encoding encoding, mjb_codepoint *codepoints, size_t *count) {
    if(buffer == NULL || byte_length == 0 || codepoints == NULL || count == NULL) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    *count = 0;

    uint8_t state = MJB_UTF_ACCEPT;
    bool in_error = false;
    mjb_codepoint codepoint;

    for(size_t i = 0; i < byte_length;) {
        mjb_decode_result result = mjb_next_codepoint(buffer, byte_length, &state, &i, encoding,
            &codepoint, &in_error);

        if(result == MJB_DECODE_END) {
            break;
        }

        if(result == MJB_DECODE_INCOMPLETE) {
            continue;
        }

        if(result == MJB_DECODE_ERROR) {
            return MJB_STATUS_MALFORMED_INPUT;
        }

        if(*count >= MJB_EMOJI_SEQUENCE_MAX_CODEPOINTS) {
            // Exceeded maximum number of codepoints for an emoji sequence.
            return MJB_STATUS_OVERFLOW;
        }

        codepoints[(*count)++] = codepoint;
    }

    return *count > 0 ? MJB_STATUS_OK : MJB_STATUS_MALFORMED_INPUT;
}

MJB_EXPORT mjb_status mjb_classify_emoji_sequence(const char *buffer, size_t byte_length,
    mjb_encoding encoding, mjb_emoji_sequence *emoji) {
    mjb_codepoint codepoints[MJB_EMOJI_SEQUENCE_MAX_CODEPOINTS];
    size_t count = 0;

    if(emoji == NULL) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    mjb_status status = mjb_emoji_decode_sequence(buffer, byte_length, encoding, codepoints,
        &count);

    if(status != MJB_STATUS_OK) {
        return status;
    }

    if(!mjb_unicode_emoji_sequence_lookup(codepoints, count, emoji)) {
        return MJB_STATUS_NOT_FOUND;
    }

    return MJB_STATUS_OK;
}

MJB_EXPORT bool mjb_string_is_emoji_sequence(const char *buffer, size_t byte_length,
    mjb_encoding encoding) {
    mjb_emoji_sequence emoji;

    return mjb_classify_emoji_sequence(buffer, byte_length, encoding, &emoji) == MJB_STATUS_OK;
}

MJB_EXPORT bool mjb_string_is_rgi_emoji(const char *buffer, size_t byte_length,
    mjb_encoding encoding) {
    mjb_emoji_sequence emoji;

    if(mjb_classify_emoji_sequence(buffer, byte_length, encoding, &emoji) != MJB_STATUS_OK) {
        return false;
    }

    return (emoji.type >= MJB_EMOJI_SEQUENCE_BASIC && emoji.type <= MJB_EMOJI_SEQUENCE_ZWJ) ||
        emoji.qualification == MJB_EMOJI_QUALIFICATION_COMPONENT ||
        emoji.qualification == MJB_EMOJI_QUALIFICATION_FULLY_QUALIFIED;
}
