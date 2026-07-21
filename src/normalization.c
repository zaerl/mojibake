/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <string.h>

#include "mojibake-internal.h"
#include "unicode-tables.h"
#include "utf.h"

typedef struct mjb_normalize_context {
    const char *buffer;
    size_t byte_length;
    size_t potential_output_size;
    mjb_encoding encoding;
    mjb_normalization form;
    mjb_encoding output_encoding;
} mjb_normalize_context;

// Normalization sort.
static void mjb_normalization_sort(mjb_n_character array[], size_t size) {
    for(size_t step = 1; step < size; ++step) {
        mjb_n_character key = array[step];
        size_t j = step;

        // Move elements of array[0..step-1], that are greater than key,
        // to one position ahead of their current position
        while(j > 0 && array[j - 1].combining > key.combining) {
            array[j] = array[j - 1];
            --j;
        }

        array[j] = key;
    }
}

// Flush the decomposition buffer to the transformation output.
static mjb_status mjb_flush_d_buffer(mjb_n_character *characters_buffer, size_t buffer_index,
    mjb_output *output, mjb_encoding output_encoding) {

    if(buffer_index == 0) {
        return MJB_STATUS_OK;
    }

    // Sort combining characters by Canonical Combining Class (required for all forms)
    // Skip sorting if only one character (already sorted)
    if(buffer_index > 1) {
        mjb_normalization_sort(characters_buffer, buffer_index);
    }

    // Write combining characters.
    for(size_t i = 0; i < buffer_index; ++i) {
        if(characters_buffer[i].codepoint == MJB_CODEPOINT_NOT_VALID) {
            continue;
        }

        mjb_status status = mjb_output_codepoint(output, characters_buffer[i].codepoint,
            output_encoding);

        if(status != MJB_STATUS_OK) {
            return status;
        }
    }

    return MJB_STATUS_OK;
}

// Flush the composition buffer to a bit array.
static mjb_status mjb_flush_c_buffer(mjb_n_character *characters_buffer, size_t buffer_index,
    mjb_buffer_character **output, size_t *output_index, size_t *output_size) {

    if(buffer_index == 0) {
        return MJB_STATUS_OK;
    }

    // Sort combining characters by Canonical Combining Class (required for all forms)
    // Skip sorting if only one character (already sorted)
    if(buffer_index > 1) {
        mjb_normalization_sort(characters_buffer, buffer_index);
    }

    for(size_t i = 0; i < buffer_index; ++i) {
        if(characters_buffer[i].codepoint == MJB_CODEPOINT_NOT_VALID) {
            continue;
        }

        // Check if we need to reallocate the output buffer
        if(*output_index >= *output_size) {
            size_t new_output_size = *output_size == 0 ? 1 : *output_size * 2;

            if(*output_size > SIZE_MAX / 2 ||
                new_output_size > SIZE_MAX / sizeof(mjb_buffer_character)) {
                return MJB_STATUS_OVERFLOW;
            }

            mjb_buffer_character *new_output = (mjb_buffer_character *)mjb_realloc(*output,
                new_output_size * sizeof(mjb_buffer_character));

            if(new_output == NULL) {
                return MJB_STATUS_NO_MEMORY;
            }

            *output = new_output;
            *output_size = new_output_size;
        }

        (*output)[*output_index].codepoint = characters_buffer[i].codepoint;
        (*output)[*output_index].combining = characters_buffer[i].combining;

        ++(*output_index);
    }

    return MJB_STATUS_OK;
}

/**
 * Recompose the string.
 * Canonical Composition Algorithm
 */
