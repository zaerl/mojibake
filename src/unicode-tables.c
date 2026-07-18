/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "mojibake-internal.h"
#include "unicode-data.h"
#include "unicode-tables.h"

#define MJB_COUNT_OF(array) (sizeof(array) / sizeof((array)[0]))

static void mjb_copy_table_string(char *destination, size_t destination_size, const char *source) {
    if(destination_size == 0) {
        return;
    }

    if(source == NULL) {
        destination[0] = '\0';
        return;
    }

    size_t length = strlen(source);

    if(length >= destination_size) {
        length = destination_size - 1;
    }

    memcpy(destination, source, length);
    destination[length] = '\0';
}

#if MJB_FEATURE_CHARACTER_NAMES
static size_t mjb_append_table_string(char *destination, size_t destination_size,
    size_t destination_index, const uint8_t *source) {
    if(destination_size == 0 || source == NULL) {
        return destination_index;
    }

    uint8_t byte = *source++;
    bool terminated = false;

    while(byte != 0 && destination_index + 1 < destination_size) {
        destination[destination_index++] = (char)byte;

        if((byte & 0x80) != 0) {
            terminated = true;
            break;
        }

        byte = *source++;
    }

    if(terminated) {
        destination[destination_index - 1] &= 0x7F;
    }

    destination[destination_index] = '\0';

    return destination_index;
}

static size_t mjb_append_table_string_unchecked(char *destination, size_t destination_index,
    const uint8_t *source) {
    uint8_t byte = *source++;

    if(byte != 0) {
        do {
            destination[destination_index++] = (char)byte;

            if((byte & 0x80) != 0) {
                break;
            }

            byte = *source++;
        } while(true);

        destination[destination_index - 1] &= 0x7F;
    }

    destination[destination_index] = '\0';

    return destination_index;
}

static const uint8_t *mjb_unicode_prefix_lookup(uint16_t prefix_start, uint8_t prefix) {
    uint16_t offset = mjb_unicode_name_page_prefix_offsets[prefix_start + prefix];

    return &mjb_unicode_prefix_data[offset];
}
#endif

static bool mjb_unicode_page_lookup(const uint8_t *page_index, size_t page_count,
    const mjb_unicode_page *pages, const uint8_t *lows, mjb_codepoint codepoint, size_t *index) {
    size_t page = codepoint >> 8;

    if(page >= page_count) {
        return false;
    }

    uint8_t compact_page = page_index[page];

    if(compact_page == 0xFF) {
        return false;
    }

    uint8_t codepoint_low = (uint8_t)(codepoint & 0xFF);
    size_t low = pages[compact_page].start;
    size_t high = low + pages[compact_page].count;

    while(low < high) {
        size_t mid = low + (high - low) / 2;
        uint8_t entry_low = lows[mid];

        if(codepoint_low < entry_low) {
            high = mid;
        } else if(codepoint_low > entry_low) {
            low = mid + 1;
        } else {
            *index = mid;

            return true;
        }
    }

    return false;
}

static uint8_t mjb_unicode_popcount64(uint64_t value) {
    value -= (value >> 1) & UINT64_C(0x5555555555555555);
    value = (value & UINT64_C(0x3333333333333333)) + ((value >> 2) & UINT64_C(0x3333333333333333));
    value = (value + (value >> 4)) & UINT64_C(0x0F0F0F0F0F0F0F0F);

    return (uint8_t)((value * UINT64_C(0x0101010101010101)) >> 56);
}

static bool mjb_unicode_page_bitset_lookup(const uint8_t *page_index, size_t page_count,
    const uint16_t *page_starts, const uint64_t *page_bits, const uint32_t *page_ranks,
    mjb_codepoint codepoint, size_t *index) {
    size_t page = codepoint >> 8;

    if(page >= page_count) {
        return false;
    }

    uint8_t compact_page = page_index[page];

    if(compact_page == 0xFF) {
        return false;
    }

    uint8_t codepoint_low = (uint8_t)codepoint;
    uint8_t word = codepoint_low >> 6;
    uint8_t bit = codepoint_low & 0x3F;
    uint64_t bits = page_bits[(size_t)compact_page * 4 + word];
    uint64_t mask = (uint64_t)1 << bit;

    if((bits & mask) == 0) {
        return false;
    }

    uint8_t rank = (uint8_t)(page_ranks[compact_page] >> (word * 8));
    *index = page_starts[compact_page] + rank + mjb_unicode_popcount64(bits & (mask - 1));

    return true;
}

