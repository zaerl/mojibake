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

MJB_EXPORT mjb_status mjb_resolved_script_set(const mjb_text *text, mjb_script *scripts,
    size_t *count, mjb_script_set_kind *kind) {
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

    if(text == NULL) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    const char *buffer = text->buffer;
    size_t byte_length = text->byte_length;
    mjb_encoding encoding = text->encoding;

    if(buffer == NULL && byte_length > 0) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    if(!mjb_encoding_is_valid_input(encoding)) {
        return MJB_STATUS_INVALID_ENCODING;
    }

    mjb_status status = mjb_resolve_input_byte_length(buffer, &byte_length, encoding);

    if(status != MJB_STATUS_OK) {
        return status;
    }

    status = mjb_check_input_encoding_byte_order(buffer, byte_length, encoding);

    if(status != MJB_STATUS_OK) {
        return status;
    }

    mjb_script_bits resolved;
    mjb_script_bits_fill(&resolved);
    bool is_all = true;
    mjb_codepoint codepoint = 0;
    size_t i = 0;

    if(!mjb_decode_begin(buffer, byte_length, &encoding, &i)) {
        return MJB_STATUS_MALFORMED_INPUT;
    }

    for(;;) {
        mjb_status decode_status = mjb_decode_next_raw(buffer, byte_length, encoding, &i,
            &codepoint, NULL);

        if(decode_status == MJB_STATUS_END_OF_INPUT) {
            break;
        }

        if(decode_status != MJB_STATUS_OK) {
            return MJB_STATUS_MALFORMED_INPUT;
        }

        const uint8_t *values = NULL;
        uint8_t value_count = 0;
        uint8_t fallback[1];

        if(!mjb_unicode_script_extensions_lookup(codepoint, &values, &value_count)) {
            fallback[0] = (uint8_t)mjb_codepoint_script(codepoint);
            values = fallback;
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
            mjb_result_borrow(result, buffer, 0);

            return MJB_STATUS_OK;
        }

        return mjb_output_copy_into(buffer, byte_length, output, output_size);
    }

    // Step 1: NFD the input.
    mjb_result nfd;

    mjb_text text = mjb_text_make(buffer, byte_length, encoding);
    mjb_status status = mjb_normalize(&text, MJB_MALFORMED_STOP, MJB_NORMALIZATION_NFD,
        MJB_ENC_UTF_8, &nfd, NULL);

    if(status != MJB_STATUS_OK) {
        return status;
    }

    // Step 2: Replace each codepoint with its skeleton mapping.
    size_t mid_size = nfd.output_size == 0 ? 1 : nfd.output_size;
    char *mid = (char *)mjb_alloc(mid_size);

    if(!mid) {
        mjb_result_free(&nfd);

        return MJB_STATUS_NO_MEMORY;
    }

    size_t mid_index = 0;
    mjb_codepoint cp = 0;

    for(size_t i = 0;;) {
        mjb_status dr = mjb_decode_next_raw(nfd.output, nfd.output_size, MJB_ENC_UTF_8, &i, &cp,
            NULL);

        if(dr == MJB_STATUS_END_OF_INPUT) {
            break;
        }

        if(dr != MJB_STATUS_OK) {
            mjb_result_free(&nfd);

            mjb_free(mid, mid_size);

            return MJB_STATUS_MALFORMED_INPUT;
        }

        if(mjb_codepoint_has_binary_property(cp, MJB_PR_DEFAULT_IGNORABLE_CODE_POINT)) {
            continue;
        }

        mjb_codepoint skeleton[MJB_UNICODE_SEQUENCE_MAX];
        uint8_t skel_count = 0;

        if(!mjb_unicode_confusable_lookup(cp, skeleton, &skel_count)) {
            skeleton[0] = cp;
            skel_count = 1;
        }

        for(uint8_t j = 0; j < skel_count; ++j) {
            char *new_mid = mjb_string_output_codepoint(skeleton[j], mid, &mid_index, &mid_size,
                MJB_ENC_UTF_8);

            if(new_mid == NULL) {
                mjb_result_free(&nfd);

                mjb_free(mid, mid_size);

                return MJB_STATUS_NO_MEMORY;
            }

            mid = new_mid;
        }
    }

    mjb_result_free(&nfd);

    // Step 3: NFD the intermediate string into the selected final output.
    if(result != NULL) {
        text = mjb_text_utf8(mid, mid_index);
        status = mjb_normalize(&text, MJB_MALFORMED_STOP, MJB_NORMALIZATION_NFD, output_encoding,
            result, NULL);

        // mjb_normalize may return result->output == mid when the string is already NFD.
        // In that case transfer ownership so the caller can free it via mjb_result_free.
        if(status == MJB_STATUS_OK && !result->owned && result->output == mid) {
            mjb_result_own(result, mid, result->output_size, mid_size);
        } else {
            mjb_free(mid, mid_size);
        }

        return status;
    }

    text = mjb_text_utf8(mid, mid_index);
    mjb_buffer output_buffer = mjb_buffer_make(output, *output_size, output_encoding);
    status = mjb_normalize_into(&text, MJB_MALFORMED_STOP, MJB_NORMALIZATION_NFD, &output_buffer,
        NULL);
    *output_size = output_buffer.size;
    mjb_free(mid, mid_size);

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

    mjb_text resolved = mjb_text_make(buffer, byte_length, encoding);
    mjb_bidi_paragraph paragraph;
    status = mjb_bidi_resolve(&resolved, MJB_DIRECTION_LTR, &paragraph);

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
            mjb_free(reordered, reordered_capacity);
            mjb_bidi_paragraph_free(&paragraph);

            return MJB_STATUS_NO_MEMORY;
        }

        status = mjb_bidi_reorder_line(&paragraph, 0, paragraph.count, visual_order);

        if(status != MJB_STATUS_OK) {
            mjb_free(visual_order, paragraph.count * sizeof(size_t));
            mjb_free(reordered, reordered_capacity);
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
                mjb_free(visual_order, paragraph.count * sizeof(size_t));
                mjb_free(reordered, reordered_capacity);
                mjb_bidi_paragraph_free(&paragraph);

                return MJB_STATUS_NO_MEMORY;
            }

            reordered = new_reordered;
        }
    }

    mjb_free(visual_order, paragraph.count * sizeof(size_t));
    mjb_bidi_paragraph_free(&paragraph);

    status = mjb_confusable_skeleton_finish(reordered, reordered_size, MJB_ENC_UTF_8,
        output_encoding, result, output, output_size);

    if(result != NULL && status == MJB_STATUS_OK && result->output == reordered) {
        mjb_result_own(result, reordered, result->output_size, reordered_capacity);
    } else {
        mjb_free(reordered, reordered_capacity);
    }

    return status;
}