static mjb_status mjb_recompose(mjb_output *output, size_t codepoints_count,
    mjb_buffer_character *composition_buffer, mjb_encoding output_encoding) {
    if(codepoints_count == 0) {
        return MJB_STATUS_OK;
    }

    // Apply Hangul composition first
    bool has_hangul = false;
    for(size_t i = 0; i < codepoints_count; ++i) {
        if(mjb_codepoint_is_hangul_jamo(composition_buffer[i].codepoint) ||
            mjb_codepoint_is_hangul_syllable(composition_buffer[i].codepoint)) {
            has_hangul = true;
            break;
        }
    }

    if(has_hangul) {
        codepoints_count = mjb_hangul_syllable_composition(composition_buffer, codepoints_count);
    }

    size_t i = 0;

    while(i < codepoints_count) {
        if(composition_buffer[i].combining != MJB_CCC_NOT_REORDERED) {
            // Non-starter: output and continue
            mjb_status status = mjb_output_codepoint(output, composition_buffer[i].codepoint,
                output_encoding);

            if(status != MJB_STATUS_OK) {
                return status;
            }

            ++i;

            continue;
        }

        // Found a starter
        mjb_codepoint starter = composition_buffer[i].codepoint;
        size_t starter_pos = i;
        uint16_t last_combining_class = 0;

        ++i; // Move to first character after starter

        // Process characters following this starter (try composition with next character first,
        // then combining chars)
        while(i < codepoints_count) {
            // If we encounter another starter after processing combining chars, stop
            if(composition_buffer[i].combining == MJB_CCC_NOT_REORDERED &&
                last_combining_class != 0) {
                break;
            }

            mjb_codepoint combining_char = composition_buffer[i].codepoint;
            uint16_t current_combining_class = composition_buffer[i].combining;

            // Check if composition is blocked
            bool blocked = (last_combining_class != 0 &&
                last_combining_class >= current_combining_class);

            if(!blocked) {
                // Try to compose starter with this combining character
                mjb_codepoint composed = mjb_unicode_compose_pair(starter, combining_char);

                if(composed != MJB_CODEPOINT_NOT_VALID) {
                    // Composition successful
                    starter = composed;
                    composition_buffer[i].codepoint = MJB_CODEPOINT_NOT_VALID; // Mark as consumed
                    // Don't update last_combining_class since the character was consumed
                } else {
                    // No composition found
                    if(current_combining_class != 0) {
                        last_combining_class = current_combining_class;
                    } else {
                        // If this was a starter (CCC=0) and no composition happened,
                        // we should stop here and process it in the next iteration
                        break;
                    }
                }
            } else {
                // Composition blocked
                if(current_combining_class != 0) {
                    last_combining_class = current_combining_class;
                } else {
                    // If this was a starter (CCC=0) and no composition happened,
                    // we should stop here and process it in the next iteration
                    break;
                }
            }

            ++i;
        }

        // Output the starter (possibly composed)
        mjb_status status = mjb_output_codepoint(output, starter, output_encoding);

        if(status != MJB_STATUS_OK) {
            return status;
        }

        // Output any non-consumed combining characters in order
        for(size_t j = starter_pos + 1; j < i; ++j) {
            if(composition_buffer[j].codepoint != MJB_CODEPOINT_NOT_VALID) {
                status = mjb_output_codepoint(output, composition_buffer[j].codepoint,
                    output_encoding);

                if(status != MJB_STATUS_OK) {
                    return status;
                }
            }
        }
    }

    return MJB_STATUS_OK;
}

static bool mjb_normalization_form_is_valid(mjb_normalization form) {
    return form == MJB_NORMALIZATION_NFD || form == MJB_NORMALIZATION_NFKD ||
        form == MJB_NORMALIZATION_NFC || form == MJB_NORMALIZATION_NFKC;
}

static mjb_status mjb_normalization_estimate(const char *buffer, size_t byte_length,
    mjb_encoding encoding, mjb_encoding output_encoding, size_t *potential_output_size) {
    if(encoding == output_encoding) {
        *potential_output_size = byte_length;
    } else {
        *potential_output_size = mjb_count_codepoints(buffer, byte_length, encoding);

        switch(output_encoding) {
            case MJB_ENC_UTF_8: {
                size_t extra = *potential_output_size / 5;

                if(*potential_output_size > SIZE_MAX - extra) {
                    return MJB_STATUS_OVERFLOW;
                }

                *potential_output_size += extra;

                break;
            }
            case MJB_ENC_UTF_16:
            case MJB_ENC_UTF_16BE:
            case MJB_ENC_UTF_16LE:
                if(*potential_output_size > SIZE_MAX / 2) {
                    return MJB_STATUS_OVERFLOW;
                }

                *potential_output_size *= 2;

                break;
            case MJB_ENC_ASCII:
            case MJB_ENC_UNKNOWN:
                break;
            case MJB_ENC_UTF_32:
            case MJB_ENC_UTF_32BE:
            case MJB_ENC_UTF_32LE:
                if(*potential_output_size > SIZE_MAX / 4) {
                    return MJB_STATUS_OVERFLOW;
                }

                *potential_output_size *= 4;

                break;
        }
    }

    if(*potential_output_size == 0) {
        *potential_output_size = 1;
    }

    return MJB_STATUS_OK;
}

