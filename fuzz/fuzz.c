/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "../src/mojibake.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

// Keeps calls to pure functions from being discarded.
static volatile size_t fuzz_sink;

// On fast paths the result APIs alias the input buffer instead of allocating, so only free an
// output that is a distinct heap allocation.
static void free_result(mjb_result *result, const char *input) {
    if(result->output != NULL && result->output != input) {
        mjb_free(result->output);
    }
}

/**
 * The libFuzzer harness. The first byte selects the API under test and some of its parameters,
 * the second byte selects the input encoding and locale, the rest is the input buffer.
 *
 * Build with: make fuzz
 */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if(size < 2) {
        return 0;
    }

    uint8_t selector = data[0];
    uint8_t variant = data[1];
    const char *buffer = (const char*)data + 2;
    size -= 2;

    static const mjb_encoding encodings[] = {
        MJB_ENCODING_UTF_8, MJB_ENCODING_UTF_16_LE, MJB_ENCODING_UTF_16_BE,
        MJB_ENCODING_UTF_32_LE, MJB_ENCODING_UTF_32_BE, MJB_ENCODING_ASCII
    };
    mjb_encoding encoding = encodings[variant % 6];

    // Exercise the language-sensitive casing and folding paths too.
    static const unsigned int locales[] = {
        MJB_LOCALE_EN, MJB_LOCALE_TR, MJB_LOCALE_AZ, MJB_LOCALE_LT
    };

    if(!mjb_locale_set(locales[(variant >> 3) % 4])) {
        return 0;
    }

    mjb_result result = {0};

    switch(selector % 14) {
        case 0: // Normalization, all four forms
            if(mjb_normalize(buffer, size, encoding, (mjb_normalization)(selector % 4),
                MJB_ENCODING_UTF_8, &result)) {
                free_result(&result, buffer);
            }

            break;

        case 1: // Normalization quick check
            mjb_string_is_normalized(buffer, size, encoding, (mjb_normalization)(selector % 4));
            break;

        case 2: { // Case conversion and folding, all six types
            char *output = mjb_case(buffer, size, (mjb_case_type)(1 + selector % 5), encoding);

            if(output != NULL && output != buffer) {
                mjb_free(output);
            }

            break;
        }

        case 3: { // Bidirectional algorithm, all three directions
            mjb_bidi_paragraph para;
            mjb_direction direction = (mjb_direction)(selector % 3);

            if(mjb_bidi_resolve(buffer, size, encoding, direction, &para)) {
                if(para.count > 0 && para.count <= 4096) {
                    size_t visual_order[4096];

                    if(mjb_bidi_reorder_line(&para, 0, para.count, visual_order)) {
                        size_t run_count = 0;
                        mjb_bidi_line_runs(&para, visual_order, para.count, NULL, &run_count);
                    }
                }

                mjb_bidi_free(&para);
            }

            break;
        }

        case 4: // Encoding detection
            fuzz_sink += (size_t)mjb_string_encoding(buffer, size);
            fuzz_sink += (size_t)mjb_string_is_utf8(buffer, size);
            break;

        case 5: // Encoding conversion
            if(mjb_string_convert_encoding(buffer, size, encoding,
                encodings[(variant >> 1) % 6], &result)) {
                free_result(&result, buffer);
            }

            break;

        case 6: // String filtering, all filter combinations
            if(mjb_string_filter(buffer, size, encoding, MJB_ENCODING_UTF_8,
                (mjb_filter)(selector % 0x20), &result)) {
                free_result(&result, buffer);
            }

            break;

        case 7: // Collation key
            if(mjb_collation_key(buffer, size, encoding,
                (selector & 0x10) ? MJB_COLLATION_SHIFTED : MJB_COLLATION_NON_IGNORABLE,
                &result)) {
                free_result(&result, buffer);
            }

            break;

        case 8: // Collation comparison, input split in two halves
            mjb_string_compare(buffer, size / 2, buffer + size / 2, size - size / 2, encoding,
                (selector & 0x10) ? MJB_COLLATION_SHIFTED : MJB_COLLATION_NON_IGNORABLE);
            break;

        case 9: // Segmentation: grapheme, word and width truncation
            fuzz_sink += mjb_strnlen(buffer, size, encoding);
            mjb_truncate(buffer, size, encoding, variant);
            mjb_truncate_word(buffer, size, encoding, variant);
            mjb_truncate_width(buffer, size, encoding,
                (mjb_width_context)(selector % 3), variant);
            break;

        case 10: { // Display width
            size_t width = 0;
            mjb_display_width(buffer, size, encoding, (mjb_width_context)(selector % 3),
                &width);
            break;
        }

        case 11: // Identifier validation, both profiles
            mjb_string_is_identifier(buffer, size, encoding,
                (selector & 0x10) ? MJB_IDENTIFIER_NFKC : MJB_IDENTIFIER_DEFAULT);
            break;

        case 12: // Confusable detection, input split in two halves
            mjb_string_is_confusable(buffer, size / 2, buffer + size / 2, size - size / 2,
                encoding);
            break;

        case 13: { // BCP 47 locale parsing
            mjb_locale_id locale;
            mjb_error error;
            mjb_locale_parse(buffer, size, encoding, &locale, &error);
            break;
        }
    }

    return 0;
}
