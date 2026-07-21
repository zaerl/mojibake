/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"

#define MJB_TEST_EMOJI_MAX_CODEPOINTS 32

static char *trim_ascii(char *text) {
    while(*text == ' ' || *text == '\t' || *text == '\r' || *text == '\n') {
        ++text;
    }

    char *end = text + strlen(text);

    while(end > text && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r' || end[-1] == '\n')) {
        *--end = '\0';
    }

    return text;
}

static bool parse_emoji_test_line(char *line, mjb_codepoint *codepoints, size_t *codepoint_count,
    char *status, size_t status_size) {
    char *comment = strchr(line, '#');

    if(comment) {
        *comment = '\0';
    }

    char *semi = strchr(line, ';');

    if(!semi) {
        return false;
    }

    *semi = '\0';

    char *codepoint_text = trim_ascii(line);
    char *status_field = trim_ascii(semi + 1);

    if(!codepoint_text[0] || !status_field[0]) {
        return false;
    }

    *codepoint_count = 0;
    char *cursor = codepoint_text;

    while(*cursor != '\0') {
        while(*cursor == ' ' || *cursor == '\t') {
            ++cursor;
        }

        if(*cursor == '\0') {
            break;
        }

        char *end;
        unsigned long cp = strtoul(cursor, &end, 16);

        if(end == cursor || cp > MJB_CODEPOINT_MAX ||
            *codepoint_count >= MJB_TEST_EMOJI_MAX_CODEPOINTS) {
            return false;
        }

        codepoints[(*codepoint_count)++] = (mjb_codepoint)cp;
        cursor = end;
    }

    size_t i = 0;

    while(status_field[i] && status_field[i] != ' ' && status_field[i] != '\t' &&
        i + 1 < status_size) {
        status[i] = status_field[i];
        ++i;
    }

    status[i] = '\0';

    return status[0] != '\0' && *codepoint_count > 0;
}

static bool encode_emoji_test_sequence(const mjb_codepoint *codepoints, size_t codepoint_count,
    char *buffer, size_t buffer_size, size_t *size) {
    *size = 0;

    for(size_t i = 0; i < codepoint_count; ++i) {
        unsigned int written = mjb_codepoint_encode(codepoints[i], buffer + *size,
            buffer_size - *size, MJB_ENC_UTF_8);

        if(written == 0) {
            return false;
        }

        *size += written;
    }

    return true;
}

static mjb_emoji_qualification emoji_status_qualification(const char *status) {
    if(strcmp(status, "component") == 0) {
        return MJB_EMOJI_QUALIFICATION_COMPONENT;
    }

    if(strcmp(status, "fully-qualified") == 0) {
        return MJB_EMOJI_QUALIFICATION_FULLY_QUALIFIED;
    }

    if(strcmp(status, "minimally-qualified") == 0) {
        return MJB_EMOJI_QUALIFICATION_MINIMALLY_QUALIFIED;
    }

    if(strcmp(status, "unqualified") == 0) {
        return MJB_EMOJI_QUALIFICATION_UNQUALIFIED;
    }

    return MJB_EMOJI_QUALIFICATION_NONE;
}