static mjb_status mjb_normalize_write(mjb_output *output, const void *context_pointer) {
    const mjb_normalize_context *context = (const mjb_normalize_context *)context_pointer;
    const char *buffer = context->buffer;
    size_t byte_length = context->byte_length;
    mjb_encoding encoding = context->encoding;
    mjb_normalization form = context->form;
    mjb_encoding output_encoding = context->output_encoding;
    uint8_t state = MJB_UTF_ACCEPT;
    mjb_codepoint codepoint;
    mjb_n_character current_character;

// Combining characters buffer.
#define MJB_MAX_COMBINING_CHARACTERS 32
    mjb_n_character characters_buffer[MJB_MAX_COMBINING_CHARACTERS];
    size_t buffer_index = 0;

    mjb_buffer_character *composition_buffer = NULL;
    size_t output_index = 0;
    bool is_composition = form == MJB_NORMALIZATION_NFC || form == MJB_NORMALIZATION_NFKC;
    bool is_compatibility = form == MJB_NORMALIZATION_NFKC || form == MJB_NORMALIZATION_NFKD;
    size_t composition_capacity = context->potential_output_size;
    mjb_status status = MJB_STATUS_OK;

    if(is_composition) {
        if(composition_capacity > SIZE_MAX / sizeof(mjb_buffer_character)) {
            return MJB_STATUS_OVERFLOW;
        }

        composition_buffer = (mjb_buffer_character *)mjb_alloc(composition_capacity *
            sizeof(mjb_buffer_character));

        if(composition_buffer == NULL) {
            return MJB_STATUS_NO_MEMORY;
        }
    }

// The flush buffer is called multiple times, let's make a macro to avoid code duplication.
#define MJB_NORMALIZE_FLUSH_BUFFER() \
    do { \
        if(is_composition) { \
            status = mjb_flush_c_buffer(characters_buffer, buffer_index, &composition_buffer, \
                &output_index, &composition_capacity); \
            if(status != MJB_STATUS_OK) { \
                goto fail; \
            } \
        } else { \
            status = mjb_flush_d_buffer(characters_buffer, buffer_index, output, output_encoding); \
            if(status != MJB_STATUS_OK) { \
                goto fail; \
            } \
        } \
        buffer_index = 0; \
    } while(0)

    // Loop through the string.
    bool in_error = false;

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

        // Get current character.
        if(!mjb_n_codepoint_character(codepoint, &current_character)) {
            continue;
        }

        // Count of characters produced by decomposition.
        int characters_decomposed = 0;
        bool should_decompose = false;

        /*
         * Determine whether this character should be decomposed based on the normalization form.
         * The should_decompose flag controls whether we attempt to decompose the character
         * or just pass it through unchanged.
         */
        if(is_compatibility) {
            should_decompose = true;
        } else {
            should_decompose = current_character.decomposition == MJB_DECOMPOSITION_CANONICAL;
        }

        // Hangul syllables have a special decomposition.
        // Only decompose in NFD/NFKD forms, not in NFC/NFKC forms
        if(mjb_codepoint_is_hangul_syllable(codepoint) &&
            (form == MJB_NORMALIZATION_NFD || form == MJB_NORMALIZATION_NFKD)) {
            mjb_codepoint codepoints[3];

            if(mjb_hangul_syllable_decomposition(codepoint, codepoints) != MJB_STATUS_OK) {
                continue;
            }

            for(size_t j = 0; j < 3; ++j) {
                if(codepoints[j] == 0) {
                    continue;
                }

                if(!mjb_n_codepoint_character(codepoints[j], &current_character)) {
                    continue;
                }

                // Starter: Any code point (assigned or not) with combining class of zero (ccc = 0)
                if(buffer_index && current_character.combining == MJB_CCC_NOT_REORDERED) {
                    MJB_NORMALIZE_FLUSH_BUFFER();
                }

                if(buffer_index >= MJB_MAX_COMBINING_CHARACTERS) {
                    // Buffer full, flush and continue
                    MJB_NORMALIZE_FLUSH_BUFFER();
                }

                characters_buffer[buffer_index++] = current_character;
                ++characters_decomposed;
            }
        } else if(should_decompose) {
            const mjb_codepoint *decompositions = NULL;
            uint8_t decomposition_count = 0;

            if(mjb_unicode_decomposition_lookup(codepoint, is_compatibility, &decompositions,
                   &decomposition_count)) {
                for(uint8_t decomposition_index = 0; decomposition_index < decomposition_count;
                    ++decomposition_index) {
                    mjb_codepoint decomposed = decompositions[decomposition_index];

                    if(decomposed == MJB_CODEPOINT_NOT_VALID) {
                        continue;
                    }

                    if(!mjb_n_codepoint_character(decomposed, &current_character)) {
                        continue;
                    }

                    // ++codepoints_count;
                    ++characters_decomposed;

                    /*
                     * When we encounter a "starter" character (CCC = 0), we must flush any pending
                     * combining characters in the buffer to ensure proper ordering.
                     *
                     * See
                     * https://www.unicode.org/versions/Unicode18.0.0/core-spec/chapter-3/#G49579
                     */
                    if(buffer_index && current_character.combining == MJB_CCC_NOT_REORDERED) {
                        MJB_NORMALIZE_FLUSH_BUFFER();
                    }

                    if(buffer_index >= MJB_MAX_COMBINING_CHARACTERS) {
                        // Buffer full, flush and continue
                        MJB_NORMALIZE_FLUSH_BUFFER();
                    }

                    characters_buffer[buffer_index++] = current_character;
                }
            }
        }

        if(!characters_decomposed) {
            // Special handling for Hangul composition.
            if(is_composition) {
                // Check if we have a Hangul syllable followed by a trailing consonant.
                if(buffer_index > 0 &&
                    mjb_codepoint_is_hangul_syllable(characters_buffer[buffer_index - 1]
                            .codepoint) &&
                    mjb_codepoint_is_hangul_trailing_jamo(codepoint)) {

                    // Check if the syllable can accept a trailing consonant
                    mjb_codepoint syllable = characters_buffer[buffer_index - 1].codepoint;
                    int s_index = syllable - MJB_CP_HANGUL_S_BASE;

                    if(s_index >= 0 && s_index < MJB_CP_HANGUL_S_COUNT &&
                        (s_index % MJB_CP_HANGUL_T_COUNT) == 0) {
                        // The syllable has no trailing consonant, so we can add one
                        mjb_codepoint trailing = codepoint;
                        mjb_codepoint composed = syllable + (trailing - MJB_CP_HANGUL_T_BASE);
                        characters_buffer[buffer_index - 1].codepoint = composed;

                        // Don't add the trailing consonant since it's been composed
                        continue;
                    }
                }
            }

            if(buffer_index && current_character.combining == MJB_CCC_NOT_REORDERED) {
                MJB_NORMALIZE_FLUSH_BUFFER();
            }

            if(buffer_index >= MJB_MAX_COMBINING_CHARACTERS) {
                // Buffer full, flush and continue.
                MJB_NORMALIZE_FLUSH_BUFFER();
            }

            characters_buffer[buffer_index++] = current_character;
        }
    }

    // We have combining characters in the buffer, we must output them.
    if(buffer_index) {
        MJB_NORMALIZE_FLUSH_BUFFER();
    }

    if(is_composition) {
        status = mjb_recompose(output, output_index, composition_buffer, output_encoding);

        if(status != MJB_STATUS_OK) {
            goto fail;
        }
    }

    mjb_free(composition_buffer);