#if MJB_FEATURE_CHARACTER_NAMES
static bool mjb_unicode_name_entry_lookup(mjb_codepoint codepoint, size_t *index,
    uint16_t *prefix_start) {
    size_t page = codepoint >> 8;

    if(page >= MJB_COUNT_OF(mjb_unicode_name_page_index)) {
        return false;
    }

    uint8_t compact_page = mjb_unicode_name_page_index[page];

    if(compact_page == 0xFF) {
        return false;
    }

    uint8_t codepoint_low = (uint8_t)codepoint;
    uint8_t word = codepoint_low >> 6;
    uint8_t bit = codepoint_low & 0x3F;
    uint64_t bits = mjb_unicode_name_page_bits[(size_t)compact_page * 4 + word];
    uint64_t mask = (uint64_t)1 << bit;

    if((bits & mask) == 0) {
        return false;
    }

    uint8_t rank = (uint8_t)(mjb_unicode_name_page_ranks[compact_page] >> (word * 8));
    uint32_t starts = mjb_unicode_name_page_starts[compact_page];
    *index = (starts & 0xFFFF) + rank + mjb_unicode_popcount64(bits & (mask - 1));
    *prefix_start = (uint16_t)(starts >> 16);

    return true;
}
#endif

static bool mjb_unicode_bitset_get(const uint8_t *data, size_t index) {
    return (data[index >> 3] & (uint8_t)(1u << (index & 7))) != 0;
}

bool mjb_unicode_name_lookup(mjb_codepoint codepoint, char *name, size_t name_size) {
#if MJB_FEATURE_CHARACTER_NAMES
    size_t entry_index = 0;
    uint16_t prefix_start = 0;

    if(!mjb_unicode_name_entry_lookup(codepoint, &entry_index, &prefix_start)) {
        return false;
    }

    if(name_size == 0) {
        return true;
    }

    name[0] = '\0';

    size_t index = 0;
    uint8_t tag = mjb_unicode_name_tags[entry_index];
    uint32_t name_offset = mjb_unicode_name_offsets[entry_index] | ((uint32_t)(tag & 1) << 16);
    uint8_t prefix = tag >> 1;

    const uint8_t *prefix_data = mjb_unicode_prefix_lookup(prefix_start, prefix);
    const uint8_t *name_data = &mjb_unicode_name_data[name_offset];

    if(name_size >= 128) {
        index = mjb_append_table_string_unchecked(name, index, prefix_data);
        mjb_append_table_string_unchecked(name, index, name_data);
    } else {
        index = mjb_append_table_string(name, name_size, index, prefix_data);
        mjb_append_table_string(name, name_size, index, name_data);
    }

    return true;
#else
    if(name_size == 0) {
        return true;
    }

    snprintf(name, name_size, "Codepoint U+%04X", (unsigned int)codepoint);

    return true;
#endif
}

bool mjb_unicode_block_lookup(mjb_codepoint codepoint, mjb_block_info *block) {
    size_t low = 0;
    size_t high = MJB_COUNT_OF(mjb_unicode_blocks);

    while(low < high) {
        size_t mid = low + (high - low) / 2;
        uint64_t entry = mjb_unicode_blocks[mid];
        mjb_codepoint start = (mjb_codepoint)(entry & 0x1FFFFF);
        mjb_codepoint end = start + (mjb_codepoint)((entry >> 21) & 0xFFFF);
        uint16_t id = (uint16_t)((entry >> 37) & 0x1FF);
        uint16_t name_offset = (uint16_t)((entry >> 46) & 0xFFF);
        uint8_t name_chunk = (uint8_t)((entry >> 58) & 0x3F);

        if(codepoint < start) {
            high = mid;
        } else if(codepoint > end) {
            low = mid + 1;
        } else {
            block->id = (mjb_block)id;
            block->start = start;
            block->end = end;
            mjb_copy_table_string(block->name, sizeof(block->name),
                &mjb_unicode_block_name_data[name_chunk][name_offset]);

            return true;
        }
    }

    return false;
}

