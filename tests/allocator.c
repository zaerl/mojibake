/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"

#define SWEEP_MAX_STEPS 4096
#define SWEEP_OUTPUT_SIZE 1024

// Mixed text that composes, decomposes, expands under casing, and contains Hangul jamo.
#define SWEEP_TEXT \
    "\xC7\x84" \
    "e\xCC\x81 \xEF\xAC\x81 \xC4\xB0stanbul \xE1\x84\x80\xE1\x85\xA1 Stra\xC3\x9F" \
    "e \xCE\xA3\xCE\x91\xCE\xA3 A\xCC\x8A\xCC\x81"

// Repeated so the growing output buffers need more than one reallocation.
static const char sweep_text[] = SWEEP_TEXT SWEEP_TEXT SWEEP_TEXT SWEEP_TEXT;
static const size_t sweep_text_size = sizeof(sweep_text) - 1;

// Run one API call. Set `cleared` to whether the outputs match the documented state for the
// returned status. Outputs owned by the caller must be released before returning.
typedef mjb_status (*sweep_call)(bool *cleared);

static bool result_cleared(const mjb_result *result) {
    return result->output == NULL && result->output_size == 0 && !result->owned;
}

static bool result_release(mjb_result *result) {
    return mjb_result_free(result) == MJB_STATUS_OK && result->output_size == 0;
}

static bool result_done(mjb_status status, mjb_result *result) {
    return status == MJB_STATUS_OK ? result_release(result) : result_cleared(result);
}

static bool into_done(mjb_status status, size_t output_size) {
    return status == MJB_STATUS_OK || output_size == 0;
}

static mjb_status sweep_convert_encoding(bool *cleared) {
    mjb_result result = { NULL, 0, false };
    mjb_text text = mjb_text_utf8(sweep_text, sweep_text_size);
    mjb_status status = mjb_convert_encoding(&text, MJB_MALFORMED_STOP, MJB_ENC_UTF_16LE, &result,
        NULL);
    *cleared = result_done(status, &result);

    return status;
}

static mjb_status sweep_convert_encoding_into(bool *cleared) {
    char storage[SWEEP_OUTPUT_SIZE];
    mjb_text text = mjb_text_utf8(sweep_text, sweep_text_size);
    mjb_buffer output = mjb_buffer_make(storage, sizeof(storage), MJB_ENC_UTF_16LE);
    mjb_status status = mjb_convert_encoding_into(&text, MJB_MALFORMED_STOP, &output, NULL);
    *cleared = into_done(status, output.size);

    return status;
}

static mjb_status sweep_normalize(bool *cleared) {
    mjb_result result = { NULL, 0, false };
    mjb_text text = mjb_text_utf8(sweep_text, sweep_text_size);
    mjb_status status = mjb_normalize(&text, MJB_MALFORMED_STOP, MJB_NORMALIZATION_NFKC,
        MJB_ENC_UTF_8, &result, NULL);
    *cleared = result_done(status, &result);

    return status;
}

static mjb_status sweep_normalize_into(bool *cleared) {
    char storage[SWEEP_OUTPUT_SIZE];
    mjb_text text = mjb_text_utf8(sweep_text, sweep_text_size);
    mjb_buffer output = mjb_buffer_make(storage, sizeof(storage), MJB_ENC_UTF_16BE);
    mjb_status status = mjb_normalize_into(&text, MJB_MALFORMED_STOP, MJB_NORMALIZATION_NFC,
        &output, NULL);
    *cleared = into_done(status, output.size);

    return status;
}

static mjb_status sweep_nfkc_casefold(bool *cleared) {
    mjb_result result = { NULL, 0, false };
    mjb_text text = mjb_text_utf8(sweep_text, sweep_text_size);
    mjb_status status = mjb_nfkc_casefold(&text, MJB_MALFORMED_STOP, MJB_ENC_UTF_8, &result, NULL);
    *cleared = result_done(status, &result);

    return status;
}