#undef MJB_NORMALIZE_FLUSH_BUFFER

    return MJB_STATUS_OK;

fail:
    mjb_free(composition_buffer);

#undef MJB_NORMALIZE_FLUSH_BUFFER

    return status;
}

/**
 * Normalize a string
 */
MJB_EXPORT mjb_status mjb_normalize(const char *buffer, size_t byte_length, mjb_encoding encoding,
    mjb_normalization form, mjb_encoding output_encoding, mjb_result *result) {
    if(result == NULL || (buffer == NULL && byte_length > 0)) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    if(!mjb_normalization_form_is_valid(form)) {
        return MJB_STATUS_INVALID_FORM;
    }

    if(byte_length == 0) {
        result->output = (char *)buffer;
        result->output_size = 0;
        result->transformed = false;

        return MJB_STATUS_OK;
    }

    result->output = NULL;
    result->output_size = 0;
    result->transformed = false;

    mjb_quick_check_result is_normalized;
    mjb_status status = mjb_normalization_quick_check(buffer, byte_length, encoding, form,
        &is_normalized);

    if(status != MJB_STATUS_OK) {
        return status;
    }

    if(is_normalized == MJB_QC_YES) {
        if(encoding != output_encoding) {
            return mjb_convert_encoding(buffer, byte_length, encoding, output_encoding, result);
        }

        result->output = (char *)buffer;
        result->output_size = byte_length;

        return MJB_STATUS_OK;
    }

    size_t potential_output_size = 0;
    status = mjb_normalization_estimate(buffer, byte_length, encoding, output_encoding,
        &potential_output_size);

    if(status != MJB_STATUS_OK) {
        return status;
    }

    char *allocated = (char *)mjb_alloc(potential_output_size);

    if(allocated == NULL) {
        return MJB_STATUS_NO_MEMORY;
    }

    mjb_output output;
    mjb_output_init_dynamic(&output, allocated, potential_output_size);
    mjb_normalize_context context = { buffer, byte_length, potential_output_size, encoding, form,
        output_encoding };
    status = mjb_normalize_write(&output, &context);

    if(status != MJB_STATUS_OK) {
        mjb_free(output.buffer);

        return status == MJB_STATUS_UNSUPPORTED ? MJB_STATUS_NO_MEMORY : status;
    }

    output.buffer[output.size] = '\0';
    result->output = output.buffer;
    result->output_size = output.size;
    result->transformed = true;

    return MJB_STATUS_OK;
}