bool mjb_unicode_emoji_lookup(mjb_codepoint codepoint, mjb_emoji_properties *emoji) {
    size_t low = 0;
    size_t high = MJB_COUNT_OF(mjb_unicode_emoji);

    while(low < high) {
        size_t mid = low + (high - low) / 2;
        uint64_t entry = mjb_unicode_emoji[mid];
        mjb_codepoint start = (mjb_codepoint)(entry & 0x1FFFFF);
        mjb_codepoint end = start + (mjb_codepoint)((entry >> 21) & 0xFFFF);
        uint8_t flags = (uint8_t)((entry >> 37) & 0x3F);

        if(codepoint < start) {
            high = mid;
        } else if(codepoint > end) {
            low = mid + 1;
        } else {
            emoji->codepoint = codepoint;
            emoji->emoji = (flags & MJB_UNICODE_EMOJI_FLAG_EMOJI) != 0;
            emoji->presentation = (flags & MJB_UNICODE_EMOJI_FLAG_PRESENTATION) != 0;
            emoji->modifier = (flags & MJB_UNICODE_EMOJI_FLAG_MODIFIER) != 0;
            emoji->modifier_base = (flags & MJB_UNICODE_EMOJI_FLAG_MODIFIER_BASE) != 0;
            emoji->component = (flags & MJB_UNICODE_EMOJI_FLAG_COMPONENT) != 0;
            emoji->extended_pictographic = (flags & MJB_UNICODE_EMOJI_FLAG_EXTENDED_PICTOGRAPHIC) !=
                0;

            return true;
        }
    }

    return false;
}

bool mjb_unicode_emoji_sequence_lookup(const mjb_codepoint *codepoints, size_t count,
    mjb_emoji_sequence *emoji) {
    if(codepoints == NULL || emoji == NULL || count == 0) {
        return false;
    }

    uint16_t node = 0;

    for(size_t i = 0; i < count; ++i) {
        mjb_unicode_emoji_sequence_node entry = mjb_unicode_emoji_sequence_nodes[node];
        size_t edge_start = (size_t)(entry & MJB_UNICODE_EMOJI_SEQUENCE_EDGE_START_MASK);
        size_t edge_count = (size_t)((entry >> MJB_UNICODE_EMOJI_SEQUENCE_EDGE_COUNT_SHIFT) &
            MJB_UNICODE_EMOJI_SEQUENCE_EDGE_COUNT_MASK);
        size_t low = edge_start;
        size_t high = edge_start + edge_count;
        bool found = false;

        while(low < high) {
            size_t mid = low + (high - low) / 2;
            mjb_codepoint edge = mjb_unicode_emoji_sequence_codepoints[mid];

            if(codepoints[i] < edge) {
                high = mid;
            } else if(codepoints[i] > edge) {
                low = mid + 1;
            } else {
                node = mjb_unicode_emoji_sequence_children[mid];
                found = true;

                break;
            }
        }

        if(!found) {
            return false;
        }
    }

    mjb_unicode_emoji_sequence_node entry = mjb_unicode_emoji_sequence_nodes[node];
    mjb_emoji_sequence_type
        type = (mjb_emoji_sequence_type)((entry >> MJB_UNICODE_EMOJI_SEQUENCE_TYPE_SHIFT) &
            MJB_UNICODE_EMOJI_SEQUENCE_TYPE_MASK);
    mjb_emoji_qualification qualification =
        (mjb_emoji_qualification)((entry >> MJB_UNICODE_EMOJI_SEQUENCE_QUALIFICATION_SHIFT) &
            MJB_UNICODE_EMOJI_SEQUENCE_QUALIFICATION_MASK);

    if(type == MJB_EMOJI_SEQUENCE_NONE && qualification == MJB_EMOJI_QUALIFICATION_NONE) {
        return false;
    }

    emoji->type = type;
    emoji->qualification = qualification;
    emoji->codepoint_count = count;

    return true;
}

static bool mjb_unicode_blob_has_property(const uint8_t *blob, uint16_t blob_size,
    mjb_property property, uint8_t *value) {
    uint16_t offset = 0;
    uint8_t bool_count = blob[offset++];

    for(uint8_t i = 0; i < bool_count && offset < blob_size; ++i) {
        if(blob[offset++] == property) {
            return true;
        }
    }

    if(offset >= blob_size) {
        return false;
    }

    uint8_t enum_count = blob[offset++];

    for(uint8_t i = 0; i < enum_count && (offset + 1) < blob_size; ++i) {
        if(blob[offset] == property) {
            if(value != NULL) {
                *value = blob[offset + 1];
            }

            return true;
        }

        offset += 2;
    }

    return false;
}

