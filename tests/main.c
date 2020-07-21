//
//  main.c
//  unicodex_test
//
//  Created by Francesco Bigiarini on 23/02/18.
//  Copyright © 2018 Francesco Bigiarini. All rights reserved.
//

#include <stdio.h>
#include <string.h>

#include "ucx.h"
#include "version.h"

typedef void (*ucxt_test)(void);

static int tests_run;
static int tests_valid;

void uctx_assert(char *message, int test) {
    printf("Test: %s ", message);
    ++tests_run;

    if(test) {
        printf("ok\n");
        ++tests_valid;
    } else {
        printf("fail\n");
    }
}

static void ucxt_run_test(char *name, ucxt_test test) {
    printf("%s\n", name);
    test();
    printf("\n");
}

static void ucx_get_version_test() {
    char *version = ucx_get_version();
    size_t size = sizeof(UCX_VERSION);
    int result = strncmp(version, UCX_VERSION, size);

    uctx_assert("valid version", result == 0);
}

static void ucx_get_version_number_test() {
    unsigned int version_number = ucx_get_version_number();

    uctx_assert("valid version number", version_number == UCX_VERSION_NUMBER);
}

static void ucx_get_unicode_version_test() {
    char *version = ucx_get_unicode_version();
    size_t size = sizeof(UCX_UNICODE_VERSION);
    int result = strncmp(version, UCX_UNICODE_VERSION, size);

    uctx_assert("valid unicode version", result == 0);
}

static void ucx_codepoint_is_valid_test() {
    int validity = ucx_codepoint_is_valid(UCX_CODEPOINT_MIN + 1);
    uctx_assert("valid codepoint", validity);

    validity = ucx_codepoint_is_valid(UCX_CODEPOINT_MIN - 1);
    uctx_assert("not valid negative codepoint", !validity);

    validity = ucx_codepoint_is_valid(UCX_CODEPOINT_MAX + 1);
    uctx_assert("not valid exceed codepoint", !validity);

    validity = ucx_codepoint_is_valid(0x1FFFE);
    uctx_assert("not valid codepoint ending in 0XFFFE", !validity);

    validity = ucx_codepoint_is_valid(0x1FFFF);
    uctx_assert("not valid codepoint ending in 0XFFFF", !validity);

    validity = ucx_codepoint_is_valid(0xFFFE);
    uctx_assert("not valid codepoint 0XFFFE", !validity);

    validity = ucx_codepoint_is_valid(0xFFFF);
    uctx_assert("not valid codepoint 0XFFFF", !validity);

    char buffer[32];

    /* 32 noncharacters: U+FDD0–U+FDEF */
    for(ucx_codepoint i = 0xFDD0; i <= 0xFDEF; ++i) {
        validity = ucx_codepoint_is_valid(i);

        snprintf(buffer, 32, "invalid codepoint %#X", i);
        uctx_assert(buffer, !validity);
    }
}

static void ucx_codespace_plane_is_valid_test() {
    int validity = ucx_codespace_plane_is_valid(UCX_CODESPACE_PLANE_MIN + 1);
    uctx_assert("valid codespace plane", validity);

    validity = ucx_codespace_plane_is_valid(UCX_CODESPACE_PLANE_MIN - 1);
    uctx_assert("not valid negative codespace plane", !validity);

    validity = ucx_codespace_plane_is_valid(UCX_CODESPACE_PLANE_MAX + 1);
    uctx_assert("not valid exceed codespace plane", !validity);
}

static void ucx_string_get_encoding_test() {
    ucx_encoding encoding = ucx_string_get_encoding(0, 10);
    uctx_assert("void string", encoding == UCX_ERRNO);

    encoding = ucx_string_get_encoding("", 0);
    uctx_assert("void length", encoding == UCX_ENCODING_UNKNOWN);

    encoding = ucx_string_get_encoding(0, 0);
    uctx_assert("void string and length", encoding == UCX_ERRNO);

    const char *test1 = "The quick brown fox jumps over the lazy dog";
    encoding = ucx_string_get_encoding(test1, 43);
    uctx_assert("Plain ASCII (and UTF-8)", encoding == (UCX_ENCODING_ASCII |
        UCX_ENCODING_UTF_8));

    test1 = "\xEF\xBB\xBFThe quick brown fox jumps over the lazy dog";
    encoding = ucx_string_get_encoding(test1, 43 + 3);
    uctx_assert("UTF-8 BOM", encoding == UCX_ENCODING_UTF_8);

    test1 = "\xFE\xFFThe quick brown fox jumps over the lazy dog";
    encoding = ucx_string_get_encoding(test1, 43 + 2);
    uctx_assert("UTF-16-BE BOM", encoding == UCX_ENCODING_UTF_16_BE);

    test1 = "\xFF\xFEThe quick brown fox jumps over the lazy dog";
    encoding = ucx_string_get_encoding(test1, 43 + 2);
    uctx_assert("UTF-16-LE BOM", encoding == UCX_ENCODING_UTF_16_LE);

    test1 = "\x00\x00\xFE\xFFThe quick brown fox jumps over the lazy dog";

    encoding = ucx_string_get_encoding(test1, 43 + 4);
    uctx_assert("UTF-32-BE BOM", encoding == UCX_ENCODING_UTF_32_BE);

    test1 = "\xFF\xFE\x00\x00The quick brown fox jumps over the lazy dog";
    encoding = ucx_string_get_encoding(test1, 43 + 4);
    uctx_assert("UTF-32-LE BOM", encoding == (UCX_ENCODING_UTF_32_LE |
        UCX_ENCODING_UTF_16_LE));
}