MJB_EXPORT mjb_status mjb_normalize_into(const char *buffer, size_t byte_length,
    mjb_encoding encoding, mjb_normalization form, mjb_encoding output_encoding, void *output,
    size_t *output_size) {
    if(output_size == NULL) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    if(buffer == NULL && byte_length > 0) {
        *output_size = 0;

        return MJB_STATUS_INVALID_ARGUMENT;
    }

    if(!mjb_normalization_form_is_valid(form)) {
        *output_size = 0;

        return MJB_STATUS_INVALID_FORM;
    }

    if(byte_length == 0) {
        return mjb_output_copy_into(buffer, byte_length, output, output_size);
    }

    mjb_quick_check_result is_normalized;
    mjb_status status = mjb_normalization_quick_check(buffer, byte_length, encoding, form,
        &is_normalized);

    if(status != MJB_STATUS_OK) {
        *output_size = 0;

        return status;
    }

    if(is_normalized == MJB_QC_YES) {
        if(encoding != output_encoding) {
            return mjb_convert_encoding_into(buffer, byte_length, encoding, output_encoding, output,
                output_size);
        }

        return mjb_output_copy_into(buffer, byte_length, output, output_size);
    }

    size_t potential_output_size = 0;
    status = mjb_normalization_estimate(buffer, byte_length, encoding, output_encoding,
        &potential_output_size);

    if(status != MJB_STATUS_OK) {
        *output_size = 0;

        return status;
    }

    mjb_normalize_context context = { buffer, byte_length, potential_output_size, encoding, form,
        output_encoding };

    return mjb_output_into(output, output_size, mjb_normalize_write, &context);
}

// Apply full default case folding and remove Default_Ignorable_Code_Point characters.
static mjb_status mjb_nfkc_casefold_pass(const char *buffer, size_t byte_length, char **output,
    size_t *output_size) {
    size_t capacity = byte_length == 0 ? 1 : byte_length;
    char *folded = (char *)mjb_alloc(capacity);

    if(folded == NULL) {
        return MJB_STATUS_NO_MEMORY;
    }

    size_t output_index = 0;
    uint8_t state = MJB_UTF_ACCEPT;
    mjb_codepoint codepoint;
    bool in_error = false;

    for(size_t i = 0; i < byte_length;) {
        mjb_decode_result decode_status = mjb_next_codepoint(buffer, byte_length, &state, &i,
            MJB_ENC_UTF_8, &codepoint, &in_error);

        if(decode_status == MJB_DECODE_END) {
            break;
        }

        if(decode_status == MJB_DECODE_INCOMPLETE) {
            continue;
        }

        if(mjb_codepoint_has_binary_property(codepoint, MJB_PR_DEFAULT_IGNORABLE_CODE_POINT)) {
            continue;
        }

        const mjb_codepoint *mapping = NULL;
        uint8_t mapping_length = 0;

        if(!mjb_unicode_case_folding_lookup(codepoint, &mapping, &mapping_length)) {
            mjb_unicode_case_mapping simple_mapping;

            if(mjb_codepoint_has_binary_property(codepoint, MJB_PR_CHANGES_WHEN_CASEFOLDED) &&
                mjb_unicode_case_lookup(codepoint, &simple_mapping) &&
                simple_mapping.lowercase != 0) {
                codepoint = simple_mapping.lowercase;
            }

            mapping = &codepoint;
            mapping_length = 1;
        }

        for(uint8_t j = 0; j < mapping_length; ++j) {
            char *new_folded = mjb_string_output_codepoint(mapping[j], folded, &output_index,
                &capacity, MJB_ENC_UTF_8);

            if(new_folded == NULL) {
                mjb_free(folded);

                return MJB_STATUS_NO_MEMORY;
            }

            folded = new_folded;
        }
    }

    if(output_index >= capacity) {
        char *new_folded = (char *)mjb_realloc(folded, output_index + 1);

        if(new_folded == NULL) {
            mjb_free(folded);

            return MJB_STATUS_NO_MEMORY;
        }

        folded = new_folded;
    }

    folded[output_index] = '\0';
    *output = folded;
    *output_size = output_index;

    return MJB_STATUS_OK;
}

