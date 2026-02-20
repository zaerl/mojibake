/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "mojibake-internal.h"
#include "unicode.h"
#include "utf.h"

extern mojibake mjb_global;

/**
 * Return display width of a string.
 */
MJB_EXPORT bool mjb_display_width(const char *buffer, size_t size, mjb_encoding encoding,
    mjb_width_context context, size_t *width) {
    *width = 0;

    if(size == 0) {
        return true;
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
        sqlite3_reset(mjb_global.stmt_line_breaking);
        sqlite3_bind_int(mjb_global.stmt_line_breaking, 1, codepoint);

        if(sqlite3_step(mjb_global.stmt_line_breaking) == SQLITE_ROW) {
            mjb_category category = (mjb_category)sqlite3_column_int(mjb_global.stmt_line_breaking, 0);

            sqlite3_reset(mjb_global.stmt_line_breaking);

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
        } else {
            sqlite3_reset(mjb_global.stmt_line_breaking);
        }

        // Handle visible characters using East Asian Width
        mjb_east_asian_width eaw = MJB_EAW_NOT_SET;

        if(mjb_codepoint_east_asian_width(codepoint, &eaw)) {
            if(eaw == MJB_EAW_NEUTRAL || eaw == MJB_EAW_NARROW || eaw == MJB_EAW_HALF_WIDTH) {
                *width += 1;
                visible_count++;
            } else if(eaw == MJB_EAW_FULL_WIDTH || eaw == MJB_EAW_WIDE) {
                *width += 2;
                wide_count++;
                visible_count++;
            } else if(eaw == MJB_EAW_AMBIGUOUS) {
                // Initially count as narrow (1), track for later adjustment
                *width += 1;
                ambiguous_count++;
                visible_count++;
            } else {
                // Fallback for any unhandled EAW values
                *width += 1;
                visible_count++;
            }
        } else {
            // No EAW property found - default to narrow
            *width += 1;
            visible_count++;
        }
    }

    // Adjust width for ambiguous characters based on context
    if(ambiguous_count > 0) {
        bool use_east_asian_width = false;

        if(context == MJB_WIDTH_CONTEXT_AUTO) {
            // If ≥50% of visible characters are wide, treat as East Asian context
            use_east_asian_width = (visible_count > 0 && wide_count * 2 >= visible_count);
        } else {
            use_east_asian_width = (context == MJB_WIDTH_CONTEXT_EAST_ASIAN);
        }

        // If East Asian context, add 1 for each ambiguous character (they were initially counted as 1, need to be 2)
        if(use_east_asian_width) {
            *width += ambiguous_count;
        }
    }

    return true;
}