static void ucx_string_is_ascii_test() {
    int is_ascii = ucx_string_is_ascii("", 0);
    uctx_assert("void string", !is_ascii);

    is_ascii = ucx_string_is_ascii("", 0);
    uctx_assert("void length", !is_ascii);

    is_ascii = ucx_string_is_ascii(0, 0);
    uctx_assert("void string and length", is_ascii == UCX_ERRNO);

    const char *test = "The quick brown fox jumps over the lazy dog";
    is_ascii = ucx_string_is_ascii(test, 43);
    uctx_assert("void string and length", is_ascii);

    /* \xF0\x9F\x99\x82 = 🙂 */
    test = "\xF0\x9F\x99\x82";
    is_ascii = ucx_string_is_ascii(test, 5);
    uctx_assert("string with emoji", !is_ascii);

    test = "\x80";
    is_ascii = ucx_string_is_ascii(test, 2);
    uctx_assert("lone continuation byte", !is_ascii);

    test = "\xC0";
    is_ascii = ucx_string_is_ascii(test, 2);
    uctx_assert("lone first 2-bytes sequence", !is_ascii);

    test = "\xE0";
    is_ascii = ucx_string_is_ascii(test, 2);
    uctx_assert("lone first 3-bytes sequence", !is_ascii);

    test = "\xF0";
    is_ascii = ucx_string_is_ascii(test, 2);
    uctx_assert("lone first 4-bytes sequence", !is_ascii);
}

static void ucx_string_is_utf8_test() {
    int is_utf8 = ucx_string_is_utf8("", 0);
    uctx_assert("void string", !is_utf8);

    is_utf8 = ucx_string_is_utf8("", 0);
    uctx_assert("void length", !is_utf8);

    is_utf8 = ucx_string_is_utf8(0, 0);
    uctx_assert("void string and length", is_utf8 == UCX_ERRNO);

    const char *test = "The quick brown fox jumps over the lazy dog";
    is_utf8 = ucx_string_is_utf8(test, 43);
    uctx_assert("void string and length", is_utf8);

    /* \xF0\x9F\x99\x82 = 🙂 */
    test = "The quick brown fox jumps over the lazy dog \xF0\x9F\x99\x82";
    is_utf8 = ucx_string_is_utf8(test, 48);
    uctx_assert("string with emoji", is_utf8);

    test = "The quick brown fox jumps over the lazy dog \xF0\x9F\x99\x82";
    is_utf8 = ucx_string_is_utf8(test, 48);
    uctx_assert("Invalid continuation byte", is_utf8);
}

int main(int argc, const char * argv[]) {
    printf("Unicodex %s test\n\n", ucx_get_version());

    ucxt_run_test("Get version", ucx_get_version_test);
    ucxt_run_test("Get version number", ucx_get_version_number_test);
    ucxt_run_test("Get unicode version", ucx_get_unicode_version_test);
    ucxt_run_test("Codepoint is valid", ucx_codepoint_is_valid_test);
    ucxt_run_test("Codespace plane is valid",
        ucx_codespace_plane_is_valid_test);
    ucxt_run_test("String get encoding", ucx_string_get_encoding_test);
    ucxt_run_test("String is ASCII", ucx_string_is_ascii_test);
    ucxt_run_test("String is UTF-8", ucx_string_is_utf8_test);

    printf("Tests valid/run: %d/%d\n", tests_valid, tests_run);

    if(tests_valid == tests_run) {
        printf("All tests passed\n");
    }

    return tests_run == tests_valid ? 0 : -1;
}
