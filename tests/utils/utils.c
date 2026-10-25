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
size_t get_string_from_codepoints(char *buffer, size_t byte_length, char *codepoints) {
    char *token, *string, *tofree;
    tofree = string = strdup(buffer != NULL ? (buffer[0] == ' ' ? buffer + 1 : buffer) : "");
    unsigned int index = 0;

    while((token = mjb_test_strsep(&string, " ")) != NULL) {
        if(strlen(token) == 0) {
            continue; // Skip empty tokens
        }

        mjb_codepoint codepoint = strtoul((const char *)token, NULL, 16);

        if(codepoint == 0) {
            continue; // Skip invalid codepoints
        }

        unsigned int encoded_size = mjb_codepoint_encode(codepoint, codepoints + index,
            byte_length - index, MJB_ENC_UTF_8);

        if(encoded_size == 0) {
            break; // Failed to encode
        }

        index += encoded_size;
    }

    codepoints[index] = '\0';
    free(tofree);

    return index;
}

// Copy a result into a NUL-terminated buffer owned by the test, then free the result.
static char *mjb_test_take_result(mjb_result *result) {
    char *copy = (char *)malloc(result->output_size + 1);

    if(copy != NULL) {
        if(result->output_size > 0) {
            memcpy(copy, result->output, result->output_size);
        }

        copy[result->output_size] = '\0';
    }

    ATT_ASSERT_STATUS(mjb_result_free(result), MJB_STATUS_OK, "Free test result");

    return copy;
}

// Take the result and attribute the caller's following assertions to mjb_map_case.
static char *run_mjb_map_case_take(mjb_result *result) {
    char *copy = mjb_test_take_result(result);
    MJB_TEST_COVERAGE(mjb_map_case);

    return copy;
}

/**
 * Call mjb_map_case and return a copy of the output string, or NULL when the call fails. Free the
 * copy with mjb_test_free.
 */
char *run_mjb_map_case(const char *buffer, size_t byte_length, mjb_map_case_type type,
    mjb_locale locale, mjb_encoding encoding) {
    mjb_result result = { NULL, 0, 0, false };

    mjb_text text = mjb_text_make(buffer, byte_length, encoding);
    if(mjb_map_case(&text, MJB_MALFORMED_STOP, type, locale, encoding, &result, NULL) !=
        MJB_STATUS_OK) {
        return NULL;
    }

    MJB_TEST_COVERAGE(mjb_map_case_into);

    mjb_buffer into = mjb_buffer_make(NULL, 0, encoding);
    mjb_status status = mjb_map_case_into(&text, MJB_MALFORMED_STOP, type, locale, &into, NULL);
    size_t required = into.size;

    ATT_ASSERT_STATUS(status, MJB_STATUS_OK, "Case into sizing matches allocating case mapping");
    ATT_ASSERT(required, result.output_size, "Case into required size matches allocated size");

    if(status != MJB_STATUS_OK || required != result.output_size) {
        return run_mjb_map_case_take(&result);
    }

    unsigned char *output = (unsigned char *)malloc(required + 1);
    ATT_ASSERT(output != NULL, true, "Allocate case into comparison buffer");

    if(output == NULL) {
        return run_mjb_map_case_take(&result);
    }

    memset(output, 0xA5, required + 1);
    into = mjb_buffer_make(output, required, encoding);
    status = mjb_map_case_into(&text, MJB_MALFORMED_STOP, type, locale, &into, NULL);

    ATT_ASSERT_STATUS(status, MJB_STATUS_OK, "Case into output matches allocating case mapping");
    ATT_ASSERT(into.size, result.output_size, "Case into written size matches allocated size");
    ATT_ASSERT(memcmp(output, result.output, result.output_size), 0,
        "Case into bytes match allocating case mapping");
    ATT_ASSERT((unsigned int)output[required], 0xA5u, "Case into does not write a terminator");

    free(output);

    return run_mjb_map_case_take(&result);
}

/**
 * Store the byte offset of every codepoint of a valid UTF-8 string, followed by its length, and
 * return the number of codepoints.
 */
size_t mjb_test_codepoint_starts(const char *buffer, size_t byte_length, size_t *starts,
    size_t capacity) {
    size_t count = 0;

    for(size_t i = 0; i < byte_length && count + 1 < capacity; ++i) {
        if(((unsigned char)buffer[i] & 0xC0) != 0x80) {
            starts[count++] = i;
        }
    }

    starts[count] = byte_length;

    return count;
}

void mjb_test_free(void *output) {
    free(output);
}

void read_test_file(const char *filename, test_file_callback callback) {
    char line[16384];
    char generated_input[1024];
    mjb_break_type expected_types[1024];
    unsigned int current_line = 1;

    FILE *file = fopen(filename, "r");

    if(file == NULL) {
        ATT_ASSERT("Not opened", "Opened file", "Valid breaking test file");

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

        for(size_t expected_i = 0; expected_i < 1024; ++expected_i) {
            expected_types[expected_i] = MJB_BT_NOT_SET;
        }

        memset(generated_input, 0, 1024);

        // × (U+00D7) = 0xC3 0x97
        // ÷ (U+00F7) = 0xC3 0xB7
        while((token = mjb_test_strsep(&string, " ")) != NULL) {
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

            mjb_codepoint codepoint = strtoul((const char *)token, NULL, 16);

            unsigned int encoded_size = mjb_codepoint_encode(codepoint,
                generated_input + generated_index, 1024 - generated_index, MJB_ENC_UTF_8);

            generated_index += encoded_size;
        }

        generated_input[generated_index] = '\0';
        (*callback)(generated_input, generated_index, current_line, expected_types);

        free(tofree);
        ++current_line;
    }

    fclose(file);
}
