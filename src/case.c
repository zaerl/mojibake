/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "mojibake-internal.h"
#include "unicode-tables.h"
#include "utf.h"

extern mojibake mjb_global;

// Cased: Lu, Ll and Lt plus Other_Uppercase and Other_Lowercase.
static bool mjb_is_cased(mjb_codepoint codepoint) {
    return mjb_unicode_has_property(codepoint, MJB_PR_CASED, NULL);
}

// Characters that do not interrupt a casing context, like combining marks and apostrophes.
static bool mjb_is_case_ignorable(mjb_codepoint codepoint) {
    return mjb_unicode_has_property(codepoint, MJB_PR_CASE_IGNORABLE, NULL);
}

static uint8_t mjb_combining_class(mjb_codepoint codepoint) {
    mjb_n_character character;

    if(!mjb_unicode_n_character_lookup(codepoint, &character)) {
        return 0;
    }

    return character.combining;
}

static void mjb_case_lookup_or_identity(mjb_codepoint codepoint,
    mjb_unicode_case_mapping *mapping) {
    if(mjb_unicode_case_lookup(codepoint, mapping)) {
        return;
    }

    // Until proven otherwise, treat unknown as non-word.
    mapping->category = MJB_CATEGORY_CN;
    mapping->uppercase = 0;
    mapping->lowercase = 0;
    mapping->titlecase = 0;

    if(mjb_unicode_category_lookup(codepoint, &mapping->category)) {
        return;
    }

    mjb_character character;

    if(mjb_codepoint_character(codepoint, &character) == MJB_STATUS_OK) {
        mapping->category = character.category;
    }
}

// Casing context for the conditional mappings of SpecialCasing.txt. The flags describe the
// characters already consumed; lookahead conditions are computed on demand.
typedef struct mjb_case_context {
    bool preceded_by_cased; // Final_Sigma: a cased letter precedes, skipping case-ignorable.
    bool after_i; // After_I (tr/az): the last character of combining class 0 or 230 was U+0049.
    bool after_soft_dotted; // After_Soft_Dotted (lt): it was a Soft_Dotted character.
    bool track_locale;      // after_i and after_soft_dotted are only needed for tr, az and lt.
} mjb_case_context;

static void mjb_case_context_update(mjb_case_context *context, mjb_codepoint codepoint) {
    if(mjb_is_cased(codepoint)) {
        context->preceded_by_cased = true;
    } else if(!mjb_is_case_ignorable(codepoint)) {
        context->preceded_by_cased = false;
    }

    if(!context->track_locale) {
        return;
    }

    uint8_t combining = mjb_combining_class(codepoint);

    if(codepoint == 0x49) { // U+0049 LATIN CAPITAL LETTER I
        context->after_i = true;
    } else if(combining == MJB_CCC_NOT_REORDERED || combining == MJB_CCC_ABOVE) {
        context->after_i = false;
    }

    if(mjb_unicode_has_property(codepoint, MJB_PR_SOFT_DOTTED, NULL)) {
        context->after_soft_dotted = true;
    } else if(combining == MJB_CCC_NOT_REORDERED || combining == MJB_CCC_ABOVE) {
        context->after_soft_dotted = false;
    }
}

static bool mjb_maybe_has_special_casing(mjb_codepoint codepoint) {
    // Codepoints with unconditional multi-char entries in SpecialCasing.txt.
    switch(codepoint) {
        case 223:  // U+00DF ß LATIN SMALL LETTER SHARP S
        case 304:  // U+0130 İ LATIN CAPITAL LETTER I WITH DOT ABOVE
        case 329:  // U+0149 ŉ LATIN SMALL LETTER N PRECEDED BY APOSTROPHE
        case 496:  // U+01F0 ǰ LATIN SMALL LETTER J WITH CARON
        case 912:  // U+0390 ΐ GREEK SMALL LETTER IOTA WITH DIALYTIKA AND TONOS
        case 944:  // U+03B0 ΰ GREEK SMALL LETTER UPSILON WITH DIALYTIKA AND TONOS
        case 1415: // U+0587 եւ ARMENIAN SMALL LIGATURE ECH YIWN
            return true;
    }

    return (codepoint >= 7830 && codepoint <= 7834) || // U+1E96–U+1E9A
        (codepoint >= 8016 && codepoint <= 8188) ||    // U+1F50–U+1FFC
        (codepoint >= 64256 && codepoint <= 64279);    // U+FB00–U+FB17
}