static void mjb_unicode_decode_properties(const uint8_t *blob, uint16_t blob_size,
    uint8_t *buffer) {
    uint16_t offset = 0;
    uint8_t bool_count = blob[offset++];

    for(uint8_t i = 0; i < bool_count && offset < blob_size; ++i) {
        buffer[blob[offset++]] = 1;
    }

    if(offset >= blob_size) {
        return;
    }

    uint8_t enum_count = blob[offset++];

    for(uint8_t i = 0; i < enum_count && (offset + 1) < blob_size; ++i) {
        buffer[blob[offset]] = blob[offset + 1];
        offset += 2;
    }
}

static bool mjb_unicode_property_page_lookup(mjb_codepoint codepoint, size_t *start,
    size_t *count) {
    uint16_t page = (uint16_t)(codepoint >> 8);
    size_t low = 0;
    size_t high = MJB_COUNT_OF(mjb_unicode_property_page_numbers);

    while(low < high) {
        size_t mid = low + (high - low) / 2;
        uint16_t entry_page = mjb_unicode_property_page_numbers[mid];

        if(page < entry_page) {
            high = mid;
        } else if(page > entry_page) {
            low = mid + 1;
        } else {
            *start = mjb_unicode_property_page_starts[mid];
            *count = mjb_unicode_property_page_counts[mid];

            return true;
        }
    }

    return false;
}

bool mjb_unicode_has_property(mjb_codepoint codepoint, mjb_property property, uint8_t *value) {
    size_t start_index = 0;
    size_t count = 0;

    if(!mjb_unicode_property_page_lookup(codepoint, &start_index, &count)) {
        return false;
    }

    uint8_t codepoint_low = (uint8_t)codepoint;

    for(size_t i = start_index; i < start_index + count; ++i) {
        uint32_t entry = mjb_unicode_property_ranges[i];
        uint8_t start = (uint8_t)(entry & 0xFF);
        uint8_t end = start + (uint8_t)((entry >> 8) & 0xFF);
        uint16_t offset = (uint16_t)(entry >> 16);
        uint8_t length = mjb_unicode_property_data[offset];

        if(start > codepoint_low) {
            break;
        }

        if(codepoint_low > end || length < 2) {
            continue;
        }

        if(mjb_unicode_blob_has_property(&mjb_unicode_property_data[offset + 1], length, property,
               value)) {
            return true;
        }
    }

    return false;
}

bool mjb_unicode_properties(mjb_codepoint codepoint, uint8_t *buffer) {
    size_t start_index = 0;
    size_t count = 0;

    if(!mjb_unicode_property_page_lookup(codepoint, &start_index, &count)) {
        return true;
    }

    uint8_t codepoint_low = (uint8_t)codepoint;

    for(size_t i = start_index; i < start_index + count; ++i) {
        uint32_t entry = mjb_unicode_property_ranges[i];
        uint8_t start = (uint8_t)(entry & 0xFF);
        uint8_t end = start + (uint8_t)((entry >> 8) & 0xFF);
        uint16_t offset = (uint16_t)(entry >> 16);
        uint8_t length = mjb_unicode_property_data[offset];

        if(start > codepoint_low) {
            break;
        }

        if(codepoint_low > end || length < 2) {
            continue;
        }

        mjb_unicode_decode_properties(&mjb_unicode_property_data[offset + 1], length, buffer);
    }

    return true;
}

bool mjb_unicode_script_extensions_lookup(mjb_codepoint codepoint, const uint8_t **scripts,
    uint8_t *count) {
    size_t low = 0;
    size_t high = MJB_COUNT_OF(mjb_unicode_script_extensions);

    while(low < high) {
        size_t mid = low + (high - low) / 2;
        uint64_t entry = mjb_unicode_script_extensions[mid];
        mjb_codepoint start = (mjb_codepoint)(entry & 0x1FFFFF);
        mjb_codepoint end = start + (mjb_codepoint)((entry >> 21) & 0x1FFFFF);

        if(codepoint < start) {
            high = mid;
        } else if(codepoint > end) {
            low = mid + 1;
        } else {
            uint16_t offset = (uint16_t)((entry >> 42) & 0xFFFF);
            *scripts = &mjb_unicode_script_extension_data[offset];
            *count = (uint8_t)(((entry >> 58) & 0x3F) + 1);
            return true;
        }
    }
    return false;
}

