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

void read_test_file(const char *filename, test_file_callback callback) {
    char line[16384];
    char generated_input[1024];
    mjb_break_type expected_types[1024];
    unsigned int current_line = 1;

    FILE *file = fopen(filename, "r");

    if(file == NULL) {
        ATT_ASSERT("Not opened", "Opened file", "Valid breaking test file")

        return;
    }

    while(fgets(line, 16384, file)) {
        if(line[0] == '#' || strnlen(line, 16384) <= 1) {
            ++current_line;

            continue;
        }

        char *token, *string, *tofree;
        tofree = string = strdup(line + 3);
        unsigned int types_i = 0;
        unsigned int i = 0;
        unsigned int generated_index = 0;
        memset(expected_types, MJB_LBP_NOT_SET, 1024);
        memset(generated_input, 0, 1024);
        bool skip_line = false;

        // × (U+00D7) = 0xC3 0x97
        // ÷ (U+00F7) = 0xC3 0xB7
        while((token = strsep(&string, " ")) != NULL) {
            if(token == NULL || token[0] == '\0') {
                i = 0;

                continue;
            }

            // Odd index means break type
            if(i++ % 2 != 0) {
                if((unsigned char)token[1] == 0xB7) { // ÷
                    expected_types[types_i++] = MJB_BT_ALLOWED;

                    if((unsigned char)token[2] == 0x09) { // Tab # comment until next line
                        break;
                    }

                    continue;
                } else if((unsigned char)token[1] == 0x97) {
                    expected_types[types_i++] = MJB_BT_NO_BREAK;

                    continue;
                }
            }

            mjb_codepoint codepoint = strtoul((const char*)(token), NULL, 16);

#if !MJB_DANGEROUSLY_ALLOW_EMBEDDED_NULLS
            if(codepoint == 0) {
                free(tofree);
                ++current_line;
                skip_line = true;

                break;
            }
#endif
            unsigned int encoded_size = mjb_codepoint_encode(codepoint, generated_input +
                generated_index, 1024 - generated_index, MJB_ENCODING_UTF_8);

            generated_index += encoded_size;
        }

        if(skip_line) {
            continue;
        }

        generated_input[generated_index] = '\0';
        (*callback)(generated_input, generated_index, current_line, expected_types);

        free(tofree);
        ++current_line;
    }

    fclose(file);
}
