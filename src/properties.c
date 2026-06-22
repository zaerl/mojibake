/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "mojibake-internal.h"
#include "unicode-tables.h"

// Return if a codepoint has a property
MJB_EXPORT bool mjb_codepoint_has_property(mjb_codepoint codepoint, mjb_property property,
    uint8_t *value) {
    if(!mjb_codepoint_is_valid(codepoint)) {
        return false;
    }

    return mjb_unicode_has_property(codepoint, property, value);
}

MJB_EXPORT bool mjb_codepoint_properties(mjb_codepoint codepoint, uint8_t *buffer) {
    if(!mjb_codepoint_is_valid(codepoint)) {
        return false;
    }

    return mjb_unicode_properties(codepoint, buffer);
}

MJB_EXPORT uint8_t mjb_codepoint_property(uint8_t *properties, mjb_property property) {
    return properties[property];
}

MJB_EXPORT mjb_script mjb_codepoint_script(mjb_codepoint codepoint) {
    if(!mjb_codepoint_is_valid(codepoint)) {
        return MJB_SC_ZZZZ;
    }

    uint8_t raw = 0;

    if(!mjb_codepoint_has_property(codepoint, MJB_PR_SCRIPT, &raw) || raw == MJB_SC_NOT_SET) {
        return MJB_SC_ZZZZ;
    }

    return (mjb_script)raw;
}

MJB_EXPORT bool mjb_codepoint_is_id_start(mjb_codepoint codepoint) {
    return mjb_codepoint_has_property(codepoint, MJB_PR_ID_START, NULL);
}

MJB_EXPORT bool mjb_codepoint_is_id_continue(mjb_codepoint codepoint) {
    return mjb_codepoint_has_property(codepoint, MJB_PR_ID_CONTINUE, NULL);
}

MJB_EXPORT bool mjb_codepoint_is_xid_start(mjb_codepoint codepoint) {
    return mjb_codepoint_has_property(codepoint, MJB_PR_XID_START, NULL);
}

MJB_EXPORT bool mjb_codepoint_is_xid_continue(mjb_codepoint codepoint) {
    return mjb_codepoint_has_property(codepoint, MJB_PR_XID_CONTINUE, NULL);
}

MJB_EXPORT bool mjb_codepoint_is_pattern_syntax(mjb_codepoint codepoint) {
    return mjb_codepoint_has_property(codepoint, MJB_PR_PATTERN_SYNTAX, NULL);
}

MJB_EXPORT bool mjb_codepoint_is_pattern_white_space(mjb_codepoint codepoint) {
    return mjb_codepoint_has_property(codepoint, MJB_PR_PATTERN_WHITE_SPACE, NULL);
}
