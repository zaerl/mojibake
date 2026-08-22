/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "mojibake-internal.h"
#include "unicode-tables.h"
#include "unicode.h"
#include "utf.h"

static bool mjb_terminal_cluster_is_wide_emoji(const char *buffer, size_t byte_length) {
    mjb_emoji_sequence emoji;

    if(mjb_emoji_sequence_info(buffer, byte_length, MJB_ENC_UTF_8, &emoji) != MJB_STATUS_OK) {
        return false;
    }

    return emoji.type == MJB_EMOJI_SEQUENCE_EMOJI_VARIATION ||
        (emoji.type >= MJB_EMOJI_SEQUENCE_BASIC && emoji.type <= MJB_EMOJI_SEQUENCE_ZWJ) ||
        emoji.qualification == MJB_EMOJI_QUALIFICATION_COMPONENT ||
        emoji.qualification == MJB_EMOJI_QUALIFICATION_FULLY_QUALIFIED;
}

static mjb_status mjb_terminal_cluster_width(const char *buffer, size_t byte_length,
    mjb_terminal_width_profile profile, size_t *width) {
    uint8_t state = MJB_UTF_ACCEPT;
    bool in_error = false;
    mjb_codepoint codepoint = 0;
    size_t scalar_width = 0;

    for(size_t i = 0; i < byte_length;) {
        mjb_decode_result decode_status = mjb_next_codepoint(buffer, byte_length, &state, &i,
            MJB_ENC_UTF_8, &codepoint, &in_error);

        if(decode_status == MJB_DECODE_END) {
            break;
        }

        if(decode_status == MJB_DECODE_INCOMPLETE) {
            continue;
        }

        if(decode_status == MJB_DECODE_ERROR) {
            return MJB_STATUS_MALFORMED_INPUT;
        }

        mjb_category category = MJB_CATEGORY_CN;

        if(mjb_unicode_category_lookup(codepoint, &category)) {
            if(category == MJB_CATEGORY_CC || category == MJB_CATEGORY_ZL ||
                category == MJB_CATEGORY_ZP) {
                return MJB_STATUS_UNSUPPORTED;
            }

            if(category == MJB_CATEGORY_MN || category == MJB_CATEGORY_ME ||
                category == MJB_CATEGORY_CF) {
                continue;
            }
        }

        mjb_east_asian_width east_asian_width = MJB_EAW_NOT_SET;
        size_t increment = 1;

        if(mjb_codepoint_east_asian_width(codepoint, &east_asian_width) == MJB_STATUS_OK) {
            if(east_asian_width == MJB_EAW_FULL_WIDTH || east_asian_width == MJB_EAW_WIDE ||
                (east_asian_width == MJB_EAW_AMBIGUOUS &&
                    profile == MJB_TERMINAL_WIDTH_EAST_ASIAN)) {
                increment = 2;
            }
        }

        if(scalar_width > SIZE_MAX - increment) {
            return MJB_STATUS_OVERFLOW;
        }

        scalar_width += increment;
    }

    *width = mjb_terminal_cluster_is_wide_emoji(buffer, byte_length) ? 2 : scalar_width;

    return MJB_STATUS_OK;
}

/**
 * Return the estimated terminal-cell width of printable, single-line text.
 */
MJB_EXPORT mjb_status mjb_terminal_width(const char *buffer, size_t byte_length,
    mjb_encoding encoding, mjb_malformed_policy malformed_policy,
    mjb_terminal_width_profile profile, size_t *width, mjb_diagnostic *diagnostic) {
    if(width == NULL) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    *width = 0;
    mjb_diagnostic_reset(diagnostic);

    if(!mjb_malformed_policy_is_valid(malformed_policy)) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    if(profile != MJB_TERMINAL_WIDTH_NARROW && profile != MJB_TERMINAL_WIDTH_EAST_ASIAN) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    if(!mjb_encoding_is_valid_input(encoding)) {
        return MJB_STATUS_INVALID_ENCODING;
    }

    if(byte_length == 0) {
        return MJB_STATUS_OK;
    }

    if(buffer == NULL) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    mjb_status status = mjb_resolve_input_byte_length(buffer, &byte_length, encoding);

    if(status != MJB_STATUS_OK || byte_length == 0) {
        return status;
    }

    mjb_result normalized = { NULL, 0, false };
    status = mjb_normalize(buffer, byte_length, encoding, malformed_policy, MJB_NORMALIZATION_NFC,
        MJB_ENC_UTF_8, &normalized, diagnostic);

    if(status != MJB_STATUS_OK) {
        return status;
    }

    mjb_next_state state;
    state.index = 0;

    mjb_break_type break_type;
    size_t total_width = 0;
    size_t previous_break = 0;

    while((break_type = mjb_next_grapheme_break(normalized.output, normalized.output_size,
               MJB_ENC_UTF_8, &state)) != MJB_BT_NOT_SET) {
        if(break_type == MJB_BT_NO_BREAK) {
            continue;
        }

        size_t break_position = mjb_monotonic_boundary_position(state.index, normalized.output_size,
            state.current_codepoint, MJB_ENC_UTF_8, state.state == MJB_UTF_TERMINATED,
            previous_break);
        size_t cluster_width = 0;
        status = mjb_terminal_cluster_width(normalized.output + previous_break,
            break_position - previous_break, profile, &cluster_width);

        if(status != MJB_STATUS_OK) {
            break;
        }

        if(total_width > SIZE_MAX - cluster_width) {
            status = MJB_STATUS_OVERFLOW;
            break;
        }

        total_width += cluster_width;
        previous_break = break_position;
    }

    mjb_status free_status = mjb_result_free(&normalized);

    if(status == MJB_STATUS_OK && free_status != MJB_STATUS_OK) {
        status = free_status;
    }

    if(status == MJB_STATUS_OK) {
        *width = total_width;
    }

    return status;
}