// Compute the Unicode security skeleton of a string (UTS #39 Section 4).
MJB_EXPORT mjb_status mjb_confusable_skeleton(const mjb_text *text, mjb_encoding output_encoding,
    mjb_result *result) {
    if(result == NULL || text == NULL || (text->buffer == NULL && text->byte_length > 0)) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    mjb_result_borrow(result, NULL, 0);

    return mjb_confusable_skeleton_process(text->buffer, text->byte_length, text->encoding,
        output_encoding, result, NULL, NULL);
}

MJB_EXPORT mjb_status mjb_confusable_skeleton_into(const mjb_text *text, mjb_buffer *output) {
    if(output == NULL) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    if(text == NULL || (text->buffer == NULL && text->byte_length > 0)) {
        output->size = 0;

        return MJB_STATUS_INVALID_ARGUMENT;
    }

    mjb_status status = mjb_confusable_skeleton_process(text->buffer, text->byte_length,
        text->encoding, output->encoding, NULL, output->data, &output->size);

    if(status != MJB_STATUS_OK && status != MJB_STATUS_OUTPUT_TOO_SMALL) {
        output->size = 0;
    }

    return status;
}

// Determine whether two strings are visually confusable (UTS#39 §4). They are confusable if their
// skeletons are identical.
static mjb_status mjb_confusable_match_texts(const mjb_text *first, const mjb_text *second,
    bool *confusable) {
    if(confusable == NULL) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    *confusable = false;

    if(first == NULL || second == NULL || (first->buffer == NULL && first->byte_length > 0) ||
        (second->buffer == NULL && second->byte_length > 0)) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    mjb_result skel1;
    mjb_result skel2;
    mjb_status status = mjb_confusable_skeleton(first, MJB_ENC_UTF_8, &skel1);

    if(status != MJB_STATUS_OK) {
        return status;
    }

    status = mjb_confusable_skeleton(second, MJB_ENC_UTF_8, &skel2);

    if(status != MJB_STATUS_OK) {
        mjb_result_free(&skel1);

        return status;
    }

    *confusable = (skel1.output_size == skel2.output_size) && skel1.output_size > 0 &&
        (memcmp(skel1.output, skel2.output, skel1.output_size) == 0);

    mjb_result_free(&skel1);

    mjb_result_free(&skel2);

    return MJB_STATUS_OK;
}