static void run_emoji_test_file(const char *filename) {
    FILE *file = fopen(filename, "r");

    if(!file) {
        ATT_ASSERT("Not opened", "Opened file", "emoji-test.txt")

        return;
    }

    char line[1024];
    unsigned int current_line = 0;
    unsigned int tested_codepoints = 0;
    unsigned int codepoint_failures = 0;
    unsigned int tested_sequences = 0;
    unsigned int sequence_failures = 0;

    while(fgets(line, (int)sizeof(line), file)) {
        ++current_line;

        mjb_codepoint codepoints[MJB_TEST_EMOJI_MAX_CODEPOINTS];
        size_t codepoint_count;
        char status[32];

        if(!parse_emoji_test_line(line, codepoints, &codepoint_count, status, sizeof(status))) {
            continue;
        }

        char sequence[128];
        size_t sequence_size;
        mjb_emoji_sequence emoji_sequence;
        mjb_emoji_qualification qualification = emoji_status_qualification(status);
        bool sequence_encoded = encode_emoji_test_sequence(codepoints, codepoint_count, sequence,
            sizeof(sequence), &sequence_size);
        bool sequence_found = sequence_encoded &&
            mjb_classify_emoji_sequence(sequence, sequence_size, MJB_ENC_UTF_8, &emoji_sequence) ==
                MJB_STATUS_OK;
        bool sequence_ok = sequence_found && emoji_sequence.codepoint_count == codepoint_count &&
            emoji_sequence.qualification == qualification;
        bool rgi = sequence_encoded &&
            mjb_string_is_rgi_emoji(sequence, sequence_size, MJB_ENC_UTF_8);
        bool should_be_rgi = qualification == MJB_EMOJI_QUALIFICATION_COMPONENT ||
            qualification == MJB_EMOJI_QUALIFICATION_FULLY_QUALIFIED;

        if(!sequence_ok || rgi != should_be_rgi) {
            ++sequence_failures;
            char test_name[128];
            snprintf(test_name, sizeof(test_name), "emoji-test.txt sequence line %u", current_line);
            MJB_TEST_COVERAGE(mjb_classify_emoji_sequence);
            ATT_ASSERT(0, 1, test_name)

            if(is_exit_on_error()) {
                break;
            }
        } else {
            MJB_TEST_COVERAGE(mjb_classify_emoji_sequence);
            ATT_ASSERT(0, 0, "emoji-test.txt sequence entry")
        }

        ++tested_sequences;

        if(codepoint_count != 1) {
            continue;
        }

        mjb_emoji_properties emoji;
        bool found = mjb_codepoint_emoji_properties(codepoints[0], &emoji) == MJB_STATUS_OK;
        bool is_component = strcmp(status, "component") == 0;
        bool ok = found && emoji.codepoint == codepoints[0];

        ok = ok && (is_component ? emoji.component : emoji.emoji);

        if(!ok) {
            ++codepoint_failures;
            char test_name[128];
            snprintf(test_name, sizeof(test_name), "emoji-test.txt line %u", current_line);
            MJB_TEST_COVERAGE(mjb_codepoint_emoji_properties);
            ATT_ASSERT(0, 1, test_name)

            if(is_exit_on_error()) {
                break;
            }
        } else {
            MJB_TEST_COVERAGE(mjb_codepoint_emoji_properties);
            ATT_ASSERT(0, 0, "emoji-test.txt single-codepoint entry")
        }

        ++tested_codepoints;
    }

    fclose(file);

    char summary[128];
    snprintf(summary, sizeof(summary), "emoji-test.txt: %u/%u single-codepoint rows passed",
        tested_codepoints - codepoint_failures, tested_codepoints);
    ATT_ASSERT(tested_codepoints > 0, true, "emoji-test.txt has single-codepoint rows")
    ATT_ASSERT(codepoint_failures, 0u, summary)

    snprintf(summary, sizeof(summary), "emoji-test.txt: %u/%u sequence rows passed",
        tested_sequences - sequence_failures, tested_sequences);
    ATT_ASSERT(tested_sequences > 0, true, "emoji-test.txt has sequence rows")
    ATT_ASSERT(sequence_failures, 0u, summary)
}

static void assert_emoji_sequence(const char *buffer, size_t byte_length,
    mjb_emoji_sequence_type type, mjb_emoji_qualification qualification, size_t codepoint_count,
    const char *name) {
    mjb_emoji_sequence emoji;

    ATT_ASSERT_STATUS(mjb_classify_emoji_sequence(buffer, byte_length, MJB_ENC_UTF_8, &emoji),
        MJB_STATUS_OK, name)
    ATT_ASSERT((int)emoji.type, (int)type, name)
    ATT_ASSERT((int)emoji.qualification, (int)qualification, name)
    ATT_ASSERT(emoji.codepoint_count, codepoint_count, name)
}

