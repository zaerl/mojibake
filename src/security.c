/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <string.h>

#include "mojibake-internal.h"
#include "unicode-tables.h"
#include "utf.h"

#if MJB_FEATURE_SECURITY

#define MJB_SCRIPT_SET_WORD_COUNT ((MJB_SC_COUNT + 63u) / 64u)

typedef struct mjb_script_bits {
    uint64_t words[MJB_SCRIPT_SET_WORD_COUNT];
} mjb_script_bits;

static void mjb_script_bits_clear(mjb_script_bits *set) {
    memset(set, 0, sizeof(*set));
}

static void mjb_script_bits_fill(mjb_script_bits *set) {
    memset(set, 0xFF, sizeof(*set));
    set->words[0] &= ~UINT64_C(1); // MJB_SC_NOT_SET is not a script.
}

static void mjb_script_bits_add(mjb_script_bits *set, mjb_script script) {
    size_t value = (size_t)script;

    if(value == (size_t)MJB_SC_NOT_SET || value >= (size_t)MJB_SC_COUNT) {
        return;
    }

    set->words[value / 64u] |= UINT64_C(1) << (value % 64u);
}

static bool mjb_script_bits_contains(const mjb_script_bits *set, mjb_script script) {
    size_t value = (size_t)script;

    return value > (size_t)MJB_SC_NOT_SET && value < (size_t)MJB_SC_COUNT &&
        (set->words[value / 64u] & (UINT64_C(1) << (value % 64u))) != 0;
}

static void mjb_script_bits_intersect(mjb_script_bits *set, const mjb_script_bits *other) {
    for(size_t i = 0; i < MJB_SCRIPT_SET_WORD_COUNT; ++i) {
        set->words[i] &= other->words[i];
    }
}

static size_t mjb_script_bits_count(const mjb_script_bits *set) {
    size_t count = 0;

    for(size_t script = (size_t)MJB_SC_NOT_SET + 1u; script < (size_t)MJB_SC_COUNT; ++script) {
        if(mjb_script_bits_contains(set, (mjb_script)script)) {
            ++count;
        }
    }

    return count;
}

static void mjb_script_bits_augment(mjb_script_bits *set) {
    if(mjb_script_bits_contains(set, MJB_SC_HANI)) {
        mjb_script_bits_add(set, MJB_SC_HANB);
        mjb_script_bits_add(set, MJB_SC_JPAN);
        mjb_script_bits_add(set, MJB_SC_KORE);
    }

    if(mjb_script_bits_contains(set, MJB_SC_HIRA) || mjb_script_bits_contains(set, MJB_SC_KANA)) {
        mjb_script_bits_add(set, MJB_SC_JPAN);
    }

    if(mjb_script_bits_contains(set, MJB_SC_HANG)) {
        mjb_script_bits_add(set, MJB_SC_KORE);
    }

    if(mjb_script_bits_contains(set, MJB_SC_BOPO)) {
        mjb_script_bits_add(set, MJB_SC_HANB);
    }
}

