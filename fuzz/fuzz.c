/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../src/mojibake.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

// Keeps calls to pure functions from being discarded.
static volatile size_t fuzz_sink;

static uint32_t fuzz_u32(const uint8_t *data, size_t size, uint8_t variant) {
    uint32_t value = variant;
    size_t limit = size < 4 ? size : 4;

    for(size_t i = 0; i < limit; ++i) {
        value |= (uint32_t)data[i] << (i * 8);
    }

    return value;
}

static mjb_codepoint fuzz_codepoint(const uint8_t *data, size_t size, uint8_t variant) {
    uint32_t value = fuzz_u32(data, size, variant);

    switch(variant & 0x7) {
        case 0:
            return value & 0x7F;

        case 1:
            return value % (MJB_CODEPOINT_MAX + 1u);

        case 2:
            return 0x0300u + (value % 0x400u);

        case 3:
            return MJB_CP_HANGUL_S_BASE + (value % MJB_CP_HANGUL_S_COUNT);

        case 4:
            if(variant & 0x20) {
                return MJB_CP_HANGUL_T_BASE + (value % MJB_CP_HANGUL_T_COUNT);
            }

            if(variant & 0x10) {
                return MJB_CP_HANGUL_V_BASE + (value % MJB_CP_HANGUL_V_COUNT);
            }

            return MJB_CP_HANGUL_L_BASE + (value % MJB_CP_HANGUL_L_COUNT);

        case 5:
            return MJB_CJK_IDEOGRAPH_START +
                (value % (MJB_CJK_IDEOGRAPH_END - MJB_CJK_IDEOGRAPH_START + 1u));

        case 6:
            return 0x1F300u + (value % 0x500u);

        default:
            return MJB_CODEPOINT_NOT_VALID + (value & 0xFFu);
    }
}

static bool fuzz_next_character(mjb_character *character, mjb_next_character_type type) {
    if(character != NULL) {
        fuzz_sink += (size_t)character->codepoint;
        fuzz_sink += (size_t)type;
    }

    return true;
}

static void fuzz_boundary_iterators(const char *buffer, size_t size, mjb_encoding encoding) {
    size_t guard_limit = (size + 8) * 4;

    mjb_next_state grapheme_state = {0};
    for(size_t guard = 0; guard < guard_limit; ++guard) {
        mjb_break_type bt = mjb_segmentation(buffer, size, encoding, &grapheme_state);
        fuzz_sink += (size_t)bt + grapheme_state.index;

        if(bt == MJB_BT_NOT_SET) {
            break;
        }

        if(guard + 1 == guard_limit) {
            abort();
        }
    }

    mjb_next_word_state word_state = {0};
    for(size_t guard = 0; guard < guard_limit; ++guard) {
        mjb_break_type bt = mjb_break_word(buffer, size, encoding, &word_state);
        fuzz_sink += (size_t)bt + word_state.index;

        if(bt == MJB_BT_NOT_SET) {
            break;
        }

        if(guard + 1 == guard_limit) {
            abort();
        }
    }

    mjb_next_line_state line_state = {0};
    for(size_t guard = 0; guard < guard_limit; ++guard) {
        mjb_break_type bt = mjb_break_line(buffer, size, encoding, &line_state);
        fuzz_sink += (size_t)bt + line_state.index;

        if(bt == MJB_BT_NOT_SET) {
            break;
        }

        if(guard + 1 == guard_limit) {
            abort();
        }
    }

    mjb_next_sentence_state sentence_state = {0};
    for(size_t guard = 0; guard < guard_limit; ++guard) {
        mjb_break_type bt = mjb_break_sentence(buffer, size, encoding, &sentence_state);
        fuzz_sink += (size_t)bt + sentence_state.index;

        if(bt == MJB_BT_NOT_SET) {
            break;
        }

        if(guard + 1 == guard_limit) {
            abort();
        }
    }
}

