/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "mojibake.h"
#include "utf.h"

/**
 * Return the next character from the string.
 */
MJB_EXPORT mjb_status mjb_next_character(const char *buffer, size_t byte_length, mjb_encoding encoding,
    mjb_next_character_fn fn) {
    if(buffer == NULL || byte_length == 0) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

#ifndef __EMSCRIPTEN__
    // Emscripten uses _mjbNextCharacterCallback.
    if(fn == NULL) {
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
            bool result = EM_ASM_INT({
                return _mjbNextCharacterCallback($0, $1);
            }, &character, first_character ? MJB_NEXT_CHAR_FIRST : MJB_NEXT_CHAR_NONE);

            if(!result) {
                return MJB_STATUS_CALLBACK_STOPPED;
            }
#else
            // Call the callback function.
            if(!fn(&character, first_character ? MJB_NEXT_CHAR_FIRST : MJB_NEXT_CHAR_NONE)) {
                return MJB_STATUS_CALLBACK_STOPPED;
            }
#endif

            has_previous_character = false;
            first_character = false;
        }

        // Get current character.
        if(mjb_codepoint_character(codepoint, &character) != MJB_STATUS_OK) {
            continue;
        }

        has_previous_character = true;
    }

    if(has_previous_character) {
#ifdef __EMSCRIPTEN__
        bool result = EM_ASM_INT({
            return _mjbNextCharacterCallback($0, $1);
        }, &character, first_character ? MJB_NEXT_CHAR_FIRST | MJB_NEXT_CHAR_LAST : MJB_NEXT_CHAR_LAST);

        if(!result) {
            return MJB_STATUS_CALLBACK_STOPPED;
        }
#else
        // Call the callback function.
        if(!fn(&character, first_character ?
            (mjb_next_character_type)(MJB_NEXT_CHAR_FIRST | MJB_NEXT_CHAR_LAST) :
            MJB_NEXT_CHAR_LAST)) {
            return MJB_STATUS_CALLBACK_STOPPED;
        }
#endif
    }

    return MJB_STATUS_OK;
}