static mjb_status sweep_nfkc_casefold_into(bool *cleared) {
    char storage[SWEEP_OUTPUT_SIZE];
    mjb_text text = mjb_text_utf8(sweep_text, sweep_text_size);
    mjb_buffer output = mjb_buffer_make(storage, sizeof(storage), MJB_ENC_UTF_8);
    mjb_status status = mjb_nfkc_casefold_into(&text, MJB_MALFORMED_STOP, &output, NULL);
    *cleared = into_done(status, output.size);

    return status;
}

static mjb_status sweep_filter(bool *cleared) {
    mjb_result result = { NULL, 0, false };
    mjb_text text = mjb_text_utf8(sweep_text, sweep_text_size);
    mjb_status status = mjb_filter(&text, MJB_MALFORMED_STOP,
        (mjb_filter_flags)(MJB_FILTER_NORMALIZE | MJB_FILTER_COLLAPSE_SPACES |
            MJB_FILTER_LIMIT_COMBINING),
        MJB_ENC_UTF_8, &result, NULL);
    *cleared = result_done(status, &result);

    return status;
}

static mjb_status sweep_filter_into(bool *cleared) {
    char storage[SWEEP_OUTPUT_SIZE];
    mjb_text text = mjb_text_utf8(sweep_text, sweep_text_size);
    mjb_buffer output = mjb_buffer_make(storage, sizeof(storage), MJB_ENC_UTF_8);
    mjb_status status = mjb_filter_into(&text, MJB_MALFORMED_STOP,
        (mjb_filter_flags)(MJB_FILTER_NORMALIZE | MJB_FILTER_SPACES), &output, NULL);
    *cleared = into_done(status, output.size);

    return status;
}

static mjb_status sweep_map_case(bool *cleared) {
    mjb_result result = { NULL, 0, false };
    mjb_text text = mjb_text_utf8(sweep_text, sweep_text_size);
    mjb_status status = mjb_map_case(&text, MJB_MALFORMED_STOP, MJB_CASE_UPPER, MJB_ENC_UTF_8,
        &result, NULL);
    *cleared = result_done(status, &result);

    return status;
}

static mjb_status sweep_map_case_into(bool *cleared) {
    char storage[SWEEP_OUTPUT_SIZE];
    mjb_text text = mjb_text_utf8(sweep_text, sweep_text_size);
    mjb_buffer output = mjb_buffer_make(storage, sizeof(storage), MJB_ENC_UTF_8);
    mjb_status status = mjb_map_case_into(&text, MJB_MALFORMED_STOP, MJB_CASE_TITLE, &output, NULL);
    *cleared = into_done(status, output.size);

    return status;
}

static mjb_status sweep_caseless_match(bool *cleared) {
    bool matches = false;
    mjb_status status = mjb_caseless_match(sweep_text, sweep_text_size, MJB_ENC_UTF_8, sweep_text,
        sweep_text_size, MJB_ENC_UTF_8, MJB_CASELESS_COMPATIBILITY, &matches);
    *cleared = status != MJB_STATUS_OK || matches;

    return status;
}

static mjb_status sweep_terminal_width(bool *cleared) {
    size_t width = SIZE_MAX;
    mjb_status status = mjb_terminal_width(sweep_text, sweep_text_size, MJB_ENC_UTF_8,
        MJB_MALFORMED_STOP, MJB_TERMINAL_WIDTH_NARROW, &width, NULL);
    *cleared = status == MJB_STATUS_OK ? width > 0 : width == 0;

    return status;
}

static mjb_status sweep_bidi_resolve(bool *cleared) {
    static const char text[] = "abc \xD7\x90\xD7\x91\xD7\x92 123 (\xD8\xB9\xD8\xB1\xD8\xA8) def";
    mjb_bidi_paragraph paragraph;
    mjb_status status = mjb_bidi_resolve(text, sizeof(text) - 1, MJB_ENC_UTF_8, MJB_DIRECTION_LTR,
        &paragraph);

    if(status == MJB_STATUS_OK) {
        *cleared = paragraph.chars != NULL && paragraph.count > 0;
        mjb_bidi_paragraph_free(&paragraph);
    } else {
        *cleared = paragraph.chars == NULL && paragraph.count == 0;
    }

    return status;
}

