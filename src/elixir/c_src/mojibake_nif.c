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

// lib/mojibake/emoji.ex and lib/mojibake/char.ex map these hand-written C enums to atoms.
_Static_assert(MJB_EMOJI_SEQUENCE_EMOJI_VARIATION == 8, "update Mojibake.Emoji sequence types");
_Static_assert(MJB_EMOJI_QUALIFICATION_UNQUALIFIED == 4, "update Mojibake.Emoji qualifications");
_Static_assert(MJB_PR_BIDI_CLASS_PDI == 23, "update the bidi classes of Mojibake.Char");
_Static_assert(MJB_IDNA_ERROR_DOMAIN_LENGTH == 0x400, "update the errors of Mojibake.IDNA");

static const char *const mjb_nif_status_names[] = { "ok", "invalid_argument", "invalid_encoding",
    "invalid_codepoint", "invalid_form", "unsupported", "no_memory", "overflow", "malformed_input",
    "output_too_small", "callback_stopped", "not_found", "feature_not_enabled", "end_of_input" };

static const char *const mjb_nif_text_error_names[] = { "none", "non_ascii", "invalid_leading_byte",
    "unexpected_continuation", "missing_continuation", "overlong_sequence", "surrogate",
    "out_of_range", "truncated_sequence", "truncated_code_unit", "unpaired_surrogate" };

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
        ERL_NIF_TERM detail_atom = detail < MJB_NIF_COUNT(mjb_nif_atoms.text_errors) ?
            mjb_nif_atoms.text_errors[detail] :
            mjb_nif_atoms.nil;

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