static bool mjb_case_output_codepoint(mjb_codepoint codepoint, char **output, size_t *output_index,
    size_t *output_size, mjb_encoding encoding) {
    char *new_output = mjb_string_output_codepoint(codepoint, *output, output_index, output_size,
        encoding);

    if(new_output == NULL) {
        return false;
    }

    *output = new_output;

    return true;
}

static bool mjb_special_casing_codepoint(mjb_codepoint codepoint, char **output,
    size_t *output_index, size_t *output_size, mjb_case_type type, mjb_encoding output_encoding,
    bool *found) {
    const mjb_codepoint *values = NULL;
    uint8_t length = 0;
    *found = false;

    if(!mjb_unicode_special_casing_lookup(codepoint, type, &values, &length)) {
        return true;
    }

    *found = true;

    for(uint8_t i = 0; i < length; ++i) {
        if(!mjb_case_output_codepoint(values[i], output, output_index, output_size,
               output_encoding)) {
            return false;
        }
    }

    return true;
}

static size_t mjb_titlecase_next_word_boundary(const char *buffer, size_t byte_length,
    mjb_encoding encoding, mjb_next_word_state *state, size_t previous) {
    mjb_break_type bt;

    while((bt = mjb_break_word(buffer, byte_length, encoding, state)) != MJB_BT_NOT_SET) {
        if(bt == MJB_BT_NO_BREAK) {
            continue;
        }

        size_t break_pos = mjb_monotonic_boundary_position(state->index, byte_length,
            state->current_codepoint, encoding, state->state == MJB_UTF_TERMINATED, previous);

        if(break_pos > previous) {
            return break_pos;
        }
    }

    return byte_length;
}

// Final_Sigma lookahead: a cased letter follows, skipping case-ignorable characters.
static bool mjb_followed_by_cased(const char *buffer, size_t byte_length, size_t i, uint8_t state,
    mjb_encoding encoding) {
    mjb_codepoint codepoint = 0;
    bool in_error = false;

    for(;;) {
        mjb_decode_result result = mjb_next_codepoint(buffer, byte_length, &state, &i, encoding,
            &codepoint, &in_error);

        if(result == MJB_DECODE_END) {
            return false;
        }

        if(result == MJB_DECODE_INCOMPLETE || mjb_is_case_ignorable(codepoint)) {
            continue;
        }

        return mjb_is_cased(codepoint);
    }
}

// More_Above (lt) lookahead: a character of combining class 230 follows, with no intervening
// character of combining class 0.
static bool mjb_more_above(const char *buffer, size_t byte_length, size_t i, uint8_t state,
    mjb_encoding encoding) {
    mjb_codepoint codepoint = 0;
    bool in_error = false;

    for(;;) {
        mjb_decode_result result = mjb_next_codepoint(buffer, byte_length, &state, &i, encoding,
            &codepoint, &in_error);

        if(result == MJB_DECODE_END) {
            return false;
        }

        if(result == MJB_DECODE_INCOMPLETE) {
            continue;
        }

        uint8_t combining = mjb_combining_class(codepoint);

        if(combining == MJB_CCC_ABOVE) {
            return true;
        }

        if(combining == MJB_CCC_NOT_REORDERED) {
            return false;
        }
    }
}

// Before_Dot (tr/az) lookahead: U+0307 COMBINING DOT ABOVE follows, skipping characters with
// combining class other than 0 and 230.
static bool mjb_before_dot(const char *buffer, size_t byte_length, size_t i, uint8_t state,
    mjb_encoding encoding) {
    mjb_codepoint codepoint = 0;
    bool in_error = false;

    for(;;) {
        mjb_decode_result result = mjb_next_codepoint(buffer, byte_length, &state, &i, encoding,
            &codepoint, &in_error);

        if(result == MJB_DECODE_END) {
            return false;
        }

        if(result == MJB_DECODE_INCOMPLETE) {
            continue;
        }

        if(codepoint == 0x307) {
            return true;
        }

        uint8_t combining = mjb_combining_class(codepoint);

        if(combining == MJB_CCC_NOT_REORDERED || combining == MJB_CCC_ABOVE) {
            return false;
        }
    }
}

