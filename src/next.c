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
MJB_EXPORT bool mjb_next_character(const char *buffer, size_t size, mjb_encoding encoding,
    mjb_next_character_fn fn) {
    if(!mjb_initialize()) {
        return false;
    }

    if(size == 0) {
        return false;
    }

    uint8_t state = MJB_UTF_ACCEPT;
    bool in_error = false;
    mjb_codepoint codepoint;
    mjb_character character;
    bool has_previous_character = false;
    bool first_character = true;

    // Loop through the string.
    for(size_t i = 0; i < size;) {
        // Find next codepoint.
        mjb_decode_result result = mjb_next_codepoint(buffer, size, &state, &i, encoding,
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
