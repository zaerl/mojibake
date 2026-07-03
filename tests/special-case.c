/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>

#include "test.h"
#include "../src/utf8.h"

static int check_case(char *source, size_t source_size, char *target, size_t target_size,
    mjb_case_type type, unsigned int current_line, const char *step) {
    mjb_encoding encoding = MJB_ENCODING_UTF_8;
    char test_name[128];

    snprintf(test_name, 128, "#%u %s", current_line, step);

    if(source_size == 0 || target_size == 0) {
        return 0;
    }

    char *result = mjb_case(source, source_size, type, encoding);

    MJB_TEST_COVERAGE(mjb_case);
    ATT_ASSERT(result, target, test_name)

    if(result != NULL && result != source) {
        mjb_free(result);
    }

    return 0;
}

static int check_conditional(const char *source, const char *target, mjb_case_type type,
    const char *name) {
    char *result = mjb_case(source, strlen(source), type, MJB_ENCODING_UTF_8);

    MJB_TEST_COVERAGE(mjb_case);
    ATT_ASSERT(result, (char*)target, name)

    if(result != NULL) {
        mjb_free(result);
    }

    return 0;
}

// Conditional mappings [SpecialCasing.txt, "Conditional Mappings" section]. The data lines are
// single codepoints, so the context each condition tests is written by hand here.
static void test_conditional_case(void) {
    mjb_locale_set(MJB_LOCALE_EN);

    // Final_Sigma, language-insensitive.
    check_conditional("\xCE\xA3", "\xCF\x83", MJB_CASE_LOWER,
        "Final_Sigma: lone Σ has no preceding cased letter");
    check_conditional("\xCE\x91\xCE\xA3", "\xCE\xB1\xCF\x82", MJB_CASE_LOWER,
        "Final_Sigma: ΑΣ -> ας");
    check_conditional("\xCE\x91\xCE\xA3\xCC\x88\xCE\x91", "\xCE\xB1\xCF\x83\xCC\x88\xCE\xB1",
        MJB_CASE_LOWER, "Final_Sigma: a combining mark does not end the word");
    check_conditional("\xCE\x91\xCE\xA3\xCC\x88", "\xCE\xB1\xCF\x82\xCC\x88", MJB_CASE_LOWER,
        "Final_Sigma: word-final with combining mark");
    check_conditional("\xCE\x91\xCE\xA3'\xCE\x91", "\xCE\xB1\xCF\x83'\xCE\xB1", MJB_CASE_LOWER,
        "Final_Sigma: apostrophe is case-ignorable");

    // Turkish: İ/i and I/ı case pairs.
    mjb_locale_set(MJB_LOCALE_TR);
    check_conditional("ISPARTA", "\xC4\xB1sparta", MJB_CASE_LOWER,
        "tr lower: I -> ı when not before dot above");
    check_conditional("\xC4\xB0STANBUL", "istanbul", MJB_CASE_LOWER, "tr lower: İ -> i");
    check_conditional("I\xCC\x87", "i", MJB_CASE_LOWER,
        "tr lower: dot above removed after I");
    check_conditional("I\xCC\xA3\xCC\x87", "i\xCC\xA3", MJB_CASE_LOWER,
        "tr lower: ccc 220 mark may intervene between I and dot above");
    check_conditional("diyarbak\xC4\xB1r", "D\xC4\xB0YARBAKIR", MJB_CASE_UPPER,
        "tr upper: i -> İ and ı -> I");
    check_conditional("istanbul", "\xC4\xB0stanbul", MJB_CASE_TITLE, "tr title: i -> İ");
    check_conditional("ILIK", "Il\xC4\xB1k", MJB_CASE_TITLE,
        "tr title: word-initial I kept, in-word I -> ı");

    // Azerbaijani follows the same rules.
    mjb_locale_set(MJB_LOCALE_AZ);
    check_conditional("i", "\xC4\xB0", MJB_CASE_UPPER, "az upper: i -> İ");
    check_conditional("\xC4\xB0", "i", MJB_CASE_LOWER, "az lower: İ -> i");

    // Lithuanian: retain the dot in a lowercase i/j when followed by accents above.
    mjb_locale_set(MJB_LOCALE_LT);
    check_conditional("I\xCC\x80", "i\xCC\x87\xCC\x80", MJB_CASE_LOWER,
        "lt lower: I gains dot above when more accents above");
    check_conditional("J\xCC\x83", "j\xCC\x87\xCC\x83", MJB_CASE_LOWER,
        "lt lower: J gains dot above when more accents above");
    check_conditional("\xC4\xAE\xCC\x83", "\xC4\xAF\xCC\x87\xCC\x83", MJB_CASE_LOWER,
        "lt lower: Į gains dot above when more accents above");
    check_conditional("IS", "is", MJB_CASE_LOWER,
        "lt lower: no dot above without accents above");
    check_conditional("\xC3\x8C", "i\xCC\x87\xCC\x80", MJB_CASE_LOWER, "lt lower: Ì -> i̇̀");
    check_conditional("\xC3\x8D", "i\xCC\x87\xCC\x81", MJB_CASE_LOWER, "lt lower: Í -> i̇́");
    check_conditional("\xC4\xA8", "i\xCC\x87\xCC\x83", MJB_CASE_LOWER, "lt lower: Ĩ -> i̇̃");
    check_conditional("i\xCC\x87\xCC\x80", "I\xCC\x80", MJB_CASE_UPPER,
        "lt upper: dot above removed after soft-dotted");
    check_conditional("i\xCC\x87", "I", MJB_CASE_UPPER, "lt upper: i + dot above -> I");

    // The language-sensitive rules must not leak into other locales.
    mjb_locale_set(MJB_LOCALE_EN);
    check_conditional("I\xCC\x87", "i\xCC\x87", MJB_CASE_LOWER,
        "en lower: dot above kept after I");
    check_conditional("I\xCC\x80", "i\xCC\x80", MJB_CASE_LOWER,
        "en lower: no dot above inserted");
}

