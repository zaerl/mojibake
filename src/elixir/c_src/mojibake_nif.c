// The Mojibake library
//
// This file is distributed under the MIT License. See LICENSE for details.

// Erlang NIF bindings for the Mojibake C library, loaded by `Mojibake.Nif`.
//
// Every function takes and returns binaries holding UTF-8. Inputs larger than
// MJB_NIF_DIRTY_THRESHOLD bytes are rescheduled onto a dirty CPU scheduler, so that a long string
// never blocks a normal scheduler for more than about a millisecond.

#include <stddef.h>
#include <string.h>

#include <erl_nif.h>

#include "mojibake.h"

// Inputs above this many bytes run on a dirty CPU scheduler.
#define MJB_NIF_DIRTY_THRESHOLD 16384

#define MJB_NIF_COUNT(array) (sizeof(array) / sizeof((array)[0]))

// The atom tables below are indexed by C enum values. Fail the build if an enum grows.
_Static_assert(MJB_STATUS_END_OF_INPUT == 13, "update mjb_nif_status_names");
_Static_assert(MJB_TEXT_ERROR_UNPAIRED_SURROGATE == 10, "update mjb_nif_text_error_names");

static const char *const mjb_nif_status_names[] = { "ok", "invalid_argument", "invalid_encoding",
    "invalid_codepoint", "invalid_form", "unsupported", "no_memory", "overflow",
    "malformed_input", "output_too_small", "callback_stopped", "not_found",
    "feature_not_enabled", "end_of_input" };

static const char *const mjb_nif_text_error_names[] = { "none", "non_ascii",
    "invalid_leading_byte", "unexpected_continuation", "missing_continuation",
    "overlong_sequence", "surrogate", "out_of_range", "truncated_sequence",
    "truncated_code_unit", "unpaired_surrogate" };

static struct {
    ERL_NIF_TERM ok;
    ERL_NIF_TERM error;
    ERL_NIF_TERM nil;
    ERL_NIF_TERM true_;
    ERL_NIF_TERM false_;
    ERL_NIF_TERM yes;
    ERL_NIF_TERM no;
    ERL_NIF_TERM maybe;
    ERL_NIF_TERM invalid_locale;
    ERL_NIF_TERM mandatory;
    ERL_NIF_TERM allowed;
    ERL_NIF_TERM statuses[MJB_NIF_COUNT(mjb_nif_status_names)];
    ERL_NIF_TERM text_errors[MJB_NIF_COUNT(mjb_nif_text_error_names)];
} mjb_nif_atoms;

typedef ERL_NIF_TERM (*mjb_nif_fn)(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]);

// Allocator

// The library passes the exact block size to realloc and free; enif_* do not need it.
static void *mjb_nif_alloc(void *context, size_t size) {
    (void)context;

    return enif_alloc(size);
}

static void *mjb_nif_realloc(void *context, void *ptr, size_t old_size, size_t new_size) {
    (void)context;
    (void)old_size;

    return enif_realloc(ptr, new_size);
}

static void mjb_nif_free(void *context, void *ptr, size_t size) {
    (void)context;
    (void)size;

    enif_free(ptr);
}

// Terms

static ERL_NIF_TERM mjb_nif_bool(bool value) {
    return value ? mjb_nif_atoms.true_ : mjb_nif_atoms.false_;
}

static ERL_NIF_TERM mjb_nif_ok(ErlNifEnv *env, ERL_NIF_TERM value) {
    return enif_make_tuple2(env, mjb_nif_atoms.ok, value);
}

static ERL_NIF_TERM mjb_nif_status_atom(mjb_status status) {
    size_t index = (size_t)status;

    if(index >= MJB_NIF_COUNT(mjb_nif_atoms.statuses)) {
        index = MJB_STATUS_INVALID_ARGUMENT;
    }

    return mjb_nif_atoms.statuses[index];
}