static mjb_status mjb_nfkc_casefold_transform(const char *buffer, size_t byte_length,
    mjb_encoding encoding, mjb_encoding output_encoding, mjb_result *result, void *output,
    size_t *output_size) {
    const char *current = buffer;
    size_t current_size = byte_length;
    mjb_encoding current_encoding = encoding;
    bool current_owned = false;

    // DerivedNormalizationProps specifies repeating NFKC, full case folding, and removal of
    // default ignorables until stable.
    for(unsigned int pass = 0; pass < 8; ++pass) {
        mjb_result normalized;
        // We use UTF-8 as the intermediate encoding for the NFKC_Casefold transform.
        mjb_status status = mjb_normalize(current, current_size, current_encoding,
            MJB_NORMALIZATION_NFKC, MJB_ENC_UTF_8, &normalized);

        if(status != MJB_STATUS_OK) {
            if(current_owned) {
                mjb_free((void *)current);
            }

            return status;
        }

        char *folded = NULL;
        size_t folded_size = 0;
        status = mjb_nfkc_casefold_pass(normalized.output, normalized.output_size, &folded,
            &folded_size);

        if(normalized.transformed) {
            mjb_free(normalized.output);
        }

        if(status != MJB_STATUS_OK) {
            if(current_owned) {
                mjb_free((void *)current);
            }

            return status;
        }

        bool stable = current_owned && current_size == folded_size &&
            memcmp(current, folded, current_size) == 0;

        if(current_owned) {
            mjb_free((void *)current);
        }

        current = folded;
        current_size = folded_size;
        current_encoding = MJB_ENC_UTF_8;
        current_owned = true;

        if(stable) {
            if(result != NULL) {
                mjb_result normalized_result;
                status = mjb_normalize(current, current_size, MJB_ENC_UTF_8, MJB_NORMALIZATION_NFC,
                    output_encoding, &normalized_result);

                if(status != MJB_STATUS_OK) {
                    mjb_free((void *)current);

                    return status;
                }

                if(normalized_result.output != current) {
                    mjb_free((void *)current);
                }

                *result = normalized_result;
                result->transformed = true;

                return MJB_STATUS_OK;
            }

            status = mjb_normalize_into(current, current_size, MJB_ENC_UTF_8, MJB_NORMALIZATION_NFC,
                output_encoding, output, output_size);
            mjb_free((void *)current);

            return status;
        }
    }

    mjb_free((void *)current);

    return MJB_STATUS_UNSUPPORTED;
}

/**
 * Apply the Unicode NFKC_Casefold transform without duplicating its derived mapping table.
 */
MJB_EXPORT mjb_status mjb_nfkc_casefold(const char *buffer, size_t byte_length,
    mjb_encoding encoding, mjb_encoding output_encoding, mjb_result *result) {
    if(result == NULL || (buffer == NULL && byte_length > 0)) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    if(byte_length == 0) {
        result->output = (char *)buffer;
        result->output_size = 0;
        result->transformed = false;

        return MJB_STATUS_OK;
    }

    return mjb_nfkc_casefold_transform(buffer, byte_length, encoding, output_encoding, result, NULL,
        NULL);
}

MJB_EXPORT mjb_status mjb_nfkc_casefold_into(const char *buffer, size_t byte_length,
    mjb_encoding encoding, mjb_encoding output_encoding, void *output, size_t *output_size) {
    if(output_size == NULL) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    if(buffer == NULL && byte_length > 0) {
        *output_size = 0;

        return MJB_STATUS_INVALID_ARGUMENT;
    }

    if(byte_length == 0) {
        return mjb_output_copy_into(buffer, byte_length, output, output_size);
    }

    mjb_status status = mjb_nfkc_casefold_transform(buffer, byte_length, encoding, output_encoding,
        NULL, output, output_size);

    if(status != MJB_STATUS_OK && status != MJB_STATUS_OUTPUT_TOO_SMALL) {
        *output_size = 0;
    }

    return status;
}
