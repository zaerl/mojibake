/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#pragma once

#ifndef MJB_MOJIBAKE_H
#define MJB_MOJIBAKE_H

// clang-format off
#if defined(__cplusplus)
    #if __cplusplus < 201703L
        #error "C++17 or a later version is required"
    #endif
#else
    #if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 201112L
        #error "C11 or a later version is required"
    #endif
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "locales.h"
#include "unicode.h"

#ifdef __cplusplus
extern "C" {
#endif

// Static assertions for important constants
// clang-format off
#if defined(__cplusplus)
    static_assert(sizeof(uint32_t) == 4, "uint32_t must be 4 bytes");
    static_assert(sizeof(char) == 1, "char must be 1 byte");
#else
    _Static_assert(sizeof(uint32_t) == 4, "uint32_t must be 4 bytes");
    _Static_assert(sizeof(char) == 1, "char must be 1 byte");
#endif

#define MJB_VERSION_NUMBER 0x28 // MAJOR << 8 | MINOR << 4 | REVISION
#define MJB_VERSION_MAJOR 0
#define MJB_VERSION_MINOR 2
#define MJB_VERSION_REVISION 8

#ifdef __EMSCRIPTEN__
    #define MJB_VERSION "0.2.8-WASM"
#else
    #define MJB_VERSION "0.2.8"
#endif

#define MJB_UNICODE_VERSION "17.0.0"
#define MJB_UNICODE_VERSION_MAJOR 17
#define MJB_UNICODE_VERSION_MINOR 0
#define MJB_UNICODE_VERSION_REVISION 0

#ifndef MJB_FEATURE_CHARACTER_NAMES
    #define MJB_FEATURE_CHARACTER_NAMES 1
#endif

#ifndef MJB_EXTERN
    #define MJB_EXTERN extern
#endif

#ifndef MJB_EXPORT
    #if defined(_WIN32) || defined(_WIN64)
        #if defined(MJB_SHARED)
            #if defined(MJB_BUILDING_LIBRARY)
                #define MJB_EXPORT __declspec(dllexport)
            #else
                #define MJB_EXPORT __declspec(dllimport)
            #endif
        #else
            #define MJB_EXPORT
        #endif
    #elif defined(__GNUC__) || defined(__clang__)
        #define MJB_EXPORT __attribute__((visibility("default")))
    #else
        #define MJB_EXPORT
    #endif
#endif

// Modern compiler attributes with fallbacks
#if defined(__GNUC__) || defined(__clang__)
    #define MJB_NODISCARD __attribute__((warn_unused_result))
    #define MJB_NONNULL(...) __attribute__((nonnull(__VA_ARGS__)))
    #define MJB_PURE __attribute__((pure))
    #define MJB_CONST __attribute__((const))
#elif defined(__cplusplus) && __cplusplus >= 201703L
    #define MJB_NODISCARD [[nodiscard]]
    #define MJB_NONNULL(...)
    #define MJB_PURE
    #define MJB_CONST
#else
    #define MJB_NODISCARD
    #define MJB_NONNULL(...)
    #define MJB_PURE
    #define MJB_CONST
#endif

// Compiler features
#ifndef MJB_FILTER_MAX_COMBINING_MARKS
    // Kept by MJB_FILTER_LIMIT_COMBINING
    #define MJB_FILTER_MAX_COMBINING_MARKS 4
#endif

// clang-format on

// See c standard memory allocation functions
typedef void *(*mjb_alloc_fn)(size_t size);
typedef void *(*mjb_realloc_fn)(void *ptr, size_t new_size);
typedef void (*mjb_free_fn)(void *ptr);

/**
 * A Unicode codepoint, a value in the range 0 to 0x10FFFF
 * [see: https://www.unicode.org/glossary/#code_point]
 */
typedef uint32_t mjb_codepoint;

#define MJB_CODEPOINT_MIN 0x0
#define MJB_CODEPOINT_MAX 0x10FFFF       // Maximum valid Unicode code point
#define MJB_CODEPOINT_REPLACEMENT 0xFFFD // The character used when there is invalid data "�"
#define MJB_PRIVATE_USE_START 0xF0000    // Private use area start
#define MJB_PRIVATE_USE_END 0x10FFFD     // Private use area end
#define MJB_CODEPOINT_NOT_VALID 0x110000 // Not a valid codepoint

// Hangul Syllables constants
// See: https://www.unicode.org/versions/Unicode17.0.0/core-spec/chapter-3/#G61399
#define MJB_CP_HANGUL_S_BASE 0xAC00
#define MJB_CP_HANGUL_L_BASE 0x1100
#define MJB_CP_HANGUL_V_BASE 0x1161
#define MJB_CP_HANGUL_T_BASE 0x11A7
#define MJB_CP_HANGUL_L_COUNT 19
#define MJB_CP_HANGUL_V_COUNT 21
#define MJB_CP_HANGUL_T_COUNT 28
#define MJB_CP_HANGUL_N_COUNT 588   // V_COUNT * T_COUNT
#define MJB_CP_HANGUL_S_COUNT 11172 // L_COUNT * N_COUNT

// CJK Ideographs
#define MJB_CJK_IDEOGRAPH_START 0x4E00
#define MJB_CJK_IDEOGRAPH_END 0x9FFF
#define MJB_CJK_EXTENSION_A_START 0x3400
#define MJB_CJK_EXTENSION_A_END 0x4DBF
#define MJB_CJK_EXTENSION_B_START 0x20000
#define MJB_CJK_EXTENSION_B_END 0x2A6DF
#define MJB_CJK_EXTENSION_C_START 0x2A700
#define MJB_CJK_EXTENSION_C_END 0x2B73F
#define MJB_CJK_EXTENSION_D_START 0x2B740
#define MJB_CJK_EXTENSION_D_END 0x2B81D
#define MJB_CJK_EXTENSION_E_START 0x2B820
#define MJB_CJK_EXTENSION_E_END 0x2CEAD
#define MJB_CJK_EXTENSION_F_START 0x2CEB0
#define MJB_CJK_EXTENSION_F_END 0x2EBE0
#define MJB_CJK_EXTENSION_G_START 0x30000
#define MJB_CJK_EXTENSION_G_END 0x3134A
#define MJB_CJK_EXTENSION_H_START 0x31350
#define MJB_CJK_EXTENSION_H_END 0x323AF
#define MJB_CJK_EXTENSION_I_START 0x2EBF0
#define MJB_CJK_EXTENSION_I_END 0x2EE5D
#define MJB_CJK_EXTENSION_J_START 0x323B0
#define MJB_CJK_EXTENSION_J_END 0x33479

#define MJB_CJK_COMPATIBILITY_IDEOGRAPH_START 0xF900
#define MJB_CJK_COMPATIBILITY_IDEOGRAPH_END 0xFAD9

#define MJB_CJK_COMPATIBILITY_IDEOGRAPH_SUPPLEMENT_START 0x2F800
#define MJB_CJK_COMPATIBILITY_IDEOGRAPH_SUPPLEMENT_END 0x2FA1D

#define MJB_EGYPTIAN_H_START 0x13000
#define MJB_EGYPTIAN_H_FORMAT_EXT_START 0x13460
#define MJB_EGYPTIAN_H_EXT_END 0x143FF

// Tangut Ideographs
#define MJB_TANGUT_IDEOGRAPH_START 0x17000
#define MJB_TANGUT_IDEOGRAPH_END 0x187F7

#define MJB_TANGUT_IDEOGRAPH_SUPPLEMENT_START 0x18D00
#define MJB_TANGUT_IDEOGRAPH_SUPPLEMENT_END 0x18D1E

// Tangut components
#define MJB_TANGUT_COMPONENT_START 0x18800
#define MJB_TANGUT_COMPONENT_END 0x18AFF

#define MJB_TANGUT_COMPONENT_SUPPLEMENT_START 0x18D80
#define MJB_TANGUT_COMPONENT_SUPPLEMENT_END 0x18DF2

// Khitan Small Script Characters
#define MJB_KHITAN_SMALL_SCRIPT_CHARACTER_START 0x18B00
#define MJB_KHITAN_SMALL_SCRIPT_CHARACTER_END 0x18CFF

// Numeric values, to be used when the decimal and digit mjb_character fields are not valid
#define MJB_NUMBER_NOT_VALID -1

/**
 * Unicode encoding
 * [see: https://www.unicode.org/glossary/#character_encoding_scheme]
 */
typedef enum mjb_encoding {
    MJB_ENC_UNKNOWN = 0x0,
    MJB_ENC_ASCII = 0x1,
    MJB_ENC_UTF_8 = 0x2,
    MJB_ENC_UTF_16 = 0x4,
    MJB_ENC_UTF_16BE = 0x8,
    MJB_ENC_UTF_16LE = 0x10,
    MJB_ENC_UTF_32 = 0x20,
    MJB_ENC_UTF_32BE = 0x40,
    MJB_ENC_UTF_32LE = 0x80
} mjb_encoding;

/**
 * Normalization form
 * [see: https://www.unicode.org/glossary/#normalization_form]
 */
typedef enum mjb_normalization {
    MJB_NORMALIZATION_NFC,  // Canonical decomposition followed by canonical composition
    MJB_NORMALIZATION_NFD,  // Canonical decomposition without recomposition
    MJB_NORMALIZATION_NFKC, // Compatibility decomposition followed by canonical composition
    MJB_NORMALIZATION_NFKD  // Compatibility decomposition without recomposition
} mjb_normalization;

typedef enum mjb_quick_check_result {
    MJB_QC_YES = 0x0,
    MJB_QC_NO = 0x1,
    MJB_QC_MAYBE = 0x2,
    // See: DerivedNormalizationProps.txt
    MJB_QC_NFD_NO = 0x4,
    MJB_QC_NFD_MAYBE = 0x8, // Impossible to happen
    MJB_QC_NFC_NO = 0x10,
    MJB_QC_NFC_MAYBE = 0x20,
    MJB_QC_NFKC_NO = 0x40,
    MJB_QC_NFKC_MAYBE = 0x80,
    MJB_QC_NFKD_NO = 0x100,
    MJB_QC_NFKD_MAYBE = 0x200 // Impossible to happen
} mjb_quick_check_result;

typedef enum mjb_filter {
    MJB_FILTER_NONE = 0x0,
    MJB_FILTER_NORMALIZE = 0x1,
    MJB_FILTER_SPACES = 0x2,
    MJB_FILTER_COLLAPSE_SPACES = 0x4,
    MJB_FILTER_CONTROLS = 0x8,
    MJB_FILTER_NUMERIC = 0x10,
    MJB_FILTER_LIMIT_COMBINING = 0x20
} mjb_filter;

typedef enum mjb_case_type {
    MJB_CASE_NONE,
    MJB_CASE_UPPER,
    MJB_CASE_LOWER,
    MJB_CASE_TITLE,
    MJB_CASE_CASEFOLD,       // C + F statuses, string may grow
    MJB_CASE_CASEFOLD_SIMPLE // C + S statuses, length-preserving
} mjb_case_type;

typedef enum mjb_error {
    MJB_ERROR_NONE,
    MJB_ERROR_INVALID_ARGUMENT,
    MJB_ERROR_UNSUPPORTED
} mjb_error;

typedef enum mjb_status {
    MJB_STATUS_OK = 0,
    MJB_STATUS_INVALID_ARGUMENT,
    MJB_STATUS_INVALID_ENCODING,
    MJB_STATUS_INVALID_CODEPOINT,
    MJB_STATUS_INVALID_FORM,
    MJB_STATUS_UNSUPPORTED,
    MJB_STATUS_NO_MEMORY,
    MJB_STATUS_OVERFLOW,
    MJB_STATUS_MALFORMED_INPUT,
    MJB_STATUS_OUTPUT_TOO_SMALL,
    MJB_STATUS_CALLBACK_STOPPED,
    MJB_STATUS_NOT_FOUND
} mjb_status;

typedef struct mjb_locale_id {
    char language[9];
    char extlang[12];
    char script[5];
    char region[4];
    char variant[32];
    char extensions[128];
    char private_use[128];
    char grandfathered[32];
} mjb_locale_id;

typedef struct mjb_result {
    char *output;
    size_t output_size;
    bool transformed;
} mjb_result;

/**
 * Unicode block
 * [see: https://www.unicode.org/glossary/#block]
 */
typedef struct mjb_block_info {
    mjb_block id;
    char name[128];
    uint32_t start;
    uint32_t end;
} mjb_block_info;

/**
 * A Unicode character
 * [see: https://www.unicode.org/glossary/#character]
 */
typedef struct mjb_character {
    mjb_codepoint codepoint;
    char name[128];
    mjb_category category;
    mjb_canonical_combining_class combining;
    unsigned short bidirectional;
    mjb_decomposition decomposition;
    int decimal;
    int digit;
    char numeric[16];
    bool mirrored;
    mjb_codepoint uppercase;
    mjb_codepoint lowercase;
    mjb_codepoint titlecase;
} mjb_character;

/**
 * Numeric value of a codepoint
 */
typedef struct mjb_numeric_value {
    int decimal;
    int digit;
    char numeric[16];
} mjb_numeric_value;

/**
 * Emoji data
 * [see: UTS #51, Unicode 17.0.0:
 * https://www.unicode.org/reports/tr51/tr51-29.html]
 */
typedef struct mjb_emoji_properties {
    mjb_codepoint codepoint;
    bool emoji;
    bool presentation;
    bool modifier;
    bool modifier_base;
    bool component;
    bool extended_pictographic;
} mjb_emoji_properties;

typedef enum mjb_emoji_sequence_type {
    MJB_EMOJI_SEQUENCE_NONE,
    MJB_EMOJI_SEQUENCE_BASIC,
    MJB_EMOJI_SEQUENCE_KEYCAP,
    MJB_EMOJI_SEQUENCE_FLAG,
    MJB_EMOJI_SEQUENCE_TAG,
    MJB_EMOJI_SEQUENCE_MODIFIER,
    MJB_EMOJI_SEQUENCE_ZWJ,
    MJB_EMOJI_SEQUENCE_TEXT_VARIATION,
    MJB_EMOJI_SEQUENCE_EMOJI_VARIATION
} mjb_emoji_sequence_type;

typedef enum mjb_emoji_qualification {
    MJB_EMOJI_QUALIFICATION_NONE,
    MJB_EMOJI_QUALIFICATION_COMPONENT,
    MJB_EMOJI_QUALIFICATION_FULLY_QUALIFIED,
    MJB_EMOJI_QUALIFICATION_MINIMALLY_QUALIFIED,
    MJB_EMOJI_QUALIFICATION_UNQUALIFIED
} mjb_emoji_qualification;

typedef struct mjb_emoji_sequence {
    mjb_emoji_sequence_type type;
    mjb_emoji_qualification qualification;
    size_t codepoint_count;
} mjb_emoji_sequence;

typedef enum mjb_break_type {
    MJB_BT_NOT_SET,   // Not set
    MJB_BT_MANDATORY, // !
    MJB_BT_NO_BREAK,  // ×
    MJB_BT_ALLOWED    // ÷
} mjb_break_type;

// Buffer character used in composition phase
typedef struct mjb_buffer_character {
    uint32_t codepoint;
    uint16_t combining;
} mjb_buffer_character;

// Position of a character in a string, used for callbacks
typedef enum mjb_character_position {
    MJB_POSITION_NONE,
    MJB_POSITION_FIRST,
    MJB_POSITION_LAST
} mjb_character_position;

// Display width context for ambiguous-width characters
typedef enum mjb_width_context {
    MJB_WIDTH_CONTEXT_WESTERN,    // Treat ambiguous characters as narrow (width 1)
    MJB_WIDTH_CONTEXT_EAST_ASIAN, // Treat ambiguous characters as wide (width 2)
    MJB_WIDTH_CONTEXT_AUTO        // Auto-detect from string content
} mjb_width_context;

typedef struct mjb_next_state {
    uint8_t state;
    size_t index;
    unsigned int previous;
    unsigned int current;
    mjb_codepoint previous_codepoint;
    mjb_codepoint current_codepoint;
    bool in_error;
    unsigned short ri_count;
    bool ext_pict_seen;
    bool zwj_seen;
    bool incb_consonant_seen;
    bool incb_linker_seen;
} mjb_next_state;

typedef struct mjb_next_line_state {
    uint8_t state;
    size_t index;
    mjb_lbp previous;
    mjb_lbp current;
    mjb_codepoint prev_prev_codepoint;
    mjb_codepoint previous_codepoint;
    mjb_codepoint current_codepoint;
    bool in_error;
    bool zw_seen;
    bool pi_qu_context;
    bool cm_merged;
    bool zwj_absorbed;
    unsigned short ri_count;
    mjb_lbp prev_resolved;
    mjb_east_asian_width prev_ea;
    mjb_east_asian_width qu_prev_ea;
    mjb_lbp prev_prev_lbp;
    mjb_lbp prev_num_lbp;
} mjb_next_line_state;

typedef struct mjb_next_word_state {
    uint8_t state;
    size_t index;
    mjb_wbp previous;
    mjb_wbp current;
    mjb_codepoint prev_prev_codepoint;
    mjb_codepoint previous_codepoint;
    mjb_codepoint current_codepoint;
    mjb_wbp prev_prev_wbp;
    bool in_error;
    unsigned short ri_count;
    bool wb4_merged;
    bool zwj_pending;
    bool prev_was_zwj;
} mjb_next_word_state;

typedef struct mjb_next_sentence_state {
    uint8_t state;
    size_t index;
    mjb_sbp previous;
    mjb_sbp current;
    mjb_sbp prev_prev;
    mjb_codepoint previous_codepoint;
    mjb_codepoint current_codepoint;
    bool in_error;
    bool sb5_merged;
    bool in_sat;
    bool sat_has_sp;
    bool sat_is_aterm;
} mjb_next_sentence_state;

typedef bool (*mjb_string_each_character_fn)(mjb_character *character, mjb_character_position type);

typedef enum mjb_direction {
    MJB_DIRECTION_LTR = 0,
    MJB_DIRECTION_RTL = 1,
    MJB_DIRECTION_AUTO = 2,
} mjb_direction;

typedef struct mjb_bidi_char {
    mjb_codepoint codepoint;
    size_t byte_offset;
    uint8_t level;
    mjb_bidi_class resolved_class;
    mjb_codepoint mirroring_glyph;
} mjb_bidi_char;

typedef struct mjb_bidi_paragraph {
    mjb_bidi_char *chars;
    size_t count;
    uint8_t paragraph_level;
    mjb_direction direction;
} mjb_bidi_paragraph;

typedef struct mjb_bidi_run {
    size_t start;
    size_t end;
    uint8_t level;
    mjb_direction direction;
} mjb_bidi_run;

// Collation variable-weighting strategy (UTS #10, Unicode 17.0.0, Section 3.6)
typedef enum mjb_collation_mode {
    MJB_COLLATION_NON_IGNORABLE, // variable elements keep their weights unchanged
    MJB_COLLATION_SHIFTED        // variable elements move primary to level 4
} mjb_collation_mode;

// UAX #31 identifier profile (Unicode 17.0.0)
typedef enum mjb_identifier_profile {
    MJB_IDENTIFIER_DEFAULT, // NFC + ID_Start / ID_Continue
    MJB_IDENTIFIER_NFKC     // NFKC + XID_Start / XID_Continue
} mjb_identifier_profile;

// This functions list is automatically generated. Do not edit.
// clang-format off

// Return the codepoint character.
MJB_EXPORT MJB_NODISCARD mjb_status mjb_codepoint_character(mjb_codepoint codepoint, mjb_character *character);

// Normalize a string to NFC/NFKC/NFD/NFKD form.
MJB_EXPORT MJB_NODISCARD mjb_status mjb_normalize(const char *buffer, size_t byte_length, mjb_encoding encoding, mjb_normalization form, mjb_encoding output_encoding, mjb_result *result);

// Filter a string with the selected mjb_filter flags.
MJB_EXPORT MJB_NODISCARD mjb_status mjb_string_filter(const char *buffer, size_t byte_length, mjb_encoding encoding, mjb_filter filters, mjb_encoding output_encoding, mjb_result *result);

// Apply the Unicode NFKC_Casefold transform to a string.
MJB_EXPORT MJB_NODISCARD mjb_status mjb_nfkc_casefold(const char *buffer, size_t byte_length, mjb_encoding encoding, mjb_encoding output_encoding, mjb_result *result);

// Check if a string is normalized to NFC/NFKC/NFD/NFKD form.
MJB_EXPORT mjb_quick_check_result mjb_string_is_normalized(const char *buffer, size_t byte_length, mjb_encoding encoding, mjb_normalization form);

// Return the string encoding (the most probable).
MJB_EXPORT MJB_PURE mjb_encoding mjb_string_encoding(const char *buffer, size_t byte_length);

// Return true if the string is encoded in ASCII.
MJB_EXPORT MJB_PURE bool mjb_string_is_ascii(const char *buffer, size_t byte_length);

// Return true if the string is encoded in UTF-8.
MJB_EXPORT MJB_PURE bool mjb_string_is_utf8(const char *buffer, size_t byte_length);

// Return true if the string is encoded in UTF-16BE or UTF-16LE.
MJB_EXPORT MJB_PURE bool mjb_string_is_utf16(const char *buffer, size_t byte_length);

// Return the length of a string.
MJB_EXPORT MJB_PURE size_t mjb_string_length(const char *buffer, size_t max_length, mjb_encoding encoding);

// Run a callback for each character of a string.
MJB_EXPORT MJB_NODISCARD mjb_status mjb_string_each_character(const char *buffer, size_t byte_length, mjb_encoding encoding, mjb_string_each_character_fn callback);

// Return the value of a binary Unicode property.
MJB_EXPORT MJB_NODISCARD mjb_status mjb_codepoint_property_binary(mjb_codepoint codepoint, mjb_property property, bool *value);

// Return the value of an enumerated or integer Unicode property.
MJB_EXPORT MJB_NODISCARD mjb_status mjb_codepoint_property_int(mjb_codepoint codepoint, mjb_property property, int32_t *value);

// Return the numeric value of a codepoint.
MJB_EXPORT MJB_NODISCARD mjb_status mjb_codepoint_numeric_value(mjb_codepoint codepoint, mjb_numeric_value *value);

// Return the character block.
MJB_EXPORT MJB_NODISCARD mjb_status mjb_codepoint_block(mjb_codepoint codepoint, mjb_block_info *block);

// Return the script of a codepoint.
MJB_EXPORT mjb_script mjb_codepoint_script(mjb_codepoint codepoint);

// Return the Script_Extensions set of a codepoint.
MJB_EXPORT MJB_NODISCARD mjb_status mjb_codepoint_script_extensions(mjb_codepoint codepoint, mjb_script *scripts, size_t *count);

// Encode a codepoint to a string.
MJB_EXPORT unsigned int mjb_codepoint_encode(mjb_codepoint codepoint, char *buffer, size_t byte_length, mjb_encoding encoding);

// Convert from one encoding to another.
MJB_EXPORT MJB_NODISCARD mjb_status mjb_string_convert_encoding(const char *buffer, size_t byte_length, mjb_encoding encoding, mjb_encoding output_encoding, mjb_result *result);

// Compare two strings using UCA.
MJB_EXPORT int mjb_string_compare(const char *s1, size_t s1_byte_length, mjb_encoding s1_encoding, const char *s2, size_t s2_byte_length, mjb_encoding s2_encoding, mjb_collation_mode mode);

// Generate a UCA sort key for a string.
MJB_EXPORT MJB_NODISCARD mjb_status mjb_collation_key(const char *buffer, size_t byte_length, mjb_encoding encoding, mjb_collation_mode mode, mjb_result *result);

// Change string case.
MJB_EXPORT MJB_NODISCARD mjb_status mjb_case(const char *buffer, size_t byte_length, mjb_encoding encoding, mjb_case_type type, mjb_encoding output_encoding, mjb_result *result);

// Return true if the codepoint is valid.
MJB_EXPORT MJB_CONST bool mjb_codepoint_is_valid(mjb_codepoint codepoint);

// Return true if the codepoint is graphic.
MJB_EXPORT MJB_CONST bool mjb_codepoint_is_graphic(mjb_codepoint codepoint);

// Return true if the codepoint is combining.
MJB_EXPORT MJB_CONST bool mjb_codepoint_is_combining(mjb_codepoint codepoint);

// Return if the codepoint is a hangul L.
MJB_EXPORT MJB_CONST bool mjb_codepoint_is_hangul_l(mjb_codepoint codepoint);

// Return if the codepoint is a hangul V.
MJB_EXPORT MJB_CONST bool mjb_codepoint_is_hangul_v(mjb_codepoint codepoint);

// Return if the codepoint is a hangul T.
MJB_EXPORT MJB_CONST bool mjb_codepoint_is_hangul_t(mjb_codepoint codepoint);

// Return if the codepoint is a hangul jamo.
MJB_EXPORT MJB_CONST bool mjb_codepoint_is_hangul_jamo(mjb_codepoint codepoint);

// Return if the codepoint is a hangul syllable.
MJB_EXPORT MJB_CONST bool mjb_codepoint_is_hangul_syllable(mjb_codepoint codepoint);

// Return if the codepoint is CJK ideograph.
MJB_EXPORT MJB_CONST bool mjb_codepoint_is_cjk_ideograph(mjb_codepoint codepoint);

// Return if the codepoint is CJK extension.
MJB_EXPORT MJB_CONST bool mjb_codepoint_is_cjk_ext(mjb_codepoint codepoint);

// Return true if the category is graphic.
MJB_EXPORT MJB_CONST bool mjb_category_is_graphic(mjb_category category);

// Return true if the category is combining.
MJB_EXPORT MJB_CONST bool mjb_category_is_combining(mjb_category category);

// Unicode line break algorithm.
MJB_EXPORT mjb_break_type mjb_break_line(const char *buffer, size_t byte_length, mjb_encoding encoding, mjb_next_line_state *state);

// Word cluster breaking.
MJB_EXPORT mjb_break_type mjb_break_word(const char *buffer, size_t byte_length, mjb_encoding encoding, mjb_next_word_state *state);

// Sentence boundaries breaking.
MJB_EXPORT mjb_break_type mjb_break_sentence(const char *buffer, size_t byte_length, mjb_encoding encoding, mjb_next_sentence_state *state);

// Grapheme cluster breaking.
MJB_EXPORT mjb_break_type mjb_break_grapheme_cluster(const char *buffer, size_t byte_length, mjb_encoding encoding, mjb_next_state *state);

// Return the number of bytes that form the first `max_graphemes` grapheme cluster segments.
MJB_EXPORT size_t mjb_truncate(const char *buffer, size_t byte_length, mjb_encoding encoding, size_t max_graphemes);

// Return the number of bytes whose grapheme clusters fit within max_columns display columns.
MJB_EXPORT size_t mjb_truncate_width(const char *buffer, size_t byte_length, mjb_encoding encoding, mjb_width_context context, size_t max_columns);

// Return the number of bytes that form the first max_segments word-break segments.
MJB_EXPORT size_t mjb_truncate_word(const char *buffer, size_t byte_length, mjb_encoding encoding, size_t max_segments);

// Return the number of bytes whose word-break segments fit within max_columns display columns.
MJB_EXPORT size_t mjb_truncate_word_width(const char *buffer, size_t byte_length, mjb_encoding encoding, mjb_width_context context, size_t max_columns);

// Resolve bidirectional text (TR9) for a paragraph.
MJB_EXPORT MJB_NODISCARD mjb_status mjb_bidi_resolve(const char *buffer, size_t byte_length, mjb_encoding encoding, mjb_direction direction, mjb_bidi_paragraph *result);

// Reorder a line visually (L1-L4); visual_order is caller-allocated.
MJB_EXPORT MJB_NODISCARD mjb_status mjb_bidi_reorder_line(const mjb_bidi_paragraph *paragraph, size_t line_start, size_t line_end, size_t *visual_order);

// Compute visual level runs; pass runs=NULL to count first.
MJB_EXPORT MJB_NODISCARD mjb_status mjb_bidi_line_runs(const mjb_bidi_paragraph *paragraph, const size_t *visual_order, size_t count, mjb_bidi_run *runs, size_t *run_count);

// Free a bidi paragraph allocated by mjb_bidi_resolve.
MJB_EXPORT void mjb_bidi_free(mjb_bidi_paragraph *paragraph);

// Return true if the codepoint is a valid Unicode identifier start (Unicode 17.0.0 UAX #31 ID_Start).
MJB_EXPORT bool mjb_codepoint_is_id_start(mjb_codepoint codepoint);

// Return true if the codepoint is a valid Unicode identifier continuation (Unicode 17.0.0 UAX #31 ID_Continue).
MJB_EXPORT bool mjb_codepoint_is_id_continue(mjb_codepoint codepoint);

// Return true if the codepoint is a valid NFKC identifier start (Unicode 17.0.0 UAX #31 XID_Start).
MJB_EXPORT bool mjb_codepoint_is_xid_start(mjb_codepoint codepoint);

// Return true if the codepoint is a valid NFKC identifier continuation (Unicode 17.0.0 UAX #31 XID_Continue).
MJB_EXPORT bool mjb_codepoint_is_xid_continue(mjb_codepoint codepoint);

// Return true if the codepoint is reserved for use in patterns (Unicode 17.0.0 UAX #31 Pattern_Syntax).
MJB_EXPORT bool mjb_codepoint_is_pattern_syntax(mjb_codepoint codepoint);

// Return true if the codepoint is pattern whitespace (Unicode 17.0.0 UAX #31 Pattern_White_Space).
MJB_EXPORT bool mjb_codepoint_is_pattern_white_space(mjb_codepoint codepoint);

// Return true if the string is a valid Unicode identifier (Unicode 17.0.0 UAX #31).
MJB_EXPORT bool mjb_string_is_identifier(const char *buffer, size_t byte_length, mjb_encoding encoding, mjb_identifier_profile profile);

// Return the name of a property, NULL if the property specified is not valid.
MJB_EXPORT MJB_CONST const char *mjb_property_name(mjb_property property);

// Compute a Unicode confusable skeleton (Unicode 17.0.0 UTS #39 Section 4).
MJB_EXPORT MJB_NODISCARD mjb_status mjb_confusable_skeleton(const char *buffer, size_t byte_length, mjb_encoding encoding, mjb_encoding output_encoding, mjb_result *result);

// Return true if two strings are visually confusable (Unicode 17.0.0 UTS #39 Section 4): skeleton(s1) == skeleton(s2).
MJB_EXPORT bool mjb_string_is_confusable(const char *s1, size_t s1_byte_length, mjb_encoding s1_encoding, const char *s2, size_t s2_byte_length, mjb_encoding s2_encoding);

// Return the emoji properties.
MJB_EXPORT MJB_NODISCARD mjb_status mjb_codepoint_emoji(mjb_codepoint codepoint, mjb_emoji_properties *emoji);

// Return true if the codepoint has the Unicode Emoji property.
MJB_EXPORT bool mjb_codepoint_is_emoji(mjb_codepoint codepoint);

// Return true if the codepoint has the Unicode Emoji_Presentation property.
MJB_EXPORT bool mjb_codepoint_is_emoji_presentation(mjb_codepoint codepoint);

// Return true if the codepoint has the Unicode Emoji_Modifier property.
MJB_EXPORT bool mjb_codepoint_is_emoji_modifier(mjb_codepoint codepoint);

// Return true if the codepoint has the Unicode Emoji_Modifier_Base property.
MJB_EXPORT bool mjb_codepoint_is_emoji_modifier_base(mjb_codepoint codepoint);

// Return true if the codepoint has the Unicode Emoji_Component property.
MJB_EXPORT bool mjb_codepoint_is_emoji_component(mjb_codepoint codepoint);

// Return true if the codepoint has the Unicode Extended_Pictographic property.
MJB_EXPORT bool mjb_codepoint_is_extended_pictographic(mjb_codepoint codepoint);

// Return the plane of the codepoint.
MJB_EXPORT MJB_CONST mjb_plane mjb_codepoint_plane(mjb_codepoint codepoint);

// Return true if the plane is valid.
MJB_EXPORT MJB_CONST bool mjb_plane_is_valid(mjb_plane plane);

// Return the name of a plane, NULL if the plane specified is not valid.
MJB_EXPORT MJB_CONST const char *mjb_plane_name(mjb_plane plane, bool abbreviation);

// Return emoji sequence metadata for a complete string.
MJB_EXPORT MJB_NODISCARD mjb_status mjb_string_emoji_sequence(const char *buffer, size_t byte_length, mjb_encoding encoding, mjb_emoji_sequence *emoji);

// Return true if the complete string is an emoji sequence listed by Unicode, including standardized emoji variation sequences.
MJB_EXPORT bool mjb_string_is_emoji_sequence(const char *buffer, size_t byte_length, mjb_encoding encoding);

// Return true if the complete string is an RGI emoji sequence, excluding plain standardized variation sequences.
MJB_EXPORT bool mjb_string_is_rgi_emoji(const char *buffer, size_t byte_length, mjb_encoding encoding);

// Return hangul syllable name.
MJB_EXPORT MJB_NODISCARD mjb_status mjb_hangul_syllable_name(mjb_codepoint codepoint, char *buffer, size_t byte_length);

// Hangul syllable decomposition.
MJB_EXPORT MJB_NODISCARD mjb_status mjb_hangul_syllable_decomposition(mjb_codepoint codepoint, mjb_codepoint *codepoints);

// Hangul syllable composition.
MJB_EXPORT size_t mjb_hangul_syllable_composition(mjb_buffer_character *characters, size_t characters_len);

// Return the east asian width of a codepoint.
MJB_EXPORT MJB_NODISCARD mjb_status mjb_codepoint_east_asian_width(mjb_codepoint codepoint, mjb_east_asian_width *width);

// Return the display width of a string.
MJB_EXPORT MJB_NODISCARD mjb_status mjb_display_width(const char *buffer, size_t byte_length, mjb_encoding encoding, mjb_width_context context, size_t *width);

// Parse a BCP 47 language tag.
MJB_EXPORT MJB_NODISCARD mjb_status mjb_locale_parse(const char *id, size_t byte_length, mjb_encoding encoding, mjb_locale_id *locale, mjb_error *error);

// Set current locale used by locale-sensitive casing.
MJB_EXPORT MJB_NODISCARD mjb_status mjb_locale_set(unsigned int locale);

// Free a mjb_result.
MJB_EXPORT mjb_status mjb_result_free(mjb_result *result);

// Output the current library version (MJB_VERSION).
MJB_EXPORT MJB_CONST const char *mjb_version(void);

// Output the current library version number (MJB_VERSION_NUMBER).
MJB_EXPORT MJB_CONST unsigned int mjb_version_number(void);

// Output the current supported Unicode version (MJB_UNICODE_VERSION).
MJB_EXPORT MJB_CONST const char *mjb_unicode_version(void);

// Set the library memory functions.
MJB_EXPORT MJB_NODISCARD mjb_status mjb_set_memory_functions(mjb_alloc_fn alloc_fn, mjb_realloc_fn realloc_fn, mjb_free_fn free_fn);

// Shutdown the library. Not needed to be called.
MJB_EXPORT void mjb_shutdown(void);

// Allocate memory.
MJB_EXPORT MJB_NODISCARD void *mjb_alloc(size_t byte_length);

// Reallocate memory.
MJB_EXPORT MJB_NODISCARD void *mjb_realloc(void *ptr, size_t new_size);

// Free memory.
MJB_EXPORT void mjb_free(void *ptr);

// clang-format on
#ifdef __cplusplus
}
#endif

#endif // MJB_MOJIBAKE_H
