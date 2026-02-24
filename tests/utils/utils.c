/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "../test.h"

/**
 * Get an UTF-8 string from a string of hex-encoded codepoints
 * Example: "0061 0062 0063", gives "abc"
 */
size_t get_string_from_codepoints(char *buffer, size_t size, char *codepoints) {
    char *token, *string, *tofree;
    tofree = string = strdup(buffer != NULL ? (buffer[0] == ' ' ? buffer + 1 : buffer) : "");
    unsigned int index = 0;

    while((token = strsep(&string, " ")) != NULL) {
        if(strlen(token) == 0) {
            continue; // Skip empty tokens
        }

        mjb_codepoint codepoint = strtoul((const char*)token, NULL, 16);

        if(codepoint == 0) {
            continue; // Skip invalid codepoints
        }

        unsigned int encoded_size = mjb_codepoint_encode(codepoint, codepoints + index,
            size - index, MJB_ENCODING_UTF_8);

        if(encoded_size == 0) {
            break; // Failed to encode
        }

        index += encoded_size;
    }

    codepoints[index] = '\0';
    free(tofree);

    return index;
}