// Language-sensitive conditional mappings of SpecialCasing.txt for Lithuanian (lt), Turkish (tr)
// and Azerbaijani (az). Final_Sigma is language-insensitive and handled in the casing loops.
// Sets *handled when a rule matched; a matched rule may emit no output (removed character).
static bool mjb_locale_special_casing(mjb_codepoint codepoint, mjb_case_type type,
    const mjb_case_context *context, const char *buffer, size_t byte_length, size_t i,
    uint8_t state, mjb_encoding encoding, mjb_encoding output_encoding, char **output,
    size_t *output_index, size_t *output_size, bool *handled) {
    mjb_codepoint mapped[3];
    uint8_t length = 0;
    bool turkic = mjb_global.locale == MJB_LOCALE_TR || mjb_global.locale == MJB_LOCALE_AZ;

    *handled = false;

    if(turkic) {
        if(type == MJB_CASE_LOWER) {
            if(codepoint == 0x130) { // İ → i
                mapped[length++] = 0x69;
            } else if(codepoint == 0x307 && context->after_i) {
                // Remove the dot above of a lowercased I: the pair becomes a plain i.
            } else if(codepoint == 0x49 &&
                !mjb_before_dot(buffer, byte_length, i, state, encoding)) {
                mapped[length++] = 0x131; // I → ı
            } else {
                return true;
            }
        } else if(codepoint == 0x69) { // i → İ when uppercasing or titlecasing
            mapped[length++] = 0x130;
        } else {
            return true;
        }
    } else if(mjb_global.locale == MJB_LOCALE_LT) {
        if(type == MJB_CASE_LOWER) {
            switch(codepoint) {
                // Introduce an explicit dot above when there are more accents above.
                case 0x49:  // I → i + dot above
                case 0x4A:  // J → j + dot above
                case 0x12E: // Į → į + dot above
                    if(!mjb_more_above(buffer, byte_length, i, state, encoding)) {
                        return true;
                    }

                    mapped[length++] = codepoint == 0x12E ? 0x12F : codepoint + 0x20;
                    mapped[length++] = 0x307;
                    break;

                case 0xCC:  // Ì → i + dot above + grave
                case 0xCD:  // Í → i + dot above + acute
                case 0x128: // Ĩ → i + dot above + tilde
                    mapped[length++] = 0x69;
                    mapped[length++] = 0x307;
                    // clang-format off
                    mapped[length++] = codepoint == 0xCC ? 0x300 :
                        (codepoint == 0xCD ? 0x301 : 0x303);
                    // clang-format on
                    break;

                default:
                    return true;
            }
        } else if(codepoint == 0x307 && context->after_soft_dotted) {
            // Remove the dot above: the soft-dotted base letter loses its dot when cased.
        } else {
            return true;
        }
    } else {
        return true;
    }

    *handled = true;

    for(uint8_t k = 0; k < length; ++k) {
        if(!mjb_case_output_codepoint(mapped[k], output, output_index, output_size,
               output_encoding)) {
            return false;
        }
    }

    return true;
}

/**
 * See: https://www.unicode.org/versions/Unicode17.0.0/core-spec/chapter-3/#G34078
 */
