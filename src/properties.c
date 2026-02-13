/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "mojibake-internal.h"

extern mojibake mjb_global;

// Return if a codepoint has a property
MJB_EXPORT bool mjb_codepoint_has_property(mjb_codepoint codepoint, mjb_property property,
    uint8_t *value) {
    if(!mjb_codepoint_is_valid(codepoint)) {
        return false;
    }

    if(!mjb_initialize()) {
        return false;
    }

    sqlite3_reset(mjb_global.stmt_get_properties);

    int rc = sqlite3_bind_int(mjb_global.stmt_get_properties, 1, codepoint);

    if(rc != SQLITE_OK) {
        sqlite3_reset(mjb_global.stmt_get_properties);

        return false;
    }

    bool found = false;

    while((rc = sqlite3_step(mjb_global.stmt_get_properties)) == SQLITE_ROW) {
        const unsigned char *blob = (unsigned char*)sqlite3_column_blob(mjb_global.stmt_get_properties, 0);
        int blob_size = sqlite3_column_bytes(mjb_global.stmt_get_properties, 0);

        if(!blob || blob_size < 2) {
            continue;
        }

        // Decode the BLOB
        // Format: [bool_count] [bool_prop_id_1] [bool_prop_id_2] ...
        //         [enum_count] [enum_prop_id_1] [value_1] [enum_prop_id_2] [value_2] ...
        unsigned int offset = 0;

        // Check boolean properties
        unsigned char bool_count = blob[offset++];

        for(unsigned int i = 0; i < bool_count && offset < (unsigned int)blob_size; ++i) {
            if(blob[offset++] == property) {
                found = true;

                break;
            }
        }

        // Check enumerated properties if not found in boolean properties
        if(!found && offset < (unsigned int)blob_size) {
            unsigned char enum_count = blob[offset++];

            for(unsigned int i = 0; i < enum_count && (offset + 1) < (unsigned int)blob_size; ++i) {
                if(blob[offset] == property) {
                    found = true;

                    if(value) {
                        *value = blob[offset + 1];
                    }

                    break;
                }

                offset += 2;
            }
        }

        if(found) {
            break;
        }
    }

    sqlite3_reset(mjb_global.stmt_get_properties);

    return found;
}

MJB_EXPORT bool mjb_codepoint_properties(mjb_codepoint codepoint, uint8_t *buffer) {
    if(!mjb_codepoint_is_valid(codepoint)) {
        return false;
    }

    if(!mjb_initialize()) {
        return false;
    }

    sqlite3_reset(mjb_global.stmt_get_properties);

    int rc = sqlite3_bind_int(mjb_global.stmt_get_properties, 1, codepoint);

    if(rc != SQLITE_OK) {
        sqlite3_reset(mjb_global.stmt_get_properties);

        return false;
    }

    while((rc = sqlite3_step(mjb_global.stmt_get_properties)) == SQLITE_ROW) {
        const unsigned char *blob = (unsigned char*)sqlite3_column_blob(mjb_global.stmt_get_properties, 0);
        int blob_size = sqlite3_column_bytes(mjb_global.stmt_get_properties, 0);

        if(!blob || blob_size < 2) {
            continue;
        }

        unsigned int offset = 0;

        // Check boolean properties
        unsigned char bool_count = blob[offset++];

        // Buffer: [enum-value-1 enum-1 enum-value-0 enum-0 ... bool-0 bool-1]
        for(unsigned int i = 0; i < bool_count && offset < (unsigned int)blob_size; ++i) {
            // Add to buffer
            buffer[blob[offset++]] = 1;
        }

        if(offset < (unsigned int)blob_size) {
            unsigned char enum_count = blob[offset++];

            for(unsigned int i = 0; i < enum_count && (offset + 1) < (unsigned int)blob_size; i += 2) {
                buffer[blob[offset]] = blob[offset + 1];
                offset += 2;
            }
        }
    }

    return true;
}

MJB_EXPORT uint8_t mjb_codepoint_property(uint8_t *properties, mjb_property property) {
    return properties[property];
}
