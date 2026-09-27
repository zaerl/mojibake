/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#pragma once

#ifndef MJB_UNICODE_TABLES_H
#define MJB_UNICODE_TABLES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "mojibake.h"

// One populated 256-codepoint page of a bitset-indexed table: four presence words, the packed
// count of entries before each word, the first entry index, and a table-specific extra value.
typedef struct mjb_unicode_bitset_page {
    uint64_t bits[4];
    uint32_t ranks;
    uint16_t start;
    uint16_t extra;
} mjb_unicode_bitset_page;

// Longest codepoint sequence any pooled-sequence lookup writes into a caller buffer.
#define MJB_UNICODE_SEQUENCE_MAX 18

typedef struct mjb_unicode_case_mapping {
    mjb_category category;
    mjb_codepoint uppercase;
    mjb_codepoint lowercase;
    mjb_codepoint titlecase;
} mjb_unicode_case_mapping;

#if MJB_FEATURE_COLLATION
typedef uint32_t mjb_unicode_collation_contraction_entry;
#endif

#if MJB_FEATURE_IDNA
typedef enum mjb_unicode_idna_status {
    MJB_UNICODE_IDNA_DISALLOWED,
    MJB_UNICODE_IDNA_VALID,
    MJB_UNICODE_IDNA_IGNORED,
    MJB_UNICODE_IDNA_MAPPED,
    MJB_UNICODE_IDNA_DEVIATION,
    MJB_UNICODE_IDNA_NONE // Table placeholder for codepoints without a row; never returned
} mjb_unicode_idna_status;

enum {
    MJB_UNICODE_IDNA_SUPPLEMENTARY_START = 0x20000
};
#endif

bool mjb_unicode_block_lookup(mjb_codepoint codepoint, mjb_block_info *block);
bool mjb_unicode_name_lookup(mjb_codepoint codepoint, char *name, size_t name_size);
bool mjb_unicode_emoji_lookup(mjb_codepoint codepoint, mjb_emoji_properties *emoji);
bool mjb_unicode_emoji_sequence_lookup(const mjb_codepoint *codepoints, size_t count,
    mjb_emoji_sequence *emoji);
bool mjb_unicode_has_property(mjb_codepoint codepoint, mjb_property property, uint8_t *value);
bool mjb_unicode_properties(mjb_codepoint codepoint, uint8_t *buffer);
bool mjb_unicode_script_extensions_lookup(mjb_codepoint codepoint, const uint8_t **scripts,
    uint8_t *count);
bool mjb_unicode_n_character_lookup(mjb_codepoint codepoint, mjb_n_character *character);
bool mjb_unicode_category_lookup(mjb_codepoint codepoint, mjb_category *category);
bool mjb_unicode_codepoint_assigned(mjb_codepoint codepoint);
bool mjb_unicode_bidi_lookup(mjb_codepoint codepoint, mjb_bidi_class *bidi, bool *mirrored);
bool mjb_unicode_numeric_value_lookup(mjb_codepoint codepoint, mjb_numeric_value *value);
bool mjb_unicode_case_lookup(mjb_codepoint codepoint, mjb_unicode_case_mapping *mapping);
bool mjb_unicode_special_casing_lookup(mjb_codepoint codepoint, mjb_map_case_type case_type,
    const mjb_codepoint **values, uint8_t *length);
bool mjb_unicode_case_folding_lookup(mjb_codepoint codepoint, const mjb_codepoint **values,
    uint8_t *length);
bool mjb_unicode_case_folding_simple_lookup(mjb_codepoint codepoint, mjb_codepoint *value);

#if MJB_FEATURE_SECURITY
bool mjb_unicode_confusable_lookup(mjb_codepoint codepoint, mjb_codepoint *values, uint8_t *length);
#endif

#if MJB_FEATURE_IDNA
bool mjb_unicode_idna_lookup(mjb_codepoint codepoint, mjb_unicode_idna_status *status,
    mjb_codepoint *mapping, uint8_t *length);
#endif

#if MJB_FEATURE_COLLATION
bool mjb_unicode_collation_entry_lookup(mjb_codepoint codepoint, uint32_t *first_weight,
    const uint32_t **expansion);
bool mjb_unicode_collation_implicit_lookup(mjb_codepoint codepoint, uint16_t *base,
    mjb_codepoint *offset);
bool mjb_unicode_collation_contraction_range(mjb_codepoint first_codepoint,
    const mjb_unicode_collation_contraction_entry **entries, size_t *count);
const mjb_codepoint *
mjb_unicode_collation_contraction_sequence(const mjb_unicode_collation_contraction_entry *entry,
    uint8_t *length);
const uint8_t *
mjb_unicode_collation_contraction_weights(const mjb_unicode_collation_contraction_entry *entry,
    uint8_t *length);
#endif

bool mjb_unicode_decomposition_lookup(mjb_codepoint codepoint, bool compatibility,
    mjb_codepoint *values, uint8_t *length);
mjb_codepoint mjb_unicode_compose_pair(mjb_codepoint starter, mjb_codepoint combining);

#endif // MJB_UNICODE_TABLES_H