static void fuzz_codepoint_apis(mjb_codepoint codepoint, uint8_t variant) {
    mjb_character character;
    mjb_numeric_value numeric;
    mjb_block_info block;
    mjb_emoji_properties emoji;
    mjb_east_asian_width width;
    mjb_codepoint decomposition[3];
    char encoded[5];
    char hangul_name[128];
    uint8_t property_value = 0;
    uint8_t properties[MJB_PR_BUFFER_SIZE];
    mjb_property property = (mjb_property)(variant % MJB_PR_COUNT);
    mjb_property property_name = (mjb_property)(variant % (MJB_PR_COUNT + 2));

    static const mjb_encoding encodings[] = {
        MJB_ENCODING_UTF_8, MJB_ENCODING_UTF_16_LE, MJB_ENCODING_UTF_16_BE,
        MJB_ENCODING_UTF_32_LE, MJB_ENCODING_UTF_32_BE, MJB_ENC_ASCII
    };

    fuzz_sink += (size_t)mjb_codepoint_character(codepoint, &character);
    fuzz_sink += (size_t)mjb_codepoint_is_valid(codepoint);
    fuzz_sink += (size_t)mjb_codepoint_is_graphic(codepoint);
    fuzz_sink += (size_t)mjb_codepoint_is_combining(codepoint);
    fuzz_sink += (size_t)mjb_codepoint_is_hangul_l(codepoint);
    fuzz_sink += (size_t)mjb_codepoint_is_hangul_v(codepoint);
    fuzz_sink += (size_t)mjb_codepoint_is_hangul_t(codepoint);
    fuzz_sink += (size_t)mjb_codepoint_is_hangul_jamo(codepoint);
    fuzz_sink += (size_t)mjb_codepoint_is_hangul_syllable(codepoint);
    fuzz_sink += (size_t)mjb_codepoint_is_cjk_ideograph(codepoint);
    fuzz_sink += (size_t)mjb_codepoint_is_cjk_ext(codepoint);
    fuzz_sink += (size_t)mjb_category_is_graphic((mjb_category)(variant % MJB_CATEGORY_COUNT));
    fuzz_sink += (size_t)mjb_category_is_combining((mjb_category)(variant % MJB_CATEGORY_COUNT));
    fuzz_sink += (size_t)mjb_codepoint_numeric_value(codepoint, &numeric);
    fuzz_sink += (size_t)mjb_codepoint_block(codepoint, &block);
    fuzz_sink += (size_t)mjb_codepoint_to_lowercase(codepoint);
    fuzz_sink += (size_t)mjb_codepoint_to_uppercase(codepoint);
    fuzz_sink += (size_t)mjb_codepoint_to_titlecase(codepoint);
    fuzz_sink += (size_t)mjb_codepoint_plane(codepoint);
    fuzz_sink += (size_t)mjb_plane_is_valid((mjb_plane)((int)(variant % (MJB_PLANE_NUM + 2)) - 1));

    const char *plane_name = mjb_plane_name((mjb_plane)((int)(variant % (MJB_PLANE_NUM + 2)) - 1),
        (variant & 0x80) != 0);
    if(plane_name != NULL) {
        fuzz_sink += (unsigned char)plane_name[0];
    }

    fuzz_sink += (size_t)mjb_codepoint_is_id_start(codepoint);
    fuzz_sink += (size_t)mjb_codepoint_is_id_continue(codepoint);
    fuzz_sink += (size_t)mjb_codepoint_is_xid_start(codepoint);
    fuzz_sink += (size_t)mjb_codepoint_is_xid_continue(codepoint);
    fuzz_sink += (size_t)mjb_codepoint_is_pattern_syntax(codepoint);
    fuzz_sink += (size_t)mjb_codepoint_is_pattern_white_space(codepoint);
    mjb_status property_status = mjb_codepoint_has_property(codepoint, property, &property_value);
    fuzz_sink += (size_t)property_status;

    if(property_status == MJB_STATUS_OK) {
        fuzz_sink += property_value;
    }

    if(mjb_codepoint_properties(codepoint, properties) == MJB_STATUS_OK) {
        fuzz_sink += mjb_codepoint_property(properties, property);
    }

    fuzz_sink += (size_t)mjb_codepoint_script(codepoint);

    const char *name = mjb_property_name(property_name);
    if(name != NULL) {
        fuzz_sink += (unsigned char)name[0];
    }

    if(mjb_codepoint_emoji(codepoint, &emoji) == MJB_STATUS_OK) {
        fuzz_sink += (size_t)emoji.emoji + (size_t)emoji.presentation +
            (size_t)emoji.modifier + (size_t)emoji.modifier_base +
            (size_t)emoji.component + (size_t)emoji.extended_pictographic;
    }

    if(mjb_codepoint_east_asian_width(codepoint, &width) == MJB_STATUS_OK) {
        fuzz_sink += (size_t)width;
    }

    fuzz_sink += (size_t)mjb_hangul_syllable_name(codepoint, hangul_name, sizeof(hangul_name));
    fuzz_sink += (size_t)mjb_hangul_syllable_decomposition(codepoint, decomposition);

    mjb_buffer_character composition[] = {
        {codepoint, 0},
        {MJB_CP_HANGUL_L_BASE + (variant % MJB_CP_HANGUL_L_COUNT), 0},
        {MJB_CP_HANGUL_V_BASE + (variant % MJB_CP_HANGUL_V_COUNT), 0},
        {MJB_CP_HANGUL_T_BASE + (variant % MJB_CP_HANGUL_T_COUNT), 0}
    };
    fuzz_sink += mjb_hangul_syllable_composition(composition,
        sizeof(composition) / sizeof(composition[0]));
    fuzz_sink += mjb_codepoint_encode(codepoint, encoded, sizeof(encoded),
        encodings[(variant >> 2) % 6]);
}

