/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "mojibake-internal.h"
#include "unicode-tables.h"
#include "utf.h"

static bool mjb_is_cased(mjb_category category) {
    return category == MJB_CATEGORY_LU || category == MJB_CATEGORY_LL ||
        category == MJB_CATEGORY_LT;
}

// Mn, Me and Cf do not break words.
static bool mjb_is_case_ignorable(mjb_category category) {
    return category == MJB_CATEGORY_MN || category == MJB_CATEGORY_ME ||
        category == MJB_CATEGORY_CF;
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

    return
        (codepoint >= 7830 && codepoint <= 7834) || // U+1E96–U+1E9A
        (codepoint >= 8016 && codepoint <= 8188) || // U+1F50–U+1FFC
        (codepoint >= 64256 && codepoint <= 64279); // U+FB00–U+FB17
}

static bool mjb_case_output_codepoint(mjb_codepoint codepoint, char **output,
    size_t *output_index, size_t *output_size, mjb_encoding encoding) {
    char *new_output = mjb_string_output_codepoint(codepoint, *output, output_index,
        output_size, encoding);

    if(new_output == NULL) {
        return false;
    }

    *output = new_output;

    return true;
}

static bool mjb_special_casing_codepoint(mjb_codepoint codepoint, char **output,
    size_t *output_index, size_t *output_size, mjb_case_type type, bool *found) {
    const mjb_codepoint *values = NULL;
    uint8_t length = 0;
    *found = false;

    if(!mjb_unicode_special_casing_lookup(codepoint, type, &values, &length)) {
        return true;
    }

    *found = true;

    for(uint8_t i = 0; i < length; ++i) {
        if(!mjb_case_output_codepoint(values[i], output, output_index, output_size,
            MJB_ENCODING_UTF_8)) {
            return false;
        }
    }

    return true;
}

// Peek at the next decoded codepoint and return whether it is cased (Lu/Ll/Lt).
static bool mjb_next_is_cased(const char *buffer, size_t size, size_t i, uint8_t state,
    mjb_encoding encoding) {
    mjb_codepoint next_cp = 0;
    bool next_in_error = false;
    mjb_decode_result ps;
    mjb_unicode_case_mapping mapping;

    do {
        ps = mjb_next_codepoint(buffer, size, &state, &i, encoding, &next_cp, &next_in_error);
    } while(ps == MJB_DECODE_INCOMPLETE);

    if(ps == MJB_DECODE_END) {
        return false;
    }

    if(!mjb_unicode_case_lookup(next_cp, &mapping)) {
        return false;
    }

    return mjb_is_cased(mapping.category);
}

/**
 * See: https://www.unicode.org/versions/Unicode17.0.0/core-spec/chapter-3/#G34078
 */