static mjb_status mjb_titlecase(const char *buffer, size_t byte_length, mjb_encoding encoding,
    mjb_encoding output_encoding, mjb_result *result) {
    uint8_t state = MJB_UTF_ACCEPT;
    bool in_error = false;
    mjb_codepoint codepoint;
    char *output = (char *)mjb_alloc(byte_length);

    if(output == NULL) {
        return MJB_STATUS_NO_MEMORY;
    }

    size_t output_index = 0;
    size_t output_size = byte_length;
    bool locale_sensitive = mjb_global.locale == MJB_LOCALE_TR ||
        mjb_global.locale == MJB_LOCALE_AZ || mjb_global.locale == MJB_LOCALE_LT;
    mjb_case_context context = { false, false, false, locale_sensitive };
    mjb_next_word_state word_state;
    word_state.index = 0;
    size_t segment_end = mjb_titlecase_next_word_boundary(buffer, byte_length, encoding,
        &word_state, 0);
    bool segment_has_cased = false;

    for(size_t i = 0; i < byte_length;) {
        size_t codepoint_start = i;

        // Find next codepoint.
        mjb_decode_result decode_status = mjb_next_codepoint(buffer, byte_length, &state, &i,
            encoding, &codepoint, &in_error);

        if(decode_status == MJB_DECODE_END) {
            break;
        }

        if(decode_status == MJB_DECODE_INCOMPLETE) {
            continue;
        }

        while(codepoint_start >= segment_end && segment_end < byte_length) {
            segment_has_cased = false;
            segment_end = mjb_titlecase_next_word_boundary(buffer, byte_length, encoding,
                &word_state, segment_end);
        }

        mjb_unicode_case_mapping mapping;
        mjb_case_lookup_or_identity(codepoint, &mapping);

        mjb_codepoint original = codepoint;
        mjb_case_type case_type = MJB_CASE_NONE;
        mjb_case_type effective_type = MJB_CASE_NONE;
        bool is_cased = mjb_is_cased(original);

        if(!segment_has_cased && is_cased) {
            effective_type = MJB_CASE_TITLE;
            case_type = MJB_CASE_TITLE;
            segment_has_cased = true;

            // Titlecase_Mapping falls back to Uppercase_Mapping when no distinct titlecase
            // mapping exists.
            if(mapping.titlecase != 0) {
                codepoint = mapping.titlecase;
            } else if(mapping.uppercase != 0) {
                codepoint = mapping.uppercase;
            }
        } else if(segment_has_cased) {
            effective_type = MJB_CASE_LOWER;
            case_type = MJB_CASE_LOWER;

            if(mapping.lowercase != 0) {
                codepoint = mapping.lowercase;
            }
        }

        // Final_Sigma (SpecialCasing.txt condition Final_Sigma):
        // In-word Σ (U+03A3) → ς (U+03C2) when preceded by a cased letter and not followed by a
        // cased letter, in both cases skipping case-ignorable characters.
        if(original == 0x03A3 && case_type == MJB_CASE_LOWER) {
            mjb_codepoint sigma_out = context.preceded_by_cased &&
                    !mjb_followed_by_cased(buffer, byte_length, i, state,
                        encoding) ?
                0x03C2 // ς: word-final sigma
                :
                0x03C3; // σ: followed by a cased letter, not word-final

            mjb_case_context_update(&context, original);

            if(!mjb_case_output_codepoint(sigma_out, &output, &output_index, &output_size,
                   output_encoding)) {
                mjb_free(output);

                return MJB_STATUS_NO_MEMORY;
            }

            continue;
        }

        // Language-sensitive conditional rules.
        if(locale_sensitive) {
            bool handled = false;

            if(!mjb_locale_special_casing(original, effective_type, &context, buffer, byte_length,
                   i, state, encoding, output_encoding, &output, &output_index, &output_size,
                   &handled)) {
                mjb_free(output);

                return MJB_STATUS_NO_MEMORY;
            }

            if(handled) {
                mjb_case_context_update(&context, original);

                continue;
            }
        }

        mjb_case_context_update(&context, original);

        // Check against the original codepoint (before any remapping by the word-boundary
        // block above). The remapped form may not be the source of any special casing rule.
        if(case_type != MJB_CASE_NONE && mjb_maybe_has_special_casing(original)) {
            bool found_special_casing = false;

            if(!mjb_special_casing_codepoint(original, &output, &output_index, &output_size,
                   case_type, output_encoding, &found_special_casing)) {
                mjb_free(output);

                return MJB_STATUS_NO_MEMORY;
            }

            if(found_special_casing) {
                continue;
            }
        }

        if(!mjb_case_output_codepoint(codepoint, &output, &output_index, &output_size,
               output_encoding)) {
            mjb_free(output);

            return MJB_STATUS_NO_MEMORY;
        }
    }

    if(output_index >= output_size) {
        char *new_output = (char *)mjb_realloc(output, output_size + 1);

        if(new_output == NULL) {
            mjb_free(output);

            return MJB_STATUS_NO_MEMORY;
        }

        output = new_output;
    }

    output[output_index] = '\0';

    result->output = output;
    result->output_size = output_index;
    result->transformed = true;

    return MJB_STATUS_OK;
}

