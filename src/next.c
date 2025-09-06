/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "mojibake.h"
#include "utf8.h"

/**
 * Return the next character from the string.
 */
MJB_EXPORT bool mjb_next_character(const char *buffer, size_t size, mjb_encoding encoding,
    mjb_next_character_fn fn) {
    if(!mjb_initialize()) {
        return false;
    }

    if(encoding != MJB_ENCODING_UTF_8) {
        return false;
    }

    if(size == 0) {
        return false;
    }

    uint8_t state = MJB_UTF8_ACCEPT;
    mjb_codepoint codepoint;
    mjb_character character;
    bool has_previous_character = false;
    bool first_character = true;

    // String buffer.
    const char *index = buffer;
    const char *end = buffer + size;

    // Loop through the string.
    for(; index < end && *index; ++index) {
        // Find next codepoint.
        state = mjb_utf8_decode_step(state, *index, &codepoint);

        if(state == MJB_UTF8_REJECT) {
            // Do nothing. The string is not well-formed.
            return false;
        }

        // Still not found a UTF-8 character, continue.
        if(state != MJB_UTF8_ACCEPT) {
            continue;
        }

        if(has_previous_character) {
#ifdef __EMSCRIPTEN__
            bool result = EM_ASM_INT({
                return _mjbNextCharacterCallback($0, $1);
            }, &character, first_character ? MJB_NEXT_CHAR_FIRST : MJB_NEXT_CHAR_NONE);

            if(!result) {
                return false;
            }
#else
            // Call the callback function.
            if(!fn(&character, first_character ? MJB_NEXT_CHAR_FIRST : MJB_NEXT_CHAR_NONE)) {
                return false;
            }
#endif

            has_previous_character = false;
            first_character = false;
        }

        // Get current character.
        if(!mjb_codepoint_character(codepoint, &character)) {
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
            return false;
        }
#else
        // Call the callback function.
        if(!fn(&character, first_character ?
            (mjb_next_character_type)(MJB_NEXT_CHAR_FIRST | MJB_NEXT_CHAR_LAST) :
            MJB_NEXT_CHAR_LAST)) {
            return false;
        }
#endif
    }

    return true;
}