// {:error, reason}, or {:error, :malformed_input, byte_offset, byte_length, detail}.
static ERL_NIF_TERM mjb_nif_error(ErlNifEnv *env, mjb_status status,
    const mjb_diagnostic *diagnostic) {
    if(status == MJB_STATUS_MALFORMED_INPUT && diagnostic != NULL &&
        diagnostic->error != MJB_TEXT_ERROR_NONE) {
        size_t detail = (size_t)diagnostic->error;
        ERL_NIF_TERM detail_atom = detail < MJB_NIF_COUNT(mjb_nif_atoms.text_errors)
            ? mjb_nif_atoms.text_errors[detail]
            : mjb_nif_atoms.nil;

        return enif_make_tuple5(env, mjb_nif_atoms.error, mjb_nif_status_atom(status),
            enif_make_uint64(env, (ErlNifUInt64)diagnostic->byte_offset),
            enif_make_uint64(env, (ErlNifUInt64)diagnostic->byte_length), detail_atom);
    }

    return enif_make_tuple2(env, mjb_nif_atoms.error, mjb_nif_status_atom(status));
}

// Arguments

static bool mjb_nif_get_text(ErlNifEnv *env, ERL_NIF_TERM term, ErlNifBinary *binary,
    mjb_text *text) {
    if(!enif_inspect_binary(env, term, binary)) {
        return false;
    }

    text->buffer = (const char *)binary->data;
    text->byte_length = binary->size;
    text->encoding = MJB_ENC_UTF_8;

    return true;
}

static bool mjb_nif_get_size(ErlNifEnv *env, ERL_NIF_TERM term, size_t *value) {
    ErlNifUInt64 number = 0;

    if(!enif_get_uint64(env, term, &number) || number > (ErlNifUInt64)SIZE_MAX) {
        return false;
    }

    *value = (size_t)number;

    return true;
}

static bool mjb_nif_atom_is(ErlNifEnv *env, ERL_NIF_TERM term, const char *name) {
    char buffer[32];

    return enif_get_atom(env, term, buffer, sizeof(buffer), ERL_NIF_LATIN1) > 0 &&
        strcmp(buffer, name) == 0;
}

static bool mjb_nif_get_policy(ErlNifEnv *env, ERL_NIF_TERM term, mjb_malformed_policy *policy) {
    if(mjb_nif_atom_is(env, term, "stop")) {
        *policy = MJB_MALFORMED_STOP;
    } else if(mjb_nif_atom_is(env, term, "replace")) {
        *policy = MJB_MALFORMED_REPLACE;
    } else if(mjb_nif_atom_is(env, term, "skip")) {
        *policy = MJB_MALFORMED_SKIP;
    } else {
        return false;
    }

    return true;
}

static bool mjb_nif_get_profile(ErlNifEnv *env, ERL_NIF_TERM term,
    mjb_terminal_width_profile *profile) {
    if(mjb_nif_atom_is(env, term, "narrow")) {
        *profile = MJB_TERMINAL_WIDTH_NARROW;
    } else if(mjb_nif_atom_is(env, term, "east_asian")) {
        *profile = MJB_TERMINAL_WIDTH_EAST_ASIAN;
    } else {
        return false;
    }

    return true;
}

static bool mjb_nif_get_form(ErlNifEnv *env, ERL_NIF_TERM term, mjb_normalization *form) {
    if(mjb_nif_atom_is(env, term, "nfc")) {
        *form = MJB_NORMALIZATION_NFC;
    } else if(mjb_nif_atom_is(env, term, "nfd")) {
        *form = MJB_NORMALIZATION_NFD;
    } else if(mjb_nif_atom_is(env, term, "nfkc")) {
        *form = MJB_NORMALIZATION_NFKC;
    } else if(mjb_nif_atom_is(env, term, "nfkd")) {
        *form = MJB_NORMALIZATION_NFKD;
    } else {
        return false;
    }

    return true;
}

// Run `fun` on a dirty CPU scheduler when the input is large and we are on a normal scheduler.
static bool mjb_nif_should_reschedule(size_t size) {
    return size > MJB_NIF_DIRTY_THRESHOLD && enif_thread_type() == ERL_NIF_THR_NORMAL_SCHEDULER;
}

