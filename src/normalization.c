/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <string.h>

#include "mojibake-internal.h"
#include "unicode-tables.h"
#include "utf.h"

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

// Flush the decomposition buffer to a UTF-8 string.
static char *mjb_flush_d_buffer(mjb_n_character *characters_buffer, size_t buffer_index,
    char *output, size_t *output_index, size_t *output_size, mjb_normalization form,
    mjb_encoding output_encoding) {

    if(buffer_index == 0) {
        return output;
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

        char *new_output = mjb_string_output_codepoint(characters_buffer[i].codepoint, output,
            output_index, output_size, output_encoding);

        if(new_output == NULL) {
            return NULL;
        }

        output = new_output;
    }

    return output;
}

// Flush the composition buffer to a bit array.
static mjb_buffer_character *mjb_flush_c_buffer(mjb_n_character *characters_buffer,
    size_t buffer_index, mjb_buffer_character *output, size_t *output_index, size_t *output_size,
    mjb_normalization form) {

    if(buffer_index == 0) {
        return output;
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
                return NULL;
            }

            mjb_buffer_character *new_output = (mjb_buffer_character*)mjb_realloc(output,
                new_output_size * sizeof(mjb_buffer_character));

            if(new_output == NULL) {
                return NULL;
            }

            output = new_output;
            *output_size = new_output_size;
        }

        output[*output_index].codepoint = characters_buffer[i].codepoint;
        output[*output_index].combining = characters_buffer[i].combining;

        ++(*output_index);
    }

    return output;
}

/**
 * Recompose the string.
 * Canonical Composition Algorithm
 */
