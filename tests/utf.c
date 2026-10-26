/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "../src/utf.h"
#include "test.h"

// Return the codepoint count, or SIZE_MAX when mjb_codepoint_count reports a failure.
static size_t count_codepoints(const char *buffer, size_t byte_length, mjb_encoding encoding) {
    size_t count = 0;
    mjb_text text = mjb_text_make(buffer, byte_length, encoding);

    if(mjb_codepoint_count(&text, MJB_MALFORMED_STOP, &count, NULL) != MJB_STATUS_OK) {
        return SIZE_MAX;
    }

    return count;
}

// Decode the codepoint at |*index| with the internal whole-scalar decoder.
static mjb_status decode(const char *buffer, size_t byte_length, mjb_encoding encoding,
    size_t *index, mjb_codepoint *codepoint) {
    return mjb_decode_next_raw(buffer, byte_length, encoding, index, codepoint, NULL);
}

ATT_TEST(utf) {
    // Test scanning single ASCII character "A"
    const char *buffer = "A";
    size_t size = 1;
    size_t index = 0;
    mjb_codepoint codepoint = 0;
    mjb_encoding encoding = MJB_ENC_UTF_8;

    ATT_ASSERT_STATUS(decode(buffer, size, MJB_ENC_UTF_8, &index, &codepoint), MJB_STATUS_OK,
        "UTF-8: decoded");
    ATT_ASSERT(codepoint, 0x41, "0x41 (A)");
    ATT_ASSERT(index, 1, "Index 1");
    ATT_ASSERT_STATUS(decode(buffer, size, MJB_ENC_UTF_8, &index, &codepoint),
        MJB_STATUS_END_OF_INPUT, "UTF-8: end of input");
    ATT_ASSERT(index, 1, "Index 1");

    // Each maximal subpart of a malformed UTF-8 subsequence is one error.
    const char buffer_utf8_malformed[] = { '\xE2', '\x82', 'A', '\x80', '\x80' };
    index = 0;

    ATT_ASSERT_STATUS(decode(buffer_utf8_malformed, sizeof(buffer_utf8_malformed), MJB_ENC_UTF_8,
                          &index, &codepoint),
        MJB_STATUS_MALFORMED_INPUT, "UTF-8 malformed: truncated sequence");
    ATT_ASSERT(codepoint, MJB_CODEPOINT_REPLACEMENT, "UTF-8 malformed: replacement");
    ATT_ASSERT(index, 2, "UTF-8 malformed: maximal subpart E2 82");
    ATT_ASSERT_STATUS(decode(buffer_utf8_malformed, sizeof(buffer_utf8_malformed), MJB_ENC_UTF_8,
                          &index, &codepoint),
        MJB_STATUS_OK, "UTF-8 malformed: A");
    ATT_ASSERT(codepoint, 0x41, "UTF-8 malformed: U+0041");
    ATT_ASSERT_STATUS(decode(buffer_utf8_malformed, sizeof(buffer_utf8_malformed), MJB_ENC_UTF_8,
                          &index, &codepoint),
        MJB_STATUS_MALFORMED_INPUT, "UTF-8 malformed: first continuation byte");
    ATT_ASSERT(index, 4, "UTF-8 malformed: index 4");
    ATT_ASSERT_STATUS(decode(buffer_utf8_malformed, sizeof(buffer_utf8_malformed), MJB_ENC_UTF_8,
                          &index, &codepoint),
        MJB_STATUS_MALFORMED_INPUT, "UTF-8 malformed: second continuation byte");
    ATT_ASSERT(index, 5, "UTF-8 malformed: index 5");

    const char *buffer_utf16be = "\x00\x41";
    index = 0;

    ATT_ASSERT_STATUS(decode(buffer_utf16be, 2, MJB_ENC_UTF_16BE, &index, &codepoint),
        MJB_STATUS_OK, "UTF-16BE: decoded");
    ATT_ASSERT(codepoint, 0x41, "UTF-16BE: 0x41 (A)");
    ATT_ASSERT(index, 2, "UTF-16BE: Index 2");
    ATT_ASSERT_STATUS(decode(buffer_utf16be, 2, MJB_ENC_UTF_16BE, &index, &codepoint),
        MJB_STATUS_END_OF_INPUT, "UTF-16BE: end of input");

    const char *buffer_utf16le = "\x41\x00";
    index = 0;

    ATT_ASSERT_STATUS(decode(buffer_utf16le, 2, MJB_ENC_UTF_16LE, &index, &codepoint),
        MJB_STATUS_OK, "UTF-16LE: decoded");
    ATT_ASSERT(codepoint, 0x41, "UTF-16LE: 0x41 (A)");
    ATT_ASSERT(index, 2, "UTF-16LE: Index 2");
    ATT_ASSERT_STATUS(decode(buffer_utf16le, 2, MJB_ENC_UTF_16LE, &index, &codepoint),
        MJB_STATUS_END_OF_INPUT, "UTF-16LE: end of input");

    // mjb_decode_begin resolves generic input and consumes only the initial signature.
    const char buffer_utf16be_inner_bom[] = {
        '\xFE',
        '\xFF',
        '\x00',
        'A',
        '\xFE',
        '\xFF',
        '\x00',
        'B',
    };

    index = 0;
    encoding = MJB_ENC_UTF_16;

    ATT_ASSERT(mjb_decode_begin(buffer_utf16be_inner_bom, sizeof(buffer_utf16be_inner_bom),
                   &encoding, &index),
        true, "UTF-16 generic BOM: byte order known");
    ATT_ASSERT((int)encoding, (int)MJB_ENC_UTF_16BE, "UTF-16 generic BOM: big endian");
    ATT_ASSERT(index, (size_t)2, "UTF-16 generic BOM: consumed signature");
    ATT_ASSERT_STATUS(decode(buffer_utf16be_inner_bom, sizeof(buffer_utf16be_inner_bom), encoding,
                          &index, &codepoint),
        MJB_STATUS_OK, "UTF-16 generic inner BOM: first codepoint");
    ATT_ASSERT(codepoint, 0x41, "UTF-16 generic inner BOM: U+0041");
    ATT_ASSERT_STATUS(decode(buffer_utf16be_inner_bom, sizeof(buffer_utf16be_inner_bom), encoding,
                          &index, &codepoint),
        MJB_STATUS_OK, "UTF-16 generic inner BOM: second codepoint");
    ATT_ASSERT(codepoint, 0xFEFF, "UTF-16 generic inner BOM: preserves inner U+FEFF");
    ATT_ASSERT(index, (size_t)6, "UTF-16 generic inner BOM: index 6");

    // A later call with a non-zero index resolves the byte order without moving the index.
    encoding = MJB_ENC_UTF_16;

    ATT_ASSERT(mjb_decode_begin(buffer_utf16be_inner_bom, sizeof(buffer_utf16be_inner_bom),
                   &encoding, &index),
        true, "UTF-16 generic inner BOM: resolved again");
    ATT_ASSERT(index, (size_t)6, "UTF-16 generic inner BOM: index kept");
    ATT_ASSERT_STATUS(decode(buffer_utf16be_inner_bom, sizeof(buffer_utf16be_inner_bom), encoding,
                          &index, &codepoint),
        MJB_STATUS_OK, "UTF-16 generic inner BOM: third codepoint");
    ATT_ASSERT(codepoint, 0x42, "UTF-16 generic inner BOM: U+0042");
    ATT_ASSERT(index, sizeof(buffer_utf16be_inner_bom), "UTF-16 generic inner BOM: end index");

    // An explicit byte order keeps the BOM as U+FEFF.
    index = 0;
    encoding = MJB_ENC_UTF_16BE;

    ATT_ASSERT(mjb_decode_begin(buffer_utf16be_inner_bom, sizeof(buffer_utf16be_inner_bom),
                   &encoding, &index),
        true, "UTF-16BE BOM: byte order known");
    ATT_ASSERT(index, (size_t)0, "UTF-16BE BOM: signature kept");
    ATT_ASSERT_STATUS(decode(buffer_utf16be_inner_bom, sizeof(buffer_utf16be_inner_bom), encoding,
                          &index, &codepoint),
        MJB_STATUS_OK, "UTF-16BE BOM: decoded");
    ATT_ASSERT(codepoint, 0xFEFF, "UTF-16BE BOM: preserves U+FEFF");

    const char buffer_utf16_no_bom[] = { '\x00', 'A' };
    index = 0;
    encoding = MJB_ENC_UTF_16;

    ATT_ASSERT(mjb_decode_begin(buffer_utf16_no_bom, sizeof(buffer_utf16_no_bom), &encoding,
                   &index),
        false, "UTF-16 generic no BOM: byte order unknown");
    encoding = MJB_ENC_UTF_16;
    ATT_ASSERT(mjb_decode_begin(buffer_utf16_no_bom, 0, &encoding, &index), true,
        "UTF-16 generic empty: nothing to decode");

    const char buffer_utf32le_bom[] = {
        '\xFF',
        '\xFE',
        '\x00',
        '\x00',
        'A',
        '\x00',
        '\x00',
        '\x00',
    };

    index = 0;
    encoding = MJB_ENC_UTF_32;

    ATT_ASSERT(mjb_decode_begin(buffer_utf32le_bom, sizeof(buffer_utf32le_bom), &encoding, &index),
        true, "UTF-32 generic BOM: byte order known");
    ATT_ASSERT((int)encoding, (int)MJB_ENC_UTF_32LE, "UTF-32 generic BOM: little endian");
    ATT_ASSERT_STATUS(decode(buffer_utf32le_bom, sizeof(buffer_utf32le_bom), encoding, &index,
                          &codepoint),
        MJB_STATUS_OK, "UTF-32 generic BOM: decoded");
    ATT_ASSERT(codepoint, 0x41, "UTF-32 generic BOM: U+0041");
    ATT_ASSERT(index, sizeof(buffer_utf32le_bom), "UTF-32 generic BOM: consumed signature");

    // A surrogate pair decodes as one scalar.
    index = 0;

    ATT_ASSERT_STATUS(decode("\xD8\x3D\xDE\x42", 4, MJB_ENC_UTF_16BE, &index, &codepoint),
        MJB_STATUS_OK, "UTF-16BE surrogate pair: decoded");
    ATT_ASSERT(codepoint, 0x1F642, "UTF-16BE surrogate pair: U+1F642");
    ATT_ASSERT(index, 4, "UTF-16BE surrogate pair: index 4");

    index = 0;

    ATT_ASSERT_STATUS(decode("\x3D\xD8\x42\xDE", 4, MJB_ENC_UTF_16LE, &index, &codepoint),
        MJB_STATUS_OK, "UTF-16LE surrogate pair: decoded");
    ATT_ASSERT(codepoint, 0x1F642, "UTF-16LE surrogate pair: U+1F642");
    ATT_ASSERT(index, 4, "UTF-16LE surrogate pair: index 4");

    // A unit that breaks an unpaired high surrogate is decoded on its own.
    const char buffer_utf16le_unpaired[] = { '\xDB', '\xDB', '\xFE', '\xF4' };
    index = 0;

    ATT_ASSERT_STATUS(decode(buffer_utf16le_unpaired, sizeof(buffer_utf16le_unpaired),
                          MJB_ENC_UTF_16LE, &index, &codepoint),
        MJB_STATUS_MALFORMED_INPUT, "UTF-16LE unpaired: malformed");
    ATT_ASSERT(codepoint, MJB_CODEPOINT_REPLACEMENT, "UTF-16LE unpaired: replacement");
    ATT_ASSERT(index, 2, "UTF-16LE unpaired: only the high surrogate");
    ATT_ASSERT_STATUS(decode(buffer_utf16le_unpaired, sizeof(buffer_utf16le_unpaired),
                          MJB_ENC_UTF_16LE, &index, &codepoint),
        MJB_STATUS_OK, "UTF-16LE unpaired: next unit decoded");
    ATT_ASSERT(codepoint, 0xF4FE, "UTF-16LE unpaired: next unit U+F4FE");
    ATT_ASSERT(index, 4, "UTF-16LE unpaired: index 4");

    // An unpaired high surrogate followed by a valid surrogate pair keeps the pair.
    const char buffer_utf16be_unpaired_pair[] = { '\xD8', '\x00', '\xD8', '\x3D', '\xDE', '\x42' };
    index = 0;

    ATT_ASSERT_STATUS(decode(buffer_utf16be_unpaired_pair, sizeof(buffer_utf16be_unpaired_pair),
                          MJB_ENC_UTF_16BE, &index, &codepoint),
        MJB_STATUS_MALFORMED_INPUT, "UTF-16BE unpaired pair: malformed");
    ATT_ASSERT(index, 2, "UTF-16BE unpaired pair: stops before the second high surrogate");
    ATT_ASSERT_STATUS(decode(buffer_utf16be_unpaired_pair, sizeof(buffer_utf16be_unpaired_pair),
                          MJB_ENC_UTF_16BE, &index, &codepoint),
        MJB_STATUS_OK, "UTF-16BE unpaired pair: pair decoded");
    ATT_ASSERT(codepoint, 0x1F642, "UTF-16BE unpaired pair: U+1F642");
    ATT_ASSERT(index, 6, "UTF-16BE unpaired pair: index 6");

    // A high surrogate followed by a truncated byte, then the truncated byte itself.
    const char buffer_utf16le_unpaired_truncated[] = { '\x00', '\xD8', 'A' };
    index = 0;

    ATT_ASSERT_STATUS(decode(buffer_utf16le_unpaired_truncated,
                          sizeof(buffer_utf16le_unpaired_truncated), MJB_ENC_UTF_16LE, &index,
                          &codepoint),
        MJB_STATUS_MALFORMED_INPUT, "UTF-16LE unpaired truncated: unpaired surrogate");
    ATT_ASSERT(index, 2, "UTF-16LE unpaired truncated: index 2");
    ATT_ASSERT_STATUS(decode(buffer_utf16le_unpaired_truncated,
                          sizeof(buffer_utf16le_unpaired_truncated), MJB_ENC_UTF_16LE, &index,
                          &codepoint),
        MJB_STATUS_MALFORMED_INPUT, "UTF-16LE unpaired truncated: truncated unit");
    ATT_ASSERT(index, sizeof(buffer_utf16le_unpaired_truncated),
        "UTF-16LE unpaired truncated: consumed input");
    ATT_ASSERT_STATUS(decode(buffer_utf16le_unpaired_truncated,
                          sizeof(buffer_utf16le_unpaired_truncated), MJB_ENC_UTF_16LE, &index,
                          &codepoint),
        MJB_STATUS_END_OF_INPUT, "UTF-16LE unpaired truncated: end of input");

    // Out-of-range and surrogate UTF-32 units are rejected, not silently dropped.
    const char buffer_utf32le_out_of_range[] = { '\x00', '\x00', '\x11', '\x00', 'A', '\0', '\0',
        '\0' };
    index = 0;

    ATT_ASSERT_STATUS(decode(buffer_utf32le_out_of_range, sizeof(buffer_utf32le_out_of_range),
                          MJB_ENC_UTF_32LE, &index, &codepoint),
        MJB_STATUS_MALFORMED_INPUT, "UTF-32LE out of range: malformed");
    ATT_ASSERT(codepoint, MJB_CODEPOINT_REPLACEMENT, "UTF-32LE out of range: replacement");
    ATT_ASSERT(index, 4, "UTF-32LE out of range: index 4");
    ATT_ASSERT_STATUS(decode(buffer_utf32le_out_of_range, sizeof(buffer_utf32le_out_of_range),
                          MJB_ENC_UTF_32LE, &index, &codepoint),
        MJB_STATUS_OK, "UTF-32LE out of range: next unit decoded");
    ATT_ASSERT(codepoint, 0x41, "UTF-32LE out of range: next unit U+0041");

    const char buffer_utf32be_surrogate[] = { '\0', '\0', '\xD8', '\x00' };
    index = 0;

    ATT_ASSERT_STATUS(decode(buffer_utf32be_surrogate, sizeof(buffer_utf32be_surrogate),
                          MJB_ENC_UTF_32BE, &index, &codepoint),
        MJB_STATUS_MALFORMED_INPUT, "UTF-32BE surrogate: malformed");
    ATT_ASSERT(codepoint, MJB_CODEPOINT_REPLACEMENT, "UTF-32BE surrogate: replacement");
    ATT_ASSERT(index, 4, "UTF-32BE surrogate: index 4");

    // Strict codepoint counting rejects truncated trailing code units.
    MJB_TEST_COVERAGE(mjb_codepoint_count);

    ATT_ASSERT(count_codepoints("A\0B", 3, MJB_ENC_UTF_16BE), SIZE_MAX,
        "UTF-16BE: truncated trailing unit is malformed");
    ATT_ASSERT(count_codepoints("A", 1, MJB_ENC_UTF_16LE), SIZE_MAX,
        "UTF-16LE: lone trailing byte is malformed");
    ATT_ASSERT(count_codepoints("\0\0\0A!", 5, MJB_ENC_UTF_32BE), SIZE_MAX,
        "UTF-32BE: truncated trailing unit is malformed");
    ATT_ASSERT(count_codepoints("A!\0", 3, MJB_ENC_UTF_32LE), SIZE_MAX,
        "UTF-32LE: lone truncated unit is malformed");
}