static bool mjb_unicode_n_character_entry_lookup(mjb_codepoint codepoint, size_t *index) {
    size_t page = codepoint >> 8;

    if(page >= MJB_COUNT_OF(mjb_unicode_n_character_page_index)) {
        return false;
    }

    uint8_t compact_page = mjb_unicode_n_character_page_index[page];

    if(compact_page == 0xFF) {
        return false;
    }

    size_t low = mjb_unicode_n_character_pages[compact_page].start;
    size_t high = low + mjb_unicode_n_character_pages[compact_page].count;

    while(low < high) {
        size_t mid = low + (high - low) / 2;
        uint32_t range = mjb_unicode_n_character_ranges[mid];
        mjb_codepoint start = range & 0x1FFFFF;
        mjb_codepoint end = start + (range >> 21);

        if(codepoint < start) {
            high = mid;
        } else if(codepoint > end) {
            low = mid + 1;
        } else {
            *index = mid;

            return true;
        }
    }

    return false;
}

bool mjb_unicode_n_character_lookup(mjb_codepoint codepoint, mjb_n_character *character) {
    size_t entry_index = 0;

    if(!mjb_unicode_n_character_entry_lookup(codepoint, &entry_index)) {
        return false;
    }

    uint32_t entry = mjb_unicode_n_character_entries[entry_index];

    character->codepoint = codepoint;
    character->combining = (uint8_t)((entry >> 14) & 0xFF);
    character->decomposition = (uint8_t)((entry >> 27) & 0x1F);
    character->quick_check = (uint16_t)(entry & 0x1FF);

    return true;
}

bool mjb_unicode_category_lookup(mjb_codepoint codepoint, mjb_category *category) {
    size_t entry_index = 0;

    if(!mjb_unicode_n_character_entry_lookup(codepoint, &entry_index)) {
        return false;
    }

    uint32_t entry = mjb_unicode_n_character_entries[entry_index];
    *category = (mjb_category)((entry >> 9) & 0x1F);

    return true;
}

bool mjb_unicode_codepoint_assigned(mjb_codepoint codepoint) {
    size_t entry_index = 0;

    return mjb_unicode_n_character_entry_lookup(codepoint, &entry_index);
}

bool mjb_unicode_bidi_lookup(mjb_codepoint codepoint, mjb_bidi_class *bidi, bool *mirrored) {
    size_t entry_index = 0;

    if(!mjb_unicode_n_character_entry_lookup(codepoint, &entry_index)) {
        return false;
    }

    uint32_t entry = mjb_unicode_n_character_entries[entry_index];
    uint8_t bidirectional = (uint8_t)((entry >> 22) & 0x1F);

    if(bidirectional <= 0 || bidirectional >= MJB_BIDI_CLASS_COUNT) {
        *bidi = MJB_PR_BIDI_CLASS_L;
    } else {
        *bidi = (mjb_bidi_class)bidirectional;
    }

    *mirrored = mjb_unicode_bitset_get(mjb_unicode_n_character_mirrored, entry_index);

    return true;
}

bool mjb_unicode_numeric_value_lookup(mjb_codepoint codepoint, mjb_numeric_value *value) {
    size_t entry_index = 0;

    if(!mjb_unicode_page_lookup(mjb_unicode_numeric_page_index,
           MJB_COUNT_OF(mjb_unicode_numeric_page_index), mjb_unicode_numeric_pages,
           mjb_unicode_numeric_lows, codepoint, &entry_index)) {
        return false;
    }

    uint32_t entry = mjb_unicode_numeric_value_data[mjb_unicode_numeric_values[entry_index]];
    uint8_t decimal = (uint8_t)((entry >> 10) & 0xF);
    uint8_t digit = (uint8_t)((entry >> 14) & 0xF);

    value->decimal = decimal == 0 ? MJB_NUMBER_NOT_VALID : (int8_t)(decimal - 1);
    value->digit = digit == 0 ? MJB_NUMBER_NOT_VALID : (int8_t)(digit - 1);
    mjb_copy_table_string(value->numeric, sizeof(value->numeric),
        &mjb_unicode_numeric_data[entry & 0x3FF]);

    return true;
}

static bool mjb_unicode_simple_case_entry_lookup(mjb_codepoint codepoint, const uint64_t **entry) {
    size_t entry_index = 0;

    if(!mjb_unicode_page_bitset_lookup(mjb_unicode_simple_case_page_index,
           MJB_COUNT_OF(mjb_unicode_simple_case_page_index), mjb_unicode_simple_case_page_starts,
           mjb_unicode_simple_case_page_bits, mjb_unicode_simple_case_page_ranks, codepoint,
           &entry_index)) {
        return false;
    }

    *entry = &mjb_unicode_simple_case_mapping_data[mjb_unicode_simple_case_mappings[entry_index]];

    return true;
}

