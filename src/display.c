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
    size_t *width) {
    if(size == 0) {
        *width = 0;

        return true;
    }

    *width = 0;
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

        sqlite3_reset(mjb_global.stmt_line_breaking);
        sqlite3_bind_int(mjb_global.stmt_line_breaking, 1, codepoint);

        if(sqlite3_step(mjb_global.stmt_line_breaking) == SQLITE_ROW) {
            mjb_category category = sqlite3_column_int(mjb_global.stmt_line_breaking, 0);

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

        // Now handle visible characters using East Asian Width
        mjb_east_asian_width eaw = MJB_EAW_NOT_SET;

        if(mjb_codepoint_east_asian_width(codepoint, &eaw)) {
            if(eaw == MJB_EAW_NEUTRAL || eaw == MJB_EAW_NARROW || eaw == MJB_EAW_HALF_WIDTH) {
                *width += 1;
            } else if(eaw == MJB_EAW_FULL_WIDTH || eaw == MJB_EAW_WIDE) {
                *width += 2;
            } else if(eaw == MJB_EAW_AMBIGUOUS) {
                // UAX#11: Ambiguous width depends on context (East Asian vs Western).
                // Default to narrow (1) for Western/non-CJK contexts.
                *width += 1;
            } else {
                // Fallback for any unhandled EAW values
                *width += 1;
            }
        } else {
            // No EAW property found - default to narrow
            *width += 1;
        }
    }

    return true;
}