MJB_EXPORT mjb_status mjb_resolved_script_set(const char *buffer, size_t byte_length,
    mjb_encoding encoding, mjb_script *scripts, size_t *count, mjb_script_set_kind *kind) {
    if(count == NULL || kind == NULL) {
        if(count != NULL) {
            *count = 0;
        }

        if(kind != NULL) {
            *kind = MJB_SCRIPT_SET_EMPTY;
        }

        return MJB_STATUS_INVALID_ARGUMENT;
    }

    size_t capacity = scripts == NULL ? 0 : *count;
    *count = 0;
    *kind = MJB_SCRIPT_SET_EMPTY;

    if(buffer == NULL && byte_length > 0) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    mjb_status status = mjb_resolve_input_byte_length(buffer, &byte_length, encoding);

    if(status != MJB_STATUS_OK) {
        return status;
    }

    status = mjb_validate_code_unit_sequence(buffer, byte_length, encoding);

    if(status != MJB_STATUS_OK) {
        return status;
    }

    mjb_script_bits resolved;
    mjb_script_bits_fill(&resolved);
    bool is_all = true;
    uint8_t state = MJB_UTF_ACCEPT;
    bool in_error = false;
    mjb_codepoint codepoint;

    for(size_t i = 0; i < byte_length;) {
        mjb_decode_result decode_status = mjb_next_codepoint(buffer, byte_length, &state, &i,
            encoding, &codepoint, &in_error);

        if(decode_status == MJB_DECODE_END) {
            break;
        }

        if(decode_status == MJB_DECODE_INCOMPLETE) {
            continue;
        }

        if(decode_status == MJB_DECODE_ERROR) {
            return MJB_STATUS_MALFORMED_INPUT;
        }

        const uint8_t *values = NULL;
        uint8_t value_count = 0;
        uint8_t fallback = (uint8_t)mjb_codepoint_script(codepoint);

        if(!mjb_unicode_script_extensions_lookup(codepoint, &values, &value_count)) {
            values = &fallback;
            value_count = 1;
        }

        bool character_is_all = false;

        for(uint8_t j = 0; j < value_count; ++j) {
            if(values[j] == MJB_SC_ZYYY || values[j] == MJB_SC_ZINH) {
                character_is_all = true;
                break;
            }
        }

        if(character_is_all) {
            continue;
        }

        mjb_script_bits character;
        mjb_script_bits_clear(&character);

        for(uint8_t j = 0; j < value_count; ++j) {
            mjb_script_bits_add(&character, (mjb_script)values[j]);
        }

        mjb_script_bits_augment(&character);
        mjb_script_bits_intersect(&resolved, &character);
        is_all = false;
    }

    if(is_all) {
        *kind = MJB_SCRIPT_SET_ALL;

        return MJB_STATUS_OK;
    }

    size_t required = mjb_script_bits_count(&resolved);
    *count = required;

    if(required == 0) {
        return MJB_STATUS_OK;
    }

    *kind = MJB_SCRIPT_SET_RESOLVED;

    if(scripts == NULL) {
        return MJB_STATUS_OK;
    }

    if(capacity < required) {
        return MJB_STATUS_OUTPUT_TOO_SMALL;
    }

    size_t output = 0;

    for(size_t script = (size_t)MJB_SC_NOT_SET + 1u; script < (size_t)MJB_SC_COUNT; ++script) {
        if(mjb_script_bits_contains(&resolved, (mjb_script)script)) {
            scripts[output++] = (mjb_script)script;
        }
    }

    return MJB_STATUS_OK;
}

// Compute the Unicode security skeleton of a string (UTS#39 §4).
// Algorithm: NFD(input) -> remove default-ignorables -> per-codepoint substitution -> NFD.
static mjb_status mjb_confusable_skeleton_finish(const char *buffer, size_t byte_length,
    mjb_encoding encoding, mjb_encoding output_encoding, mjb_result *result, void *output,
    size_t *output_size) {
    if(byte_length == 0) {
        if(result != NULL) {
            result->output = (char *)buffer;
            result->output_size = 0;
            result->transformed = false;

            return MJB_STATUS_OK;
        }

        return mjb_output_copy_into(buffer, byte_length, output, output_size);
    }

    // Step 1: NFD the input.
    mjb_result nfd;

    mjb_status status = mjb_normalize(buffer, byte_length, encoding, MJB_NORMALIZATION_NFD,
        MJB_ENC_UTF_8, &nfd);

    if(status != MJB_STATUS_OK) {
        return status;
    }

    // Step 2: Replace each codepoint with its skeleton mapping.
    size_t mid_size = nfd.output_size == 0 ? 1 : nfd.output_size;
    char *mid = (char *)mjb_alloc(mid_size);

    if(!mid) {
        if(nfd.transformed) {
            mjb_free(nfd.output);
        }

        return MJB_STATUS_NO_MEMORY;
    }

    size_t mid_index = 0;
    uint8_t state = MJB_UTF_ACCEPT;
    mjb_codepoint cp;
    bool in_error = false;

    for(size_t i = 0; i < nfd.output_size;) {
        mjb_decode_result dr = mjb_next_codepoint(nfd.output, nfd.output_size, &state, &i,
            MJB_ENC_UTF_8, &cp, &in_error);

        if(dr == MJB_DECODE_END) {
            break;
        }

        if(dr == MJB_DECODE_INCOMPLETE) {
            continue;
        }

        if(dr == MJB_DECODE_ERROR) {
            if(nfd.transformed) {
                mjb_free(nfd.output);
            }

            mjb_free(mid);

            return MJB_STATUS_MALFORMED_INPUT;
        }

        if(mjb_codepoint_has_binary_property(cp, MJB_PR_DEFAULT_IGNORABLE_CODE_POINT)) {
            continue;
        }

        const mjb_codepoint *skeleton = NULL;
        uint8_t skel_count = 0;

        if(!mjb_unicode_confusable_lookup(cp, &skeleton, &skel_count)) {
            skeleton = &cp;
            skel_count = 1;
        }

        for(uint8_t j = 0; j < skel_count; ++j) {
            char *new_mid = mjb_string_output_codepoint(skeleton[j], mid, &mid_index, &mid_size,
                MJB_ENC_UTF_8);

            if(new_mid == NULL) {
                if(nfd.transformed) {
                    mjb_free(nfd.output);
                }

                mjb_free(mid);

                return MJB_STATUS_NO_MEMORY;
            }

            mid = new_mid;
        }
    }

    if(nfd.transformed) {
        mjb_free(nfd.output);
    }

    // Step 3: NFD the intermediate string into the selected final output.
    if(result != NULL) {
        status = mjb_normalize(mid, mid_index, MJB_ENC_UTF_8, MJB_NORMALIZATION_NFD,
            output_encoding, result);

        // mjb_normalize may return result->output == mid when the string is already NFD.
        // In that case transfer ownership so the caller can free it via mjb_result_free.
        if(status == MJB_STATUS_OK && !result->transformed) {
            result->transformed = true;
        } else {
            mjb_free(mid);
        }

        return status;
    }

    status = mjb_normalize_into(mid, mid_index, MJB_ENC_UTF_8, MJB_NORMALIZATION_NFD,
        output_encoding, output, output_size);
    mjb_free(mid);

    return status;
}