#define MJB_NIF_MAYBE_DIRTY(name, size) \
    do { \
        if(mjb_nif_should_reschedule(size)) { \
            return enif_schedule_nif(env, #name, ERL_NIF_DIRTY_JOB_CPU_BOUND, name, argc, argv); \
        } \
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
    mjb_status status = casefold ?
        mjb_nfkc_casefold(&text, policy, MJB_ENC_UTF_8, &result, &diagnostic) :
        mjb_normalize(&text, policy, form, MJB_ENC_UTF_8, &result, &diagnostic);

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
static ERL_NIF_TERM mjb_nif_caseless_match(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
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
            ERL_NIF_TERM kind = type == MJB_BT_MANDATORY ? mjb_nif_atoms.mandatory :
                                                           mjb_nif_atoms.allowed;

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

    size_t length = word ? mjb_word_prefix_length(&text, count) :
                           mjb_grapheme_prefix_length(&text, count);

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
        !mjb_nif_get_profile(env, argv[2], &profile) || !mjb_nif_get_size(env, argv[3], &columns)) {
        return enif_make_badarg(env);
    }

    bool word = mjb_nif_atom_is(env, argv[1], "word");

    if(!word && !mjb_nif_atom_is(env, argv[1], "grapheme")) {
        return enif_make_badarg(env);
    }

    MJB_NIF_MAYBE_DIRTY(mjb_nif_prefix_length_for_width, binary.size);

    size_t length = word ? mjb_word_prefix_length_for_width(&text, profile, columns) :
                           mjb_grapheme_prefix_length_for_width(&text, profile, columns);

    return enif_make_uint64(env, (ErlNifUInt64)(length > binary.size ? binary.size : length));
}

// terminal_width(text, profile, policy) -> {:ok, columns}
static ERL_NIF_TERM mjb_nif_terminal_width(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
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

static ERL_NIF_TERM mjb_nif_unicode_version(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
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

// Character data. These take a codepoint and return integers for the C enums; the Elixir side
// turns them into atoms with the generated tables of lib/mojibake/unicode_data.ex.

static bool mjb_nif_get_codepoint(ErlNifEnv *env, ERL_NIF_TERM term, mjb_codepoint *codepoint) {
    unsigned int value = 0;

    if(!enif_get_uint(env, term, &value) || value > MJB_CODEPOINT_MAX) {
        return false;
    }

    *codepoint = (mjb_codepoint)value;

    return true;
}

// A NUL-terminated C string, or "" for NULL, as a binary.
static ERL_NIF_TERM mjb_nif_string(ErlNifEnv *env, const char *string, size_t max_length) {
    size_t length = 0;

    while(string != NULL && length < max_length && string[length] != '\0') {
        ++length;
    }

    ERL_NIF_TERM term;
    unsigned char *data = enif_make_new_binary(env, length, &term);

    if(data != NULL && length != 0) {
        memcpy(data, string, length);
    }

    return term;
}

// codepoint_info(codepoint) -> {:ok, {name, category, combining_class, bidi_class,
// decomposition, decimal, digit, numeric, mirrored, uppercase, lowercase, titlecase}}
static ERL_NIF_TERM mjb_nif_codepoint_info(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
    mjb_codepoint codepoint;

    if(argc != 1 || !mjb_nif_get_codepoint(env, argv[0], &codepoint)) {
        return enif_make_badarg(env);
    }

    mjb_character character;
    memset(&character, 0, sizeof(character));

    mjb_status status = mjb_codepoint_info(codepoint, &character);

    if(status != MJB_STATUS_OK) {
        return mjb_nif_error(env, status, NULL);
    }

    ERL_NIF_TERM fields[] = {
        mjb_nif_string(env, character.name, sizeof(character.name)),
        enif_make_int(env, (int)character.category),
        enif_make_int(env, (int)character.combining),
        enif_make_int(env, (int)character.bidirectional),
        enif_make_int(env, (int)character.decomposition),
        enif_make_int(env, character.decimal),
        enif_make_int(env, character.digit),
        mjb_nif_string(env, character.numeric, sizeof(character.numeric)),
        mjb_nif_bool(character.mirrored),
        enif_make_uint(env, character.uppercase),
        enif_make_uint(env, character.lowercase),
        enif_make_uint(env, character.titlecase),
    };

    return mjb_nif_ok(env, enif_make_tuple_from_array(env, fields, MJB_NIF_COUNT(fields)));
}

// numeric_value(codepoint) -> {:ok, {decimal, digit, numeric}}. decimal and digit are -1 and
// numeric is "" when the codepoint has no such value.
static ERL_NIF_TERM mjb_nif_numeric_value(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
    mjb_codepoint codepoint;

    if(argc != 1 || !mjb_nif_get_codepoint(env, argv[0], &codepoint)) {
        return enif_make_badarg(env);
    }

    mjb_numeric_value value;
    memset(&value, 0, sizeof(value));

    mjb_status status = mjb_codepoint_numeric_value(codepoint, &value);

    if(status != MJB_STATUS_OK) {
        return mjb_nif_error(env, status, NULL);
    }

    return mjb_nif_ok(env,
        enif_make_tuple3(env, enif_make_int(env, value.decimal), enif_make_int(env, value.digit),
            mjb_nif_string(env, value.numeric, sizeof(value.numeric))));
}

// property(codepoint, property, binary) -> {:ok, boolean} for a binary property, {:ok, value} for
// an enumerated one, or {:error, :not_found} when the codepoint has no stored value.
static ERL_NIF_TERM mjb_nif_property(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
    mjb_codepoint codepoint;
    int property = 0;

    if(argc != 3 || !mjb_nif_get_codepoint(env, argv[0], &codepoint) ||
        !enif_get_int(env, argv[1], &property) || property < 0) {
        return enif_make_badarg(env);
    }

    if(enif_is_identical(argv[2], mjb_nif_atoms.true_)) {
        bool value = false;
        mjb_status status = mjb_codepoint_property_binary(codepoint, (mjb_property)property,
            &value);

        if(status != MJB_STATUS_OK) {
            return mjb_nif_error(env, status, NULL);
        }

        return mjb_nif_ok(env, mjb_nif_bool(value));
    }

    int32_t value = 0;
    mjb_status status = mjb_codepoint_property_int(codepoint, (mjb_property)property, &value);

    if(status != MJB_STATUS_OK) {
        return mjb_nif_error(env, status, NULL);
    }

    return mjb_nif_ok(env, enif_make_int(env, (int)value));
}

// block(codepoint) -> {:ok, mjb_block} or {:error, :not_found}
static ERL_NIF_TERM mjb_nif_block(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
    mjb_codepoint codepoint;

    if(argc != 1 || !mjb_nif_get_codepoint(env, argv[0], &codepoint)) {
        return enif_make_badarg(env);
    }

    mjb_block_info block;
    memset(&block, 0, sizeof(block));

    mjb_status status = mjb_codepoint_block(codepoint, &block);

    if(status != MJB_STATUS_OK) {
        return mjb_nif_error(env, status, NULL);
    }

    return mjb_nif_ok(env, enif_make_int(env, (int)block.id));
}

// script(codepoint) -> mjb_script
static ERL_NIF_TERM mjb_nif_script(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
    mjb_codepoint codepoint;

    if(argc != 1 || !mjb_nif_get_codepoint(env, argv[0], &codepoint)) {
        return enif_make_badarg(env);
    }

    return enif_make_int(env, (int)mjb_codepoint_script(codepoint));
}

// script_extensions(codepoint) -> {:ok, [mjb_script]}
static ERL_NIF_TERM mjb_nif_script_extensions(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
    mjb_codepoint codepoint;

    if(argc != 1 || !mjb_nif_get_codepoint(env, argv[0], &codepoint)) {
        return enif_make_badarg(env);
    }

    // Every Script_Extensions set fits; the second call reports a larger one as an error.
    mjb_script scripts[MJB_SC_COUNT];
    size_t count = MJB_NIF_COUNT(scripts);
    mjb_status status = mjb_codepoint_script_extensions(codepoint, scripts, &count);

    if(status != MJB_STATUS_OK) {
        return mjb_nif_error(env, status, NULL);
    }

    ERL_NIF_TERM list = enif_make_list(env, 0);

    for(size_t i = count; i > 0; --i) {
        list = enif_make_list_cell(env, enif_make_int(env, (int)scripts[i - 1]), list);
    }

    return mjb_nif_ok(env, list);
}

// emoji_properties(codepoint) -> {:ok, {emoji, presentation, modifier, modifier_base, component,
// extended_pictographic}}
static ERL_NIF_TERM mjb_nif_emoji_properties(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
    mjb_codepoint codepoint;

    if(argc != 1 || !mjb_nif_get_codepoint(env, argv[0], &codepoint)) {
        return enif_make_badarg(env);
    }

    mjb_emoji_properties emoji;
    memset(&emoji, 0, sizeof(emoji));

    mjb_status status = mjb_codepoint_emoji_properties(codepoint, &emoji);

    if(status != MJB_STATUS_OK && status != MJB_STATUS_NOT_FOUND) {
        return mjb_nif_error(env, status, NULL);
    }

    ERL_NIF_TERM fields[] = {
        mjb_nif_bool(emoji.emoji),
        mjb_nif_bool(emoji.presentation),
        mjb_nif_bool(emoji.modifier),
        mjb_nif_bool(emoji.modifier_base),
        mjb_nif_bool(emoji.component),
        mjb_nif_bool(emoji.extended_pictographic),
    };

    return mjb_nif_ok(env, enif_make_tuple_from_array(env, fields, MJB_NIF_COUNT(fields)));
}

// codepoint_is(codepoint, kind) -> boolean
static ERL_NIF_TERM mjb_nif_codepoint_is(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
    mjb_codepoint codepoint;

    if(argc != 2 || !mjb_nif_get_codepoint(env, argv[0], &codepoint)) {
        return enif_make_badarg(env);
    }

    static const struct {
        const char *name;
        bool (*predicate)(mjb_codepoint);
    } predicates[] = {
        { "graphic", mjb_codepoint_is_graphic },
        { "combining", mjb_codepoint_is_combining },
        { "cjk_ideograph", mjb_codepoint_is_cjk_ideograph },
        { "cjk_extension_ideograph", mjb_codepoint_is_cjk_extension_ideograph },
        { "hangul_leading_jamo", mjb_codepoint_is_hangul_leading_jamo },
        { "hangul_vowel_jamo", mjb_codepoint_is_hangul_vowel_jamo },
        { "hangul_trailing_jamo", mjb_codepoint_is_hangul_trailing_jamo },
        { "hangul_jamo", mjb_codepoint_is_hangul_jamo },
        { "hangul_syllable", mjb_codepoint_is_hangul_syllable },
        { "id_start", mjb_codepoint_is_id_start },
        { "id_continue", mjb_codepoint_is_id_continue },
        { "xid_start", mjb_codepoint_is_xid_start },
        { "xid_continue", mjb_codepoint_is_xid_continue },
        { "pattern_syntax", mjb_codepoint_is_pattern_syntax },
        { "pattern_white_space", mjb_codepoint_is_pattern_white_space },
    };

    for(size_t i = 0; i < MJB_NIF_COUNT(predicates); ++i) {
        if(mjb_nif_atom_is(env, argv[1], predicates[i].name)) {
            return mjb_nif_bool(predicates[i].predicate(codepoint));
        }
    }

    return enif_make_badarg(env);
}

// plane_name(plane, abbreviation) -> binary, or nil for a plane with no name
static ERL_NIF_TERM mjb_nif_plane_name(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
    int plane = 0;

    if(argc != 2 || !enif_get_int(env, argv[0], &plane) || plane < 0 || plane >= MJB_PLANE_NUM) {
        return enif_make_badarg(env);
    }

    const char *name = mjb_plane_name((mjb_plane)plane,
        enif_is_identical(argv[1], mjb_nif_atoms.true_));

    return name == NULL ? mjb_nif_atoms.nil : mjb_nif_static_string(env, name);
}

// emoji_sequence(text) -> {:ok, {type, qualification, codepoint_count}} or {:error, :not_found}
static ERL_NIF_TERM mjb_nif_emoji_sequence(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
    ErlNifBinary binary;
    mjb_text text;

    if(argc != 1 || !mjb_nif_get_text(env, argv[0], &binary, &text)) {
        return enif_make_badarg(env);
    }

    mjb_emoji_sequence emoji;
    memset(&emoji, 0, sizeof(emoji));

    if(binary.size == 0) {
        return mjb_nif_error(env, MJB_STATUS_NOT_FOUND, NULL);
    }

    mjb_status status = mjb_emoji_sequence_info(&text, &emoji);

    if(status != MJB_STATUS_OK) {
        return mjb_nif_error(env, status, NULL);
    }

    return mjb_nif_ok(env,
        enif_make_tuple3(env, enif_make_int(env, (int)emoji.type),
            enif_make_int(env, (int)emoji.qualification),
            enif_make_uint64(env, (ErlNifUInt64)emoji.codepoint_count)));
}

// hangul_syllable_name(codepoint) -> {:ok, name} or {:error, :not_found}
static ERL_NIF_TERM mjb_nif_hangul_syllable_name(ErlNifEnv *env, int argc,
    const ERL_NIF_TERM argv[]) {
    mjb_codepoint codepoint;

    if(argc != 1 || !mjb_nif_get_codepoint(env, argv[0], &codepoint)) {
        return enif_make_badarg(env);
    }

    char name[64];
    name[0] = '\0';

    mjb_status status = mjb_hangul_syllable_name(codepoint, name, sizeof(name));

    if(status != MJB_STATUS_OK) {
        return mjb_nif_error(env, status, NULL);
    }

    return mjb_nif_ok(env, mjb_nif_string(env, name, sizeof(name)));
}

// hangul_decompose(syllable) -> {:ok, {leading, vowel, trailing}}, trailing 0 for none
static ERL_NIF_TERM mjb_nif_hangul_decompose(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
    mjb_codepoint codepoint;

    if(argc != 1 || !mjb_nif_get_codepoint(env, argv[0], &codepoint)) {
        return enif_make_badarg(env);
    }

    mjb_codepoint jamo[3] = { 0, 0, 0 };
    mjb_status status = mjb_hangul_syllable_decompose(codepoint, jamo);

    if(status != MJB_STATUS_OK) {
        return mjb_nif_error(env, status, NULL);
    }

    return mjb_nif_ok(env,
        enif_make_tuple3(env, enif_make_uint(env, jamo[0]), enif_make_uint(env, jamo[1]),
            enif_make_uint(env, jamo[2])));
}

// hangul_compose(leading, vowel, trailing) -> {:ok, syllable}, trailing 0 for none
static ERL_NIF_TERM mjb_nif_hangul_compose(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
    mjb_codepoint leading;
    mjb_codepoint vowel;
    mjb_codepoint trailing;

    if(argc != 3 || !mjb_nif_get_codepoint(env, argv[0], &leading) ||
        !mjb_nif_get_codepoint(env, argv[1], &vowel) ||
        !mjb_nif_get_codepoint(env, argv[2], &trailing)) {
        return enif_make_badarg(env);
    }

    mjb_codepoint syllable = 0;
    mjb_status status = mjb_hangul_syllable_compose(leading, vowel, trailing, &syllable);

    if(status != MJB_STATUS_OK) {
        return mjb_nif_error(env, status, NULL);
    }

    return mjb_nif_ok(env, enif_make_uint(env, syllable));
}

// Collation, IDNA, security, identifiers, encodings, and language tags

static bool mjb_nif_get_collation(ErlNifEnv *env, ERL_NIF_TERM variable_term,
    ERL_NIF_TERM strength_term, mjb_collation_variable_weighting *variable,
    mjb_collation_strength *strength) {
    if(mjb_nif_atom_is(env, variable_term, "non_ignorable")) {
        *variable = MJB_COLLATION_NON_IGNORABLE;
    } else if(mjb_nif_atom_is(env, variable_term, "shifted")) {
        *variable = MJB_COLLATION_SHIFTED;
    } else {
        return false;
    }

    if(mjb_nif_atom_is(env, strength_term, "primary")) {
        *strength = MJB_COLLATION_PRIMARY;
    } else if(mjb_nif_atom_is(env, strength_term, "secondary")) {
        *strength = MJB_COLLATION_SECONDARY;
    } else if(mjb_nif_atom_is(env, strength_term, "tertiary")) {
        *strength = MJB_COLLATION_TERTIARY;
    } else if(mjb_nif_atom_is(env, strength_term, "quaternary")) {
        *strength = MJB_COLLATION_QUATERNARY;
    } else {
        return false;
    }

    return true;
}

// collation_compare(first, second, variable, strength, policy) -> {:ok, -1 | 0 | 1}
static ERL_NIF_TERM mjb_nif_collation_compare(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
    ErlNifBinary first_binary;
    ErlNifBinary second_binary;
    mjb_text first;
    mjb_text second;
    mjb_collation_variable_weighting variable;
    mjb_collation_strength strength;
    mjb_malformed_policy policy;

    if(argc != 5 || !mjb_nif_get_text(env, argv[0], &first_binary, &first) ||
        !mjb_nif_get_text(env, argv[1], &second_binary, &second) ||
        !mjb_nif_get_collation(env, argv[2], argv[3], &variable, &strength) ||
        !mjb_nif_get_policy(env, argv[4], &policy)) {
        return enif_make_badarg(env);
    }

    MJB_NIF_MAYBE_DIRTY(mjb_nif_collation_compare, first_binary.size + second_binary.size);

    int order = 0;
    mjb_diagnostic first_diagnostic = { MJB_TEXT_ERROR_NONE, 0, 0, 0 };
    mjb_diagnostic second_diagnostic = { MJB_TEXT_ERROR_NONE, 0, 0, 0 };
    mjb_status status = mjb_collation_compare(&first, &second, policy, variable, strength, &order,
        &first_diagnostic, &second_diagnostic);

    if(status != MJB_STATUS_OK) {
        return mjb_nif_error(env, status,
            first_diagnostic.error != MJB_TEXT_ERROR_NONE ? &first_diagnostic : &second_diagnostic);
    }

    return mjb_nif_ok(env, enif_make_int(env, order < 0 ? -1 : (order > 0 ? 1 : 0)));
}

// collation_key(text, variable, strength, policy) -> {:ok, binary}
static ERL_NIF_TERM mjb_nif_collation_key(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
    ErlNifBinary binary;
    mjb_text text;
    mjb_collation_variable_weighting variable;
    mjb_collation_strength strength;
    mjb_malformed_policy policy;

    if(argc != 4 || !mjb_nif_get_text(env, argv[0], &binary, &text) ||
        !mjb_nif_get_collation(env, argv[1], argv[2], &variable, &strength) ||
        !mjb_nif_get_policy(env, argv[3], &policy)) {
        return enif_make_badarg(env);
    }

    MJB_NIF_MAYBE_DIRTY(mjb_nif_collation_key, binary.size);

    mjb_result result = { NULL, 0, 0, false };
    mjb_diagnostic diagnostic = { MJB_TEXT_ERROR_NONE, 0, 0, 0 };
    mjb_status status = mjb_collation_key(&text, policy, variable, strength, &result, &diagnostic);

    if(status != MJB_STATUS_OK) {
        return mjb_nif_error(env, status, &diagnostic);
    }

    return mjb_nif_result(env, argv[0], &binary, &result);
}

// idna(domain, :ascii | :unicode) -> {:ok, {binary, error_bits}}. The binary is the best-effort
// result even when error_bits, a mask of mjb_idna_error, is not zero.
static ERL_NIF_TERM mjb_nif_idna(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
    ErlNifBinary binary;
    mjb_text text;
    bool ascii = false;

    if(argc != 2 || !mjb_nif_get_text(env, argv[0], &binary, &text)) {
        return enif_make_badarg(env);
    }

    if(mjb_nif_atom_is(env, argv[1], "ascii")) {
        ascii = true;
    } else if(!mjb_nif_atom_is(env, argv[1], "unicode")) {
        return enif_make_badarg(env);
    }

    MJB_NIF_MAYBE_DIRTY(mjb_nif_idna, binary.size);

    mjb_result result = { NULL, 0, 0, false };
    mjb_idna_info info = { MJB_IDNA_ERROR_NONE };
    mjb_status status = ascii ? mjb_idna_to_ascii(&text, MJB_ENC_UTF_8, &info, &result) :
                                mjb_idna_to_unicode(&text, MJB_ENC_UTF_8, &info, &result);

    if(status != MJB_STATUS_OK) {
        return mjb_nif_error(env, status, NULL);
    }

    ERL_NIF_TERM converted = mjb_nif_result(env, argv[0], &binary, &result);
    const ERL_NIF_TERM *tuple = NULL;
    int arity = 0;

    // mjb_nif_result returns {:ok, binary} or an error tuple.
    if(!enif_get_tuple(env, converted, &arity, &tuple) || arity != 2 ||
        !enif_is_identical(tuple[0], mjb_nif_atoms.ok)) {
        return converted;
    }

    return mjb_nif_ok(env, enif_make_tuple2(env, tuple[1], enif_make_uint(env, info.errors)));
}

// confusable_skeleton(text) -> {:ok, binary}
static ERL_NIF_TERM mjb_nif_confusable_skeleton(ErlNifEnv *env, int argc,
    const ERL_NIF_TERM argv[]) {
    ErlNifBinary binary;
    mjb_text text;

    if(argc != 1 || !mjb_nif_get_text(env, argv[0], &binary, &text)) {
        return enif_make_badarg(env);
    }

    MJB_NIF_MAYBE_DIRTY(mjb_nif_confusable_skeleton, binary.size);

    mjb_result result = { NULL, 0, 0, false };
    mjb_status status = mjb_confusable_skeleton(&text, MJB_ENC_UTF_8, &result);

    if(status != MJB_STATUS_OK) {
        return mjb_nif_error(env, status, NULL);
    }

    return mjb_nif_result(env, argv[0], &binary, &result);
}

// confusable_match(first, second, policy) -> {:ok, boolean}
static ERL_NIF_TERM mjb_nif_confusable_match(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
    ErlNifBinary first_binary;
    ErlNifBinary second_binary;
    mjb_text first;
    mjb_text second;
    mjb_malformed_policy policy;

    if(argc != 3 || !mjb_nif_get_text(env, argv[0], &first_binary, &first) ||
        !mjb_nif_get_text(env, argv[1], &second_binary, &second) ||
        !mjb_nif_get_policy(env, argv[2], &policy)) {
        return enif_make_badarg(env);
    }

    MJB_NIF_MAYBE_DIRTY(mjb_nif_confusable_match, first_binary.size + second_binary.size);

    bool confusable = false;
    mjb_diagnostic first_diagnostic = { MJB_TEXT_ERROR_NONE, 0, 0, 0 };
    mjb_diagnostic second_diagnostic = { MJB_TEXT_ERROR_NONE, 0, 0, 0 };
    mjb_status status = mjb_confusable_match(&first, &second, policy, &confusable,
        &first_diagnostic, &second_diagnostic);

    if(status != MJB_STATUS_OK) {
        return mjb_nif_error(env, status,
            first_diagnostic.error != MJB_TEXT_ERROR_NONE ? &first_diagnostic : &second_diagnostic);
    }

    return mjb_nif_ok(env, mjb_nif_bool(confusable));
}

// resolved_script_set(text) -> {:ok, :all | :empty | [mjb_script]}
static ERL_NIF_TERM mjb_nif_resolved_script_set(ErlNifEnv *env, int argc,
    const ERL_NIF_TERM argv[]) {
    ErlNifBinary binary;
    mjb_text text;

    if(argc != 1 || !mjb_nif_get_text(env, argv[0], &binary, &text)) {
        return enif_make_badarg(env);
    }

    MJB_NIF_MAYBE_DIRTY(mjb_nif_resolved_script_set, binary.size);

    // A resolved set never holds more scripts than there are.
    mjb_script scripts[MJB_SC_COUNT];
    size_t count = MJB_NIF_COUNT(scripts);
    mjb_script_set_kind kind = MJB_SCRIPT_SET_EMPTY;
    mjb_status status = mjb_resolved_script_set(&text, scripts, &count, &kind);

    if(status != MJB_STATUS_OK) {
        return mjb_nif_error(env, status, NULL);
    }

    if(kind == MJB_SCRIPT_SET_ALL) {
        return mjb_nif_ok(env, enif_make_atom(env, "all"));
    }

    if(kind == MJB_SCRIPT_SET_EMPTY) {
        return mjb_nif_ok(env, enif_make_atom(env, "empty"));
    }

    ERL_NIF_TERM list = enif_make_list(env, 0);

    for(size_t i = count; i > 0; --i) {
        list = enif_make_list_cell(env, enif_make_int(env, (int)scripts[i - 1]), list);
    }

    return mjb_nif_ok(env, list);
}

// is_identifier(text, :default | :nfkc) -> boolean
static ERL_NIF_TERM mjb_nif_is_identifier(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
    ErlNifBinary binary;
    mjb_text text;
    mjb_identifier_profile profile;

    if(argc != 2 || !mjb_nif_get_text(env, argv[0], &binary, &text)) {
        return enif_make_badarg(env);
    }

    if(mjb_nif_atom_is(env, argv[1], "default")) {
        profile = MJB_IDENTIFIER_DEFAULT;
    } else if(mjb_nif_atom_is(env, argv[1], "nfkc")) {
        profile = MJB_IDENTIFIER_NFKC;
    } else {
        return enif_make_badarg(env);
    }

    MJB_NIF_MAYBE_DIRTY(mjb_nif_is_identifier, binary.size);

    return mjb_nif_bool(mjb_is_identifier(&text, profile));
}

static bool mjb_nif_get_encoding(ErlNifEnv *env, ERL_NIF_TERM term, mjb_encoding *encoding) {
    static const struct {
        const char *name;
        mjb_encoding encoding;
    } encodings[] = {
        { "ascii", MJB_ENC_ASCII },
        { "utf8", MJB_ENC_UTF_8 },
        { "utf16", MJB_ENC_UTF_16 },
        { "utf16be", MJB_ENC_UTF_16BE },
        { "utf16le", MJB_ENC_UTF_16LE },
        { "utf32", MJB_ENC_UTF_32 },
        { "utf32be", MJB_ENC_UTF_32BE },
        { "utf32le", MJB_ENC_UTF_32LE },
    };

    for(size_t i = 0; i < MJB_NIF_COUNT(encodings); ++i) {
        if(mjb_nif_atom_is(env, term, encodings[i].name)) {
            *encoding = encodings[i].encoding;

            return true;
        }
    }

    return false;
}

// detect_encoding(binary) -> the mjb_encoding bit mask
static ERL_NIF_TERM mjb_nif_detect_encoding(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
    ErlNifBinary binary;

    if(argc != 1 || !enif_inspect_binary(env, argv[0], &binary)) {
        return enif_make_badarg(env);
    }

    MJB_NIF_MAYBE_DIRTY(mjb_nif_detect_encoding, binary.size);

    return enif_make_uint(env,
        (unsigned int)mjb_detect_encoding((const char *)binary.data, binary.size));
}

// validate_encoding(binary, encoding) -> :ok or an error
static ERL_NIF_TERM mjb_nif_validate_encoding(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
    ErlNifBinary binary;
    mjb_encoding encoding;

    if(argc != 2 || !enif_inspect_binary(env, argv[0], &binary) ||
        !mjb_nif_get_encoding(env, argv[1], &encoding)) {
        return enif_make_badarg(env);
    }

    MJB_NIF_MAYBE_DIRTY(mjb_nif_validate_encoding, binary.size);

    mjb_text text = mjb_text_make((const char *)binary.data, binary.size, encoding);
    mjb_diagnostic diagnostic = { MJB_TEXT_ERROR_NONE, 0, 0, 0 };
    mjb_status status = mjb_validate_encoding(&text, &diagnostic);

    if(status != MJB_STATUS_OK) {
        return mjb_nif_error(env, status, &diagnostic);
    }

    return mjb_nif_atoms.ok;
}

// convert_encoding(binary, from, to, policy) -> {:ok, binary}
static ERL_NIF_TERM mjb_nif_convert_encoding(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
    ErlNifBinary binary;
    mjb_encoding from;
    mjb_encoding to;
    mjb_malformed_policy policy;

    if(argc != 4 || !enif_inspect_binary(env, argv[0], &binary) ||
        !mjb_nif_get_encoding(env, argv[1], &from) || !mjb_nif_get_encoding(env, argv[2], &to) ||
        !mjb_nif_get_policy(env, argv[3], &policy)) {
        return enif_make_badarg(env);
    }

    MJB_NIF_MAYBE_DIRTY(mjb_nif_convert_encoding, binary.size);

    mjb_text text = mjb_text_make((const char *)binary.data, binary.size, from);
    mjb_result result = { NULL, 0, 0, false };
    mjb_diagnostic diagnostic = { MJB_TEXT_ERROR_NONE, 0, 0, 0 };
    mjb_status status = mjb_convert_encoding(&text, policy, to, &result, &diagnostic);

    if(status != MJB_STATUS_OK) {
        return mjb_nif_error(env, status, &diagnostic);
    }

    return mjb_nif_result(env, argv[0], &binary, &result);
}

// locale_parse(tag) -> {:ok, {language, extlang, script, region, variant, extensions,
// private_use, grandfathered, casing_code}}, with "" for absent fields.
static ERL_NIF_TERM mjb_nif_locale_parse(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
    ErlNifBinary binary;
    mjb_text text;

    if(argc != 1 || !mjb_nif_get_text(env, argv[0], &binary, &text)) {
        return enif_make_badarg(env);
    }

    mjb_locale_id id;
    memset(&id, 0, sizeof(id));

    mjb_status status = mjb_locale_parse(&text, &id);

    if(status != MJB_STATUS_OK) {
        return mjb_nif_error(env, status, NULL);
    }

    ERL_NIF_TERM fields[] = {
        mjb_nif_string(env, id.language, sizeof(id.language)),
        mjb_nif_string(env, id.extlang, sizeof(id.extlang)),
        mjb_nif_string(env, id.script, sizeof(id.script)),
        mjb_nif_string(env, id.region, sizeof(id.region)),
        mjb_nif_string(env, id.variant, sizeof(id.variant)),
        mjb_nif_string(env, id.extensions, sizeof(id.extensions)),
        mjb_nif_string(env, id.private_use, sizeof(id.private_use)),
        mjb_nif_string(env, id.grandfathered, sizeof(id.grandfathered)),
        mjb_nif_string(env, id.code, sizeof(id.code)),
    };

    return mjb_nif_ok(env, enif_make_tuple_from_array(env, fields, MJB_NIF_COUNT(fields)));
}

// limits() -> the sizes of the C enums, to check the generated Elixir tables against.
static ERL_NIF_TERM mjb_nif_limits(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
    (void)argc;
    (void)argv;

    ERL_NIF_TERM keys[] = {
        enif_make_atom(env, "categories"),
        enif_make_atom(env, "blocks"),
        enif_make_atom(env, "scripts"),
        enif_make_atom(env, "properties"),
        enif_make_atom(env, "bidi_classes"),
        enif_make_atom(env, "east_asian_widths"),
    };
    ERL_NIF_TERM values[] = {
        enif_make_int(env, MJB_CATEGORY_COUNT),
        enif_make_int(env, MJB_BLOCK_NUM),
        enif_make_int(env, MJB_SC_COUNT),
        enif_make_int(env, MJB_PR_COUNT),
        enif_make_int(env, MJB_BIDI_CLASS_COUNT),
        enif_make_int(env, MJB_EAW_COUNT),
    };
    ERL_NIF_TERM map;

    if(!enif_make_map_from_arrays(env, keys, values, MJB_NIF_COUNT(keys), &map)) {
        return enif_make_badarg(env);
    }

    return map;
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
    // object in the same process (an upgrade) shares the first one's library state, and then
    // mjb_set_allocator reports MJB_STATUS_INVALID_ARGUMENT, which is fine. GCC ignores a (void)
    // cast on a warn_unused_result call, so the status goes through a variable.
    mjb_allocator allocator = { NULL, mjb_nif_alloc, mjb_nif_realloc, mjb_nif_free };
    mjb_status status = mjb_set_allocator(&allocator);
    (void)status;

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
    { "codepoint_info", 1, mjb_nif_codepoint_info, 0 },
    { "numeric_value", 1, mjb_nif_numeric_value, 0 },
    { "property", 3, mjb_nif_property, 0 },
    { "block", 1, mjb_nif_block, 0 },
    { "script", 1, mjb_nif_script, 0 },
    { "script_extensions", 1, mjb_nif_script_extensions, 0 },
    { "emoji_properties", 1, mjb_nif_emoji_properties, 0 },
    { "codepoint_is", 2, mjb_nif_codepoint_is, 0 },
    { "plane_name", 2, mjb_nif_plane_name, 0 },
    { "emoji_sequence", 1, mjb_nif_emoji_sequence, 0 },
    { "hangul_syllable_name", 1, mjb_nif_hangul_syllable_name, 0 },
    { "hangul_decompose", 1, mjb_nif_hangul_decompose, 0 },
    { "hangul_compose", 3, mjb_nif_hangul_compose, 0 },
    { "collation_compare", 5, mjb_nif_collation_compare, 0 },
    { "collation_key", 4, mjb_nif_collation_key, 0 },
    { "idna", 2, mjb_nif_idna, 0 },
    { "confusable_skeleton", 1, mjb_nif_confusable_skeleton, 0 },
    { "confusable_match", 3, mjb_nif_confusable_match, 0 },
    { "resolved_script_set", 1, mjb_nif_resolved_script_set, 0 },
    { "is_identifier", 2, mjb_nif_is_identifier, 0 },
    { "detect_encoding", 1, mjb_nif_detect_encoding, 0 },
    { "validate_encoding", 2, mjb_nif_validate_encoding, 0 },
    { "convert_encoding", 4, mjb_nif_convert_encoding, 0 },
    { "locale_parse", 1, mjb_nif_locale_parse, 0 },
    { "limits", 0, mjb_nif_limits, 0 },
};

ERL_NIF_INIT(Elixir.Mojibake.Nif, mjb_nif_funcs, mjb_nif_load, NULL, mjb_nif_upgrade, NULL)
