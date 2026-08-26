/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "mojibake.h"
#include "utf.h"

/**
 * Run a callback for each codepoint in the string.
 */
MJB_EXPORT mjb_status mjb_for_each_codepoint(const char *buffer, size_t byte_length,
    mjb_encoding encoding, mjb_malformed_policy malformed_policy,
    mjb_for_each_codepoint_fn callback, mjb_diagnostic *diagnostic) {
    if((buffer == NULL && byte_length > 0) || !mjb_malformed_policy_is_valid(malformed_policy)) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    mjb_diagnostic_reset(diagnostic);

    if(!mjb_encoding_is_valid_input(encoding)) {
        return MJB_STATUS_INVALID_ENCODING;
    }

    mjb_status status = mjb_resolve_input_byte_length(buffer, &byte_length, encoding);

    if(status != MJB_STATUS_OK) {
        return status;
    }

#ifndef __EMSCRIPTEN__
    // Emscripten uses _mjbForEachCodepointCallback.
    if(callback == NULL) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }
#endif

    mjb_codepoint codepoint = 0;
    mjb_character character;
    bool has_previous_character = false;
    bool first_character = true;

    size_t offset = 0;

    for(;;) {
        mjb_diagnostic current;
        mjb_status result = mjb_decode_next(buffer, byte_length, encoding, malformed_policy,
            &offset, &codepoint, &current);
        mjb_diagnostic_record(diagnostic, &current);

        if(result == MJB_STATUS_END_OF_INPUT) {
            break;
        }

        if(result != MJB_STATUS_OK) {
            return result;
        }

        if(has_previous_character) {
#ifdef __EMSCRIPTEN__
            bool asm_result = EM_ASM_INT(
                { return _mjbForEachCodepointCallback($0, $1); }, &character,
                first_character ? MJB_POSITION_FIRST : MJB_POSITION_NONE);

            if(!asm_result) {
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
        bool asm_result = EM_ASM_INT(
            { return _mjbForEachCodepointCallback($0, $1); }, &character,
            first_character ? MJB_POSITION_FIRST | MJB_POSITION_LAST : MJB_POSITION_LAST);

        if(!asm_result) {
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