static mjb_status sweep_locale_parse(bool *cleared) {
    static const char locale_id[] = "sr-Latn-RS-u-ca-gregory-x-private";
    mjb_locale_id locale;
    mjb_status status = mjb_locale_parse(locale_id, sizeof(locale_id) - 1, MJB_ENC_UTF_8, &locale);
    *cleared = true;

    return status;
}

static mjb_status sweep_utf8_grapheme_snprintf(bool *cleared) {
    // Truncation inside a grapheme is the only path that allocates.
    char output[4];
    int required = mjb_utf8_grapheme_snprintf(output, sizeof(output), "%s", "Ae\xCC\x81\xCC\x8A");

    if(required < 0) {
        *cleared = output[0] == '\0';

        return MJB_STATUS_NO_MEMORY;
    }

    *cleared = true;

    return MJB_STATUS_OK;
}

#if MJB_FEATURE_COLLATION
static mjb_status sweep_collation_key(bool *cleared) {
    mjb_result result = { NULL, 0, false };
    mjb_status status = mjb_collation_key(sweep_text, sweep_text_size, MJB_ENC_UTF_8,
        MJB_MALFORMED_STOP, MJB_COLLATION_SHIFTED, MJB_COLLATION_QUATERNARY, &result, NULL);
    *cleared = result_done(status, &result);

    return status;
}

static mjb_status sweep_collation_key_into(bool *cleared) {
    size_t output_size = 0;
    mjb_status status = mjb_collation_key_into(sweep_text, sweep_text_size, MJB_ENC_UTF_8,
        MJB_MALFORMED_STOP, MJB_COLLATION_NON_IGNORABLE, MJB_COLLATION_TERTIARY, NULL, &output_size,
        NULL);
    *cleared = status == MJB_STATUS_OK ? output_size > 0 : output_size == 0;

    return status;
}

static mjb_status sweep_collation_compare(bool *cleared) {
    int order = 0;
    mjb_status status = mjb_collation_compare(sweep_text, sweep_text_size, MJB_ENC_UTF_8,
        "Stra\xC3\x9F"
        "e",
        7, MJB_ENC_UTF_8, MJB_COLLATION_NON_IGNORABLE, MJB_COLLATION_TERTIARY, &order);
    *cleared = status != MJB_STATUS_OK || order != 0;

    return status;
}
#endif

#if MJB_FEATURE_IDNA
static mjb_status sweep_idna_to_ascii(bool *cleared) {
    static const char domain[] = "B\xC3\xBC"
                                 "cher.\xE4\xBE\x8B\xE3\x81\x88.example";
    mjb_idna_info info;
    mjb_result result = { NULL, 0, false };
    mjb_status status = mjb_idna_to_ascii(domain, sizeof(domain) - 1, MJB_ENC_UTF_8, MJB_ENC_UTF_8,
        &info, &result);
    *cleared = result_done(status, &result);

    return status;
}

static mjb_status sweep_idna_to_ascii_into(bool *cleared) {
    static const char domain[] = "B\xC3\xBC"
                                 "cher.\xE4\xBE\x8B\xE3\x81\x88.example";
    mjb_idna_info info;
    char output[SWEEP_OUTPUT_SIZE];
    size_t output_size = sizeof(output);
    mjb_status status = mjb_idna_to_ascii_into(domain, sizeof(domain) - 1, MJB_ENC_UTF_8,
        MJB_ENC_UTF_8, &info, output, &output_size);
    *cleared = into_done(status, output_size);

    return status;
}