static bool mjb_recompose(char **output, size_t *output_size, size_t codepoints_count,
    mjb_buffer_character *composition_buffer, mjb_encoding output_encoding) {
    // Nothing to recompose. Output an empty string.
    if(codepoints_count == 0) {
        *output = (char*)mjb_alloc(1);

        if(*output == NULL) {
            return false;
        }

        *output_size = 0;
        (*output)[0] = '\0';

        return true;
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

    // Apply Canonical Composition Algorithm
    if(codepoints_count > SIZE_MAX / 2) {
        return false;
    }

    *output_size = codepoints_count * 2; // A good starting size.
    char *composed_output = (char*)mjb_alloc(*output_size);

    if(composed_output == NULL) {
        return false;
    }

    size_t composed_output_index = 0;
    size_t i = 0;

    while(i < codepoints_count) {
        if(composition_buffer[i].combining != MJB_CCC_NOT_REORDERED) {
            // Non-starter: output and continue
            char *new_output = mjb_string_output_codepoint(composition_buffer[i].codepoint,
                composed_output, &composed_output_index, output_size, output_encoding);

            if(new_output == NULL) {
                mjb_free(composed_output);

                return false;
            }

            composed_output = new_output;

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
        char *new_output = mjb_string_output_codepoint(starter, composed_output,
            &composed_output_index, output_size, output_encoding);

        if(new_output == NULL) {
            mjb_free(composed_output);

            return false;
        }

        composed_output = new_output;

        // Output any non-consumed combining characters in order
        for(size_t j = starter_pos + 1; j < i; ++j) {
            if(composition_buffer[j].codepoint != MJB_CODEPOINT_NOT_VALID) {
                new_output = mjb_string_output_codepoint(composition_buffer[j].codepoint,
                    composed_output, &composed_output_index, output_size, output_encoding);

                if(new_output == NULL) {
                    mjb_free(composed_output);

                    return false;
                }

                composed_output = new_output;
            }
        }
    }

    if(composed_output_index >= *output_size) {
        char *new_output = (char*)mjb_realloc(composed_output, composed_output_index + 1);

        if(new_output == NULL) {
            mjb_free(composed_output);

            return false;
        }

        composed_output = new_output;
    }

    *output = composed_output;
    *output_size = composed_output_index;
    composed_output[composed_output_index] = '\0';

    return true;
}

/**
 * Normalize a string
 */
MJB_EXPORT mjb_status mjb_normalize(const char *buffer, size_t size, mjb_encoding encoding,
    mjb_normalization form, mjb_encoding output_encoding, mjb_result *result) {
    if(result == NULL || (buffer == NULL && size > 0)) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    if(mjb_initialize() != MJB_STATUS_OK) {
        return MJB_STATUS_UNSUPPORTED;
    }

    if(form != MJB_NORMALIZATION_NFD && form != MJB_NORMALIZATION_NFKD &&
        form != MJB_NORMALIZATION_NFC && form != MJB_NORMALIZATION_NFKC) {
        return MJB_STATUS_INVALID_FORM;
    }

    if(size == 0) {
        result->output = (char*)buffer;
        result->output_size = 0;
        result->transformed = false;

        return MJB_STATUS_OK;
    }

    uint8_t state = MJB_UTF_ACCEPT;
    mjb_codepoint codepoint;
    mjb_n_character current_character;
    // size_t codepoints_count = 0;

    // Combining characters buffer.
    #define MJB_MAX_COMBINING_CHARACTERS 32
    mjb_n_character characters_buffer[MJB_MAX_COMBINING_CHARACTERS];
    size_t buffer_index = 0;

    // We directly return the string for NFD/NFKD forms.
    result->output = NULL;

    // A composition buffer is used for NFC/NFKC forms. A string is returned later.
    mjb_buffer_character *composition_buffer = NULL;

    // This is the index of the next character to be written to the output string or the composition
    // buffer.
    size_t output_index = 0;

    bool is_composition = form == MJB_NORMALIZATION_NFC || form == MJB_NORMALIZATION_NFKC;
    bool is_compatibility = form == MJB_NORMALIZATION_NFKC || form == MJB_NORMALIZATION_NFKD;
    mjb_quick_check_result is_normalized = mjb_string_is_normalized(buffer, size, encoding, form);

    if(is_normalized == MJB_QC_YES) {
        // No need to normalize.
        if(encoding != output_encoding) {
            mjb_status status = mjb_string_convert_encoding(buffer, size, encoding,
                output_encoding, result);

            if(status != MJB_STATUS_OK) {
                return status;
            }

            return MJB_STATUS_OK;
        }

        result->output = (char*)buffer;
        result->output_size = size;
        result->transformed = false;

        return MJB_STATUS_OK;
    }

    // Estimate the potential output size.
    size_t potential_output_size = 0;

    if(encoding == output_encoding) {
        // The output encoding is the same as the input encoding, we can use the input size as the
        // potential output size.
        potential_output_size = size;
    } else {
        potential_output_size = mjb_strnlen(buffer, size, encoding);

        switch(output_encoding) {
            case MJB_ENCODING_UTF_8:
                // Tipically, a UTF-8 character in mainly english text is ~1.0-1.2 bytes.
            {
                size_t extra = potential_output_size / 5;

                if(potential_output_size > SIZE_MAX - extra) {
                    return MJB_STATUS_OVERFLOW;
                }

                potential_output_size += extra;

                break;
            }
            case MJB_ENCODING_UTF_16:
            case MJB_ENCODING_UTF_16_BE:
            case MJB_ENCODING_UTF_16_LE:
                // Tipically, a UTF-16 character in mainly english text is ~2 bytes.
                if(potential_output_size > SIZE_MAX / 2) {
                    return MJB_STATUS_OVERFLOW;
                }

                potential_output_size *= 2;

                break;
            case MJB_ENCODING_ASCII:
            case MJB_ENCODING_UNKNOWN:
                break;
            case MJB_ENCODING_UTF_32:
            case MJB_ENCODING_UTF_32_BE:
            case MJB_ENCODING_UTF_32_LE:
                // Always 4 bytes.
                if(potential_output_size > SIZE_MAX / 4) {
                    return MJB_STATUS_OVERFLOW;
                }

                potential_output_size *= 4;

                break;
        }
    }

    if(potential_output_size == 0) {
        potential_output_size = 1;
    }

    if(is_composition) {
        if(potential_output_size > SIZE_MAX / sizeof(mjb_buffer_character)) {
            return MJB_STATUS_OVERFLOW;
        }

        composition_buffer = (mjb_buffer_character*)mjb_alloc(potential_output_size *
            sizeof(mjb_buffer_character));

        if(composition_buffer == NULL) {
            return MJB_STATUS_NO_MEMORY;
        }
    } else {
        result->output = (char*)mjb_alloc(potential_output_size);

        if(result->output == NULL) {
            return MJB_STATUS_NO_MEMORY;
        }
    }

    result->output_size = potential_output_size;

    // The flush buffer is called multiple times, let's make a macro to avoid code duplication.
    #define MJB_NORMALIZE_FLUSH_BUFFER() \
        do { \
            if(is_composition) { \
                mjb_buffer_character *new_composition_buffer = \
                    mjb_flush_c_buffer(characters_buffer, buffer_index, \
                    composition_buffer, &output_index, &result->output_size, form); \
                if(new_composition_buffer == NULL) { \
                    goto fail; \
                } \
                composition_buffer = new_composition_buffer; \
            } else { \
                char *new_output = mjb_flush_d_buffer(characters_buffer, buffer_index, \
                    result->output, &output_index, &result->output_size, form, output_encoding); \
                if(new_output == NULL) { \
                    goto fail; \
                } \
                result->output = new_output; \
            } \
            buffer_index = 0; \
        } while(0)

    // Loop through the string.
    bool in_error = false;

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
                     * See https://www.unicode.org/versions/Unicode17.0.0/core-spec/chapter-3/#G49579
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
                    mjb_codepoint_is_hangul_syllable(
                        characters_buffer[buffer_index - 1].codepoint) &&
                        mjb_codepoint_is_hangul_t(codepoint)) {

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
        // Recompose the string.
        if(!mjb_recompose(&result->output, &result->output_size, output_index,
            composition_buffer, output_encoding)) {
            goto fail;
        }

        mjb_free(composition_buffer);
        composition_buffer = NULL;
    } else {
        // Guarantee null-terminated string
        if(output_index >= result->output_size) {
            char *new_output = (char*)mjb_realloc(result->output, result->output_size + 1);

            if(new_output == NULL) {
                goto fail;
            }

            result->output = new_output;
        }

        result->output[output_index] = '\0';
        result->output_size = output_index;
    }

    result->transformed = true;

    #undef MJB_NORMALIZE_FLUSH_BUFFER

    return MJB_STATUS_OK;

fail:
    mjb_free(composition_buffer);

    if(!is_composition) {
        mjb_free(result->output);
    }

    result->output = NULL;
    result->output_size = 0;
    result->transformed = false;

    #undef MJB_NORMALIZE_FLUSH_BUFFER

    return MJB_STATUS_NO_MEMORY;
}
