/**
 * The Mojibake library
 *
 * Unicode Collation Algorithm conformance tests.
 * Runs CollationTest_NON_IGNORABLE_SHORT.txt and CollationTest_SHIFTED_SHORT.txt.
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "test.h"

/* Maximum UTF-8 bytes for a single test line (each codepoint ≤ 4 bytes,
   the longest test lines have ~6 codepoints = 24 bytes, but we keep slack). */
#define COLLATION_BUF 256

/**
 * Convert a whitespace-separated list of hex codepoints to UTF-8.
 * Returns the number of bytes written (excluding NUL terminator).
 * `hex`  : "0041 0042 0043"
 * `buf`  : output buffer (at least COLLATION_BUF bytes)
 */
static size_t hex_codepoints_to_utf8(const char *hex, char *buf, size_t buf_size) {
    char tmp[128];
    size_t out = 0;

    /* Strip trailing comment (anything after '#') */
    const char *hash = strchr(hex, '#');
    size_t copy_len = hash ? (size_t)(hash - hex) : strlen(hex);

    if(copy_len >= sizeof(tmp)) copy_len = sizeof(tmp) - 1;

    memcpy(tmp, hex, copy_len);
    tmp[copy_len] = '\0';

    char *p = tmp;

    while(*p) {
        /* skip whitespace */
        while(*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;

        if(*p == '\0') break;

        char *end;
        unsigned long cp = strtoul(p, &end, 16);

        if(end == p) break; /* not a hex number */

        p = end;

        if(cp == 0) continue; /* skip U+0000 (embedded NUL) */

        unsigned int enc = mjb_codepoint_encode((mjb_codepoint)cp,
            buf + out, (unsigned int)(buf_size - out - 1),
            MJB_ENCODING_UTF_8);

        out += enc;
    }

    buf[out] = '\0';
    return out;
}

/**
 * Run collation conformance test against one SHORT test file.
 * Each non-comment line contains a string (as hex codepoints).
 * Consecutive strings must be in non-descending collation order.
 */
static void run_collation_test_file(const char *filename, mjb_collation_mode mode,
    const char *test_name) {
    FILE *f = fopen(filename, "r");

    if(!f) {
        char msg[512];
        snprintf(msg, sizeof(msg), "Cannot open %s", filename);
        ATT_ASSERT("Opened", "Not opened", msg)
        return;
    }

    char line[512];
    char prev_utf8[COLLATION_BUF] = {0};
    size_t prev_len = 0;
    unsigned int lineno = 0;
    unsigned int tested = 0;
    unsigned int failures = 0;

    while(fgets(line, sizeof(line), f)) {
        lineno++;

        /* Skip comment lines and empty lines */
        if(line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;

        char curr_utf8[COLLATION_BUF];
        size_t curr_len = hex_codepoints_to_utf8(line, curr_utf8, sizeof(curr_utf8));

        if(curr_len == 0) continue;

        if(prev_len > 0) {
            int cmp = mjb_string_compare(prev_utf8, prev_len, curr_utf8, curr_len,
                MJB_ENCODING_UTF_8, mode);

            /* Must be <= 0 (prev collates before or equal to curr) */
            if(cmp > 0) {
                ++failures;
                char msg[512];
                snprintf(msg, sizeof(msg), "%s line %u: prev > curr (cmp=%d)", test_name, lineno, cmp);
                ATT_ASSERT(0, 1, msg)

                if(is_exit_on_error()) break;
            } else {
                // CURRENT_ASSERT mjb_string_compare
                // CURRENT_COUNT 437932
                ATT_ASSERT(0, 0, "Collation: prev <= curr")
            }

            ++tested;
        }

        memcpy(prev_utf8, curr_utf8, curr_len + 1);
        prev_len = curr_len;
    }

    fclose(f);

    char summary[512];
    snprintf(summary, sizeof(summary), "%s: %u/%u pairs passed",
        test_name, tested - failures, tested);

    ATT_ASSERT(failures, 0u, summary)
}

void *test_collation(void *arg) {
    /* Sanity checks */
    ATT_ASSERT(mjb_string_compare("", 0, "", 0, MJB_ENCODING_UTF_8, MJB_COLLATION_NON_IGNORABLE), 0, "Collation: '' == ''")
    ATT_ASSERT(mjb_string_compare("hello", 5, "hello", 5, MJB_ENCODING_UTF_8, MJB_COLLATION_NON_IGNORABLE), 0, "Collation: hello == hello")

    /* UCA conformance tests */
    run_collation_test_file(
        "./utils/generate/collation/CollationTest/CollationTest_NON_IGNORABLE_SHORT.txt",
        MJB_COLLATION_NON_IGNORABLE,
        "NON_IGNORABLE");

    run_collation_test_file(
        "./utils/generate/collation/CollationTest/CollationTest_SHIFTED_SHORT.txt",
        MJB_COLLATION_SHIFTED,
        "SHIFTED");

    return NULL;
}
