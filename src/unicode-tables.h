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

typedef struct mjb_unicode_case_mapping {
    mjb_category category;
    mjb_codepoint uppercase;
    mjb_codepoint lowercase;
    mjb_codepoint titlecase;
} mjb_unicode_case_mapping;

typedef uint64_t mjb_unicode_collation_contraction_entry;

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
bool mjb_unicode_special_casing_lookup(mjb_codepoint codepoint, mjb_case_type case_type,
    const mjb_codepoint **values, uint8_t *length);
bool mjb_unicode_case_folding_lookup(mjb_codepoint codepoint, const mjb_codepoint **values,
    uint8_t *length);
bool mjb_unicode_case_folding_simple_lookup(mjb_codepoint codepoint, mjb_codepoint *value);
bool mjb_unicode_confusable_lookup(mjb_codepoint codepoint, const mjb_codepoint **values,
    uint8_t *length);
bool mjb_unicode_collation_entry_lookup(mjb_codepoint codepoint, const uint8_t **weights,
    uint8_t *length);
bool mjb_unicode_collation_contraction_range(mjb_codepoint first_codepoint,
    const mjb_unicode_collation_contraction_entry **entries, size_t *count);
const mjb_codepoint *mjb_unicode_collation_contraction_sequence(
    const mjb_unicode_collation_contraction_entry *entry, uint8_t *length);
const uint8_t *mjb_unicode_collation_contraction_weights(
    const mjb_unicode_collation_contraction_entry *entry, uint8_t *length);
bool mjb_unicode_decomposition_lookup(mjb_codepoint codepoint, bool compatibility,
    const mjb_codepoint **values, uint8_t *length);
mjb_codepoint mjb_unicode_compose_pair(mjb_codepoint starter, mjb_codepoint combining);

#endif // MJB_UNICODE_TABLES_H