static int32_t mjb_unicode_simple_case_delta(uint64_t entry, uint8_t shift) {
    uint32_t value = (uint32_t)((entry >> shift) & 0x3FFFF);

    return (value & 0x20000) != 0 ? (int32_t)(value | 0xFFFC0000) : (int32_t)value;
}

bool mjb_unicode_case_lookup(mjb_codepoint codepoint, mjb_unicode_case_mapping *mapping) {
    size_t character_index = 0;

    if(!mjb_unicode_n_character_entry_lookup(codepoint, &character_index)) {
        return false;
    }

    uint32_t character = mjb_unicode_n_character_entries[character_index];
    const uint64_t *entry = NULL;
    bool has_case_mapping = mjb_unicode_simple_case_entry_lookup(codepoint, &entry);
    uint64_t entry_data = has_case_mapping ? *entry : 0;
    uint8_t mask = (uint8_t)((entry_data >> 54) & 0x7);

    mapping->category = (mjb_category)((character >> 9) & 0x1F);
    mapping->uppercase = has_case_mapping && (mask & 1) != 0 ?
        (mjb_codepoint)((int32_t)codepoint + mjb_unicode_simple_case_delta(entry_data, 0)) :
        0;
    mapping->lowercase = has_case_mapping && (mask & 2) != 0 ?
        (mjb_codepoint)((int32_t)codepoint + mjb_unicode_simple_case_delta(entry_data, 18)) :
        0;
    mapping->titlecase = has_case_mapping && (mask & 4) != 0 ?
        (mjb_codepoint)((int32_t)codepoint + mjb_unicode_simple_case_delta(entry_data, 36)) :
        0;

    return true;
}

bool mjb_unicode_special_casing_lookup(mjb_codepoint codepoint, mjb_case_type case_type,
    const mjb_codepoint **values, uint8_t *length) {
    size_t low = 0;
    size_t high = MJB_COUNT_OF(mjb_unicode_special_case_mappings);

    while(low < high) {
        size_t mid = low + (high - low) / 2;
        uint32_t entry = mjb_unicode_special_case_mappings[mid];
        mjb_codepoint entry_codepoint = (mjb_codepoint)(entry & 0x1FFFFF);
        uint8_t entry_case_type = (uint8_t)((entry >> 21) & 0x7);

        if(codepoint < entry_codepoint ||
            (codepoint == entry_codepoint && case_type < entry_case_type)) {
            high = mid;
        } else if(codepoint > entry_codepoint ||
            (codepoint == entry_codepoint && case_type > entry_case_type)) {
            low = mid + 1;
        } else {
            *values = &mjb_unicode_special_case_data[mjb_unicode_special_case_offsets[mid]];
            *length = (uint8_t)((entry >> 24) & 0x3);

            return true;
        }
    }

    return false;
}

bool mjb_unicode_case_folding_lookup(mjb_codepoint codepoint, const mjb_codepoint **values,
    uint8_t *length) {
    size_t low = 0;
    size_t high = MJB_COUNT_OF(mjb_unicode_case_fold_mappings);

    while(low < high) {
        size_t mid = low + (high - low) / 2;
        uint32_t entry = mjb_unicode_case_fold_mappings[mid];
        mjb_codepoint entry_codepoint = (mjb_codepoint)(entry & 0x1FFFFF);

        if(codepoint < entry_codepoint) {
            high = mid;
        } else if(codepoint > entry_codepoint) {
            low = mid + 1;
        } else {
            *values = &mjb_unicode_case_fold_data[entry >> 23];
            *length = (uint8_t)((entry >> 21) & 0x3);

            return true;
        }
    }

    return false;
}

