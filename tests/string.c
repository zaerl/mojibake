/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>

#if defined(MJB_SHARED)
#include "../src/mojibake.h"
#else
#include "../src/mojibake-internal.h"
#endif
#include "test.h"

#if !defined(MJB_SHARED)
static mjb_status test_output_writer(mjb_output *output, const void *context) {
    (void)context;
    mjb_status status = mjb_output_write(output, "abc", 3);

    if(status != MJB_STATUS_OK) {
        return status;
    }

    return mjb_output_codepoint(output, 0xE9, MJB_ENC_UTF_8);
}

static mjb_status test_output_failure_writer(mjb_output *output, const void *context) {
    (void)output;
    (void)context;

    return MJB_STATUS_INVALID_ENCODING;
}
#endif

int test_string(void *arg) {
    mjb_encoding enc = MJB_ENC_UTF_8;

#if !defined(MJB_SHARED)
    size_t output_index = 0;
    size_t output_size = 0;
    char output_input[] = "A";

    ATT_ASSERT(mjb_string_output(NULL, NULL, 1, &output_index, &output_size), (char *)NULL,
        "String output rejects NULL input")
    ATT_ASSERT(mjb_string_output(NULL, output_input, 1, NULL, &output_size), (char *)NULL,
        "String output rejects NULL output index")
    ATT_ASSERT(mjb_string_output(NULL, output_input, 1, &output_index, NULL), (char *)NULL,
        "String output rejects NULL output size")

    size_t into_size = 0;
    ATT_ASSERT_STATUS(mjb_output_into(NULL, &into_size, test_output_writer, NULL), MJB_STATUS_OK,
        "Output sink measures a transformation")
    ATT_ASSERT(into_size, (size_t)5, "Output sink reports the required size")

    char into_output[6] = { '#', '#', '#', '#', '#', '#' };
    into_size = 4;
    ATT_ASSERT_STATUS(mjb_output_into(into_output, &into_size, test_output_writer, NULL),
        MJB_STATUS_OUTPUT_TOO_SMALL, "Output sink rejects insufficient capacity")
    ATT_ASSERT(into_size, (size_t)5, "Output sink preserves the required size")
    ATT_ASSERT(memcmp(into_output, "######", sizeof(into_output)), 0,
        "Output sink leaves a small buffer untouched")

    into_size = 5;
    ATT_ASSERT_STATUS(mjb_output_into(into_output, &into_size, test_output_writer, NULL),
        MJB_STATUS_OK, "Output sink writes into exact capacity")
    ATT_ASSERT(into_size, (size_t)5, "Output sink reports the written size")
    ATT_ASSERT(memcmp(into_output, "abc\xC3\xA9", into_size), 0,
        "Output sink writes bytes and encoded codepoints")
    ATT_ASSERT(into_output[5], '#', "Output sink does not write a terminator")

    into_size = sizeof(into_output);
    ATT_ASSERT_STATUS(mjb_output_into(into_output, &into_size, test_output_failure_writer, NULL),
        MJB_STATUS_INVALID_ENCODING, "Output sink propagates writer errors")
    ATT_ASSERT(into_size, (size_t)0, "Output sink clears the size after a writer error")
    ATT_ASSERT_STATUS(mjb_output_into(NULL, NULL, test_output_writer, NULL),
        MJB_STATUS_INVALID_ARGUMENT, "Output sink rejects a NULL size")
    ATT_ASSERT_STATUS(mjb_output_into(NULL, &into_size, NULL, NULL),
        MJB_STATUS_INVALID_ARGUMENT, "Output sink rejects a NULL writer")

    mjb_output measured = { NULL, SIZE_MAX, 0, MJB_OUTPUT_MEASURE };
    ATT_ASSERT_STATUS(mjb_output_write(&measured, "a", 1), MJB_STATUS_OVERFLOW,
        "Output sink detects size overflow")
    measured.size = 0;
    ATT_ASSERT_STATUS(mjb_output_codepoint(&measured, 0xE9, MJB_ENC_ASCII),
        MJB_STATUS_UNSUPPORTED, "Output sink rejects an unrepresentable codepoint")
#endif

    ATT_ASSERT(mjb_count_codepoints("Hello", 5, enc), 5, "UTF-8 length: Hello")
    ATT_ASSERT(mjb_count_codepoints("Hello", 4, enc), 4, "UTF-8 length: Hello")
    ATT_ASSERT(mjb_count_codepoints("Hello", 3, enc), 3, "UTF-8 length: Hello")
    ATT_ASSERT(mjb_count_codepoints("Hello", 2, enc), 2, "UTF-8 length: Hello")
    ATT_ASSERT(mjb_count_codepoints("Hello", 1, enc), 1, "UTF-8 length: Hello")
    ATT_ASSERT(mjb_count_codepoints("Hello", 0, enc), 0, "UTF-8 length: Hello")
    ATT_ASSERT(mjb_count_codepoints(NULL, 0, enc), 0, "UTF-8 length: NULL")

    ATT_ASSERT(mjb_count_codepoints("Héllö", 7, enc), 5, "UTF-8 length: Héllö")
    ATT_ASSERT(mjb_count_codepoints("Héllö", 4, enc), 3, "UTF-8 length: Héllö")
    ATT_ASSERT(mjb_count_codepoints("Héllö", 2, enc), 1, "UTF-8 length: Héllö")
    ATT_ASSERT(mjb_count_codepoints("Héllö", 0, enc), 0, "UTF-8 length: Héllö")
    ATT_ASSERT(mjb_count_codepoints("Hèllõ ツ", 11, enc), 7, "UTF-8 length: Hèllõ ツ")
    ATT_ASSERT(mjb_count_codepoints("Hèllõ ツ", 5, enc), 4, "UTF-8 length: Hèllõ ツ")
    ATT_ASSERT(mjb_count_codepoints("こんにちは", 15, enc), 5, "UTF-8 length: こんにちは")
    ATT_ASSERT(mjb_count_codepoints("Γειά σου", 15, enc), 8, "UTF-8 length: Γειά σου")
    ATT_ASSERT(mjb_count_codepoints("Héllö", 1, enc), 1, "UTF-8 length: Héllö (1 max value)")
    ATT_ASSERT(mjb_count_codepoints("Hello", 5, MJB_ENC_ASCII), 5, "ASCII length: Hello")
    ATT_ASSERT(mjb_count_codepoints("Hello", 5, MJB_ENC_UNKNOWN), 5,
        "Unknown encoding advances with replacement")

    enc = MJB_ENC_UTF_16LE;
    const char utf16le_hello[] = "H\0e\0l\0l\0o\0";
    ATT_ASSERT(mjb_count_codepoints(utf16le_hello, 10, enc), 5, "UTF-16LE length: Hello")
    ATT_ASSERT(mjb_count_codepoints(utf16le_hello, 8, enc), 4, "UTF-16LE length: Hello")
    ATT_ASSERT(mjb_count_codepoints(utf16le_hello, 6, enc), 3, "UTF-16LE length: Hello")
    ATT_ASSERT(mjb_count_codepoints(utf16le_hello, 4, enc), 2, "UTF-16LE length: Hello")
    ATT_ASSERT(mjb_count_codepoints(utf16le_hello, 2, enc), 1, "UTF-16LE length: Hello")
    ATT_ASSERT(mjb_count_codepoints(utf16le_hello, 0, enc), 0, "UTF-16LE length: Hello")
    ATT_ASSERT(mjb_count_codepoints(NULL, 0, enc), 0, "UTF-16LE length: NULL")

    const char utf16le_hello_accents[] = "H\0\xE9\0l\0l\0\xF6\0";
    ATT_ASSERT(mjb_count_codepoints(utf16le_hello_accents, 10, enc), 5, "UTF-16LE length: Héllö")
    ATT_ASSERT(mjb_count_codepoints(utf16le_hello_accents, 8, enc), 4, "UTF-16LE length: Héllö")
    ATT_ASSERT(mjb_count_codepoints(utf16le_hello_accents, 4, enc), 2, "UTF-16LE length: Héllö")
    ATT_ASSERT(mjb_count_codepoints(utf16le_hello_accents, 0, enc), 0, "UTF-16LE length: Héllö")

    const char utf16le_hello_katakana[] = "H\0\xE8\0l\0l\0\xF5\0 \0\x30\x30";
    ATT_ASSERT(mjb_count_codepoints(utf16le_hello_katakana, 14, enc), 7, "UTF-16LE length: Hèllõ ツ")
    ATT_ASSERT(mjb_count_codepoints(utf16le_hello_katakana, 10, enc), 5, "UTF-16LE length: Hèllõ ツ")

    const char utf16le_konnichiwa[] = "\x93\x30\x93\x30\x6B\x30\x61\x30\x6F\x30";
    ATT_ASSERT(mjb_count_codepoints(utf16le_konnichiwa, 10, enc), 5, "UTF-16LE length: こんにちは")

    const char utf16le_geia_sou[] = "\x93\x03\x65\x03\x69\x03\x3F\x03 \0\xC3\x03\x6F\x03\x75\x03";
    ATT_ASSERT(mjb_count_codepoints(utf16le_geia_sou, 16, enc), 8, "UTF-16LE length: Γειά σου")
    ATT_ASSERT(mjb_count_codepoints(utf16le_hello_accents, 2, enc), 1,
        "UTF-16LE length: Héllö (1 max value)")

    enc = MJB_ENC_UTF_16BE;
    const char utf16be_hello[] = "\0H\0e\0l\0l\0o";
    ATT_ASSERT(mjb_count_codepoints(utf16be_hello, 10, enc), 5, "UTF-16BE length: Hello")
    ATT_ASSERT(mjb_count_codepoints(utf16be_hello, 8, enc), 4, "UTF-16BE length: Hello")
    ATT_ASSERT(mjb_count_codepoints(utf16be_hello, 6, enc), 3, "UTF-16BE length: Hello")
    ATT_ASSERT(mjb_count_codepoints(utf16be_hello, 4, enc), 2, "UTF-16BE length: Hello")
    ATT_ASSERT(mjb_count_codepoints(utf16be_hello, 2, enc), 1, "UTF-16BE length: Hello")
    ATT_ASSERT(mjb_count_codepoints(utf16be_hello, 0, enc), 0, "UTF-16BE length: Hello")
    ATT_ASSERT(mjb_count_codepoints(NULL, 0, enc), 0, "UTF-16BE length: NULL")

    const char utf16be_hello_accents[] = "\0H\0\xE9\0l\0l\0\xF6";
    ATT_ASSERT(mjb_count_codepoints(utf16be_hello_accents, 10, enc), 5, "UTF-16BE length: Héllö")
    ATT_ASSERT(mjb_count_codepoints(utf16be_hello_accents, 8, enc), 4, "UTF-16BE length: Héllö")
    ATT_ASSERT(mjb_count_codepoints(utf16be_hello_accents, 4, enc), 2, "UTF-16BE length: Héllö")
    ATT_ASSERT(mjb_count_codepoints(utf16be_hello_accents, 0, enc), 0, "UTF-16BE length: Héllö")

    const char utf16be_hello_katakana[] = "\0H\0\xE8\0l\0l\0\xF5\0 \x30\x30";
    ATT_ASSERT(mjb_count_codepoints(utf16be_hello_katakana, 14, enc), 7, "UTF-16BE length: Hèllõ ツ")
    ATT_ASSERT(mjb_count_codepoints(utf16be_hello_katakana, 10, enc), 5, "UTF-16BE length: Hèllõ ツ")

    const char utf16be_konnichiwa[] = "\x30\x93\x30\x93\x30\x6B\x30\x61\x30\x6F";
    ATT_ASSERT(mjb_count_codepoints(utf16be_konnichiwa, 10, enc), 5, "UTF-16BE length: こんにちは")

    const char utf16be_geia_sou[] = "\x03\x93\x03\x65\x03\x69\x03\x3F\0 \x03\xC3\x03\x6F\x03\x75";
    ATT_ASSERT(mjb_count_codepoints(utf16be_geia_sou, 16, enc), 8, "UTF-16BE length: Γειά σου")
    ATT_ASSERT(mjb_count_codepoints(utf16be_hello_accents, 2, enc), 1,
        "UTF-16BE length: Héllö (1 max value)")

    enc = MJB_ENC_UTF_32LE;
    const char utf32le_hello[] = "H\0\0\0e\0\0\0l\0\0\0l\0\0\0o\0\0\0";
    ATT_ASSERT(mjb_count_codepoints(utf32le_hello, 20, enc), 5, "UTF-32LE length: Hello")
    ATT_ASSERT(mjb_count_codepoints(utf32le_hello, 16, enc), 4, "UTF-32LE length: Hello")
    ATT_ASSERT(mjb_count_codepoints(utf32le_hello, 12, enc), 3, "UTF-32LE length: Hello")
    ATT_ASSERT(mjb_count_codepoints(utf32le_hello, 8, enc), 2, "UTF-32LE length: Hello")
    ATT_ASSERT(mjb_count_codepoints(utf32le_hello, 4, enc), 1, "UTF-32LE length: Hello")
    ATT_ASSERT(mjb_count_codepoints(utf32le_hello, 0, enc), 0, "UTF-32LE length: Hello")
    ATT_ASSERT(mjb_count_codepoints(NULL, 0, enc), 0, "UTF-32LE length: NULL")

    const char utf32le_hello_accents[] = "H\0\0\0\xE9\0\0\0l\0\0\0l\0\0\0\xF6\0\0\0";
    ATT_ASSERT(mjb_count_codepoints(utf32le_hello_accents, 20, enc), 5, "UTF-32LE length: Héllö")
    ATT_ASSERT(mjb_count_codepoints(utf32le_hello_accents, 16, enc), 4, "UTF-32LE length: Héllö")
    ATT_ASSERT(mjb_count_codepoints(utf32le_hello_accents, 8, enc), 2, "UTF-32LE length: Héllö")
    ATT_ASSERT(mjb_count_codepoints(utf32le_hello_accents, 0, enc), 0, "UTF-32LE length: Héllö")

    const char
        utf32le_hello_katakana[] = "H\0\0\0\xE8\0\0\0l\0\0\0l\0\0\0\xF5\0\0\0 \0\0\0\x30\x30\0\0";
    ATT_ASSERT(mjb_count_codepoints(utf32le_hello_katakana, 28, enc), 7, "UTF-32LE length: Hèllõ ツ")
    ATT_ASSERT(mjb_count_codepoints(utf32le_hello_katakana, 20, enc), 5, "UTF-32LE length: Hèllõ ツ")

    const char
        utf32le_konnichiwa[] = "\x93\x30\0\0\x93\x30\0\0\x6B\x30\0\0\x61\x30\0\0\x6F\x30\0\0";
    ATT_ASSERT(mjb_count_codepoints(utf32le_konnichiwa, 20, enc), 5, "UTF-32LE length: こんにちは")

    const char utf32le_geia_sou[] = "\x93\x03\0\0\x65\x03\0\0\x69\x03\0\0\x3F\x03\0\0 \0\0\0"
                                    "\xC3\x03\0\0\x6F\x03\0\0\x75\x03\0\0";
    ATT_ASSERT(mjb_count_codepoints(utf32le_geia_sou, 32, enc), 8, "UTF-32LE length: Γειά σου")
    ATT_ASSERT(mjb_count_codepoints(utf32le_hello_accents, 4, enc), 1,
        "UTF-32LE length: Héllö (1 max value)")

    enc = MJB_ENC_UTF_32BE;
    const char utf32be_hello[] = "\0\0\0H\0\0\0e\0\0\0l\0\0\0l\0\0\0o";
    ATT_ASSERT(mjb_count_codepoints(utf32be_hello, 20, enc), 5, "UTF-32BE length: Hello")
    ATT_ASSERT(mjb_count_codepoints(utf32be_hello, 16, enc), 4, "UTF-32BE length: Hello")
    ATT_ASSERT(mjb_count_codepoints(utf32be_hello, 12, enc), 3, "UTF-32BE length: Hello")
    ATT_ASSERT(mjb_count_codepoints(utf32be_hello, 8, enc), 2, "UTF-32BE length: Hello")
    ATT_ASSERT(mjb_count_codepoints(utf32be_hello, 4, enc), 1, "UTF-32BE length: Hello")
    ATT_ASSERT(mjb_count_codepoints(utf32be_hello, 0, enc), 0, "UTF-32BE length: Hello")
    ATT_ASSERT(mjb_count_codepoints(NULL, 0, enc), 0, "UTF-32BE length: NULL")

    const char utf32be_hello_accents[] = "\0\0\0H\0\0\0\xE9\0\0\0l\0\0\0l\0\0\0\xF6";
    ATT_ASSERT(mjb_count_codepoints(utf32be_hello_accents, 20, enc), 5, "UTF-32BE length: Héllö")
    ATT_ASSERT(mjb_count_codepoints(utf32be_hello_accents, 16, enc), 4, "UTF-32BE length: Héllö")
    ATT_ASSERT(mjb_count_codepoints(utf32be_hello_accents, 8, enc), 2, "UTF-32BE length: Héllö")
    ATT_ASSERT(mjb_count_codepoints(utf32be_hello_accents, 0, enc), 0, "UTF-32BE length: Héllö")

    const char
        utf32be_hello_katakana[] = "\0\0\0H\0\0\0\xE8\0\0\0l\0\0\0l\0\0\0\xF5\0\0\0 \0\0\x30\x30";
    ATT_ASSERT(mjb_count_codepoints(utf32be_hello_katakana, 28, enc), 7, "UTF-32BE length: Hèllõ ツ")
    ATT_ASSERT(mjb_count_codepoints(utf32be_hello_katakana, 20, enc), 5, "UTF-32BE length: Hèllõ ツ")

    const char
        utf32be_konnichiwa[] = "\0\0\x30\x93\0\0\x30\x93\0\0\x30\x6B\0\0\x30\x61\0\0\x30\x6F";
    ATT_ASSERT(mjb_count_codepoints(utf32be_konnichiwa, 20, enc), 5, "UTF-32BE length: こんにちは")

    const char utf32be_geia_sou[] = "\0\0\x03\x93\0\0\x03\x65\0\0\x03\x69\0\0\x03\x3F\0\0\0 "
                                    "\0\0\x03\xC3\0\0\x03\x6F\0\0\x03\x75";
    ATT_ASSERT(mjb_count_codepoints(utf32be_geia_sou, 32, enc), 8, "UTF-32BE length: Γειά σου")
    ATT_ASSERT(mjb_count_codepoints(utf32be_hello_accents, 4, enc), 1,
        "UTF-32BE length: Héllö (1 max value)")

    return 0;
}
