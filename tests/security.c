/**
 * The Mojibake library
 *
 * UTS#39 Unicode Security Mechanisms - confusable detection tests.
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "../src/mojibake-internal.h"
#if !defined(MJB_SHARED)
#include "../src/unicode-tables.h"
#endif
#include "test.h"

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

static size_t codepoint_field_to_utf8(char *field, char *output, size_t output_size) {
    char *p = trim_ascii(field);
    size_t output_len = 0;

    while(*p) {
        char *end;
        unsigned long cp = strtoul(p, &end, 16);

        if(end == p || cp > MJB_CODEPOINT_MAX) {
            return 0;
        }

        unsigned int encoded = mjb_codepoint_encode((mjb_codepoint)cp, output + output_len,
            output_size - output_len - 1, MJB_ENC_UTF_8);

        if(encoded == 0) {
            return 0;
        }

        output_len += encoded;
        p = trim_ascii(end);
    }

    output[output_len] = '\0';

    return output_len;
}

static bool parse_intentional_line(char *line, char *left, size_t *left_len, char *right,
    size_t *right_len) {
    char *comment = strchr(line, '#');

    if(comment) {
        *comment = '\0';
    }

    char *semi = strchr(line, ';');

    if(!semi) {
        return false;
    }

    *semi = '\0';

    *left_len = codepoint_field_to_utf8(line, left, 64);
    *right_len = codepoint_field_to_utf8(semi + 1, right, 64);

    return *left_len > 0 && *right_len > 0;
}

static void run_intentional_confusable_file(const char *filename) {
    FILE *file = fopen(filename, "r");

    if(!file) {
        ATT_ASSERT("Not opened", "Opened file", "intentional.txt")

        return;
    }

    char line[1024];
    char left[64];
    char right[64];
    unsigned int current_line = 0;
    unsigned int tested = 0;
    unsigned int failures = 0;

    while(fgets(line, (int)sizeof(line), file)) {
        ++current_line;

        size_t left_len = 0;
        size_t right_len = 0;

        if(!parse_intentional_line(line, left, &left_len, right, &right_len)) {
            continue;
        }

        bool ok = mjb_are_confusable(left, left_len, MJB_ENC_UTF_8, right, right_len,
            MJB_ENC_UTF_8);
        MJB_TEST_COVERAGE(mjb_are_confusable);

        if(!ok) {
            ++failures;
            char test_name[128];
            snprintf(test_name, sizeof(test_name), "intentional.txt line %u", current_line);
            ATT_ASSERT(0, 1, test_name)

            if(is_exit_on_error()) {
                break;
            }
        } else {
            ATT_ASSERT(0, 0, "intentional.txt confusable pair")
        }

        ++tested;
    }

    fclose(file);

    char summary[128];
    snprintf(summary, sizeof(summary), "intentional.txt: %u/%u pairs passed", tested - failures,
        tested);
    ATT_ASSERT(tested > 0, true, "intentional.txt has confusable pairs")
    ATT_ASSERT(failures, 0u, summary)
}

static void check_skeleton(const char *input, size_t input_size, const char *expected,
    size_t expected_size, const char *name) {
    mjb_result skeleton;

    MJB_TEST_COVERAGE(mjb_confusable_skeleton);
    ATT_ASSERT_STATUS(mjb_confusable_skeleton(input, input_size, MJB_ENC_UTF_8, MJB_ENC_UTF_8,
        &skeleton), MJB_STATUS_OK, name)
    ATT_ASSERT(skeleton.output_size, expected_size, name)
    ATT_ASSERT((int)memcmp(skeleton.output, expected, expected_size), 0, name)
    ATT_ASSERT_STATUS(mjb_result_free(&skeleton), MJB_STATUS_OK, name)
}

#if !defined(MJB_SHARED)
static void run_confusables_file(const char *filename) {
    FILE *file = fopen(filename, "r");

    if(file == NULL) {
        ATT_ASSERT("Not opened", "Opened file", "confusables.txt")
        return;
    }

    char line[2048];
    unsigned int current_line = 0;
    unsigned int tested = 0;

    while(fgets(line, sizeof(line), file) != NULL) {
        ++current_line;
        char *comment = strchr(line, '#');

        if(comment != NULL) {
            *comment = '\0';
        }

        char *first_semicolon = strchr(line, ';');

        if(first_semicolon == NULL) {
            continue;
        }

        *first_semicolon = '\0';

        char *second_semicolon = strchr(first_semicolon + 1, ';');

        if(second_semicolon == NULL) {
            continue;
        }

        *second_semicolon = '\0';

        char source[128];
        char target[128];
        size_t source_size = codepoint_field_to_utf8(line, source, sizeof(source));
        size_t target_size = codepoint_field_to_utf8(first_semicolon + 1, target, sizeof(target));

        if(source_size == 0 || target_size == 0) {
            continue;
        }

        char test_name[128];
        snprintf(test_name, sizeof(test_name), "confusables.txt line %u", current_line);
        mjb_codepoint source_cp = (mjb_codepoint)strtoul(trim_ascii(line), NULL, 16);

        const mjb_codepoint *mapping = NULL;
        uint8_t mapping_length = 0;
        ATT_ASSERT(mjb_unicode_confusable_lookup(source_cp, &mapping, &mapping_length), true,
            test_name)

        char actual[128];
        size_t actual_size = 0;

        for(uint8_t i = 0; i < mapping_length; ++i) {
            actual_size += mjb_codepoint_encode(mapping[i], actual + actual_size,
                sizeof(actual) - actual_size, MJB_ENC_UTF_8);
        }

        ATT_ASSERT(actual_size, target_size, test_name)
        ATT_ASSERT((int)memcmp(actual, target, target_size), 0, test_name)

        ++tested;
    }

    fclose(file);

    ATT_ASSERT(tested > 0, true, "confusables.txt has skeleton mappings")
}
#endif

int test_security(void *arg) {
    mjb_encoding enc = MJB_ENC_UTF_8;

    mjb_result skeleton;
    ATT_ASSERT_STATUS(mjb_confusable_skeleton(NULL, 1, enc, enc, &skeleton),
        MJB_STATUS_INVALID_ARGUMENT, "Skeleton rejects NULL input")
    ATT_ASSERT_STATUS(mjb_confusable_skeleton("A", 1, enc, enc, NULL), MJB_STATUS_INVALID_ARGUMENT,
        "Skeleton rejects NULL result")

    check_skeleton("h\xD0\xB5llo", 6, "hello", 5, "Skeleton maps Cyrillic e");
    check_skeleton("a\xE2\x80\x8D"
                   "b",
        5, "ab", 2, "Skeleton removes default-ignorables");
    check_skeleton("A1<\xD7\xA9\xD7\x82", 7, "Al<\xD7\xA9\xCC\x87", 7,
        "Skeleton applies LTR bidi processing");
    check_skeleton("\xEF\xB7\xBA", 3,
        "\xD8\xB5\xD9\x84\xD9\x89 l\xD9\x84\xD9\x84o \xD8\xB9\xD9\x84\xD9\x89o "
        "\xD9\x88\xD8\xB3\xD9\x84\xD9\x85",
        30, "Skeleton preserves full 18-codepoint expansion");

    ATT_ASSERT_STATUS(mjb_confusable_skeleton("A", 1, enc, MJB_ENC_UTF_16LE, &skeleton),
        MJB_STATUS_OK, "Skeleton supports UTF-16 output")
    ATT_ASSERT(skeleton.output_size, (size_t)2, "Skeleton UTF-16 output size")
    ATT_ASSERT((int)memcmp(skeleton.output, "A\0", 2), 0, "Skeleton UTF-16 output")
    ATT_ASSERT_STATUS(mjb_result_free(&skeleton), MJB_STATUS_OK, "Free UTF-16 skeleton")

    ATT_ASSERT(mjb_are_confusable(NULL, 1, enc, "A", 1, enc), false,
        "confusable rejects NULL left string")

    // Cyrillic "А" (U+0410, UTF-8: 0xD0 0x90) is confusable with Latin "A"
    // Both have skeleton "A": skeleton("А")="A", skeleton("A")="A"
    ATT_ASSERT(mjb_are_confusable("\xD0\x90", 2, enc, "A", 1, enc), true,
        "Cyrillic A confusable with Latin A")

    // Cyrillic "а" (U+0430) is confusable with Latin "a" (U+0061)
    // skeleton("а")="a", skeleton("a")="a"
    ATT_ASSERT(mjb_are_confusable("\xD0\xB0", 2, enc, "a", 1, enc), true,
        "Cyrillic a confusable with Latin a")

    // "A" (Latin capital) is NOT confusable with "a" (Latin lowercase)
    // skeleton("A")="A", skeleton("a")="a" → different
    ATT_ASSERT(mjb_are_confusable("A", 1, enc, "a", 1, enc), false, "A not confusable with a")

    // "a" is not confusable with "b"
    ATT_ASSERT(mjb_are_confusable("a", 1, enc, "b", 1, enc), false, "a not confusable with b")

    // A string is confusable with itself
    ATT_ASSERT(mjb_are_confusable("hello", 5, enc, "hello", 5, enc), true,
        "hello confusable with itself")

    // "hello" vs "hеllo" (second 'e' is Cyrillic U+0435, UTF-8: 0xD0 0xB5)
    // skeleton("hello")="hello", skeleton("hеllo")="hello" → confusable
    ATT_ASSERT(mjb_are_confusable("hello", 5, enc, "h\xD0\xB5llo", 6, enc), true,
        "hello confusable with h(Cyrillic e)llo")

    // Empty strings: both have empty skeleton (output_size==0), returns false
    ATT_ASSERT(mjb_are_confusable("", 0, enc, "", 0, enc), false,
        "empty strings not confusable")

    // Different lengths with no possible match
    ATT_ASSERT(mjb_are_confusable("a", 1, enc, "ab", 2, enc), false,
        "a not confusable with ab")

    // Digit '1', capital 'I', and pipe '|' all map to skeleton 'l'
    ATT_ASSERT(mjb_are_confusable("1", 1, enc, "l", 1, enc), true, "1 confusable with l")
    ATT_ASSERT(mjb_are_confusable("I", 1, enc, "l", 1, enc), true, "I confusable with l")
    ATT_ASSERT(mjb_are_confusable("|", 1, enc, "l", 1, enc), true, "| confusable with l")
    ATT_ASSERT(mjb_are_confusable("1", 1, enc, "I", 1, enc), true, "1 confusable with I")

    // Digit '0' maps to skeleton 'O' (letter)
    ATT_ASSERT(mjb_are_confusable("0", 1, enc, "O", 1, enc), true, "0 confusable with O")
    ATT_ASSERT(mjb_are_confusable("0", 1, enc, "o", 1, enc), false,
        "0 not confusable with o (O != o)")

    // 'm' maps to a two-codepoint skeleton "rn": skeleton("m") == skeleton("rn")
    ATT_ASSERT(mjb_are_confusable("m", 1, enc, "rn", 2, enc), true,
        "m confusable with rn (multi-codepoint skeleton)")
    ATT_ASSERT(mjb_are_confusable("mm", 2, enc, "rnrn", 4, enc), true,
        "mm confusable with rnrn")

    // Cyrillic р (U+0440, UTF-8: 0xD1 0x80) maps to Latin p
    // "рal" (with Cyrillic р) is confusable with "pal"
    ATT_ASSERT(mjb_are_confusable("рal", 4, enc, "pal", 3, enc), true,
        "Cyrillic рal confusable with pal")

    // Cyrillic С (U+0421, UTF-8: 0xD0 0xA1) maps to Latin C
    ATT_ASSERT(mjb_are_confusable("\xD0\xA1" "at", 4, enc, "Cat", 3, enc), true,
        "Cyrillic С + at confusable with Cat")

    // "gооd" (Cyrillic о U+043E, UTF-8: 0xD0 0xBE) confusable with "good"
    // skeleton(Cyrillic о) = Latin o -> both strings have skeleton "good"
    ATT_ASSERT(mjb_are_confusable("gооd", 6, enc, "good", 4, enc), true,
        "g(Cyrillic o)(Cyrillic o)d confusable with good")

    ATT_ASSERT(mjb_are_confusable("gооd", 6, enc, "\0g\0o\0o\0d", 8, MJB_ENC_UTF_16BE), true,
        "Confusable with different encodings")

    // Confusability is symmetric
    ATT_ASSERT(mjb_are_confusable("pal", 3, enc, "\xD1\x80" "al", 4, enc), true,
        "confusability is symmetric")

    run_intentional_confusable_file("./utils/generate/unicode-data/security/intentional.txt");
#if !defined(MJB_SHARED)
    run_confusables_file("./utils/generate/unicode-data/security/confusables.txt");
#endif

    return 0;
}