// Simple (S) case folds: single-char alternatives for codepoints whose full fold is multi-char.
bool mjb_unicode_case_folding_simple_lookup(mjb_codepoint codepoint, mjb_codepoint *value) {
    size_t low = 0;

    if(codepoint <= 0xFFFF) {
        size_t high = MJB_COUNT_OF(mjb_unicode_case_fold_simple_mappings);

        while(low < high) {
            size_t mid = low + (high - low) / 2;
            uint32_t entry = mjb_unicode_case_fold_simple_mappings[mid];
            mjb_codepoint entry_codepoint = (mjb_codepoint)(entry >> 16);

            if(codepoint < entry_codepoint) {
                high = mid;
            } else if(codepoint > entry_codepoint) {
                low = mid + 1;
            } else {
                *value = (mjb_codepoint)(entry & 0xFFFF);

                return true;
            }
        }

        return false;
    }

    size_t high = MJB_COUNT_OF(mjb_unicode_case_fold_simple_supplementary_mappings);

    while(low < high) {
        size_t mid = low + (high - low) / 2;
        uint64_t entry = mjb_unicode_case_fold_simple_supplementary_mappings[mid];
        mjb_codepoint entry_codepoint = (mjb_codepoint)(entry >> 21);

        if(codepoint < entry_codepoint) {
            high = mid;
        } else if(codepoint > entry_codepoint) {
            low = mid + 1;
        } else {
            *value = (mjb_codepoint)(entry & 0x1FFFFF);

            return true;
        }
    }

    return false;
}

bool mjb_unicode_confusable_lookup(mjb_codepoint codepoint, const mjb_codepoint **values,
    uint8_t *length) {
    size_t entry_index = 0;

    if(!mjb_unicode_page_bitset_lookup(mjb_unicode_confusable_page_index,
           MJB_COUNT_OF(mjb_unicode_confusable_page_index), mjb_unicode_confusable_page_starts,
           mjb_unicode_confusable_page_bits, mjb_unicode_confusable_page_ranks, codepoint,
           &entry_index)) {
        return false;
    }

    uint16_t entry = mjb_unicode_confusables[entry_index];
    uint16_t offset = entry & 0x1FFF;
    uint8_t encoded_length = (uint8_t)(entry >> 13);

    *values = &mjb_unicode_confusable_data[offset];
    *length = encoded_length == 7 && offset == 0 ? MJB_UNICODE_CONFUSABLE_LONG_LENGTH :
                                                   encoded_length + 1;

    return true;
}

bool mjb_unicode_collation_implicit_lookup(mjb_codepoint codepoint, uint16_t *base,
    mjb_codepoint *offset) {
    for(size_t i = 0; i < MJB_UNICODE_COLLATION_IMPLICIT_RANGE_COUNT; ++i) {
        const mjb_unicode_collation_implicit_range
            *range = &mjb_unicode_collation_implicit_ranges[i];

        if(codepoint >= range->start && codepoint <= range->end) {
            *base = range->base;
            *offset = range->offset;

            return true;
        }
    }

    return false;
}

bool mjb_unicode_collation_entry_lookup(mjb_codepoint codepoint, const uint32_t **weights) {
    size_t entry_index = 0;

    if(!mjb_unicode_page_bitset_lookup(mjb_unicode_collation_page_index,
           MJB_COUNT_OF(mjb_unicode_collation_page_index), mjb_unicode_collation_page_starts,
           mjb_unicode_collation_page_bits, mjb_unicode_collation_page_ranks, codepoint,
           &entry_index)) {
        return false;
    }

    *weights = &mjb_unicode_collation_weight_data[mjb_unicode_collation_entry_offsets[entry_index]];

    return true;
}

bool mjb_unicode_collation_contraction_range(mjb_codepoint first_codepoint,
    const mjb_unicode_collation_contraction_entry **entries, size_t *count) {
    size_t low = 0;
    size_t high = MJB_COUNT_OF(mjb_unicode_collation_contraction_first_codepoints);

    while(low < high) {
        size_t mid = low + (high - low) / 2;
        mjb_codepoint entry_first = mjb_unicode_collation_contraction_first_codepoints[mid];

        if(first_codepoint < entry_first) {
            high = mid;
        } else if(first_codepoint > entry_first) {
            low = mid + 1;
        } else {
            uint16_t range = mjb_unicode_collation_contraction_ranges[mid];
            size_t start = range & 0x3FF;

            *entries = &mjb_unicode_collation_contractions[start];
            *count = (range >> 10) + 1;

            return true;
        }
    }

    *entries = NULL;
    *count = 0;

    return false;
}

const mjb_codepoint *
mjb_unicode_collation_contraction_sequence(const mjb_unicode_collation_contraction_entry *entry,
    uint8_t *length) {
    uint32_t entry_data = *entry;
    uint8_t offset = (uint8_t)entry_data;

    *length = (uint8_t)(((entry_data >> 21) & 0x1) + 1);

    return &mjb_unicode_collation_contraction_sequence_data[offset];
}