#define MJB_NIF_MAYBE_DIRTY(name, size)                                                         \
    do {                                                                                        \
        if(mjb_nif_should_reschedule(size)) {                                                   \
            return enif_schedule_nif(env, #name, ERL_NIF_DIRTY_JOB_CPU_BOUND, name, argc, argv); \
        }                                                                                       \
    } while(0)

// Turn an mjb_result into a binary term. A borrowed result points into `input`, so it becomes the
// input term itself or a sub-binary of it, without a copy. An owned result is copied and freed.
static ERL_NIF_TERM mjb_nif_result(ErlNifEnv *env, ERL_NIF_TERM input_term,
    const ErlNifBinary *input, mjb_result *result) {
    ERL_NIF_TERM term;

    if(!result->owned) {
        if(result->output_size == 0) {
            enif_make_new_binary(env, 0, &term);

            return mjb_nif_ok(env, term);
        }

        const unsigned char *output = (const unsigned char *)result->output;

        if(output < input->data || (size_t)(output - input->data) > input->size ||
            result->output_size > input->size - (size_t)(output - input->data)) {
            return mjb_nif_error(env, MJB_STATUS_INVALID_ARGUMENT, NULL);
        }

        size_t start = (size_t)(output - input->data);

        if(start == 0 && result->output_size == input->size) {
            return mjb_nif_ok(env, input_term);
        }

        return mjb_nif_ok(env, enif_make_sub_binary(env, input_term, start, result->output_size));
    }

    unsigned char *data = enif_make_new_binary(env, result->output_size, &term);

    if(data == NULL) {
        (void)mjb_result_free(result);

        return mjb_nif_error(env, MJB_STATUS_NO_MEMORY, NULL);
    }

    if(result->output_size != 0) {
        memcpy(data, result->output, result->output_size);
    }

    (void)mjb_result_free(result);

    return mjb_nif_ok(env, term);
}

// Parse the casing locale: nil for locale-independent casing, or a BCP 47 tag binary.
static bool mjb_nif_get_locale(ErlNifEnv *env, ERL_NIF_TERM term, mjb_locale *locale) {
    if(enif_is_identical(term, mjb_nif_atoms.nil)) {
        *locale = MJB_LOCALE_NOT_SET;

        return true;
    }

    ErlNifBinary binary;
    mjb_text text;
    mjb_locale_id id;

    if(!mjb_nif_get_text(env, term, &binary, &text)) {
        return false;
    }

    memset(&id, 0, sizeof(id));

    if(mjb_locale_parse(&text, &id) != MJB_STATUS_OK) {
        return false;
    }

    *locale = id.locale;

    return true;
}

// NIFs

// normalize(text, form, policy). form is :nfc, :nfd, :nfkc, :nfkd, or :nfkc_casefold.
static ERL_NIF_TERM mjb_nif_normalize(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
    ErlNifBinary binary;
    mjb_text text;
    mjb_normalization form = MJB_NORMALIZATION_NFC;
    mjb_malformed_policy policy;
    bool casefold = mjb_nif_atom_is(env, argv[1], "nfkc_casefold");

    if(argc != 3 || !mjb_nif_get_text(env, argv[0], &binary, &text) ||
        (!casefold && !mjb_nif_get_form(env, argv[1], &form)) ||
        !mjb_nif_get_policy(env, argv[2], &policy)) {
        return enif_make_badarg(env);
    }

    MJB_NIF_MAYBE_DIRTY(mjb_nif_normalize, binary.size);

    mjb_result result = { NULL, 0, 0, false };
    mjb_diagnostic diagnostic = { MJB_TEXT_ERROR_NONE, 0, 0, 0 };
    mjb_status status = casefold
        ? mjb_nfkc_casefold(&text, policy, MJB_ENC_UTF_8, &result, &diagnostic)
        : mjb_normalize(&text, policy, form, MJB_ENC_UTF_8, &result, &diagnostic);

    if(status != MJB_STATUS_OK) {
        return mjb_nif_error(env, status, &diagnostic);
    }

    return mjb_nif_result(env, argv[0], &binary, &result);
}

// quick_check(text, form) -> {:ok, :yes | :no | :maybe}
static ERL_NIF_TERM mjb_nif_quick_check(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
    ErlNifBinary binary;
    mjb_text text;
    mjb_normalization form;

    if(argc != 2 || !mjb_nif_get_text(env, argv[0], &binary, &text) ||
        !mjb_nif_get_form(env, argv[1], &form)) {
        return enif_make_badarg(env);
    }

    MJB_NIF_MAYBE_DIRTY(mjb_nif_quick_check, binary.size);

    mjb_quick_check_result quick_check = MJB_QC_NO;
    mjb_status status = mjb_normalization_quick_check(&text, form, &quick_check);

    if(status != MJB_STATUS_OK) {
        return mjb_nif_error(env, status, NULL);
    }

    if(quick_check == MJB_QC_YES) {
        return mjb_nif_ok(env, mjb_nif_atoms.yes);
    }

    return mjb_nif_ok(env, quick_check == MJB_QC_MAYBE ? mjb_nif_atoms.maybe : mjb_nif_atoms.no);
}

// map_case(text, type, locale, policy). type is :upper, :lower, :title, :fold, or :fold_simple;
// locale is nil or a BCP 47 tag.
static ERL_NIF_TERM mjb_nif_map_case(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
    ErlNifBinary binary;
    mjb_text text;
    mjb_map_case_type type;
    mjb_malformed_policy policy;
    mjb_locale locale = MJB_LOCALE_NOT_SET;

    if(argc != 4 || !mjb_nif_get_text(env, argv[0], &binary, &text) ||
        !mjb_nif_get_policy(env, argv[3], &policy)) {
        return enif_make_badarg(env);
    }

    if(mjb_nif_atom_is(env, argv[1], "upper")) {
        type = MJB_CASE_UPPER;
    } else if(mjb_nif_atom_is(env, argv[1], "lower")) {
        type = MJB_CASE_LOWER;
    } else if(mjb_nif_atom_is(env, argv[1], "title")) {
        type = MJB_CASE_TITLE;
    } else if(mjb_nif_atom_is(env, argv[1], "fold")) {
        type = MJB_CASE_CASEFOLD;
    } else if(mjb_nif_atom_is(env, argv[1], "fold_simple")) {
        type = MJB_CASE_CASEFOLD_SIMPLE;
    } else {
        return enif_make_badarg(env);
    }

    if(!mjb_nif_get_locale(env, argv[2], &locale)) {
        return enif_make_tuple2(env, mjb_nif_atoms.error, mjb_nif_atoms.invalid_locale);
    }

    MJB_NIF_MAYBE_DIRTY(mjb_nif_map_case, binary.size);

    mjb_result result = { NULL, 0, 0, false };
    mjb_diagnostic diagnostic = { MJB_TEXT_ERROR_NONE, 0, 0, 0 };
    mjb_status status = mjb_map_case(&text, policy, type, locale, MJB_ENC_UTF_8, &result,
        &diagnostic);

    if(status != MJB_STATUS_OK) {
        return mjb_nif_error(env, status, &diagnostic);
    }

    return mjb_nif_result(env, argv[0], &binary, &result);
}

// caseless_match(first, second, mode, policy) -> {:ok, boolean}
static ERL_NIF_TERM mjb_nif_caseless_match(ErlNifEnv *env, int argc,
    const ERL_NIF_TERM argv[]) {
    ErlNifBinary first_binary;
    ErlNifBinary second_binary;
    mjb_text first;
    mjb_text second;
    mjb_caseless_mode mode;
    mjb_malformed_policy policy;

    if(argc != 4 || !mjb_nif_get_text(env, argv[0], &first_binary, &first) ||
        !mjb_nif_get_text(env, argv[1], &second_binary, &second) ||
        !mjb_nif_get_policy(env, argv[3], &policy)) {
        return enif_make_badarg(env);
    }

    if(mjb_nif_atom_is(env, argv[2], "canonical")) {
        mode = MJB_CASELESS_CANONICAL;
    } else if(mjb_nif_atom_is(env, argv[2], "default")) {
        mode = MJB_CASELESS_UNNORMALIZED;
    } else if(mjb_nif_atom_is(env, argv[2], "compatibility")) {
        mode = MJB_CASELESS_COMPATIBILITY;
    } else if(mjb_nif_atom_is(env, argv[2], "identifier")) {
        mode = MJB_CASELESS_IDENTIFIER;
    } else {
        return enif_make_badarg(env);
    }

    MJB_NIF_MAYBE_DIRTY(mjb_nif_caseless_match, first_binary.size + second_binary.size);

    bool matches = false;
    mjb_diagnostic first_diagnostic = { MJB_TEXT_ERROR_NONE, 0, 0, 0 };
    mjb_diagnostic second_diagnostic = { MJB_TEXT_ERROR_NONE, 0, 0, 0 };
    mjb_status status = mjb_caseless_match(&first, &second, policy, mode, &matches,
        &first_diagnostic, &second_diagnostic);

    if(status != MJB_STATUS_OK) {
        return mjb_nif_error(env, status,
            first_diagnostic.error != MJB_TEXT_ERROR_NONE ? &first_diagnostic : &second_diagnostic);
    }

    return mjb_nif_ok(env, mjb_nif_bool(matches));
}

// The C break states differ per kind; hold any of them.
typedef union mjb_nif_break_state {
    mjb_grapheme_break_state grapheme;
    mjb_word_break_state word;
    mjb_sentence_break_state sentence;
} mjb_nif_break_state;

typedef enum mjb_nif_segment_kind {
    MJB_NIF_GRAPHEME,
    MJB_NIF_WORD,
    MJB_NIF_SENTENCE
} mjb_nif_segment_kind;

static bool mjb_nif_get_segment_kind(ErlNifEnv *env, ERL_NIF_TERM term,
    mjb_nif_segment_kind *kind) {
    if(mjb_nif_atom_is(env, term, "grapheme")) {
        *kind = MJB_NIF_GRAPHEME;
    } else if(mjb_nif_atom_is(env, term, "word")) {
        *kind = MJB_NIF_WORD;
    } else if(mjb_nif_atom_is(env, term, "sentence")) {
        *kind = MJB_NIF_SENTENCE;
    } else {
        return false;
    }

    return true;
}

static mjb_break_type mjb_nif_break_next(mjb_nif_segment_kind kind, const mjb_text *text,
    mjb_nif_break_state *state, size_t *position) {
    mjb_break_type type;

    switch(kind) {
        case MJB_NIF_WORD:
            type = mjb_word_break_next(text, &state->word);
            *position = state->word.position;
            break;
        case MJB_NIF_SENTENCE:
            type = mjb_sentence_break_next(text, &state->sentence);
            *position = state->sentence.position;
            break;
        case MJB_NIF_GRAPHEME:
        default:
            type = mjb_grapheme_break_next(text, &state->grapheme);
            *position = state->grapheme.position;
            break;
    }

    return type;
}

// segments(text, kind) -> [binary]. Each segment is a sub-binary of the input. Malformed input
// is segmented as if each maximal ill-formed subsequence were U+FFFD.
static ERL_NIF_TERM mjb_nif_segments(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
    ErlNifBinary binary;
    mjb_text text;
    mjb_nif_segment_kind kind;

    if(argc != 2 || !mjb_nif_get_text(env, argv[0], &binary, &text) ||
        !mjb_nif_get_segment_kind(env, argv[1], &kind)) {
        return enif_make_badarg(env);
    }

    MJB_NIF_MAYBE_DIRTY(mjb_nif_segments, binary.size);

    ERL_NIF_TERM list = enif_make_list(env, 0);
    mjb_nif_break_state state;
    size_t start = 0;

    memset(&state, 0, sizeof(state));

    if(binary.size == 0) {
        return list;
    }

    for(;;) {
        size_t position = 0;
        mjb_break_type type = mjb_nif_break_next(kind, &text, &state, &position);

        if(type == MJB_BT_NOT_SET) {
            break;
        }

        if(position > binary.size) {
            position = binary.size;
        }

        if(type != MJB_BT_NO_BREAK && position > start) {
            list = enif_make_list_cell(env,
                enif_make_sub_binary(env, argv[0], start, position - start), list);
            start = position;
        }
    }

    // Whatever follows the last reported boundary.
    if(start < binary.size) {
        list = enif_make_list_cell(env,
            enif_make_sub_binary(env, argv[0], start, binary.size - start), list);
    }

    ERL_NIF_TERM reversed;

    if(!enif_make_reverse_list(env, list, &reversed)) {
        return enif_make_badarg(env);
    }

    return reversed;
}

// line_breaks(text) -> [{:mandatory | :allowed, byte_offset}]
static ERL_NIF_TERM mjb_nif_line_breaks(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
    ErlNifBinary binary;
    mjb_text text;

    if(argc != 1 || !mjb_nif_get_text(env, argv[0], &binary, &text)) {
        return enif_make_badarg(env);
    }

    MJB_NIF_MAYBE_DIRTY(mjb_nif_line_breaks, binary.size);

    ERL_NIF_TERM list = enif_make_list(env, 0);
    mjb_line_break_state state;

    memset(&state, 0, sizeof(state));

    if(binary.size == 0) {
        return list;
    }

    for(;;) {
        mjb_break_type type = mjb_line_break_next(&text, &state);
        size_t position = state.position > binary.size ? binary.size : state.position;

        if(type == MJB_BT_NOT_SET) {
            break;
        }

        if(type == MJB_BT_MANDATORY || (type == MJB_BT_ALLOWED && position > 0)) {
            ERL_NIF_TERM kind = type == MJB_BT_MANDATORY ? mjb_nif_atoms.mandatory
                                                         : mjb_nif_atoms.allowed;

            list = enif_make_list_cell(env,
                enif_make_tuple2(env, kind, enif_make_uint64(env, (ErlNifUInt64)position)), list);
        }
    }

    ERL_NIF_TERM reversed;

    if(!enif_make_reverse_list(env, list, &reversed)) {
        return enif_make_badarg(env);
    }

    return reversed;
}

// count(text, kind, policy) -> {:ok, count}. kind is :codepoint, :grapheme, :word, or :sentence.
static ERL_NIF_TERM mjb_nif_count(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
    ErlNifBinary binary;
    mjb_text text;
    mjb_malformed_policy policy;

    if(argc != 3 || !mjb_nif_get_text(env, argv[0], &binary, &text) ||
        !mjb_nif_get_policy(env, argv[2], &policy)) {
        return enif_make_badarg(env);
    }

    mjb_status (*count_fn)(const mjb_text *, mjb_malformed_policy, size_t *, mjb_diagnostic *);

    if(mjb_nif_atom_is(env, argv[1], "codepoint")) {
        count_fn = mjb_codepoint_count;
    } else if(mjb_nif_atom_is(env, argv[1], "grapheme")) {
        count_fn = mjb_grapheme_count;
    } else if(mjb_nif_atom_is(env, argv[1], "word")) {
        count_fn = mjb_word_count;
    } else if(mjb_nif_atom_is(env, argv[1], "sentence")) {
        count_fn = mjb_sentence_count;
    } else {
        return enif_make_badarg(env);
    }

    MJB_NIF_MAYBE_DIRTY(mjb_nif_count, binary.size);

    size_t count = 0;
    mjb_diagnostic diagnostic = { MJB_TEXT_ERROR_NONE, 0, 0, 0 };
    mjb_status status = count_fn(&text, policy, &count, &diagnostic);

    if(status != MJB_STATUS_OK) {
        return mjb_nif_error(env, status, &diagnostic);
    }

    return mjb_nif_ok(env, enif_make_uint64(env, (ErlNifUInt64)count));
}

// prefix_length(text, kind, count) -> byte length of the first `count` graphemes or word-break
// segments.
static ERL_NIF_TERM mjb_nif_prefix_length(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
    ErlNifBinary binary;
    mjb_text text;
    size_t count;

    if(argc != 3 || !mjb_nif_get_text(env, argv[0], &binary, &text) ||
        !mjb_nif_get_size(env, argv[2], &count)) {
        return enif_make_badarg(env);
    }

    bool word = mjb_nif_atom_is(env, argv[1], "word");

    if(!word && !mjb_nif_atom_is(env, argv[1], "grapheme")) {
        return enif_make_badarg(env);
    }

    MJB_NIF_MAYBE_DIRTY(mjb_nif_prefix_length, binary.size);

    size_t length = word ? mjb_word_prefix_length(&text, count)
                         : mjb_grapheme_prefix_length(&text, count);

    return enif_make_uint64(env, (ErlNifUInt64)(length > binary.size ? binary.size : length));
}

// prefix_length_for_width(text, kind, profile, max_columns) -> byte length of the longest prefix
// of whole graphemes or word-break segments that fits in `max_columns` terminal cells.
static ERL_NIF_TERM mjb_nif_prefix_length_for_width(ErlNifEnv *env, int argc,
    const ERL_NIF_TERM argv[]) {
    ErlNifBinary binary;
    mjb_text text;
    mjb_terminal_width_profile profile;
    size_t columns;

    if(argc != 4 || !mjb_nif_get_text(env, argv[0], &binary, &text) ||
        !mjb_nif_get_profile(env, argv[2], &profile) ||
        !mjb_nif_get_size(env, argv[3], &columns)) {
        return enif_make_badarg(env);
    }

    bool word = mjb_nif_atom_is(env, argv[1], "word");

    if(!word && !mjb_nif_atom_is(env, argv[1], "grapheme")) {
        return enif_make_badarg(env);
    }

    MJB_NIF_MAYBE_DIRTY(mjb_nif_prefix_length_for_width, binary.size);

    size_t length = word ? mjb_word_prefix_length_for_width(&text, profile, columns)
                         : mjb_grapheme_prefix_length_for_width(&text, profile, columns);

    return enif_make_uint64(env, (ErlNifUInt64)(length > binary.size ? binary.size : length));
}

// terminal_width(text, profile, policy) -> {:ok, columns}
static ERL_NIF_TERM mjb_nif_terminal_width(ErlNifEnv *env, int argc,
    const ERL_NIF_TERM argv[]) {
    ErlNifBinary binary;
    mjb_text text;
    mjb_terminal_width_profile profile;
    mjb_malformed_policy policy;

    if(argc != 3 || !mjb_nif_get_text(env, argv[0], &binary, &text) ||
        !mjb_nif_get_profile(env, argv[1], &profile) ||
        !mjb_nif_get_policy(env, argv[2], &policy)) {
        return enif_make_badarg(env);
    }

    MJB_NIF_MAYBE_DIRTY(mjb_nif_terminal_width, binary.size);

    size_t width = 0;
    mjb_diagnostic diagnostic = { MJB_TEXT_ERROR_NONE, 0, 0, 0 };
    mjb_status status = mjb_terminal_width(&text, policy, profile, &width, &diagnostic);

    if(status != MJB_STATUS_OK) {
        return mjb_nif_error(env, status, &diagnostic);
    }

    return mjb_nif_ok(env, enif_make_uint64(env, (ErlNifUInt64)width));
}

static ERL_NIF_TERM mjb_nif_static_string(ErlNifEnv *env, const char *string) {
    ERL_NIF_TERM term;
    size_t length = string == NULL ? 0 : strlen(string);
    unsigned char *data = enif_make_new_binary(env, length, &term);

    if(data != NULL && length != 0) {
        memcpy(data, string, length);
    }

    return term;
}

static ERL_NIF_TERM mjb_nif_version(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
    (void)argc;
    (void)argv;

    return mjb_nif_static_string(env, mjb_version());
}

static ERL_NIF_TERM mjb_nif_unicode_version(ErlNifEnv *env, int argc,
    const ERL_NIF_TERM argv[]) {
    (void)argc;
    (void)argv;

    return mjb_nif_static_string(env, mjb_unicode_version());
}

// features() -> the optional C features this library was compiled with.
static ERL_NIF_TERM mjb_nif_features(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
    (void)argc;
    (void)argv;

    ERL_NIF_TERM list = enif_make_list(env, 0);

#if MJB_FEATURE_SECURITY
    list = enif_make_list_cell(env, enif_make_atom(env, "security"), list);
#endif
#if MJB_FEATURE_IDNA
    list = enif_make_list_cell(env, enif_make_atom(env, "idna"), list);
#endif
#if MJB_FEATURE_COLLATION
    list = enif_make_list_cell(env, enif_make_atom(env, "collation"), list);
#endif
#if MJB_FEATURE_CHARACTER_NAMES
    list = enif_make_list_cell(env, enif_make_atom(env, "names"), list);
#endif

    return list;
}

// Loading

static int mjb_nif_load(ErlNifEnv *env, void **priv_data, ERL_NIF_TERM load_info) {
    (void)priv_data;
    (void)load_info;

    mjb_nif_atoms.ok = enif_make_atom(env, "ok");
    mjb_nif_atoms.error = enif_make_atom(env, "error");
    mjb_nif_atoms.nil = enif_make_atom(env, "nil");
    mjb_nif_atoms.true_ = enif_make_atom(env, "true");
    mjb_nif_atoms.false_ = enif_make_atom(env, "false");
    mjb_nif_atoms.yes = enif_make_atom(env, "yes");
    mjb_nif_atoms.no = enif_make_atom(env, "no");
    mjb_nif_atoms.maybe = enif_make_atom(env, "maybe");
    mjb_nif_atoms.invalid_locale = enif_make_atom(env, "invalid_locale");
    mjb_nif_atoms.mandatory = enif_make_atom(env, "mandatory");
    mjb_nif_atoms.allowed = enif_make_atom(env, "allowed");

    for(size_t i = 0; i < MJB_NIF_COUNT(mjb_nif_status_names); ++i) {
        mjb_nif_atoms.statuses[i] = enif_make_atom(env, mjb_nif_status_names[i]);
    }

    for(size_t i = 0; i < MJB_NIF_COUNT(mjb_nif_text_error_names); ++i) {
        mjb_nif_atoms.text_errors[i] = enif_make_atom(env, mjb_nif_text_error_names[i]);
    }

    // The allocator is process-global and can be set only once. A second load of this shared
    // object in the same process (an upgrade) shares the first one's library state.
    mjb_allocator allocator = { NULL, mjb_nif_alloc, mjb_nif_realloc, mjb_nif_free };
    (void)mjb_set_allocator(&allocator);

    return 0;
}

static int mjb_nif_upgrade(ErlNifEnv *env, void **priv_data, void **old_priv_data,
    ERL_NIF_TERM load_info) {
    (void)old_priv_data;

    return mjb_nif_load(env, priv_data, load_info);
}

static ErlNifFunc mjb_nif_funcs[] = {
    { "normalize", 3, mjb_nif_normalize, 0 },
    { "quick_check", 2, mjb_nif_quick_check, 0 },
    { "map_case", 4, mjb_nif_map_case, 0 },
    { "caseless_match", 4, mjb_nif_caseless_match, 0 },
    { "segments", 2, mjb_nif_segments, 0 },
    { "line_breaks", 1, mjb_nif_line_breaks, 0 },
    { "count", 3, mjb_nif_count, 0 },
    { "prefix_length", 3, mjb_nif_prefix_length, 0 },
    { "prefix_length_for_width", 4, mjb_nif_prefix_length_for_width, 0 },
    { "terminal_width", 3, mjb_nif_terminal_width, 0 },
    { "version", 0, mjb_nif_version, 0 },
    { "unicode_version", 0, mjb_nif_unicode_version, 0 },
    { "features", 0, mjb_nif_features, 0 },
};

ERL_NIF_INIT(Elixir.Mojibake.Nif, mjb_nif_funcs, mjb_nif_load, NULL, mjb_nif_upgrade, NULL)