static void fuzz_emoji_string_api_input(const char *buffer, size_t size, mjb_encoding encoding) {
    mjb_emoji_sequence emoji;

    if(mjb_string_emoji_sequence(buffer, size, encoding, &emoji) == MJB_STATUS_OK) {
        fuzz_sink += (size_t)emoji.type + (size_t)emoji.qualification +
            emoji.codepoint_count;
    }

    fuzz_sink += (size_t)mjb_string_is_emoji_sequence(buffer, size, encoding);
    fuzz_sink += (size_t)mjb_string_is_rgi_emoji(buffer, size, encoding);
}

static void fuzz_emoji_string_apis(const char *buffer, size_t size, mjb_encoding encoding) {
    fuzz_emoji_string_api_input(buffer, size, encoding);

    if(size > 0) {
        fuzz_emoji_string_api_input(buffer, size - 1, encoding);
    }
}

// Free an output that is a distinct heap allocation.
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
        MJB_ENCODING_UTF_32_LE, MJB_ENCODING_UTF_32_BE, MJB_ENC_ASCII
    };
    mjb_encoding encoding = encodings[variant % 6];

    // Exercise the language-sensitive casing and folding paths too.
    static const unsigned int locales[] = {
        MJB_LOCALE_EN, MJB_LOCALE_TR, MJB_LOCALE_AZ, MJB_LOCALE_LT
    };

    if(mjb_locale_set(locales[(variant >> 3) % 4]) != MJB_STATUS_OK) {
        return 0;
    }

    mjb_result result = {0};
    mjb_codepoint codepoint = fuzz_codepoint((const uint8_t*)buffer, size, variant);

    switch(selector % 17) {
        case 0: // Normalization, all four forms
            if(mjb_normalize(buffer, size, (mjb_normalization)(variant % 4), encoding,
                MJB_ENCODING_UTF_8, &result) == MJB_STATUS_OK) {
                free_result(&result, buffer);
            }

            break;

        case 1: // Normalization quick check
            mjb_string_is_normalized(buffer, size, encoding, (mjb_normalization)(variant % 4));
            break;

        case 2: // Case conversion and folding, all transforming types
            if(mjb_case(buffer, size, (mjb_case_type)(1 + (variant % 5)), encoding,
                MJB_ENCODING_UTF_8, &result) == MJB_STATUS_OK) {
                free_result(&result, buffer);
            }

            break;

        case 3: { // Bidirectional algorithm, all three directions
            mjb_bidi_paragraph para;
            mjb_direction direction = (mjb_direction)(variant % 3);

            if(mjb_bidi_resolve(buffer, size, encoding, direction, &para) == MJB_STATUS_OK) {
                if(para.count > 0 && para.count <= 4096) {
                    size_t visual_order[4096];

                    if(mjb_bidi_reorder_line(&para, 0, para.count, visual_order) ==
                        MJB_STATUS_OK) {
                        size_t run_count = 0;
                        mjb_status runs_status = mjb_bidi_line_runs(&para, visual_order,
                            para.count, NULL, &run_count);
                        fuzz_sink += (size_t)runs_status;
                    }
                }

                mjb_bidi_free(&para);
            }

            break;
        }

        case 4: // Encoding detection
            fuzz_sink += (size_t)mjb_string_encoding(buffer, size);
            fuzz_sink += (size_t)mjb_string_is_utf8(buffer, size);
            fuzz_sink += (size_t)mjb_string_is_utf16(buffer, size);
            fuzz_sink += (size_t)mjb_string_is_ascii(buffer, size);
            break;

        case 5: // Encoding conversion
            if(mjb_string_convert_encoding(buffer, size, encoding,
                encodings[(variant >> 1) % 6], &result) == MJB_STATUS_OK) {
                free_result(&result, buffer);
            }

            break;

        case 6: // String filtering, all filter combinations
            if(mjb_string_filter(buffer, size, encoding, MJB_ENCODING_UTF_8,
                (mjb_filter)(variant & 0x1F), &result) == MJB_STATUS_OK) {
                free_result(&result, buffer);
            }

            break;

        case 7: // Collation key
            if(mjb_collation_key(buffer, size, encoding,
                (variant & 0x10) ? MJB_COLLATION_SHIFTED : MJB_COLLATION_NON_IGNORABLE,
                &result) == MJB_STATUS_OK) {
                free_result(&result, buffer);
            }

            break;

        case 8: // Collation comparison, input split in two halves
            mjb_string_compare(buffer, size / 2, buffer + size / 2, size - size / 2, encoding,
                (variant & 0x10) ? MJB_COLLATION_SHIFTED : MJB_COLLATION_NON_IGNORABLE);
            break;

        case 9: // Segmentation: grapheme, word and width truncation
            fuzz_sink += mjb_strnlen(buffer, size, encoding);
            mjb_truncate(buffer, size, encoding, variant);
            mjb_truncate_word(buffer, size, encoding, variant);
            mjb_truncate_width(buffer, size, encoding,
                (mjb_width_context)(variant % 3), variant);
            mjb_truncate_word_width(buffer, size, encoding,
                (mjb_width_context)(variant % 3), variant);
            break;

        case 10: { // Display width
            size_t width = 0;
            mjb_status status = mjb_display_width(buffer, size, encoding,
                (mjb_width_context)(variant % 3), &width);
            fuzz_sink += (size_t)status;
            if(status == MJB_STATUS_OK) {
                fuzz_sink += width;
            }
            break;
        }

        case 11: // Identifier validation, both profiles
            mjb_string_is_identifier(buffer, size, encoding,
                (variant & 0x10) ? MJB_IDENTIFIER_NFKC : MJB_IDENTIFIER_DEFAULT);
            break;

        case 12: // Confusable detection, input split in two halves
            mjb_string_is_confusable(buffer, size / 2, buffer + size / 2, size - size / 2,
                encoding);
            break;

        case 13: { // BCP 47 locale parsing
            mjb_locale_id locale;
            mjb_error error;
            mjb_status locale_status = mjb_locale_parse(buffer, size, encoding, &locale,
                &error);
            fuzz_sink += (size_t)locale_status;
            break;
        }

        case 14: // Codepoint metadata, properties, Hangul, emoji and encoding helpers
            fuzz_codepoint_apis(codepoint, variant);
            break;

        case 15: // Emoji sequence string APIs
            fuzz_emoji_string_apis(buffer, size, encoding);
            break;

        case 16: // Raw boundary iterators and character callback API
            fuzz_boundary_iterators(buffer, size, encoding);
            fuzz_sink += (size_t)mjb_next_character(buffer, size, encoding, fuzz_next_character);
            break;
    }

    return 0;
}
