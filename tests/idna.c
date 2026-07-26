/**
 * The Mojibake library
 *
 * UTS #46 Unicode IDNA Compatibility Processing tests.
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"

static char *idna_trim(char *field) {
    while(*field == ' ' || *field == '\t') {
        ++field;
    }

    char *end = field + strlen(field);

    while(end > field &&
        (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r' || end[-1] == '\n')) {
        *--end = '\0';
    }

    return field;
}

static int idna_hex_digit(char character) {
    if(character >= '0' && character <= '9') {
        return character - '0';
    }

    if(character >= 'A' && character <= 'F') {
        return character - 'A' + 10;
    }

    if(character >= 'a' && character <= 'f') {
        return character - 'a' + 10;
    }

    return -1;
}

static bool idna_parse_hex(const char *text, size_t length, mjb_codepoint *codepoint) {
    if(length == 0) {
        return false;
    }

    mjb_codepoint value = 0;

    for(size_t i = 0; i < length; ++i) {
        int digit = idna_hex_digit(text[i]);

        if(digit < 0 || value > (MJB_CODEPOINT_MAX - (mjb_codepoint)digit) / 16) {
            return false;
        }

        value = value * 16 + (mjb_codepoint)digit;
    }

    if(!mjb_codepoint_is_valid(value)) {
        return false;
    }

    *codepoint = value;
    return true;
}

static bool idna_parse_field(char *field, char *output, size_t output_capacity,
    size_t *output_size) {
    field = idna_trim(field);
    *output_size = 0;

    if(strcmp(field, "\"\"") == 0) {
        return true;
    }

    for(size_t i = 0; field[i] != '\0';) {
        mjb_codepoint codepoint;
        size_t escaped_length = 0;

        if(field[i] == '\\' && field[i + 1] == 'u') {
            escaped_length = 4;

            if(strlen(field + i + 2) < escaped_length ||
                !idna_parse_hex(field + i + 2, escaped_length, &codepoint)) {
                return false;
            }

            i += 2 + escaped_length;
        } else if(field[i] == '\\' && field[i + 1] == 'x' && field[i + 2] == '{') {
            const char *end = strchr(field + i + 3, '}');

            if(end == NULL ||
                !idna_parse_hex(field + i + 3, (size_t)(end - field - i - 3), &codepoint)) {
                return false;
            }

            i = (size_t)(end - field) + 1;
        } else {
            if(*output_size + 1 >= output_capacity) {
                return false;
            }

            output[(*output_size)++] = field[i++];
            continue;
        }

        unsigned int encoded = mjb_codepoint_encode(codepoint, output + *output_size,
            output_capacity - *output_size, MJB_ENC_UTF_8);

        if(encoded == 0) {
            return false;
        }

        *output_size += encoded;
    }

    if(*output_size < output_capacity) {
        output[*output_size] = '\0';
    }

    return true;
}

static void idna_print_bytes(const char *label, const char *buffer, size_t byte_length) {
    printf("%s", label);

    for(size_t i = 0; i < byte_length; ++i) {
        printf("%02X", (unsigned int)(uint8_t)buffer[i]);
    }

    puts("");
}

static bool idna_check_result(const mjb_result *result, const char *expected,
    size_t expected_size) {
    return result->output_size == expected_size &&
        (expected_size == 0 || memcmp(result->output, expected, expected_size) == 0);
}

static void test_idna_conformance(void) {
    FILE *file = fopen("./utils/generate/unicode-data/idna/IdnaTestV2.txt", "r");

    if(file == NULL) {
        ATT_ASSERT("Not opened", "Opened file", "Valid IdnaTestV2.txt data file")
        return;
    }

    char line[4096];
    char test_name[128];
    unsigned int line_number = 0;
    unsigned int tested = 0;
    unsigned int skipped = 0;
    unsigned int failures = 0;
    unsigned int printed = 0;

    while(fgets(line, (int)sizeof(line), file) != NULL) {
        ++line_number;
        char *comment = strchr(line, '#');

        if(comment != NULL) {
            *comment = '\0';
        }

        char *fields[7] = { NULL };
        char *current = line;

        for(size_t i = 0; i < 7; ++i) {
            fields[i] = mjb_test_strsep(&current, ";");

            if(fields[i] == NULL) {
                break;
            }
        }

        if(fields[6] == NULL) {
            continue;
        }

        char source[4096];
        char expected_unicode[4096];
        char expected_ascii[4096];
        size_t source_size;
        size_t expected_unicode_size;
        size_t expected_ascii_size;

        if(!idna_parse_field(fields[0], source, sizeof(source), &source_size)) {
            ++skipped;
            continue;
        }

        if(*idna_trim(fields[1]) == '\0') {
            memcpy(expected_unicode, source, source_size);
            expected_unicode_size = source_size;
        } else if(!idna_parse_field(fields[1], expected_unicode, sizeof(expected_unicode),
                      &expected_unicode_size)) {
            ++skipped;
            continue;
        }

        if(*idna_trim(fields[3]) == '\0') {
            memcpy(expected_ascii, expected_unicode, expected_unicode_size);
            expected_ascii_size = expected_unicode_size;
        } else if(!idna_parse_field(fields[3], expected_ascii, sizeof(expected_ascii),
                      &expected_ascii_size)) {
            ++skipped;
            continue;
        }

        bool unicode_error = *idna_trim(fields[2]) != '\0' &&
            strcmp(idna_trim(fields[2]), "[]") != 0;
        bool ascii_error = *idna_trim(fields[4]) == '\0' ? unicode_error :
                                                           strcmp(idna_trim(fields[4]), "[]") != 0;
        mjb_result unicode_result = { NULL, 0, false };
        mjb_result ascii_result = { NULL, 0, false };
        mjb_idna_info unicode_info = { 0 };
        mjb_idna_info ascii_info = { 0 };

        MJB_TEST_COVERAGE(mjb_idna_to_unicode);
        mjb_status unicode_status = mjb_idna_to_unicode(source, source_size, MJB_ENC_UTF_8,
            MJB_ENC_UTF_8, &unicode_info, &unicode_result);

        bool unicode_matches = unicode_status == MJB_STATUS_OK &&
            idna_check_result(&unicode_result, expected_unicode, expected_unicode_size) &&
            (unicode_info.errors != MJB_IDNA_ERROR_NONE) == unicode_error;

        snprintf(test_name, 128, "mjb_idna_to_unicode #%u", line_number);
        ATT_ASSERT(unicode_matches, true, test_name)

        MJB_TEST_COVERAGE(mjb_idna_to_ascii);
        mjb_status ascii_status = mjb_idna_to_ascii(source, source_size, MJB_ENC_UTF_8,
            MJB_ENC_UTF_8, &ascii_info, &ascii_result);
        bool ascii_matches = ascii_status == MJB_STATUS_OK &&
            idna_check_result(&ascii_result, expected_ascii, expected_ascii_size) &&
            (ascii_info.errors != MJB_IDNA_ERROR_NONE) == ascii_error;

        snprintf(test_name, 128, "mjb_idna_to_ascii #%u", line_number);
        ATT_ASSERT(ascii_matches, true, test_name)

        if(unicode_status == MJB_STATUS_OK) {
            mjb_result_free(&unicode_result);
        }

        if(ascii_status == MJB_STATUS_OK) {
            mjb_result_free(&ascii_result);
        }

        ++tested;
    }

    fclose(file);

    ATT_ASSERT(skipped, 2u, "Only ill-formed surrogate test cases skipped")
}

static void test_idna_api(void) {
    const char *unicode = "b\xC3\xBC"
                          "cher.de";
    const char *ascii = "xn--bcher-kva.de";
    mjb_idna_info info;
    mjb_result result;

    ATT_ASSERT_STATUS(mjb_idna_to_ascii(unicode, 10, MJB_ENC_UTF_8, MJB_ENC_UTF_8, &info,
                          &result),
        MJB_STATUS_OK, "IDNA ToASCII succeeds")
    ATT_ASSERT(info.errors, (uint32_t)MJB_IDNA_ERROR_NONE, "IDNA ToASCII is valid")
    ATT_ASSERT(result.output_size, strlen(ascii), "IDNA ToASCII size")
    ATT_ASSERT((int)memcmp(result.output, ascii, result.output_size), 0, "IDNA ToASCII Punycode")
    ATT_ASSERT_STATUS(mjb_result_free(&result), MJB_STATUS_OK, "IDNA ToASCII result frees")

    ATT_ASSERT_STATUS(mjb_idna_to_unicode(ascii, strlen(ascii), MJB_ENC_UTF_8, MJB_ENC_UTF_8,
                          &info, &result),
        MJB_STATUS_OK, "IDNA ToUnicode succeeds")
    ATT_ASSERT(info.errors, (uint32_t)MJB_IDNA_ERROR_NONE, "IDNA ToUnicode is valid")
    ATT_ASSERT(result.output_size, (size_t)10, "IDNA ToUnicode size")
    ATT_ASSERT((int)memcmp(result.output, unicode, result.output_size), 0,
        "IDNA ToUnicode decodes Punycode")
    ATT_ASSERT_STATUS(mjb_result_free(&result), MJB_STATUS_OK, "IDNA ToUnicode result frees")

    const char utf16le[] = { 'b', 0, (char)0xFC, 0, 'c', 0, 'h', 0, 'e', 0, 'r', 0, '.', 0, 'd', 0,
        'e', 0 };
    ATT_ASSERT_STATUS(mjb_idna_to_unicode(ascii, strlen(ascii), MJB_ENC_ASCII,
                          MJB_ENC_UTF_16LE, &info, &result),
        MJB_STATUS_OK, "IDNA ToUnicode converts its output encoding")
    ATT_ASSERT(result.output_size, sizeof(utf16le), "IDNA UTF-16LE output size")
    ATT_ASSERT((int)memcmp(result.output, utf16le, sizeof(utf16le)), 0,
        "IDNA UTF-16LE output")
    ATT_ASSERT_STATUS(mjb_result_free(&result), MJB_STATUS_OK, "IDNA UTF-16LE result frees")

    size_t output_size = 0;
    ATT_ASSERT_STATUS(mjb_idna_to_ascii_into(unicode, 10, MJB_ENC_UTF_8, MJB_ENC_UTF_8, &info,
                          NULL, &output_size),
        MJB_STATUS_OK, "IDNA ToASCII into measures")
    ATT_ASSERT(output_size, strlen(ascii), "IDNA ToASCII into measured size")

    char output[32];
    memset(output, '#', sizeof(output));
    size_t small_size = output_size - 1;
    ATT_ASSERT_STATUS(mjb_idna_to_ascii_into(unicode, 10, MJB_ENC_UTF_8, MJB_ENC_UTF_8, &info,
                          output, &small_size),
        MJB_STATUS_OUTPUT_TOO_SMALL, "IDNA ToASCII into reports small buffer")
    ATT_ASSERT(small_size, output_size, "IDNA ToASCII into returns required size")
    ATT_ASSERT(output[0], '#', "IDNA ToASCII into leaves small buffer unchanged")

    ATT_ASSERT_STATUS(mjb_idna_to_ascii_into(unicode, 10, MJB_ENC_UTF_8, MJB_ENC_UTF_8, &info,
                          output, &output_size),
        MJB_STATUS_OK, "IDNA ToASCII into writes")
    ATT_ASSERT((int)memcmp(output, ascii, output_size), 0, "IDNA ToASCII into output")
    ATT_ASSERT(output[output_size], '#', "IDNA ToASCII into omits terminator")

    memset(output, '#', sizeof(output));
    output_size = sizeof(output);
    MJB_TEST_COVERAGE(mjb_idna_to_unicode_into);
    ATT_ASSERT_STATUS(mjb_idna_to_unicode_into(ascii, strlen(ascii), MJB_ENC_ASCII, MJB_ENC_UTF_8,
                          &info, output, &output_size),
        MJB_STATUS_OK, "IDNA ToUnicode into writes")
    ATT_ASSERT(output_size, (size_t)10, "IDNA ToUnicode into output size")
    ATT_ASSERT((int)memcmp(output, unicode, output_size), 0, "IDNA ToUnicode into output")
    ATT_ASSERT(output[output_size], '#', "IDNA ToUnicode into omits terminator")

    ATT_ASSERT_STATUS(mjb_idna_to_ascii("a..b", 4, MJB_ENC_UTF_8, MJB_ENC_UTF_8, &info, &result),
        MJB_STATUS_OK, "IDNA produces output with validation errors")
    ATT_ASSERT((info.errors & MJB_IDNA_ERROR_EMPTY_LABEL) != 0, true,
        "IDNA reports an empty label")
    ATT_ASSERT_STATUS(mjb_result_free(&result), MJB_STATUS_OK, "IDNA errored result frees")

    const char embedded_nul[] = { 'a', '\0', 'b' };
    ATT_ASSERT_STATUS(mjb_idna_to_unicode(embedded_nul, sizeof(embedded_nul), MJB_ENC_UTF_8,
                          MJB_ENC_UTF_8, &info, &result),
        MJB_STATUS_OK, "IDNA preserves an embedded NUL with an explicit byte length")
    ATT_ASSERT(result.output_size, sizeof(embedded_nul), "IDNA embedded-NUL output size")
    ATT_ASSERT((int)memcmp(result.output, embedded_nul, sizeof(embedded_nul)), 0,
        "IDNA embedded-NUL output")
    ATT_ASSERT((info.errors & MJB_IDNA_ERROR_STD3) != 0, true,
        "IDNA reports an embedded NUL as a STD3 violation")
    ATT_ASSERT_STATUS(mjb_result_free(&result), MJB_STATUS_OK, "IDNA embedded-NUL result frees")

    const char malformed[] = { (char)0xC3 };
    ATT_ASSERT_STATUS(mjb_idna_to_unicode(malformed, sizeof(malformed), MJB_ENC_UTF_8,
                          MJB_ENC_UTF_8, &info, &result),
        MJB_STATUS_MALFORMED_INPUT, "IDNA rejects malformed UTF-8")
    const char non_ascii[] = { (char)0xC3, (char)0xBC };
    ATT_ASSERT_STATUS(mjb_idna_to_unicode(non_ascii, sizeof(non_ascii), MJB_ENC_ASCII,
                          MJB_ENC_UTF_8, &info, &result),
        MJB_STATUS_MALFORMED_INPUT, "IDNA rejects non-ASCII input declared as ASCII")
    ATT_ASSERT_STATUS(mjb_idna_to_ascii(NULL, 1, MJB_ENC_UTF_8, MJB_ENC_UTF_8, &info, &result),
        MJB_STATUS_INVALID_ARGUMENT, "IDNA rejects a NULL input with nonzero size")
    ATT_ASSERT_STATUS(mjb_idna_to_ascii("a", 1, MJB_ENC_UTF_8, MJB_ENC_UTF_8, NULL, &result),
        MJB_STATUS_INVALID_ARGUMENT, "IDNA requires validation info")
    ATT_ASSERT_STATUS(mjb_idna_to_ascii("a", 1, MJB_ENC_UTF_8, MJB_ENC_UTF_8, &info, NULL),
        MJB_STATUS_INVALID_ARGUMENT, "IDNA requires a result")
}

int test_idna(void *arg) {
    (void)arg;
    test_idna_api();
    test_idna_conformance();

    return 0;
}