static char *mjb_titlecase(const char *buffer, size_t size, mjb_encoding encoding) {
    uint8_t state = MJB_UTF_ACCEPT;
    bool in_error = false;
    mjb_codepoint codepoint;
    char *output = (char*)mjb_alloc(size);

    if(output == NULL) {
        return NULL;
    }

    // char *output = mjb_alloc(length);
    size_t output_index = 0;
    size_t output_size = size;
    bool in_word = false;

    for(size_t i = 0; i < size;) {
        // Find next codepoint.
        mjb_decode_result decode_status = mjb_next_codepoint(buffer, size, &state, &i, encoding,
            &codepoint, &in_error);

        if(decode_status == MJB_DECODE_END) {
            break;
        }

        if(decode_status == MJB_DECODE_INCOMPLETE) {
            continue;
        }

        mjb_unicode_case_mapping mapping;

        if(!mjb_unicode_case_lookup(codepoint, &mapping)) {
            mjb_free(output);

            return NULL;
        }

        mjb_codepoint original = codepoint;
        mjb_category category = mapping.category;
        mjb_case_type case_type = MJB_CASE_NONE;

        // Word boundary.
        if(
            // Is cased.
            category == MJB_CATEGORY_LU || category == MJB_CATEGORY_LL ||
            category == MJB_CATEGORY_LT ||
            // Is a modifier.
            category == MJB_CATEGORY_LM ||
            // Is a number.
            category == MJB_CATEGORY_LO || category == MJB_CATEGORY_NL
        ) {

            if(!in_word) {
                // Try titlecase first.
                if(mapping.titlecase == 0) {
                    // If titlecase is not available, try uppercase.
                    if(mapping.uppercase != 0) {
                        codepoint = mapping.uppercase;
                        case_type = MJB_CASE_UPPER;
                    }
                } else {
                    codepoint = mapping.titlecase;
                    case_type = MJB_CASE_TITLE;
                }

                in_word = true;
            } else {
                // Try lowercase.
                if(mapping.lowercase != 0) {
                    codepoint = mapping.lowercase;
                    case_type = MJB_CASE_LOWER;
                }
            }
        } else {
            in_word = false;
        }

        // Final_Sigma (SpecialCasing.txt condition Final_Sigma):
        // In-word Σ (U+03A3) → ς (U+03C2) when not followed by a cased letter.
        if(original == 0x03A3 && case_type == MJB_CASE_LOWER) {
            mjb_codepoint sigma_out = mjb_next_is_cased(buffer, size, i, state, encoding)
                ? 0x03C3   // σ: followed by a cased letter, not word-final
                : 0x03C2;  // ς: word-final sigma

            if(!mjb_case_output_codepoint(sigma_out, &output, &output_index,
                &output_size, encoding)) {
                mjb_free(output);

                return NULL;
            }

            continue;
        }

        // Check against the original codepoint (before any remapping by the word-boundary
        // block above). The remapped form may not be the source of any special casing rule.
        if(mjb_maybe_has_special_casing(original)) {
            bool found_special_casing = false;

            if(!mjb_special_casing_codepoint(original, &output, &output_index, &output_size,
                case_type == MJB_CASE_NONE ? MJB_CASE_TITLE : case_type,
                &found_special_casing)) {
                mjb_free(output);

                return NULL;
            }

            if(found_special_casing) {
                continue;
            }
        }

        if(!mjb_case_output_codepoint(codepoint, &output, &output_index, &output_size,
            encoding)) {
            mjb_free(output);

            return NULL;
        }
    }

    if(output_index >= output_size) {
        char *new_output = (char*)mjb_realloc(output, output_size + 1);

        if(new_output == NULL) {
            mjb_free(output);

            return NULL;
        }

        output = new_output;
    }

    output[output_index] = '\0';

    return output;
}

