/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"

static char *trim_ascii(char *text) {
    while(*text == ' ' || *text == '\t' || *text == '\r' || *text == '\n') {
        ++text;
    }

    char *end = text + strlen(text);

    while(end > text &&
        (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r' || end[-1] == '\n')) {
        *--end = '\0';
    }

    return text;
}

static bool parse_emoji_test_line(char *line, mjb_codepoint *codepoint, char *status,
    size_t status_size) {
    char *comment = strchr(line, '#');

    if(comment) {
        *comment = '\0';
    }

    char *semi = strchr(line, ';');

    if(!semi) {
        return false;
    }

    *semi = '\0';

    char *codepoints = trim_ascii(line);
    char *status_field = trim_ascii(semi + 1);

    if(!codepoints[0] || !status_field[0]) {
        return false;
    }

    char *end;
    unsigned long cp = strtoul(codepoints, &end, 16);

    if(end == codepoints || cp > MJB_CODEPOINT_MAX) {
        return false;
    }

    end = trim_ascii(end);

    if(*end != '\0') {
        return false;
    }

    size_t i = 0;

    while(status_field[i] && status_field[i] != ' ' && status_field[i] != '\t' &&
        i + 1 < status_size) {
        status[i] = status_field[i];
        ++i;
    }

    status[i] = '\0';
    *codepoint = (mjb_codepoint)cp;

    return status[0] != '\0';
}

static void run_emoji_test_file(const char *filename) {
    FILE *file = fopen(filename, "r");

    if(!file) {
        ATT_ASSERT("Not opened", "Opened file", "emoji-test.txt")

        return;
    }

    char line[1024];
    unsigned int current_line = 0;
    unsigned int tested = 0;
    unsigned int failures = 0;

    while(fgets(line, (int)sizeof(line), file)) {
        ++current_line;

        mjb_codepoint codepoint;
        char status[32];

        if(!parse_emoji_test_line(line, &codepoint, status, sizeof(status))) {
            continue;
        }

        mjb_emoji_properties emoji;
        bool found = mjb_codepoint_emoji(codepoint, &emoji);
        bool is_component = strcmp(status, "component") == 0;
        bool ok = found && emoji.codepoint == codepoint;

        ok = ok && (is_component ? emoji.component : emoji.emoji);

        if(!ok) {
            ++failures;
            char test_name[128];
            snprintf(test_name, sizeof(test_name), "emoji-test.txt line %u", current_line);
            ATT_ASSERT(0, 1, test_name)

            if(is_exit_on_error()) {
                break;
            }
        } else {
            // CURRENT_ASSERT mjb_codepoint_emoji
            // CURRENT_COUNT 1400
            ATT_ASSERT(0, 0, "emoji-test.txt single-codepoint entry")
        }

        ++tested;
    }

    fclose(file);

    char summary[128];
    snprintf(summary, sizeof(summary), "emoji-test.txt: %u/%u single-codepoint rows passed",
        tested - failures, tested);
    ATT_ASSERT(tested > 0, true, "emoji-test.txt has single-codepoint rows")
    ATT_ASSERT(failures, 0u, summary)
}

void *test_emoji(void *arg) {
    mjb_emoji_properties emoji;

    ATT_ASSERT(mjb_codepoint_emoji(MJB_CODEPOINT_MAX + 1, &emoji), false, "Invalid codepoint")
    ATT_ASSERT(mjb_codepoint_emoji(0x23, NULL), false, "NULL emoji pointer")

    // CURRENT_ASSERT mjb_codepoint_emoji
    ATT_ASSERT(mjb_codepoint_emoji(0x0, &emoji), false, "NULL")

    ATT_ASSERT(mjb_codepoint_emoji(0x23, &emoji), true, "U+23: #")
    ATT_ASSERT(emoji.codepoint, 0x23, "U+23: codepoint")
    ATT_ASSERT(emoji.emoji, true, "U+23: emoji")
    ATT_ASSERT(emoji.presentation, false, "U+23: presentation")
    ATT_ASSERT(emoji.modifier, false, "U+23: modifier")
    ATT_ASSERT(emoji.modifier_base, false, "U+23: modifier base")
    ATT_ASSERT(emoji.component, true, "U+23: component")
    ATT_ASSERT(emoji.extended_pictographic, false, "U+23: extended pictographic")

    ATT_ASSERT(mjb_codepoint_emoji(0x1F600, &emoji), true, "U+1F600: 😀")
    ATT_ASSERT(emoji.codepoint, 0x1F600, "U+1F600: codepoint")
    ATT_ASSERT(emoji.emoji, true, "U+1F600: emoji")
    ATT_ASSERT(emoji.presentation, true, "U+1F600: presentation")
    ATT_ASSERT(emoji.modifier, false, "U+1F600: modifier")
    ATT_ASSERT(emoji.modifier_base, false, "U+1F600: modifier base")
    ATT_ASSERT(emoji.component, false, "U+1F600: component")
    ATT_ASSERT(emoji.extended_pictographic, true, "U+1F600: extended pictographic")

    run_emoji_test_file("./utils/generate/unicode-data/emoji/emoji-test.txt");

    return NULL;
}