int test_emoji(void *arg) {
    mjb_emoji_properties emoji;

    ATT_ASSERT_STATUS(mjb_codepoint_emoji_properties(MJB_CODEPOINT_MAX + 1, &emoji),
        MJB_STATUS_INVALID_ARGUMENT, "Invalid codepoint")
    ATT_ASSERT_STATUS(mjb_codepoint_emoji_properties(0x23, NULL), MJB_STATUS_INVALID_ARGUMENT,
        "NULL emoji pointer")

    ATT_ASSERT_STATUS(mjb_codepoint_emoji_properties(0x0, &emoji), MJB_STATUS_NOT_FOUND, "NULL")

    ATT_ASSERT_STATUS(mjb_codepoint_emoji_properties(0x23, &emoji), MJB_STATUS_OK, "U+23: #")
    ATT_ASSERT(emoji.codepoint, 0x23, "U+23: codepoint")
    ATT_ASSERT(emoji.emoji, true, "U+23: emoji")
    ATT_ASSERT(emoji.presentation, false, "U+23: presentation")
    ATT_ASSERT(emoji.modifier, false, "U+23: modifier")
    ATT_ASSERT(emoji.modifier_base, false, "U+23: modifier base")
    ATT_ASSERT(emoji.component, true, "U+23: component")
    ATT_ASSERT(emoji.extended_pictographic, false, "U+23: extended pictographic")

    ATT_ASSERT_STATUS(mjb_codepoint_emoji_properties(0x1F600, &emoji), MJB_STATUS_OK, "U+1F600: 😀")
    ATT_ASSERT(emoji.codepoint, 0x1F600, "U+1F600: codepoint")
    ATT_ASSERT(emoji.emoji, true, "U+1F600: emoji")
    ATT_ASSERT(emoji.presentation, true, "U+1F600: presentation")
    ATT_ASSERT(emoji.modifier, false, "U+1F600: modifier")
    ATT_ASSERT(emoji.modifier_base, false, "U+1F600: modifier base")
    ATT_ASSERT(emoji.component, false, "U+1F600: component")
    ATT_ASSERT(emoji.extended_pictographic, true, "U+1F600: extended pictographic")

    ATT_ASSERT(mjb_codepoint_is_emoji(0x23), true, "U+23 is Emoji")
    ATT_ASSERT(mjb_codepoint_is_emoji_presentation(0x23), false, "U+23 is not Emoji_Presentation")
    ATT_ASSERT(mjb_codepoint_is_emoji_component(0x23), true, "U+23 is Emoji_Component")
    ATT_ASSERT(mjb_codepoint_is_emoji_modifier(0x1F3FB), true, "U+1F3FB is Emoji_Modifier")
    ATT_ASSERT(mjb_codepoint_is_emoji_modifier_base(0x1F44B), true,
        "U+1F44B is Emoji_Modifier_Base")
    ATT_ASSERT(mjb_codepoint_is_extended_pictographic(0x1F600), true,
        "U+1F600 is Extended_Pictographic")
    ATT_ASSERT(mjb_codepoint_is_emoji(MJB_CODEPOINT_MAX + 1), false,
        "Invalid codepoint is not Emoji")

    // 😀
    assert_emoji_sequence("\xF0\x9F\x98\x80", 4, MJB_EMOJI_SEQUENCE_BASIC,
        MJB_EMOJI_QUALIFICATION_FULLY_QUALIFIED, 1, "Basic emoji sequence");
    // ☺
    assert_emoji_sequence("\xE2\x98\xBA", 3, MJB_EMOJI_SEQUENCE_NONE,
        MJB_EMOJI_QUALIFICATION_UNQUALIFIED, 1, "Unqualified emoji sequence");
    // ☺️
    assert_emoji_sequence("\xE2\x98\xBA\xEF\xB8\x8F", 6, MJB_EMOJI_SEQUENCE_BASIC,
        MJB_EMOJI_QUALIFICATION_FULLY_QUALIFIED, 2, "Qualified basic emoji sequence");
    // #️⃣
    assert_emoji_sequence("\x23\xEF\xB8\x8F\xE2\x83\xA3", 7, MJB_EMOJI_SEQUENCE_KEYCAP,
        MJB_EMOJI_QUALIFICATION_FULLY_QUALIFIED, 3, "Keycap emoji sequence");
    // ⌚︎
    assert_emoji_sequence("\xE2\x8C\x9A\xEF\xB8\x8E", 6, MJB_EMOJI_SEQUENCE_TEXT_VARIATION,
        MJB_EMOJI_QUALIFICATION_NONE, 2, "Text variation emoji sequence");
    // ⌚️
    assert_emoji_sequence("\xE2\x8C\x9A\xEF\xB8\x8F", 6, MJB_EMOJI_SEQUENCE_EMOJI_VARIATION,
        MJB_EMOJI_QUALIFICATION_NONE, 2, "Emoji variation emoji sequence");
    // 🇺🇸
    assert_emoji_sequence("\xF0\x9F\x87\xBA\xF0\x9F\x87\xB8", 8, MJB_EMOJI_SEQUENCE_FLAG,
        MJB_EMOJI_QUALIFICATION_FULLY_QUALIFIED, 2, "Flag emoji sequence");
    // 👋🏻
    assert_emoji_sequence("\xF0\x9F\x91\x8B\xF0\x9F\x8F\xBB", 8, MJB_EMOJI_SEQUENCE_MODIFIER,
        MJB_EMOJI_QUALIFICATION_FULLY_QUALIFIED, 2, "Modifier emoji sequence");
    // 👨‍👩‍👧
    assert_emoji_sequence("\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91"
                          "\xA7",
        18, MJB_EMOJI_SEQUENCE_ZWJ, MJB_EMOJI_QUALIFICATION_FULLY_QUALIFIED, 5,
        "ZWJ emoji sequence");

    // ☺
    ATT_ASSERT(mjb_string_is_emoji_sequence("\xE2\x98\xBA", 3, MJB_ENC_UTF_8), true,
        "Unqualified emoji is a listed emoji sequence")
    // ☺
    ATT_ASSERT(mjb_string_is_rgi_emoji("\xE2\x98\xBA", 3, MJB_ENC_UTF_8), false,
        "Unqualified emoji is not RGI")
    // ☺️
    ATT_ASSERT(mjb_string_is_rgi_emoji("\xE2\x98\xBA\xEF\xB8\x8F", 6, MJB_ENC_UTF_8), true,
        "Fully-qualified emoji is RGI")
    // ⌚️
    ATT_ASSERT(mjb_string_is_rgi_emoji("\xE2\x8C\x9A\xEF\xB8\x8F", 6, MJB_ENC_UTF_8), false,
        "Emoji variation sequence is not RGI by itself")
    ATT_ASSERT(mjb_string_is_emoji_sequence("ABC", 3, MJB_ENC_UTF_8), false,
        "ASCII word is not an emoji sequence")
    ATT_ASSERT_STATUS(mjb_classify_emoji_sequence(NULL, 0, MJB_ENC_UTF_8, NULL),
        MJB_STATUS_INVALID_ARGUMENT, "NULL string emoji sequence")

    run_emoji_test_file("./utils/generate/unicode-data/emoji/emoji-test.txt");

    return 0;
}