static mjb_status mjb_confusable_skeleton_process(const char *buffer, size_t byte_length,
    mjb_encoding encoding, mjb_encoding output_encoding, mjb_result *result, void *output,
    size_t *output_size) {
    if(buffer == NULL && byte_length > 0) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    mjb_status status = mjb_resolve_input_byte_length(buffer, &byte_length, encoding);

    if(status != MJB_STATUS_OK) {
        return status;
    }

    status = mjb_validate_code_unit_sequence(buffer, byte_length, encoding);

    if(status != MJB_STATUS_OK) {
        return status;
    }

    mjb_bidi_paragraph paragraph;
    status = mjb_bidi_resolve(buffer, byte_length, encoding, MJB_DIRECTION_LTR, &paragraph);

    if(status != MJB_STATUS_OK) {
        return status;
    }

    size_t reordered_capacity = byte_length == 0 ? 1 : byte_length;
    char *reordered = (char *)mjb_alloc(reordered_capacity);

    if(reordered == NULL) {
        mjb_bidi_paragraph_free(&paragraph);

        return MJB_STATUS_NO_MEMORY;
    }

    size_t reordered_size = 0;
    size_t *visual_order = NULL;

    if(paragraph.count > 0) {
        visual_order = (size_t *)mjb_alloc(paragraph.count * sizeof(size_t));

        if(visual_order == NULL) {
            mjb_free(reordered);
            mjb_bidi_paragraph_free(&paragraph);

            return MJB_STATUS_NO_MEMORY;
        }

        status = mjb_bidi_reorder_line(&paragraph, 0, paragraph.count, visual_order);

        if(status != MJB_STATUS_OK) {
            mjb_free(visual_order);
            mjb_free(reordered);
            mjb_bidi_paragraph_free(&paragraph);

            return status;
        }

        // UAX #9 L3: when L2 reversal placed nonspacing marks before their base,
        // move the base back in front of the mark sequence.
        for(size_t i = 0; i < paragraph.count;) {
            mjb_bidi_class bidi_class = MJB_PR_BIDI_CLASS_NOT_SET;
            bool mirrored;

            mjb_unicode_bidi_lookup(paragraph.chars[visual_order[i]].codepoint, &bidi_class,
                &mirrored);

            if(bidi_class != MJB_PR_BIDI_CLASS_NSM) {
                ++i;
                continue;
            }

            size_t end = i;
            while(end < paragraph.count) {
                mjb_unicode_bidi_lookup(paragraph.chars[visual_order[end]].codepoint, &bidi_class,
                    &mirrored);

                if(bidi_class != MJB_PR_BIDI_CLASS_NSM) {
                    break;
                }

                ++end;
            }

            if(end < paragraph.count) {
                size_t base = visual_order[end];
                memmove(visual_order + i + 1, visual_order + i, (end - i) * sizeof(size_t));
                visual_order[i] = base;
            }

            i = end + 1;
        }

        for(size_t i = 0; i < paragraph.count; ++i) {
            const mjb_bidi_char *ch = &paragraph.chars[visual_order[i]];
            mjb_codepoint cp = ch->mirroring_glyph == 0 ? ch->codepoint : ch->mirroring_glyph;
            char *new_reordered = mjb_string_output_codepoint(cp, reordered, &reordered_size,
                &reordered_capacity, MJB_ENC_UTF_8);

            if(new_reordered == NULL) {
                mjb_free(visual_order);
                mjb_free(reordered);
                mjb_bidi_paragraph_free(&paragraph);

                return MJB_STATUS_NO_MEMORY;
            }

            reordered = new_reordered;
        }
    }

    mjb_free(visual_order);
    mjb_bidi_paragraph_free(&paragraph);

    status = mjb_confusable_skeleton_finish(reordered, reordered_size, MJB_ENC_UTF_8,
        output_encoding, result, output, output_size);

    if(result != NULL && status == MJB_STATUS_OK && result->output == reordered) {
        result->transformed = true;
    } else {
        mjb_free(reordered);
    }

    return status;
}

