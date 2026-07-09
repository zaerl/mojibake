/**
 * The Mojibake library
 *
 * Unicode Collation Algorithm conformance tests.
 * Runs CollationTest_NON_IGNORABLE.txt and CollationTest_SHIFTED.txt.
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "test.h"

// Maximum UTF-8 bytes for a single test line (each codepoint <= 4 bytes, the longest test lines
// have ~6 codepoints = 24 bytes, but we keep slack).
#define COLLATION_BUF 256

static bool is_surrogate_codepoint(unsigned long cp) {
    return cp >= 0xD800 && cp <= 0xDFFF;
}

static size_t encode_collation_codepoint(unsigned long cp, char *buf, size_t buf_size) {
    return mjb_codepoint_encode((mjb_codepoint)cp, buf, buf_size, MJB_ENC_UTF_8);
}

/**
 * Convert a whitespace-separated list of hex codepoints to UTF-8.
 * Returns false for lines containing surrogate code points, because those code points cannot be
 * represented as well-formed public UTF-8 input. CollationTest.html explicitly allows
 * implementations that do not weight surrogates like reserved code points to filter those lines.
 * `hex`: "0041 0042 0043"
 * `buf`: output buffer (at least COLLATION_BUF bytes)
 */
static bool hex_codepoints_to_utf8(const char *hex, char *buf, size_t buf_size, size_t *out_len) {
    char tmp[128];
    size_t out = 0;

    *out_len = 0;

    // Strip trailing comment (anything after '#')
    const char *hash = strchr(hex, '#');
    size_t copy_len = hash ? (size_t)(hash - hex) : strlen(hex);

    if(copy_len >= sizeof(tmp)) copy_len = sizeof(tmp) - 1;

    memcpy(tmp, hex, copy_len);
    tmp[copy_len] = '\0';

    char *p = tmp;

    while(*p) {
        // skip whitespace
        while(*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;

        if(*p == '\0') break;

        char *end;
        unsigned long cp = strtoul(p, &end, 16);

        if(end == p) break; // not a hex number

        p = end;

        if(cp == 0) continue; // skip U+0000 (embedded NUL)

        if(is_surrogate_codepoint(cp)) {
            return false;
        }

        size_t enc = encode_collation_codepoint(cp, buf + out, buf_size - out - 1);

        if(enc == 0) {
            return false;
        }

        out += enc;
    }

    buf[out] = '\0';
    *out_len = out;

    return true;
}

static void assert_collation_malformed_utf8(const unsigned char *buffer, size_t byte_length,
    const char *message) {
    mjb_result result;
    const char *bytes = (const char*)buffer;

    ATT_ASSERT_STATUS(mjb_collation_key(bytes, byte_length, MJB_ENC_UTF_8,
        MJB_COLLATION_NON_IGNORABLE, &result), MJB_STATUS_MALFORMED_INPUT, message)
    ATT_ASSERT(mjb_string_compare(bytes, byte_length, MJB_ENC_UTF_8, bytes, byte_length,
        MJB_ENC_UTF_8, MJB_COLLATION_NON_IGNORABLE), -1, message)
}

/**
 * Run collation conformance test against one UCA test file.
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

        // Skip comment lines and empty lines
        if(line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;

        char curr_utf8[COLLATION_BUF];
        size_t curr_len = 0;

        if(!hex_codepoints_to_utf8(line, curr_utf8, sizeof(curr_utf8), &curr_len) ||
            curr_len == 0) {
            continue;
        }

        if(prev_len > 0) {
            int cmp = mjb_string_compare(prev_utf8, prev_len, MJB_ENC_UTF_8, curr_utf8, curr_len,
                MJB_ENC_UTF_8, mode);
            MJB_TEST_COVERAGE(mjb_string_compare);

            // Must be <= 0 (prev collates before or equal to curr)
            if(cmp > 0) {
                ++failures;
                char msg[512];
                snprintf(msg, sizeof(msg), "%s line %u: prev > curr (cmp=%d)", test_name, lineno, cmp);
                ATT_ASSERT(0, 1, msg)

                if(is_exit_on_error()) break;
            } else {
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

int test_collation(void *arg) {
    // mjb_collation_key
    mjb_result ka, kb, kc, kd;

    ATT_ASSERT_STATUS(mjb_collation_key(NULL, 1, MJB_ENC_UTF_8,
        MJB_COLLATION_NON_IGNORABLE, &ka), MJB_STATUS_INVALID_ARGUMENT,
        "Key rejects NULL buffer")
    ATT_ASSERT_STATUS(mjb_collation_key("a", 1, MJB_ENC_UTF_8,
        MJB_COLLATION_NON_IGNORABLE, NULL), MJB_STATUS_INVALID_ARGUMENT,
        "Key rejects NULL result")
    ATT_ASSERT(mjb_string_compare(NULL, 1, MJB_ENC_UTF_8, "a", 1, MJB_ENC_UTF_8,
        MJB_COLLATION_NON_IGNORABLE), -1, "Compare rejects NULL left")

    const unsigned char cesu8_high_surrogate[] = { 0xED, 0xA0, 0x80 };
    const unsigned char cesu8_low_surrogate[] = { 0xED, 0xBF, 0xBF };
    const unsigned char lone_continuation[] = { 0x80 };
    const unsigned char overlong_slash[] = { 0xC0, 0xAF };
    const unsigned char truncated_three_byte[] = { 0xE2, 0x82 };
    const unsigned char invalid_four_byte[] = { 0xF5, 0x80, 0x80, 0x80 };

    assert_collation_malformed_utf8(cesu8_high_surrogate, sizeof(cesu8_high_surrogate),
        "Collation rejects CESU-8 high surrogate");
    assert_collation_malformed_utf8(cesu8_low_surrogate, sizeof(cesu8_low_surrogate),
        "Collation rejects CESU-8 low surrogate");
    assert_collation_malformed_utf8(lone_continuation, sizeof(lone_continuation),
        "Collation rejects lone continuation byte");
    assert_collation_malformed_utf8(overlong_slash, sizeof(overlong_slash),
        "Collation rejects overlong UTF-8");
    assert_collation_malformed_utf8(truncated_three_byte, sizeof(truncated_three_byte),
        "Collation rejects truncated UTF-8");
    assert_collation_malformed_utf8(invalid_four_byte, sizeof(invalid_four_byte),
        "Collation rejects invalid four-byte UTF-8");

    // Key generation succeeds
    ATT_ASSERT_STATUS(mjb_collation_key("a", 1, MJB_ENC_UTF_8,
        MJB_COLLATION_NON_IGNORABLE, &ka), MJB_STATUS_OK, "Key: 'a' succeeds")
    ATT_ASSERT_STATUS(mjb_collation_key("b", 1, MJB_ENC_UTF_8,
        MJB_COLLATION_NON_IGNORABLE, &kb), MJB_STATUS_OK, "Key: 'b' succeeds")

    // key("a") < key("b") byte-for-byte
    ATT_ASSERT((int)(ka.output_size > 0), 1, "Key: 'a' non-empty")
    ATT_ASSERT((int)(kb.output_size > 0), 1, "Key: 'b' non-empty")
    ATT_ASSERT((int)(memcmp(ka.output, kb.output, ka.output_size < kb.output_size ? ka.output_size : kb.output_size) < 0), 1, "Key: 'a' < 'b'")

    // Same string -> identical keys
    ATT_ASSERT_STATUS(mjb_collation_key("hello", 5, MJB_ENC_UTF_8,
        MJB_COLLATION_NON_IGNORABLE, &kc), MJB_STATUS_OK, "Key: 'hello' NI succeeds")
    ATT_ASSERT_STATUS(mjb_collation_key("hello", 5, MJB_ENC_UTF_8,
        MJB_COLLATION_NON_IGNORABLE, &kd), MJB_STATUS_OK,
        "Key: 'hello' NI second succeeds")
    ATT_ASSERT((int)(kc.output_size == kd.output_size), 1, "Key: same size")
    ATT_ASSERT((int)(memcmp(kc.output, kd.output, kc.output_size) == 0), 1, "Key: 'hello' == 'hello'")

    mjb_free(kc.output);
    mjb_free(kd.output);

    // NON_IGNORABLE vs SHIFTED differ for strings with variable-weight punctuation
    ATT_ASSERT_STATUS(mjb_collation_key("a-b", 3, MJB_ENC_UTF_8,
        MJB_COLLATION_NON_IGNORABLE, &kc), MJB_STATUS_OK, "Key: 'a-b' NI succeeds")
    ATT_ASSERT_STATUS(mjb_collation_key("a-b", 3, MJB_ENC_UTF_8, MJB_COLLATION_SHIFTED,
        &kd), MJB_STATUS_OK, "Key: 'a-b' SHIFTED succeeds")
    ATT_ASSERT((int)(kc.output_size != kd.output_size || memcmp(kc.output, kd.output, kc.output_size) != 0), 1, "Key: NI != SHIFTED for 'a-b'")

    mjb_free(ka.output);
    mjb_free(kb.output);
    mjb_free(kc.output);
    mjb_free(kd.output);

    // Key ordering matches mjb_string_compare
    ATT_ASSERT_STATUS(mjb_collation_key("apple", 5, MJB_ENC_UTF_8,
        MJB_COLLATION_NON_IGNORABLE, &ka), MJB_STATUS_OK, "Key: 'apple' succeeds")
    ATT_ASSERT_STATUS(mjb_collation_key("banana", 6, MJB_ENC_UTF_8,
        MJB_COLLATION_NON_IGNORABLE, &kb), MJB_STATUS_OK, "Key: 'banana' succeeds")

    int cmp_direct = mjb_string_compare("apple", 5, MJB_ENC_UTF_8, "banana", 6, MJB_ENC_UTF_8,
        MJB_COLLATION_NON_IGNORABLE);
    size_t min_size = ka.output_size < kb.output_size ? ka.output_size : kb.output_size;
    int cmp_keys = memcmp(ka.output, kb.output, min_size);

    if(cmp_keys == 0) {
        cmp_keys = (int)ka.output_size - (int)kb.output_size;
    }

    ATT_ASSERT((int)((cmp_direct < 0) == (cmp_keys < 0)), 1, "Key: ordering matches mjb_string_compare")

    mjb_free(ka.output);
    mjb_free(kb.output);

    // Sanity checks
    ATT_ASSERT(mjb_string_compare("", 0, MJB_ENC_UTF_8, "", 0, MJB_ENC_UTF_8,
        MJB_COLLATION_NON_IGNORABLE), 0, "Collation: '' == ''")
    ATT_ASSERT(mjb_string_compare("hello", 5, MJB_ENC_UTF_8, "hello", 5, MJB_ENC_UTF_8,
        MJB_COLLATION_NON_IGNORABLE), 0, "Collation: hello == hello")

    const char hello_utf16le[] = { 'h', '\0', 'e', '\0', 'l', '\0', 'l', '\0', 'o', '\0' };
    ATT_ASSERT(mjb_string_compare("hello", 5, MJB_ENC_UTF_8, hello_utf16le, sizeof(hello_utf16le),
        MJB_ENC_UTF_16LE, MJB_COLLATION_NON_IGNORABLE), 0,
        "Collation: UTF-8 hello == UTF-16LE hello")

    const char apple_utf32le[] = {
        'a', '\0', '\0', '\0',
        'p', '\0', '\0', '\0',
        'p', '\0', '\0', '\0',
        'l', '\0', '\0', '\0',
        'e', '\0', '\0', '\0',
    };
    ATT_ASSERT((int)(mjb_string_compare(apple_utf32le, sizeof(apple_utf32le), MJB_ENC_UTF_32LE,
        "banana", 6, MJB_ENC_UTF_8, MJB_COLLATION_NON_IGNORABLE) < 0), 1,
        "Collation: UTF-32LE apple < UTF-8 banana")

    // UCA conformance tests
    run_collation_test_file(
        "./utils/generate/unicode-data/collation/CollationTest/"
        "CollationTest_NON_IGNORABLE.txt",
        MJB_COLLATION_NON_IGNORABLE,
        "NON_IGNORABLE");

    run_collation_test_file(
        "./utils/generate/unicode-data/collation/CollationTest/"
        "CollationTest_SHIFTED.txt",
        MJB_COLLATION_SHIFTED,
        "SHIFTED");

    return 0;
}