MJB_EXPORT mjb_status mjb_confusable_match(const mjb_text *first, const mjb_text *second,
    mjb_malformed_policy malformed_policy, bool *confusable, mjb_diagnostic *first_diagnostic,
    mjb_diagnostic *second_diagnostic) {
    mjb_diagnostic_reset(first_diagnostic);
    mjb_diagnostic_reset(second_diagnostic);

    if(!mjb_malformed_policy_is_valid(malformed_policy)) {
        if(confusable != NULL) {
            *confusable = false;
        }

        return MJB_STATUS_INVALID_ARGUMENT;
    }

    // Well-formed input is compared directly, malformed input is validated and repaired only then.
    mjb_status status = mjb_confusable_match_texts(first, second, confusable);

    if(status != MJB_STATUS_MALFORMED_INPUT) {
        return status;
    }

    mjb_text first_prepared;
    mjb_text second_prepared;
    mjb_result first_sanitized = { NULL, 0, 0, false };
    mjb_result second_sanitized = { NULL, 0, 0, false };
    status = mjb_prepare_text_input(first, malformed_policy, first_diagnostic, &first_prepared,
        &first_sanitized);

    if(status == MJB_STATUS_OK) {
        status = mjb_prepare_text_input(second, malformed_policy, second_diagnostic,
            &second_prepared, &second_sanitized);
    }

    if(status == MJB_STATUS_OK) {
        status = mjb_confusable_match_texts(&first_prepared, &second_prepared, confusable);
    }

    mjb_result_free(&first_sanitized);
    mjb_result_free(&second_sanitized);

    return status;
}

#else

MJB_EXPORT mjb_status mjb_resolved_script_set(const mjb_text *text, mjb_script *scripts,
    size_t *count, mjb_script_set_kind *kind) {
    (void)text;
    (void)scripts;
    (void)count;
    (void)kind;
    return MJB_STATUS_FEATURE_NOT_ENABLED;
}

MJB_EXPORT mjb_status mjb_confusable_skeleton(const mjb_text *text, mjb_encoding output_encoding,
    mjb_result *result) {
    (void)text;
    (void)output_encoding;
    (void)result;
    return MJB_STATUS_FEATURE_NOT_ENABLED;
}

MJB_EXPORT mjb_status mjb_confusable_skeleton_into(const mjb_text *text, mjb_buffer *output) {
    (void)text;
    (void)output;
    return MJB_STATUS_FEATURE_NOT_ENABLED;
}

MJB_EXPORT mjb_status mjb_confusable_match(const mjb_text *first, const mjb_text *second,
    mjb_malformed_policy malformed_policy, bool *confusable, mjb_diagnostic *first_diagnostic,
    mjb_diagnostic *second_diagnostic) {
    (void)first;
    (void)second;
    (void)malformed_policy;
    (void)confusable;
    (void)first_diagnostic;
    (void)second_diagnostic;
    return MJB_STATUS_FEATURE_NOT_ENABLED;
}

#endif // MJB_FEATURE_SECURITY
