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

        char cp_copy[512];
        strncpy(cp_copy, fields[0], sizeof(cp_copy) - 1);
        cp_copy[sizeof(cp_copy) - 1] = '\0';
        char *cp_ptr = cp_copy;
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

        char lvl_copy[512];
        strncpy(lvl_copy, fields[3], sizeof(lvl_copy) - 1);
        lvl_copy[sizeof(lvl_copy) - 1] = '\0';
        char *lvl_ptr = lvl_copy;
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

        char ord_copy[512];
        strncpy(ord_copy, fields[4], sizeof(ord_copy) - 1);
        ord_copy[sizeof(ord_copy) - 1] = '\0';
        char *ord_ptr = ord_copy;
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
            // CURRENT_ASSERT mjb_bidi_resolve
            // CURRENT_COUNT 91745
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
        ATT_ASSERT(total, successful, test_name)

        mjb_bidi_free(&para);
    }

    fclose(file);
}

void *test_bidi(void *arg) {
    mjb_bidi_paragraph para;
    bool ok;

    ok = mjb_bidi_resolve("", 0, MJB_ENCODING_UTF_8, MJB_DIRECTION_AUTO, &para);
    ATT_ASSERT(ok, true, "empty string resolve");
    ATT_ASSERT(para.count, (size_t)0, "empty string count");
    mjb_bidi_free(&para);

    const char *ltr = "ABC";
    ok = mjb_bidi_resolve(ltr, strlen(ltr), MJB_ENCODING_UTF_8, MJB_DIRECTION_AUTO, &para);
    ATT_ASSERT(ok, true, "LTR resolve ok");
    ATT_ASSERT(para.count, (size_t)3, "LTR count");
    ATT_ASSERT(para.paragraph_level, (uint8_t)0, "LTR paragraph level");
    ATT_ASSERT((unsigned int)para.direction, (unsigned int)MJB_DIRECTION_LTR, "LTR direction");

    if(para.count == 3) {
        ATT_ASSERT(para.chars[0].level, (uint8_t)0, "LTR A level");
        ATT_ASSERT(para.chars[1].level, (uint8_t)0, "LTR B level");
        ATT_ASSERT(para.chars[2].level, (uint8_t)0, "LTR C level");
        ATT_ASSERT(para.chars[0].codepoint, (mjb_codepoint)0x41, "LTR A codepoint");
    }

    mjb_bidi_free(&para);
    ATT_ASSERT(para.chars, (mjb_bidi_char *)NULL, "free clears chars");
    ATT_ASSERT(para.count, (size_t)0, "free clears count");

    const char *rtl = "\xD9\x85\xD8\xB1\xD8\xAD\xD8\xA8\xD8\xA7"; /* مرحبا */
    ok = mjb_bidi_resolve(rtl, strlen(rtl), MJB_ENCODING_UTF_8, MJB_DIRECTION_AUTO, &para);
    ATT_ASSERT(ok, true, "RTL resolve ok");
    ATT_ASSERT(para.paragraph_level, (uint8_t)1, "RTL paragraph level");
    ATT_ASSERT((unsigned int)para.direction, (unsigned int)MJB_DIRECTION_RTL, "RTL direction");

    if(para.count > 0) {
        // All characters should be at an odd level
        ATT_ASSERT((para.chars[0].level & 1), (uint8_t)1, "RTL char level is odd");
    }

    mjb_bidi_free(&para);

    ok = mjb_bidi_resolve(rtl, strlen(rtl), MJB_ENCODING_UTF_8, MJB_DIRECTION_LTR, &para);
    ATT_ASSERT(ok, true, "explicit LTR dir resolve");
    ATT_ASSERT(para.paragraph_level, (uint8_t)0, "explicit LTR paragraph level");
    mjb_bidi_free(&para);

    ok = mjb_bidi_resolve(ltr, strlen(ltr), MJB_ENCODING_UTF_8, MJB_DIRECTION_RTL, &para);
    ATT_ASSERT(ok, true, "explicit RTL dir resolve");
    ATT_ASSERT(para.paragraph_level, (uint8_t)1, "explicit RTL paragraph level");
    mjb_bidi_free(&para);

    // Hello مرحبا
    const char *mixed = "Hello \xD9\x85\xD8\xB1\xD8\xAD\xD8\xA8\xD8\xA7";
    ok = mjb_bidi_resolve(mixed, strlen(mixed), MJB_ENCODING_UTF_8, MJB_DIRECTION_AUTO, &para);
    ATT_ASSERT(ok, true, "mixed resolve ok");
    ATT_ASSERT(para.paragraph_level, (uint8_t)0, "mixed paragraph level LTR");

    if(para.count >= 11) {
        // "Hello " = 6 chars at level 0
        ATT_ASSERT(para.chars[0].level, (uint8_t)0, "mixed H level");
        ATT_ASSERT(para.chars[5].level, (uint8_t)0, "mixed space level (pre-Arabic)");
        // Arabic chars should be at odd level
        ATT_ASSERT((para.chars[6].level & 1), (uint8_t)1, "mixed Arabic char level odd");
    }

    mjb_bidi_free(&para);

    ok = mjb_bidi_resolve(ltr, strlen(ltr), MJB_ENCODING_UTF_8, MJB_DIRECTION_LTR, &para);
    ATT_ASSERT(ok, true, "reorder ltr resolve");

    if(para.count == 3) {
        size_t order[3];
        ok = mjb_bidi_reorder_line(&para, 0, 3, order);
        ATT_ASSERT(ok, true, "reorder ltr ok");
        ATT_ASSERT(order[0], (size_t)0, "LTR visual[0] = 0");
        ATT_ASSERT(order[1], (size_t)1, "LTR visual[1] = 1");
        ATT_ASSERT(order[2], (size_t)2, "LTR visual[2] = 2");
    }

    mjb_bidi_free(&para);

    const char *rtl3 = "\xD9\x85\xD8\xB1\xD8\xAD"; // مرح (3 Arabic chars)
    ok = mjb_bidi_resolve(rtl3, strlen(rtl3), MJB_ENCODING_UTF_8, MJB_DIRECTION_AUTO, &para);
    ATT_ASSERT(ok, true, "reorder rtl resolve");

    if(para.count == 3) {
        size_t order[3];
        ok = mjb_bidi_reorder_line(&para, 0, 3, order);
        ATT_ASSERT(ok, true, "reorder rtl ok");
        // All chars at odd level; L2 reversal reverses the sequence
        ATT_ASSERT(order[0], (size_t)2, "RTL visual[0] = 2");
        ATT_ASSERT(order[1], (size_t)1, "RTL visual[1] = 1");
        ATT_ASSERT(order[2], (size_t)0, "RTL visual[2] = 0");
    }

    mjb_bidi_free(&para);

    ok = mjb_bidi_resolve(ltr, strlen(ltr), MJB_ENCODING_UTF_8, MJB_DIRECTION_LTR, &para);

    if(ok && para.count == 3) {
        size_t order[3];
        mjb_bidi_reorder_line(&para, 0, 3, order);

        size_t run_count = 0;
        ok = mjb_bidi_line_runs(&para, order, 3, NULL, &run_count);
        ATT_ASSERT(ok, true, "line runs count ok");
        ATT_ASSERT(run_count, (size_t)1, "LTR one run");

        mjb_bidi_run runs[4];
        ok = mjb_bidi_line_runs(&para, order, 3, runs, &run_count);
        ATT_ASSERT(ok, true, "line runs fill ok");
        ATT_ASSERT((unsigned int)runs[0].direction, (unsigned int)MJB_DIRECTION_LTR, "LTR run direction");
        ATT_ASSERT(runs[0].start, (size_t)0, "LTR run start");
        ATT_ASSERT(runs[0].end, (size_t)3, "LTR run end");
    }

    mjb_bidi_free(&para);

    read_bidi_test_file("./utils/generate/UCD/BidiCharacterTest.txt");

    return NULL;
}