MJB_EXPORT mjb_status mjb_case(const char *buffer, size_t byte_length, mjb_encoding encoding,
    mjb_case_type type, mjb_encoding output_encoding, mjb_result *result) {
    if(result == NULL || (buffer == NULL && byte_length > 0)) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    if(type != MJB_CASE_UPPER && type != MJB_CASE_LOWER && type != MJB_CASE_TITLE &&
        type != MJB_CASE_CASEFOLD && type != MJB_CASE_CASEFOLD_SIMPLE) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    if(byte_length == 0) {
        result->output = (char *)buffer;
        result->output_size = 0;
        result->transformed = false;

        return MJB_STATUS_OK;
    }

    if(type == MJB_CASE_TITLE) {
        return mjb_titlecase(buffer, byte_length, encoding, output_encoding, result);
    }

    mjb_codepoint codepoint;
    uint8_t state = MJB_UTF_ACCEPT;
    bool in_error = false;
    char *output = (char *)mjb_alloc(byte_length);

    if(output == NULL) {
        return MJB_STATUS_NO_MEMORY;
    }

    size_t output_index = 0;
    size_t output_size = byte_length;
    bool turkic = mjb_global.locale == MJB_LOCALE_TR || mjb_global.locale == MJB_LOCALE_AZ;
    bool locale_sensitive = turkic || mjb_global.locale == MJB_LOCALE_LT;
    // The context is only needed for Final_Sigma (lowercase) and the language-sensitive rules.
    bool track_context = type == MJB_CASE_LOWER || locale_sensitive;
    mjb_case_context context = { false, false, false, locale_sensitive };

    for(size_t i = 0; i < byte_length;) {
        // Find next codepoint.
        mjb_decode_result decode_status = mjb_next_codepoint(buffer, byte_length, &state, &i,
            encoding, &codepoint, &in_error);

        if(decode_status == MJB_DECODE_END) {
            break;
        }

        if(decode_status == MJB_DECODE_INCOMPLETE) {
            continue;
        }

        if(type == MJB_CASE_CASEFOLD || type == MJB_CASE_CASEFOLD_SIMPLE) {
            // Turkic (T) foldings [CaseFolding.txt]: in tr/az, I folds to ı and İ to i, in
            // both full and simple folding.
            if(turkic && (codepoint == 0x49 || codepoint == 0x130)) {
                if(!mjb_case_output_codepoint(codepoint == 0x49 ? 0x131 : 0x69, &output,
                       &output_index, &output_size, output_encoding)) {
                    mjb_free(output);

                    return MJB_STATUS_NO_MEMORY;
                }

                continue;
            }

            // Simple (S) foldings: single-char alternatives to multi-char full folds.
            mjb_codepoint simple = 0;

            if(type == MJB_CASE_CASEFOLD_SIMPLE &&
                mjb_unicode_case_folding_simple_lookup(codepoint, &simple)) {
                if(!mjb_case_output_codepoint(simple, &output, &output_index, &output_size,
                       output_encoding)) {
                    mjb_free(output);

                    return MJB_STATUS_NO_MEMORY;
                }

                continue;
            }

            const mjb_codepoint *values = NULL;
            uint8_t length = 0;

            if(mjb_unicode_case_folding_lookup(codepoint, &values, &length)) {
                // A single codepoint is a common (C) fold, valid for both folding types. A
                // multi-char fold is full (F) only: without a simple (S) alternative, simple
                // folding maps the character to itself.
                if(type == MJB_CASE_CASEFOLD_SIMPLE) {
                    if(!mjb_case_output_codepoint(length == 1 ? values[0] : codepoint, &output,
                           &output_index, &output_size, output_encoding)) {
                        mjb_free(output);

                        return MJB_STATUS_NO_MEMORY;
                    }

                    continue;
                }

                // Emit up to 3 mapped codepoints (F entries have 2-3, C exceptions have 1)
                for(uint8_t k = 0; k < length; ++k) {
                    if(!mjb_case_output_codepoint(values[k], &output, &output_index, &output_size,
                           output_encoding)) {
                        mjb_free(output);

                        return MJB_STATUS_NO_MEMORY;
                    }
                }

                continue;
            }

            // Common one-codepoint folds that equal the lowercase mapping are omitted from the
            // compact fold table.
            // Changes_When_Casefolded distinguishes them from true identity mappings, including
            // uppercase Cherokee.
            mjb_unicode_case_mapping mapping;

            if(mjb_codepoint_has_binary_property(codepoint, MJB_PR_CHANGES_WHEN_CASEFOLDED) &&
                mjb_unicode_case_lookup(codepoint, &mapping) && mapping.lowercase != 0) {
                codepoint = mapping.lowercase;
            }

            // Identity: codepoint unchanged if no lowercase found
            if(!mjb_case_output_codepoint(codepoint, &output, &output_index, &output_size,
                   output_encoding)) {
                mjb_free(output);

                return MJB_STATUS_NO_MEMORY;
            }

            continue;
        }

        // Language-sensitive conditional rules (SpecialCasing.txt "Conditional Mappings").
        if(locale_sensitive) {
            bool handled = false;

            if(!mjb_locale_special_casing(codepoint, type, &context, buffer, byte_length, i, state,
                   encoding, output_encoding, &output, &output_index, &output_size, &handled)) {
                mjb_free(output);

                return MJB_STATUS_NO_MEMORY;
            }

            if(handled) {
                mjb_case_context_update(&context, codepoint);

                continue;
            }
        }

        // Final_Sigma (SpecialCasing.txt condition Final_Sigma):
        // Σ (U+03A3) → ς (U+03C2) when preceded by a cased letter and not followed by a cased
        // letter, in both cases skipping case-ignorable characters.
        if(type == MJB_CASE_LOWER && codepoint == 0x03A3) {
            mjb_codepoint sigma_out;

            if(context.preceded_by_cased &&
                !mjb_followed_by_cased(buffer, byte_length, i, state, encoding)) {
                sigma_out = 0x03C2; // ς: word-final sigma
            } else {
                sigma_out = 0x03C3; // σ: non-final or no preceding cased letter
            }

            mjb_case_context_update(&context, codepoint);

            if(!mjb_case_output_codepoint(sigma_out, &output, &output_index, &output_size,
                   output_encoding)) {
                mjb_free(output);

                return MJB_STATUS_NO_MEMORY;
            }

            continue;
        }

        if(track_context) {
            mjb_case_context_update(&context, codepoint);
        }

        if(mjb_maybe_has_special_casing(codepoint)) {
            bool found_special_casing = false;

            if(!mjb_special_casing_codepoint(codepoint, &output, &output_index, &output_size, type,
                   output_encoding, &found_special_casing)) {
                mjb_free(output);

                return MJB_STATUS_NO_MEMORY;
            }

            if(found_special_casing) {
                continue;
            }
        }

        mjb_unicode_case_mapping mapping;
        bool found_mapping = mjb_unicode_case_lookup(codepoint, &mapping);

        mjb_codepoint mapped = 0;

        if(found_mapping) {
            switch(type) {
                case MJB_CASE_UPPER:
                    mapped = mapping.uppercase;
                    break;
                case MJB_CASE_LOWER:
                    mapped = mapping.lowercase;
                    break;
                case MJB_CASE_TITLE:
                    mapped = mapping.titlecase;
                    break;
                default:
                    break;
            }
        }

        if(mapped != 0) {
            codepoint = mapped;
        }

        if(!mjb_case_output_codepoint(codepoint, &output, &output_index, &output_size,
               output_encoding)) {
            mjb_free(output);

            return MJB_STATUS_NO_MEMORY;
        }
    }

    if(output_index >= output_size) {
        char *new_output = (char *)mjb_realloc(output, output_size + 1);

        if(new_output == NULL) {
            mjb_free(output);

            return MJB_STATUS_NO_MEMORY;
        }

        output = new_output;
    }

    output[output_index] = '\0';

    result->output = output;
    result->output_size = output_index;
    result->transformed = true;

    return MJB_STATUS_OK;
}
