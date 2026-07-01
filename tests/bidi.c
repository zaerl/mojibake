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

static void read_bidi_test_file(const char *filename) {
    FILE *file = fopen(filename, "r");

    if(!file) {
        ATT_ASSERT("Not opened", "Opened file", "BidiCharacterTest.txt")

        return;
    }

    char line[4096];
    unsigned int current_line = 0;

    while(fgets(line, (int)sizeof(line), file)) {
        ++current_line;

        char c = line[0];

        if(c == '#' || c == '\n' || c == '\r' || c == '\0') {
            continue;
        }

        char *fields[5];
        char *p = line;
        int f = 0;

        while(f < 4) {
            char *semi = strchr(p, ';');

            if(!semi) {
                break;
            }

            *semi = '\0';
            fields[f++] = p;
            p = semi + 1;
        }

        if(f < 4) {
            continue;
        }

        // Trim trailing newline from last field
        size_t last_len = strlen(p);

        while(last_len > 0 && (p[last_len - 1] == '\n' || p[last_len - 1] == '\r')) {
            p[--last_len] = '\0';
        }

        fields[4] = p;

        // Field 0: hex codepoints
        char utf8_buf[512];
        size_t utf8_len = 0;
        bool skip = false;

        char *cp_ptr = fields[0];
        char *cp_tok;

        while((cp_tok = strsep(&cp_ptr, " ")) != NULL) {
            if(!cp_tok[0]) {
                continue;
            }

            mjb_codepoint cp = (mjb_codepoint)strtoul(cp_tok, NULL, 16);

            if(cp == 0) {
                skip = true;
                break;
            }

            unsigned int enc = mjb_codepoint_encode(cp, utf8_buf + utf8_len,
                sizeof(utf8_buf) - utf8_len - 1, MJB_ENCODING_UTF_8);

            if(enc == 0) {
                skip = true;
                break;
            }

            utf8_len += enc;
        }

        if(skip) {
            continue;
        }

        utf8_buf[utf8_len] = '\0';

        // Field 1: paragraph direction
        int dir_v = atoi(fields[1]);
        mjb_direction dir = (dir_v == 1) ? MJB_DIRECTION_RTL : (dir_v == 2) ? MJB_DIRECTION_AUTO :
            MJB_DIRECTION_LTR;

        // Field 2: expected paragraph level
        uint8_t expected_para_level = (uint8_t)atoi(fields[2]);

        // Field 3: expected resolved levels
        uint8_t expected_levels[256];
        bool is_removed[256];
        size_t total_cp = 0;
        size_t non_removed = 0;

        char *lvl_ptr = fields[3];
        char *lvl_tok;

        while((lvl_tok = strsep(&lvl_ptr, " ")) != NULL && total_cp < 256) {
            if(!lvl_tok[0]) {
                continue;
            }

            if(lvl_tok[0] == 'x') {
                is_removed[total_cp]     = true;
                expected_levels[total_cp] = 0xFF;
            } else {
                is_removed[total_cp]     = false;
                expected_levels[total_cp] = (uint8_t)atoi(lvl_tok);
                ++non_removed;
            }

            ++total_cp;
        }

        // Field 4: expected visual order
        size_t expected_order[256];
        size_t order_count = 0;

        char *ord_ptr = fields[4];
        char *ord_tok;

        while((ord_tok = strsep(&ord_ptr, " ")) != NULL && order_count < 256) {
            if(!ord_tok[0]) {
                continue;
            }

            expected_order[order_count++] = (size_t)atoi(ord_tok);
        }

        // Run algorithm
        mjb_bidi_paragraph para;
        bool ok = mjb_bidi_resolve(utf8_buf, utf8_len, MJB_ENCODING_UTF_8, dir, &para);

        char test_name[64];
        snprintf(test_name, sizeof(test_name), "#%u", current_line);

        size_t successful = 0;
        size_t total      = 0;

        if(!ok) {
            ATT_ASSERT(total, successful, test_name)

            continue;
        }

        // Check 1: paragraph level
        ++total;

        if(para.paragraph_level == expected_para_level) {
            ++successful;
        }

        // Check 2: per-char resolved levels
        size_t non_removed_idx = 0;

        for(size_t i = 0; i < total_cp && non_removed_idx < para.count; ++i) {
            if(is_removed[i]) {
                continue;
            }

            ++total;

            if(para.chars[non_removed_idx].level == expected_levels[i]) {
                ++successful;
            }

            ++non_removed_idx;
        }

        /*
         * Check 3: visual order after L2 reordering.
         *
         * expected_order[v] is an index into the ORIGINAL codepoint sequence (field 0),
         * counting x-chars. visual_order[v] is an index into para.chars[] which only contains
         * non-x chars. Build orig_pos[] to map non-x index -> original position for comparison.
         */
        if(non_removed > 0 && order_count == non_removed && para.count == non_removed) {
            size_t orig_pos[256];
            size_t nri = 0;

            for(size_t i = 0; i < total_cp && nri < 256; ++i) {
                if(!is_removed[i]) {
                    orig_pos[nri++] = i;
                }
            }

            size_t visual_order[256];
            mjb_bidi_reorder_line(&para, 0, para.count, visual_order);

            for(size_t v = 0; v < order_count; ++v) {
                ++total;

                if(orig_pos[visual_order[v]] == expected_order[v]) {
                    ++successful;
                }
            }
        }

        // CURRENT_ASSERT mjb_bidi_resolve
        // CURRENT_COUNT 91745
        ATT_ASSERT(total, successful, test_name)

        mjb_bidi_free(&para);
    }

    fclose(file);
}