// Compute the Unicode security skeleton of a string (UTS #39 Section 4).
MJB_EXPORT mjb_status mjb_confusable_skeleton(const char *buffer, size_t byte_length,
    mjb_encoding encoding, mjb_encoding output_encoding, mjb_result *result) {
    if(result == NULL || (buffer == NULL && byte_length > 0)) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    result->output = NULL;
    result->output_size = 0;
    result->transformed = false;

    return mjb_confusable_skeleton_process(buffer, byte_length, encoding, output_encoding, result,
        NULL, NULL);
}

MJB_EXPORT mjb_status mjb_confusable_skeleton_into(const char *buffer, size_t byte_length,
    mjb_encoding encoding, mjb_encoding output_encoding, void *output, size_t *output_size) {
    if(output_size == NULL) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    if(buffer == NULL && byte_length > 0) {
        *output_size = 0;

        return MJB_STATUS_INVALID_ARGUMENT;
    }

    mjb_status status = mjb_confusable_skeleton_process(buffer, byte_length, encoding,
        output_encoding, NULL, output, output_size);

    if(status != MJB_STATUS_OK && status != MJB_STATUS_OUTPUT_TOO_SMALL) {
        *output_size = 0;
    }

    return status;
}

// Determine whether two strings are visually confusable (UTS#39 §4). They are confusable if their
// skeletons are identical.
MJB_EXPORT mjb_status mjb_are_confusable(const char *s1, size_t s1_byte_length,
    mjb_encoding s1_encoding, const char *s2, size_t s2_byte_length, mjb_encoding s2_encoding,
    bool *confusable) {
    if(confusable == NULL) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    *confusable = false;

    if((s1 == NULL && s1_byte_length > 0) || (s2 == NULL && s2_byte_length > 0)) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    mjb_result skel1;
    mjb_result skel2;
    mjb_status status = mjb_confusable_skeleton(s1, s1_byte_length, s1_encoding, MJB_ENC_UTF_8,
        &skel1);

    if(status != MJB_STATUS_OK) {
        return status;
    }

    status = mjb_confusable_skeleton(s2, s2_byte_length, s2_encoding, MJB_ENC_UTF_8, &skel2);

    if(status != MJB_STATUS_OK) {
        if(skel1.transformed) {
            mjb_free(skel1.output);
        }

        return status;
    }

    *confusable = (skel1.output_size == skel2.output_size) && skel1.output_size > 0 &&
        (memcmp(skel1.output, skel2.output, skel1.output_size) == 0);

    if(skel1.transformed) {
        mjb_free(skel1.output);
    }

    if(skel2.transformed) {
        mjb_free(skel2.output);
    }

    return MJB_STATUS_OK;
}

#else

MJB_EXPORT mjb_status mjb_resolved_script_set(const char *buffer, size_t byte_length,
    mjb_encoding encoding, mjb_script *scripts, size_t *count, mjb_script_set_kind *kind) {
    (void)buffer;
    (void)byte_length;
    (void)encoding;
    (void)scripts;
    (void)count;
    (void)kind;
    return MJB_STATUS_FEATURE_NOT_ENABLED;
}

MJB_EXPORT mjb_status mjb_confusable_skeleton(const char *buffer, size_t byte_length,
    mjb_encoding encoding, mjb_encoding output_encoding, mjb_result *result) {
    (void)buffer;
    (void)byte_length;
    (void)encoding;
    (void)output_encoding;
    (void)result;
    return MJB_STATUS_FEATURE_NOT_ENABLED;
}

MJB_EXPORT mjb_status mjb_confusable_skeleton_into(const char *buffer, size_t byte_length,
    mjb_encoding encoding, mjb_encoding output_encoding, void *output, size_t *output_size) {
    (void)buffer;
    (void)byte_length;
    (void)encoding;
    (void)output_encoding;
    (void)output;
    (void)output_size;
    return MJB_STATUS_FEATURE_NOT_ENABLED;
}

MJB_EXPORT mjb_status mjb_are_confusable(const char *s1, size_t s1_byte_length,
    mjb_encoding s1_encoding, const char *s2, size_t s2_byte_length, mjb_encoding s2_encoding,
    bool *confusable) {
    (void)s1;
    (void)s1_byte_length;
    (void)s1_encoding;
    (void)s2;
    (void)s2_byte_length;
    (void)s2_encoding;
    (void)confusable;
    return MJB_STATUS_FEATURE_NOT_ENABLED;
}

#endif // MJB_FEATURE_SECURITY