static mjb_status sweep_idna_to_unicode(bool *cleared) {
    static const char domain[] = "xn--bcher-kva.xn--r8jz45g.example";
    mjb_idna_info info;
    mjb_result result = { NULL, 0, false };
    mjb_status status = mjb_idna_to_unicode(domain, sizeof(domain) - 1, MJB_ENC_UTF_8,
        MJB_ENC_UTF_16LE, &info, &result);
    *cleared = result_done(status, &result);

    return status;
}

static mjb_status sweep_idna_to_unicode_into(bool *cleared) {
    static const char domain[] = "xn--bcher-kva.xn--r8jz45g.example";
    mjb_idna_info info;
    char output[SWEEP_OUTPUT_SIZE];
    size_t output_size = sizeof(output);
    mjb_status status = mjb_idna_to_unicode_into(domain, sizeof(domain) - 1, MJB_ENC_UTF_8,
        MJB_ENC_UTF_8, &info, output, &output_size);
    *cleared = into_done(status, output_size);

    return status;
}
#endif

#if MJB_FEATURE_SECURITY
static mjb_status sweep_confusable_skeleton(bool *cleared) {
    mjb_result result = { NULL, 0, false };
    mjb_status status = mjb_confusable_skeleton(sweep_text, sweep_text_size, MJB_ENC_UTF_8,
        MJB_ENC_UTF_8, &result);
    *cleared = result_done(status, &result);

    return status;
}

static mjb_status sweep_confusable_skeleton_into(bool *cleared) {
    char output[SWEEP_OUTPUT_SIZE];
    size_t output_size = sizeof(output);
    mjb_status status = mjb_confusable_skeleton_into(sweep_text, sweep_text_size, MJB_ENC_UTF_8,
        MJB_ENC_UTF_16LE, output, &output_size);
    *cleared = into_done(status, output_size);

    return status;
}

static mjb_status sweep_confusable_match(bool *cleared) {
    bool confusable = false;
    mjb_status status = mjb_confusable_match("pa\xD1\x83pal", 7, MJB_ENC_UTF_8, "paypal", 6,
        MJB_ENC_UTF_8, &confusable);
    *cleared = status != MJB_STATUS_OK || confusable;

    return status;
}
#endif

static void sweep(const char *name, sweep_call call) {
    char message[128];
    size_t step = 0;
    mjb_status status = MJB_STATUS_NO_MEMORY;

    // The assertions below do not call the API directly, so attribute them to it.
    mjb_test_coverage_set(name);

    for(; step < SWEEP_MAX_STEPS && status == MJB_STATUS_NO_MEMORY; ++step) {
        size_t live_before = mjb_test_allocator_live_count();
        bool cleared = false;

        mjb_test_allocator_fail_after(step);
        status = call(&cleared);
        size_t attempts = mjb_test_allocator_allocation_count();
        mjb_test_allocator_reset();
        size_t live_after = mjb_test_allocator_live_count();

        snprintf(message, sizeof(message), "%s: fail at allocation %zu", name, step);

        if(status != MJB_STATUS_OK) {
            ATT_ASSERT_STATUS(status, MJB_STATUS_NO_MEMORY, message);
            // Reporting MJB_STATUS_NO_MEMORY must come from the injected failure.
            ATT_ASSERT((int)(attempts > step), true, message);
        }

        ATT_ASSERT(cleared, true, message);
        ATT_ASSERT(live_after, live_before, message);
    }

    snprintf(message, sizeof(message), "%s: succeeds after %zu steps", name, step);
    ATT_ASSERT_STATUS(status, MJB_STATUS_OK, message);
    mjb_test_coverage_clear();
}

#define SWEEP(NAME) sweep("mjb_" #NAME, sweep_##NAME)