const uint8_t *
mjb_unicode_collation_contraction_weights(const mjb_unicode_collation_contraction_entry *entry,
    uint8_t *length) {
    uint32_t entry_data = *entry;
    uint16_t offset = (uint16_t)((entry_data >> 8) & 0x1FFF);

    *length = mjb_unicode_collation_contraction_weight_lengths[(entry_data >> 22) & 0x3];

    return &mjb_unicode_collation_contraction_weight_data[offset];
}

static bool mjb_unicode_decomposition_table_lookup(const uint8_t *page_index, size_t page_count,
    const uint16_t *page_starts, const uint64_t *page_bits, const uint32_t *page_ranks,
    const uint16_t *mappings, const uint16_t *exception_indices, const uint8_t *exception_lengths,
    size_t exception_count, mjb_codepoint codepoint, const mjb_codepoint **values,
    uint8_t *length) {
    size_t entry_index = 0;

    if(!mjb_unicode_page_bitset_lookup(page_index, page_count, page_starts, page_bits, page_ranks,
           codepoint, &entry_index)) {
        return false;
    }

    uint16_t mapping = mappings[entry_index];
    uint8_t mapping_length = (uint8_t)((mapping >> 13) + 1);

    // The three-bit inline length uses 8 as an escape. Unicode currently has a single longer
    // compatibility decomposition, so this loop is absent from every normal lookup.
    if(mapping_length == 8) {
        for(size_t i = 0; i < exception_count; ++i) {
            if(exception_indices[i] == entry_index) {
                mapping_length = exception_lengths[i];
                break;
            }
        }
    }

    *values = &mjb_unicode_decomposition_data[mapping & 0x1FFF];
    *length = mapping_length;

    return true;
}

bool mjb_unicode_decomposition_lookup(mjb_codepoint codepoint, bool compatibility,
    const mjb_codepoint **values, uint8_t *length) {
    if(compatibility) {
        return mjb_unicode_decomposition_table_lookup(
            mjb_unicode_compatibility_decomposition_page_index,
            MJB_COUNT_OF(mjb_unicode_compatibility_decomposition_page_index),
            mjb_unicode_compatibility_decomposition_page_starts,
            mjb_unicode_compatibility_decomposition_page_bits,
            mjb_unicode_compatibility_decomposition_page_ranks,
            mjb_unicode_compatibility_decompositions,
            mjb_unicode_compatibility_decomposition_exception_indices,
            mjb_unicode_compatibility_decomposition_exception_lengths,
            MJB_UNICODE_COMPATIBILITY_DECOMPOSITION_EXCEPTION_COUNT, codepoint, values, length);
    }

    return mjb_unicode_decomposition_table_lookup(mjb_unicode_canonical_decomposition_page_index,
        MJB_COUNT_OF(mjb_unicode_canonical_decomposition_page_index),
        mjb_unicode_canonical_decomposition_page_starts,
        mjb_unicode_canonical_decomposition_page_bits,
        mjb_unicode_canonical_decomposition_page_ranks, mjb_unicode_canonical_decompositions,
        mjb_unicode_canonical_decomposition_exception_indices,
        mjb_unicode_canonical_decomposition_exception_lengths,
        MJB_UNICODE_CANONICAL_DECOMPOSITION_EXCEPTION_COUNT, codepoint, values, length);
}

mjb_codepoint mjb_unicode_compose_pair(mjb_codepoint starter, mjb_codepoint combining) {
    size_t low = 0;
    size_t high = MJB_COUNT_OF(mjb_unicode_compositions);

    while(low < high) {
        size_t mid = low + (high - low) / 2;
        uint64_t entry = mjb_unicode_compositions[mid];
        mjb_codepoint entry_starter = (mjb_codepoint)(entry & 0x1FFFFF);
        mjb_codepoint entry_combining = (mjb_codepoint)((entry >> 21) & 0x1FFFFF);
        mjb_codepoint entry_composite = (mjb_codepoint)((entry >> 42) & 0x1FFFFF);

        if(starter < entry_starter || (starter == entry_starter && combining < entry_combining)) {
            high = mid;
        } else if(starter > entry_starter ||
            (starter == entry_starter && combining > entry_combining)) {
            low = mid + 1;
        } else {
            return entry_composite;
        }
    }

    return MJB_CODEPOINT_NOT_VALID;
}
