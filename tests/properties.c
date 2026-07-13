/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"

typedef struct test_script_alias {
    char name[8];
    mjb_script script;
} test_script_alias;

static char *property_field_trim(char *field) {
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

static size_t property_split(char *line, char **fields, size_t capacity) {
    char *comment = strchr(line, '#');
    if(comment != NULL)
        *comment = '\0';
    size_t count = 0;
    char *field = line;

    while(count < capacity) {
        char *separator = strchr(field, ';');

        if(separator != NULL) {
            *separator = '\0';
        }

        fields[count++] = property_field_trim(field);

        if(separator == NULL) {
            break;
        }

        field = separator + 1;
    }
    return count;
}

static size_t load_script_aliases(const char *filename, test_script_alias *aliases,
    size_t capacity) {
    FILE *file = fopen(filename, "r");
    if(file == NULL)
        return 0;
    char line[1024];
    size_t count = 0;

    while(fgets(line, sizeof(line), file) != NULL && count < capacity) {
        char *fields[8];
        size_t field_count = property_split(line, fields, 8);

        if(field_count < 3 || strcmp(fields[0], "sc") != 0) {
            continue;
        }

        snprintf(aliases[count].name, sizeof(aliases[count].name), "%s", fields[1]);
        aliases[count].script = (mjb_script)(count + 1);

        ++count;
    }

    fclose(file);

    return count;
}

static mjb_script find_script_alias(const test_script_alias *aliases, size_t count,
    const char *name) {
    for(size_t i = 0; i < count; ++i) {
        if(strcmp(aliases[i].name, name) == 0) {
            return aliases[i].script;
        }
    }

    return MJB_SC_NOT_SET;
}

static void test_script_extensions_file(const char *filename, const test_script_alias *aliases,
    size_t alias_count) {
    FILE *file = fopen(filename, "r");
    ATT_ASSERT(file != NULL, true, "Open ScriptExtensions.txt")
    if(file == NULL) {
        return;
    }

    char line[1024];
    unsigned int tested = 0;

    while(fgets(line, sizeof(line), file) != NULL) {
        char *fields[3];
        size_t field_count = property_split(line, fields, 3);

        if(field_count < 2 || fields[0][0] == '\0') {
            continue;
        }

        char *range_separator = strstr(fields[0], "..");

        if(range_separator != NULL) {
            *range_separator = '\0';
        }

        mjb_codepoint codepoint = (mjb_codepoint)strtoul(fields[0], NULL, 16);
        mjb_script expected[32];
        size_t expected_count = 0;

        for(char *name = strtok(fields[1], " "); name != NULL; name = strtok(NULL, " ")) {
            expected[expected_count++] = find_script_alias(aliases, alias_count, name);
        }

        mjb_script actual[32];
        size_t actual_count = 32;

        ATT_ASSERT_STATUS(mjb_codepoint_script_extensions(codepoint, actual, &actual_count),
            MJB_STATUS_OK, "ScriptExtensions.txt lookup")
        ATT_ASSERT(actual_count, expected_count, "ScriptExtensions.txt count")
        ATT_ASSERT((int)memcmp(actual, expected, expected_count * sizeof(mjb_script)), 0,
            "ScriptExtensions.txt scripts")

        ++tested;
    }

    fclose(file);

    ATT_ASSERT(tested > 0, true, "ScriptExtensions.txt ranges tested")
}

int test_properties(void *arg) {
    bool binary = false;
    int32_t enumerated = -1;

    MJB_TEST_COVERAGE(mjb_codepoint_script_extensions);

    size_t script_count = 0;

    ATT_ASSERT_STATUS(mjb_codepoint_script_extensions(0x30FC, NULL, &script_count), MJB_STATUS_OK,
        "Query Script_Extensions count")
    ATT_ASSERT(script_count, 2u, "U+30FC has two script extensions")

    mjb_script scripts[8];
    size_t script_capacity = 1;

    ATT_ASSERT_STATUS(mjb_codepoint_script_extensions(0x30FC, scripts, &script_capacity),
        MJB_STATUS_OUTPUT_TOO_SMALL, "Script_Extensions small buffer")
    ATT_ASSERT(script_capacity, 2u, "Script_Extensions required count")

    script_capacity = 8;

    ATT_ASSERT_STATUS(mjb_codepoint_script_extensions(0x41, scripts, &script_capacity),
        MJB_STATUS_OK, "Script_Extensions Script fallback")
    ATT_ASSERT(script_capacity, 1u, "Fallback contains one script")
    ATT_ASSERT((int)scripts[0], MJB_SC_LATN, "Fallback script is Latin")
    ATT_ASSERT_STATUS(mjb_codepoint_script_extensions(0x41, scripts, NULL),
        MJB_STATUS_INVALID_ARGUMENT, "Script_Extensions rejects NULL count")

    char aliases_path[512];
    char extensions_path[512];

    snprintf(aliases_path, sizeof(aliases_path),
        "%s/utils/generate/unicode-data/UCD/PropertyValueAliases.txt", MJB_TEST_SOURCE_DIR);
    snprintf(extensions_path, sizeof(extensions_path),
        "%s/utils/generate/unicode-data/UCD/ScriptExtensions.txt", MJB_TEST_SOURCE_DIR);

    test_script_alias aliases[MJB_SC_COUNT];
    size_t alias_count = load_script_aliases(aliases_path, aliases, MJB_SC_COUNT);

    ATT_ASSERT(alias_count, (size_t)MJB_SC_COUNT - 1, "All Script aliases loaded")

    test_script_extensions_file(extensions_path, aliases, alias_count);

    MJB_TEST_COVERAGE(mjb_codepoint_property_binary);
    ATT_ASSERT_STATUS(mjb_codepoint_property_binary(0x41, MJB_PR_ALPHABETIC, &binary),
        MJB_STATUS_OK, "Typed binary property present")
    ATT_ASSERT(binary, true, "U+0041 Alphabetic is true")
    ATT_ASSERT_STATUS(mjb_codepoint_property_binary(0x20, MJB_PR_ALPHABETIC, &binary),
        MJB_STATUS_OK, "Typed binary property absent")
    ATT_ASSERT(binary, false, "U+0020 Alphabetic is false")
    ATT_ASSERT_STATUS(mjb_codepoint_property_binary(0x41, MJB_PR_SCRIPT, &binary),
        MJB_STATUS_INVALID_ARGUMENT, "Binary getter rejects enumerated property")
    ATT_ASSERT_STATUS(mjb_codepoint_property_binary(0x41, MJB_PR_ALPHABETIC, NULL),
        MJB_STATUS_INVALID_ARGUMENT, "Binary getter rejects NULL output")

    MJB_TEST_COVERAGE(mjb_codepoint_property_int);
    ATT_ASSERT_STATUS(mjb_codepoint_property_int(0x41, MJB_PR_SCRIPT, &enumerated), MJB_STATUS_OK,
        "Typed enumerated property present")
    ATT_ASSERT(enumerated, MJB_SC_LATN, "U+0041 Script is Latin")
    ATT_ASSERT_STATUS(mjb_codepoint_property_int(0x41, MJB_PR_ALPHABETIC, &enumerated),
        MJB_STATUS_INVALID_ARGUMENT, "Enumerated getter rejects binary property")
    ATT_ASSERT_STATUS(mjb_codepoint_property_int(0x41, MJB_PR_SCRIPT, NULL),
        MJB_STATUS_INVALID_ARGUMENT, "Enumerated getter rejects NULL output")
    ATT_ASSERT_STATUS(mjb_codepoint_property_int(MJB_CODEPOINT_MAX + 1, MJB_PR_SCRIPT, &enumerated),
        MJB_STATUS_INVALID_ARGUMENT, "Typed getter rejects invalid codepoint")

    // mjb_codepoint_script
    ATT_ASSERT((int)mjb_codepoint_script(MJB_CODEPOINT_MAX + 1), MJB_SC_ZZZZ,
        "Invalid codepoint script is Unknown")
    ATT_ASSERT((int)mjb_codepoint_script(0x41), MJB_SC_LATN, "U+0041 'A' is Latin")
    ATT_ASSERT((int)mjb_codepoint_script(0x0391), MJB_SC_GREK, "U+0391 'A' is Greek")
    ATT_ASSERT((int)mjb_codepoint_script(0x0410), MJB_SC_CYRL, "U+0410 'A' is Cyrillic")
    ATT_ASSERT((int)mjb_codepoint_script(0x05D0), MJB_SC_HEBR, "U+05D0 Alef is Hebrew")
    ATT_ASSERT((int)mjb_codepoint_script(0x0600), MJB_SC_ARAB, "U+0600 is Arabic")
    ATT_ASSERT((int)mjb_codepoint_script(0x4E00), MJB_SC_HANI, "U+4E00 CJK is Han")
    ATT_ASSERT((int)mjb_codepoint_script(0xAC00), MJB_SC_HANG, "U+AC00 is Hangul")
    ATT_ASSERT((int)mjb_codepoint_script(0x0030), MJB_SC_ZYYY, "U+0030 '0' is Common")

    ATT_ASSERT(mjb_property_name(MJB_PR_CASED), "Cased",
        "Property name for MJB_PR_CASED is 'Cased'")
    ATT_ASSERT(mjb_property_name((mjb_property)MJB_PR_COUNT), "Unknown",
        "Property name with invalid number is 'Unknown'")

    return 0;
}