// Caller-buffer fast paths that must succeed without allocating.
static void no_allocation(void) {
    char output[8];

    mjb_test_allocator_fail_after(0);

    mjb_text case_text = mjb_text_utf8("a", 1);
    mjb_buffer case_output = mjb_buffer_make(output, sizeof(output), MJB_ENC_UTF_8);
    ATT_ASSERT_STATUS(mjb_map_case_into(&case_text, MJB_MALFORMED_STOP, MJB_CASE_UPPER,
                          &case_output, NULL),
        MJB_STATUS_OK, "Caller-buffer case mapping does not allocate");
    ATT_ASSERT(case_output.size, (size_t)1, "Caller-buffer case mapping output size");
    ATT_ASSERT((unsigned int)output[0], (unsigned int)'A', "Caller-buffer case mapping output");

    mjb_text filter_text = mjb_text_utf8("a", 1);
    mjb_buffer filter_output = mjb_buffer_make(output, sizeof(output), MJB_ENC_UTF_8);
    ATT_ASSERT_STATUS(mjb_filter_into(&filter_text, MJB_MALFORMED_STOP, MJB_FILTER_NONE,
                          &filter_output, NULL),
        MJB_STATUS_OK, "Caller-buffer filtering does not allocate");
    ATT_ASSERT(filter_output.size, (size_t)1, "Caller-buffer filtering output size");
    ATT_ASSERT((unsigned int)output[0], (unsigned int)'a', "Caller-buffer filtering output");

    mjb_text text = mjb_text_utf8("\xC3\xA9", 2);
    mjb_buffer into = mjb_buffer_make(output, sizeof(output), MJB_ENC_UTF_8);
    ATT_ASSERT_STATUS(mjb_normalize_into(&text, MJB_MALFORMED_STOP, MJB_NORMALIZATION_NFD, &into,
                          NULL),
        MJB_STATUS_OK, "Caller-buffer decomposition does not allocate");
    ATT_ASSERT(into.size, (size_t)3, "Caller-buffer decomposition output size");
    ATT_ASSERT((int)memcmp(output, "e\xCC\x81", 3), 0, "Caller-buffer decomposition output");

    text = mjb_text_utf8("A", 1);
    into = mjb_buffer_make(output, sizeof(output), MJB_ENC_UTF_16LE);
    ATT_ASSERT_STATUS(mjb_normalize_into(&text, MJB_MALFORMED_STOP, MJB_NORMALIZATION_NFC, &into,
                          NULL),
        MJB_STATUS_OK, "Caller-buffer normalized encoding conversion does not allocate");
    ATT_ASSERT(into.size, (size_t)2, "Caller-buffer normalized encoding conversion output size");
    ATT_ASSERT((int)memcmp(output, "A\0", 2), 0,
        "Caller-buffer normalized encoding conversion output");

    size_t attempts = mjb_test_allocator_allocation_count();
    ATT_ASSERT(attempts, (size_t)0,
        "Caller-buffer fast paths make no allocation attempts");

    mjb_test_allocator_reset();
}

ATT_TEST(allocator) {
    no_allocation();

    SWEEP(convert_encoding);
    SWEEP(convert_encoding_into);
    SWEEP(normalize);
    SWEEP(normalize_into);
    SWEEP(nfkc_casefold);
    SWEEP(nfkc_casefold_into);
    SWEEP(filter);
    SWEEP(filter_into);
    SWEEP(map_case);
    SWEEP(map_case_into);
    SWEEP(caseless_match);
    SWEEP(terminal_width);
    SWEEP(bidi_resolve);
    SWEEP(locale_parse);
    SWEEP(utf8_grapheme_snprintf);

#if MJB_FEATURE_COLLATION
    SWEEP(collation_key);
    SWEEP(collation_key_into);
    SWEEP(collation_compare);
#endif

#if MJB_FEATURE_IDNA
    SWEEP(idna_to_ascii);
    SWEEP(idna_to_ascii_into);
    SWEEP(idna_to_unicode);
    SWEEP(idna_to_unicode_into);
#endif

#if MJB_FEATURE_SECURITY
    SWEEP(confusable_skeleton);
    SWEEP(confusable_skeleton_into);
    SWEEP(confusable_match);
#endif
}