MJB_EXPORT char *mjb_case(const char *buffer, size_t size, mjb_case_type type,
    mjb_encoding encoding) {
    if(buffer == NULL && size > 0) {
        return NULL;
    }

    if(size == 0) {
        return (char*)buffer;
    }

    if(!mjb_initialize()) {
        return NULL;
    }

    if(type == MJB_CASE_TITLE) {
        return mjb_titlecase(buffer, size, encoding);
    }

    mjb_codepoint codepoint;
    uint8_t state = MJB_UTF_ACCEPT;
    bool in_error = false;
    char *output = (char*)mjb_alloc(size);

    if(output == NULL) {
        return NULL;
    }

    size_t output_index = 0;
    size_t output_size = size;
    bool in_word = false; // tracks word context for Final_Sigma (MJB_CASE_LOWER only)

    for(size_t i = 0; i < size;) {
        // Find next codepoint.
        mjb_decode_result decode_status = mjb_next_codepoint(buffer, size, &state, &i, encoding,
            &codepoint, &in_error);

        if(decode_status == MJB_DECODE_END) {
            break;
        }

        if(decode_status == MJB_DECODE_INCOMPLETE) {
            continue;
        }

        if(type == MJB_CASE_CASEFOLD) {
            const mjb_codepoint *values = NULL;
            uint8_t length = 0;

            if(mjb_unicode_case_folding_lookup(codepoint, &values, &length)) {
                // Emit up to 3 mapped codepoints (F entries have 2-3, C exceptions have 1)
                for(uint8_t k = 0; k < length; ++k) {
                    if(!mjb_case_output_codepoint(values[k], &output, &output_index,
                        &output_size, encoding)) {
                        mjb_free(output);

                        return NULL;
                    }
                }

                continue;
            }

            // Fall back to unicode_data lowercase
            mjb_unicode_case_mapping mapping;

            if(mjb_unicode_case_lookup(codepoint, &mapping) && mapping.lowercase != 0) {
                codepoint = mapping.lowercase;
            }

            // Identity: codepoint unchanged if no lowercase found
            if(!mjb_case_output_codepoint(codepoint, &output, &output_index,
                &output_size, encoding)) {
                mjb_free(output);

                return NULL;
            }

            continue;
        }

        // Final_Sigma (SpecialCasing.txt condition Final_Sigma):
        // Σ (U+03A3) → ς (U+03C2) when preceded by a cased letter in the same word
        // and not followed by a cased letter.
        if(type == MJB_CASE_LOWER && codepoint == 0x03A3) {
            mjb_codepoint sigma_out;

            if(in_word && !mjb_next_is_cased(buffer, size, i, state, encoding)) {
                sigma_out = 0x03C2; // ς: word-final sigma
            } else {
                sigma_out = 0x03C3; // σ: non-final or no preceding cased letter
            }

            in_word = true;  // Σ is a cased letter

            if(!mjb_case_output_codepoint(sigma_out, &output, &output_index,
                &output_size, encoding)) {
                mjb_free(output);

                return NULL;
            }

            continue;
        }

        if(mjb_maybe_has_special_casing(codepoint)) {
            bool found_special_casing = false;

            if(!mjb_special_casing_codepoint(codepoint, &output, &output_index, &output_size,
                type, &found_special_casing)) {
                mjb_free(output);

                return NULL;
            }

            if(found_special_casing) {
                if(type == MJB_CASE_LOWER) {
                    in_word = true;
                }

                continue;
            }
        }

        mjb_unicode_case_mapping mapping;

        if(!mjb_unicode_case_lookup(codepoint, &mapping)) {
            mjb_free(output);

            return NULL;
        }

        if(type == MJB_CASE_LOWER) {
            if(mjb_is_cased(mapping.category)) {
                in_word = true;
            } else if(!mjb_is_case_ignorable(mapping.category)) {
                in_word = false;
            }
        }

        mjb_codepoint mapped = 0;

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

        if(mapped != 0) {
            codepoint = mapped;
        }

        if(!mjb_case_output_codepoint(codepoint, &output, &output_index, &output_size,
            encoding)) {
            mjb_free(output);

            return NULL;
        }
    }

    if(output_index >= output_size) {
        char *new_output = (char*)mjb_realloc(output, output_size + 1);

        if(new_output == NULL) {
            mjb_free(output);

            return NULL;
        }

        output = new_output;
    }

    output[output_index] = '\0';

    return output;
}

// Return the codepoint lowercase codepoint
MJB_EXPORT mjb_codepoint mjb_codepoint_to_lowercase(mjb_codepoint codepoint) {
    mjb_unicode_case_mapping mapping;

    if(!mjb_unicode_case_lookup(codepoint, &mapping)) {
        return codepoint;
    }

    return mapping.lowercase == 0 ? codepoint : mapping.lowercase;
}

// Return the codepoint uppercase codepoint
MJB_EXPORT mjb_codepoint mjb_codepoint_to_uppercase(mjb_codepoint codepoint) {
    mjb_unicode_case_mapping mapping;

    if(!mjb_unicode_case_lookup(codepoint, &mapping)) {
        return codepoint;
    }

    return mapping.uppercase == 0 ? codepoint : mapping.uppercase;
}

// Return the codepoint titlecase codepoint
MJB_EXPORT mjb_codepoint mjb_codepoint_to_titlecase(mjb_codepoint codepoint) {
    mjb_unicode_case_mapping mapping;

    if(!mjb_unicode_case_lookup(codepoint, &mapping)) {
        return codepoint;
    }

    return mapping.titlecase == 0 ? codepoint : mapping.titlecase;
}
