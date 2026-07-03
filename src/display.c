/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "mojibake-internal.h"
#include "unicode-tables.h"
#include "unicode.h"
#include "utf.h"

/**
 * Return display width of a string.
 */
MJB_EXPORT mjb_status mjb_display_width(const char *buffer, size_t size, mjb_encoding encoding,
    mjb_width_context context, size_t *width) {
    if(width == NULL) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    *width = 0;

    if(context != MJB_WIDTH_CONTEXT_WESTERN && context != MJB_WIDTH_CONTEXT_EAST_ASIAN &&
        context != MJB_WIDTH_CONTEXT_AUTO) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    if(size == 0) {
        return MJB_STATUS_OK;
    }

    if(buffer == NULL) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    size_t ambiguous_count = 0;
    size_t wide_count = 0;
    size_t visible_count = 0;
    uint8_t state = MJB_UTF_ACCEPT;
    bool in_error = false;
    mjb_codepoint codepoint;

    for(size_t i = 0; i < size;) {
        mjb_decode_result decode_status = mjb_next_codepoint(buffer, size, &state, &i,
            encoding, &codepoint, &in_error);

        if(decode_status == MJB_DECODE_END) {
            break;
        }

        if(decode_status == MJB_DECODE_INCOMPLETE) {
            continue;
        }

        // Check General Category for zero-width cases
        mjb_category category = MJB_CATEGORY_CN;

        if(mjb_unicode_category_lookup(codepoint, &category)) {
            if(category == MJB_CATEGORY_MN || category == MJB_CATEGORY_ME) {
                // Combining marks
                continue;
            }

            if(category == MJB_CATEGORY_CF) {
                // Format characters
                continue;
            }

            if(category == MJB_CATEGORY_CC) {
                // Control characters
                continue;
            }
        }

        // Handle visible characters using East Asian Width
        mjb_east_asian_width eaw = MJB_EAW_NOT_SET;
        size_t width_increment = 1;
        bool is_visible = true;

        if(mjb_codepoint_east_asian_width(codepoint, &eaw) == MJB_STATUS_OK) {
            if(eaw == MJB_EAW_NEUTRAL || eaw == MJB_EAW_NARROW || eaw == MJB_EAW_HALF_WIDTH) {
                width_increment = 1;
            } else if(eaw == MJB_EAW_FULL_WIDTH || eaw == MJB_EAW_WIDE) {
                width_increment = 2;
                ++wide_count;
            } else if(eaw == MJB_EAW_AMBIGUOUS) {
                // Initially count as narrow (1), track for later adjustment
                width_increment = 1;
                ++ambiguous_count;
            } else {
                // Fallback for any unhandled EAW values
                width_increment = 1;
            }
        } else {
            // No EAW property found - default to narrow
            width_increment = 1;
        }

        if(is_visible) {
            if(*width > SIZE_MAX - width_increment) {
                return MJB_STATUS_OVERFLOW;
            }

            ++visible_count;
            *width += width_increment;
        }
    }

    // Adjust width for ambiguous characters based on context
    if(ambiguous_count > 0) {
        bool use_east_asian_width = false;

        if(context == MJB_WIDTH_CONTEXT_AUTO) {
            // If ≥50% of visible characters are wide, treat as East Asian context
            use_east_asian_width = (visible_count > 0 && wide_count >= visible_count - wide_count);
        } else {
            use_east_asian_width = (context == MJB_WIDTH_CONTEXT_EAST_ASIAN);
        }

        // If East Asian context, add 1 for each ambiguous character (they were initially counted as 1, need to be 2)
        if(use_east_asian_width) {
            if(*width > SIZE_MAX - ambiguous_count) {
                return MJB_STATUS_OVERFLOW;
            }

            *width += ambiguous_count;
        }
    }

    return MJB_STATUS_OK;
}
