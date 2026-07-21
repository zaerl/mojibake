/**
 * The Mojibake library
 *
 * Tests for the bidirectional text algorithm (TR9)
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test.h"

// One representative codepoint per Bidi_Class, for BidiTest.txt inputs. The test file is
// designed so any character with the given class produces the same result.
typedef struct mjbt_bidi_class_example {
    const char *name;
    mjb_codepoint codepoint;
} mjbt_bidi_class_example;

static const mjbt_bidi_class_example bidi_class_examples[] = {
    { "AL", 0x0627 },  // ARABIC LETTER ALEF
    { "AN", 0x0660 },  // ARABIC-INDIC DIGIT ZERO
    { "B", 0x2029 },   // PARAGRAPH SEPARATOR
    { "BN", 0x00AD },  // SOFT HYPHEN
    { "CS", 0x002C },  // COMMA
    { "EN", 0x0030 },  // DIGIT ZERO
    { "ES", 0x002B },  // PLUS SIGN
    { "ET", 0x0024 },  // DOLLAR SIGN
    { "FSI", 0x2068 }, // FIRST STRONG ISOLATE
    { "L", 0x0041 },   // LATIN CAPITAL LETTER A
    { "LRE", 0x202A }, // LEFT-TO-RIGHT EMBEDDING
    { "LRI", 0x2066 }, // LEFT-TO-RIGHT ISOLATE
    { "LRO", 0x202D }, // LEFT-TO-RIGHT OVERRIDE
    { "NSM", 0x0300 }, // COMBINING GRAVE ACCENT
    { "ON", 0x0021 },  // EXCLAMATION MARK
    { "PDF", 0x202C }, // POP DIRECTIONAL FORMATTING
    { "PDI", 0x2069 }, // POP DIRECTIONAL ISOLATE
    { "R", 0x05D0 },   // HEBREW LETTER ALEF
    { "RLE", 0x202B }, // RIGHT-TO-LEFT EMBEDDING
    { "RLI", 0x2067 }, // RIGHT-TO-LEFT ISOLATE
    { "RLO", 0x202E }, // RIGHT-TO-LEFT OVERRIDE
    { "S", 0x0009 },   // CHARACTER TABULATION
    { "WS", 0x0020 },  // SPACE
};

static mjb_codepoint bidi_class_codepoint(const char *name) {
    for(size_t i = 0; i < sizeof(bidi_class_examples) / sizeof(bidi_class_examples[0]); ++i) {
        if(strcmp(bidi_class_examples[i].name, name) == 0) {
            return bidi_class_examples[i].codepoint;
        }
    }

    return 0;
}

/*
 * BidiTest.txt: data lines are sequences of Bidi_Class names, checked against the current
 * @Levels and @Reorder directives for each paragraph direction in the bitset
 * (1 = auto-LTR, 2 = LTR, 4 = RTL).
 */