void *test_special_case(void *arg) {
    char line[1024];
    unsigned int current_line = 1;
    FILE *file = fopen("./utils/generate/unicode-data/UCD/SpecialCasing.txt", "r");

    // 256 characters is enough for any test.
    const char source[256] = { 0 };
    const char lower[256] = { 0 };
    const char title[256] = { 0 };
    const char upper[256] = { 0 };

    size_t source_size = 0;
    size_t lower_size = 0;
    size_t title_size = 0;
    size_t upper_size = 0;

    if(file == NULL) {
        ATT_ASSERT("Not opened", "Opened file", "Valid special casing test file")

        return NULL;
    }

    while(fgets(line, 1024, file)) {
        if(line[0] == '#' || line[0] == '@' || strnlen(line, 512) <= 1) {
            // printf("skipping line %u (%s)\n", current_line, line);
            ++current_line;

            // Conditional mappings are covered by test_conditional_case.
            if(strncmp(line, "# Conditional Mappings", 21) == 0) {
                break;
            }

            continue;
        }

        char *token, *string, *tofree;
        tofree = string = strdup(line);
        unsigned int field = 0;

        while((token = strsep(&string, ";")) != NULL) {
            switch(field) {
                case 0: // Source
                    source_size = get_string_from_codepoints(token, 256, (char*)source);
                    break;

                case 1: // Lower
                    lower_size = get_string_from_codepoints(token, 256, (char*)lower);
                    break;

                case 2: // Title
                    title_size = get_string_from_codepoints(token, 256, (char*)title);
                    break;

                case 3: // Upper
                    upper_size = get_string_from_codepoints(token, 256, (char*)upper);
                    break;
            }

            // Skip trailing comments
            if(++field == 4) {
                break;
            }
        }

        free(tofree);

        check_case((char*)source, source_size, (char*)lower, lower_size, MJB_CASE_LOWER, current_line, "lower");
        check_case((char*)source, source_size, (char*)title, title_size, MJB_CASE_TITLE, current_line, "title");
        check_case((char*)source, source_size, (char*)upper, upper_size, MJB_CASE_UPPER, current_line, "upper");

        memset((void*)source, 0, 256);
        memset((void*)lower, 0, 256);
        memset((void*)title, 0, 256);
        memset((void*)upper, 0, 256);

        ++current_line;
    }

    fclose(file);

    test_conditional_case();

    return NULL;
}
