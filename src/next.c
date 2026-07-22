/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "mojibake.h"
#include "utf.h"

/**
 * Run a callback for each character in the string.
 */
MJB_EXPORT mjb_status mjb_for_each_character(const char *buffer, size_t byte_length,
    mjb_encoding encoding, mjb_for_each_character_fn callback) {
    if(buffer == NULL || byte_length == 0) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    mjb_status status = mjb_resolve_input_byte_length(buffer, &byte_length, encoding);

    if(status != MJB_STATUS_OK) {
        return status;
    }

    if(byte_length == 0) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

#ifndef __EMSCRIPTEN__
    // Emscripten uses _mjbForEachCharacterCallback.
    if(callback == NULL) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }
#endif

    uint8_t state = MJB_UTF_ACCEPT;
    bool in_error = false;
    mjb_codepoint codepoint;
    mjb_character character;
    bool has_previous_character = false;
    bool first_character = true;

    // Loop through the string.
    for(size_t i = 0; i < byte_length;) {
        // Find next codepoint.
        mjb_decode_result result = mjb_next_codepoint(buffer, byte_length, &state, &i, encoding,
            &codepoint, &in_error);

        if(result == MJB_DECODE_END) {
            break;
        }

        if(result == MJB_DECODE_INCOMPLETE) {
            continue;
        }

        // result is MJB_DECODE_OK or MJB_DECODE_ERROR (both have valid codepoint)

        if(has_previous_character) {
#ifdef __EMSCRIPTEN__
            bool result = EM_ASM_INT(
                { return _mjbForEachCharacterCallback($0, $1); }, &character,
                first_character ? MJB_POSITION_FIRST : MJB_POSITION_NONE);

            if(!result) {
                return MJB_STATUS_CALLBACK_STOPPED;
            }
#else
            // Call the callback function.
            if(!callback(&character, first_character ? MJB_POSITION_FIRST : MJB_POSITION_NONE)) {
                return MJB_STATUS_CALLBACK_STOPPED;
            }
#endif

            has_previous_character = false;
            first_character = false;
        }

        // Get current character.
        if(mjb_codepoint_info(codepoint, &character) != MJB_STATUS_OK) {
            continue;
        }

        has_previous_character = true;
    }

    if(has_previous_character) {
#ifdef __EMSCRIPTEN__
        bool result = EM_ASM_INT(
            { return _mjbForEachCharacterCallback($0, $1); }, &character,
            first_character ? MJB_POSITION_FIRST | MJB_POSITION_LAST : MJB_POSITION_LAST);

        if(!result) {
            return MJB_STATUS_CALLBACK_STOPPED;
        }
#else
        // Call the callback function.
        if(!callback(&character,
               first_character ? (mjb_character_position)(MJB_POSITION_FIRST | MJB_POSITION_LAST) :
                                 MJB_POSITION_LAST)) {
            return MJB_STATUS_CALLBACK_STOPPED;
        }
#endif
    }

    return MJB_STATUS_OK;
}