static void read_bidi_class_test_file(const char *filename) {
    FILE *file = fopen(filename, "r");

    if(!file) {
        ATT_ASSERT("Not opened", "Opened file", "BidiTest.txt")

        return;
    }

    char line[1024];
    unsigned int current_line = 0;

    uint8_t expected_levels[64];
    bool is_removed[64];
    size_t levels_count = 0;
    size_t non_removed = 0;
    size_t expected_order[64];
    size_t order_count = 0;

    static const mjb_direction directions[3] = { MJB_DIRECTION_AUTO, MJB_DIRECTION_LTR,
        MJB_DIRECTION_RTL };

    while(fgets(line, (int)sizeof(line), file)) {
        ++current_line;

        size_t len = strlen(line);

        while(len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[--len] = '\0';
        }

        if(line[0] == '#' || line[0] == '\0') {
            continue;
        }

        if(strncmp(line, "@Levels:", 8) == 0) {
            levels_count = 0;
            non_removed = 0;

            char *p = line + 8;
            char *tok;

            while((tok = mjb_test_strsep(&p, " \t")) != NULL && levels_count < 64) {
                if(!tok[0]) {
                    continue;
                }

                if(tok[0] == 'x') {
                    is_removed[levels_count] = true;
                    expected_levels[levels_count] = 0xFF;
                } else {
                    is_removed[levels_count] = false;
                    expected_levels[levels_count] = (uint8_t)atoi(tok);
                    ++non_removed;
                }

                ++levels_count;
            }

            continue;
        }

        if(strncmp(line, "@Reorder:", 9) == 0) {
            order_count = 0;

            char *p = line + 9;
            char *tok;

            while((tok = mjb_test_strsep(&p, " \t")) != NULL && order_count < 64) {
                if(!tok[0]) {
                    continue;
                }

                expected_order[order_count++] = (size_t)atoi(tok);
            }

            continue;
        }

        if(line[0] == '@') { // Unknown directive, ignore (forward compatibility)
            continue;
        }

        // Data line: <Bidi_Class names> ; <hex bitset>
        char *semi = strchr(line, ';');

        if(!semi) {
            continue;
        }

        *semi = '\0';

        unsigned long bitset = strtoul(semi + 1, NULL, 16);

        char utf8_buf[256];
        size_t utf8_len = 0;
        size_t cp_count = 0;
        bool skip = false;

        char *p = line;
        char *tok;

        while((tok = mjb_test_strsep(&p, " \t")) != NULL) {
            if(!tok[0]) {
                continue;
            }

            mjb_codepoint cp = bidi_class_codepoint(tok);

            if(cp == 0) {
                skip = true;
                break;
            }

            unsigned int enc = mjb_codepoint_encode(cp, utf8_buf + utf8_len,
                sizeof(utf8_buf) - utf8_len - 1, MJB_ENC_UTF_8);

            if(enc == 0) {
                skip = true;
                break;
            }

            utf8_len += enc;
            ++cp_count;
        }

        if(skip || cp_count == 0 || cp_count != levels_count) {
            continue;
        }

        size_t successful = 0;
        size_t total = 0;

        for(int d = 0; d < 3; ++d) {
            if(!(bitset & (1UL << d))) {
                continue;
            }

            mjb_bidi_paragraph para;

            ++total;

            if(mjb_bidi_resolve(utf8_buf, utf8_len, MJB_ENC_UTF_8, directions[d], &para) !=
                MJB_STATUS_OK) {
                continue;
            }

            ++successful;

            // Resolved levels, skipping positions the UBA does not assign levels to (x).
            size_t non_removed_idx = 0;

            for(size_t i = 0; i < levels_count && non_removed_idx < para.count; ++i) {
                if(is_removed[i]) {
                    continue;
                }

                ++total;

                if(para.chars[non_removed_idx].level == expected_levels[i]) {
                    ++successful;
                }

                ++non_removed_idx;
            }

            // Visual order after L2 reordering, mapped back to original positions.
            if(non_removed > 0 && order_count == non_removed && para.count == non_removed) {
                size_t orig_pos[64];
                size_t nri = 0;

                for(size_t i = 0; i < levels_count && nri < 64; ++i) {
                    if(!is_removed[i]) {
                        orig_pos[nri++] = i;
                    }
                }

                size_t visual_order[64];

                if(mjb_bidi_reorder_line(&para, 0, para.count, visual_order) == MJB_STATUS_OK) {
                    for(size_t v = 0; v < order_count; ++v) {
                        ++total;

                        if(orig_pos[visual_order[v]] == expected_order[v]) {
                            ++successful;
                        }
                    }
                }
            }

            mjb_bidi_paragraph_free(&para);
        }

        char test_name[64];
        snprintf(test_name, sizeof(test_name), "BidiTest #%u", current_line);

        MJB_TEST_COVERAGE(mjb_bidi_resolve);
        ATT_ASSERT(total, successful, test_name)
    }

    fclose(file);
}

int test_bidi_class(void *arg) {
    read_bidi_class_test_file("./utils/generate/unicode-data/UCD/BidiTest.txt");

    return 0;
}