// One representative codepoint per Bidi_Class, for BidiTest.txt inputs. The test file is
// designed so any character with the given class produces the same result.
typedef struct mjbt_bidi_class_example {
    const char *name;
    mjb_codepoint codepoint;
} mjbt_bidi_class_example;

static const mjbt_bidi_class_example bidi_class_examples[] = {
    { "AL",  0x0627 }, // ARABIC LETTER ALEF
    { "AN",  0x0660 }, // ARABIC-INDIC DIGIT ZERO
    { "B",   0x2029 }, // PARAGRAPH SEPARATOR
    { "BN",  0x00AD }, // SOFT HYPHEN
    { "CS",  0x002C }, // COMMA
    { "EN",  0x0030 }, // DIGIT ZERO
    { "ES",  0x002B }, // PLUS SIGN
    { "ET",  0x0024 }, // DOLLAR SIGN
    { "FSI", 0x2068 }, // FIRST STRONG ISOLATE
    { "L",   0x0041 }, // LATIN CAPITAL LETTER A
    { "LRE", 0x202A }, // LEFT-TO-RIGHT EMBEDDING
    { "LRI", 0x2066 }, // LEFT-TO-RIGHT ISOLATE
    { "LRO", 0x202D }, // LEFT-TO-RIGHT OVERRIDE
    { "NSM", 0x0300 }, // COMBINING GRAVE ACCENT
    { "ON",  0x0021 }, // EXCLAMATION MARK
    { "PDF", 0x202C }, // POP DIRECTIONAL FORMATTING
    { "PDI", 0x2069 }, // POP DIRECTIONAL ISOLATE
    { "R",   0x05D0 }, // HEBREW LETTER ALEF
    { "RLE", 0x202B }, // RIGHT-TO-LEFT EMBEDDING
    { "RLI", 0x2067 }, // RIGHT-TO-LEFT ISOLATE
    { "RLO", 0x202E }, // RIGHT-TO-LEFT OVERRIDE
    { "S",   0x0009 }, // CHARACTER TABULATION
    { "WS",  0x0020 }, // SPACE
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

    static const mjb_direction directions[3] = {
        MJB_DIRECTION_AUTO, MJB_DIRECTION_LTR, MJB_DIRECTION_RTL
    };

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

            while((tok = strsep(&p, " \t")) != NULL && levels_count < 64) {
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

            while((tok = strsep(&p, " \t")) != NULL && order_count < 64) {
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

        while((tok = strsep(&p, " \t")) != NULL) {
            if(!tok[0]) {
                continue;
            }

            mjb_codepoint cp = bidi_class_codepoint(tok);

            if(cp == 0) {
                skip = true;
                break;
            }

            unsigned int enc = mjb_codepoint_encode(cp, utf8_buf + utf8_len,
                sizeof(utf8_buf) - utf8_len - 1, MJB_ENCODING_UTF_8);

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

            if(!mjb_bidi_resolve(utf8_buf, utf8_len, MJB_ENCODING_UTF_8, directions[d],
                &para)) {
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

                if(mjb_bidi_reorder_line(&para, 0, para.count, visual_order)) {
                    for(size_t v = 0; v < order_count; ++v) {
                        ++total;

                        if(orig_pos[visual_order[v]] == expected_order[v]) {
                            ++successful;
                        }
                    }
                }
            }

            mjb_bidi_free(&para);
        }

        char test_name[64];
        snprintf(test_name, sizeof(test_name), "BidiTest #%u", current_line);

        // CURRENT_ASSERT mjb_bidi_resolve
        // CURRENT_COUNT 490846
        ATT_ASSERT(total, successful, test_name)
    }

    fclose(file);
}

void *test_bidi(void *arg) {
    mjb_bidi_paragraph para;
    bool ok;

    ATT_ASSERT(mjb_bidi_resolve(NULL, 1, MJB_ENCODING_UTF_8, MJB_DIRECTION_AUTO, &para),
        false, "resolve rejects NULL buffer")
    ATT_ASSERT(mjb_bidi_resolve("", 0, MJB_ENCODING_UTF_8, MJB_DIRECTION_AUTO, NULL),
        false, "resolve rejects NULL result")
    mjb_bidi_free(NULL);

    ok = mjb_bidi_resolve("", 0, MJB_ENCODING_UTF_8, MJB_DIRECTION_AUTO, &para);
    ATT_ASSERT(ok, true, "empty string resolve")
    ATT_ASSERT(para.count, (size_t)0, "empty string count")
    mjb_bidi_free(&para);

    const char *ltr = "ABC";
    ok = mjb_bidi_resolve(ltr, strlen(ltr), MJB_ENCODING_UTF_8, MJB_DIRECTION_AUTO, &para);
    ATT_ASSERT(ok, true, "LTR resolve ok")
    ATT_ASSERT(para.count, (size_t)3, "LTR count")
    ATT_ASSERT(para.paragraph_level, (uint8_t)0, "LTR paragraph level")
    ATT_ASSERT((unsigned int)para.direction, (unsigned int)MJB_DIRECTION_LTR, "LTR direction")

    if(para.count == 3) {
        ATT_ASSERT(para.chars[0].level, (uint8_t)0, "LTR A level")
        ATT_ASSERT(para.chars[1].level, (uint8_t)0, "LTR B level")
        ATT_ASSERT(para.chars[2].level, (uint8_t)0, "LTR C level")
        ATT_ASSERT(para.chars[0].codepoint, (mjb_codepoint)0x41, "LTR A codepoint")
    }

    ATT_ASSERT((mjb_bidi_free(&para), (void *)para.chars), (void *)NULL, "free clears chars")
    ATT_ASSERT((mjb_bidi_free(&para), para.count), (size_t)0, "free clears count")

    // CURRENT_ASSERT mjb_bidi_resolve
    const char *rtl = "\xD9\x85\xD8\xB1\xD8\xAD\xD8\xA8\xD8\xA7"; /* مرحبا */
    ok = mjb_bidi_resolve(rtl, strlen(rtl), MJB_ENCODING_UTF_8, MJB_DIRECTION_AUTO, &para);
    ATT_ASSERT(ok, true, "RTL resolve ok")
    ATT_ASSERT(para.paragraph_level, (uint8_t)1, "RTL paragraph level")
    ATT_ASSERT((unsigned int)para.direction, (unsigned int)MJB_DIRECTION_RTL, "RTL direction")

    if(para.count > 0) {
        // All characters should be at an odd level
        ATT_ASSERT((para.chars[0].level & 1), (uint8_t)1, "RTL char level is odd")
    }

    mjb_bidi_free(&para);

    ok = mjb_bidi_resolve(rtl, strlen(rtl), MJB_ENCODING_UTF_8, MJB_DIRECTION_LTR, &para);
    ATT_ASSERT(ok, true, "explicit LTR dir resolve")
    ATT_ASSERT(para.paragraph_level, (uint8_t)0, "explicit LTR paragraph level")
    mjb_bidi_free(&para);

    ok = mjb_bidi_resolve(ltr, strlen(ltr), MJB_ENCODING_UTF_8, MJB_DIRECTION_RTL, &para);
    ATT_ASSERT(ok, true, "explicit RTL dir resolve")
    ATT_ASSERT(para.paragraph_level, (uint8_t)1, "explicit RTL paragraph level")
    mjb_bidi_free(&para);

    // Hello مرحبا
    const char *mixed = "Hello \xD9\x85\xD8\xB1\xD8\xAD\xD8\xA8\xD8\xA7";
    ok = mjb_bidi_resolve(mixed, strlen(mixed), MJB_ENCODING_UTF_8, MJB_DIRECTION_AUTO, &para);
    ATT_ASSERT(ok, true, "mixed resolve ok")
    ATT_ASSERT(para.paragraph_level, (uint8_t)0, "mixed paragraph level LTR")

    if(para.count >= 11) {
        // "Hello " = 6 chars at level 0
        ATT_ASSERT(para.chars[0].level, (uint8_t)0, "mixed H level")
        ATT_ASSERT(para.chars[5].level, (uint8_t)0, "mixed space level (pre-Arabic)")
        // Arabic chars should be at odd level
        ATT_ASSERT((para.chars[6].level & 1), (uint8_t)1, "mixed Arabic char level odd")
    }

    mjb_bidi_free(&para);

    ok = mjb_bidi_resolve(ltr, strlen(ltr), MJB_ENCODING_UTF_8, MJB_DIRECTION_LTR, &para);
    ATT_ASSERT(ok, true, "reorder ltr resolve")

    if(para.count == 3) {
        size_t order[3];
        ATT_ASSERT(mjb_bidi_reorder_line(NULL, 0, 3, order), false,
            "reorder rejects NULL paragraph")
        ATT_ASSERT(mjb_bidi_reorder_line(&para, 0, 3, NULL), false,
            "reorder rejects NULL visual order")
        ATT_ASSERT(mjb_bidi_reorder_line(&para, 0, 3, order), true, "reorder ltr ok")
        ATT_ASSERT(order[0], (size_t)0, "LTR visual[0] = 0")
        ATT_ASSERT(order[1], (size_t)1, "LTR visual[1] = 1")
        ATT_ASSERT(order[2], (size_t)2, "LTR visual[2] = 2")
        ATT_ASSERT(mjb_bidi_reorder_line(&para, 1, 1, order), false, "reorder empty range")
        ATT_ASSERT(mjb_bidi_reorder_line(&para, 2, 1, order), false, "reorder reversed range")
        ATT_ASSERT(mjb_bidi_reorder_line(&para, 0, 4, order), false, "reorder beyond paragraph")
    }

    mjb_bidi_free(&para);

    const char *rtl3 = "\xD9\x85\xD8\xB1\xD8\xAD"; // مرح (3 Arabic chars)
    ok = mjb_bidi_resolve(rtl3, strlen(rtl3), MJB_ENCODING_UTF_8, MJB_DIRECTION_AUTO, &para);
    ATT_ASSERT(ok, true, "reorder rtl resolve")

    if(para.count == 3) {
        size_t order[3];
        ATT_ASSERT(mjb_bidi_reorder_line(&para, 0, 3, order), true, "reorder rtl ok")
        // All chars at odd level; L2 reversal reverses the sequence
        ATT_ASSERT(order[0], (size_t)2, "RTL visual[0] = 2")
        ATT_ASSERT(order[1], (size_t)1, "RTL visual[1] = 1")
        ATT_ASSERT(order[2], (size_t)0, "RTL visual[2] = 0")
    }

    mjb_bidi_free(&para);

    ok = mjb_bidi_resolve(ltr, strlen(ltr), MJB_ENCODING_UTF_8, MJB_DIRECTION_LTR, &para);

    if(ok && para.count == 3) {
        size_t order[3];
        ATT_ASSERT(mjb_bidi_reorder_line(&para, 0, 3, order), true, "line runs reorder ok")

        size_t run_count = 0;
        ATT_ASSERT(mjb_bidi_line_runs(NULL, order, 3, NULL, &run_count), false,
            "line runs rejects NULL paragraph")
        ATT_ASSERT(mjb_bidi_line_runs(&para, NULL, 3, NULL, &run_count), false,
            "line runs rejects NULL visual order")
        ATT_ASSERT(mjb_bidi_line_runs(&para, order, 3, NULL, NULL), false,
            "line runs rejects NULL run count")
        ATT_ASSERT(mjb_bidi_line_runs(&para, order, 0, NULL, &run_count), true,
            "empty line runs ok")
        ATT_ASSERT(run_count, (size_t)0, "empty line runs count")
        ATT_ASSERT(mjb_bidi_line_runs(&para, order, 3, NULL, &run_count), true,
            "line runs count ok")
        ATT_ASSERT(run_count, (size_t)1, "LTR one run")

        mjb_bidi_run runs[4];
        ATT_ASSERT(mjb_bidi_line_runs(&para, order, 3, runs, &run_count), true,
            "line runs fill ok")
        ATT_ASSERT((unsigned int)runs[0].direction, (unsigned int)MJB_DIRECTION_LTR,
            "LTR run direction")
        ATT_ASSERT(runs[0].start, (size_t)0, "LTR run start")
        ATT_ASSERT(runs[0].end, (size_t)3, "LTR run end")
    }

    mjb_bidi_free(&para);

    read_bidi_test_file("./utils/generate/unicode-data/UCD/BidiCharacterTest.txt");
    read_bidi_class_test_file("./utils/generate/unicode-data/UCD/BidiTest.txt");

    return NULL;
}
