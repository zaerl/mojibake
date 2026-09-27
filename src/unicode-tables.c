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
// Appends one character when it fits, leaving room for the terminator.
static size_t mjb_unicode_name_append(char *name, size_t name_size, size_t index, char value) {
    if(index + 1 < name_size) {
        name[index] = value;

        return index + 1;
    }

    return index;
}

// Appends a word of the given length. With room for a whole word the copy is three fixed
// 8-byte pieces (word tables are padded for that). Otherwise it is truncated to the room left.
static size_t mjb_unicode_name_append_word(char *name, size_t name_size, size_t index,
    const uint8_t *word, size_t length) {
    size_t room = name_size - 1 - index;

    if(room >= MJB_UNICODE_NAME_WORD_MAX) {
        memcpy(name + index, word, 8);
        memcpy(name + index + 8, word + 8, 8);
        memcpy(name + index + 16, word + 16, 8);

        return index + length;
    }

    if(length > room) {
        length = room;
    }

    memcpy(name + index, word, length);

    return index + length;
}

// Decodes a name token stream. Words are lexicon ids or spelled inline, and a space is implied
// between two consecutive words; hyphens and explicit spaces carry no implied space.
static size_t mjb_unicode_name_decode(const uint8_t *tokens, char *name, size_t name_size,
    size_t index, bool *after_word) {
    for(;;) {
        uint8_t token = *tokens++;

        if(token == MJB_UNICODE_NAME_TOKEN_END) {
            return index;
        }

        if(token < MJB_UNICODE_NAME_TOKEN_SPELL_FIRST) {
            index = mjb_unicode_name_append(name, name_size, index,
                token == MJB_UNICODE_NAME_TOKEN_HYPHEN ? '-' : ' ');
            *after_word = false;

            continue;
        }

        if(*after_word) {
            index = mjb_unicode_name_append(name, name_size, index, ' ');
        }

        *after_word = true;

        if(token < MJB_UNICODE_NAME_TOKEN_WORD_FIRST) {
            size_t length = (size_t)(token - MJB_UNICODE_NAME_TOKEN_SPELL_FIRST) + 1;

            index = mjb_unicode_name_append_word(name, name_size, index, tokens, length);
            tokens += length;

            continue;
        }

        size_t id;

        if(token < MJB_UNICODE_NAME_TOKEN_LEAD_FIRST) {
            id = (size_t)(token - MJB_UNICODE_NAME_TOKEN_WORD_FIRST);
        } else {
            id = (MJB_UNICODE_NAME_TOKEN_LEAD_FIRST - MJB_UNICODE_NAME_TOKEN_WORD_FIRST) +
                (((size_t)(token - MJB_UNICODE_NAME_TOKEN_LEAD_FIRST) << 8) | *tokens++);
        }

        const uint8_t *word = &mjb_unicode_name_lexicon_data[mjb_unicode_name_lexicon_offsets[id]];

        index = mjb_unicode_name_append_word(name, name_size, index, word + 1, word[0]);
    }
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

// Finds the entry index of a codepoint in one populated page: the entries before its bitset
// word, plus the set bits below it in that word.
static bool mjb_unicode_bitset_page_lookup(const mjb_unicode_bitset_page *page,
    mjb_codepoint codepoint, size_t *index) {
    uint8_t codepoint_low = (uint8_t)codepoint;
    uint8_t word = codepoint_low >> 6;
    uint8_t bit = codepoint_low & 0x3F;
    uint64_t bits = page->bits[word];
    uint64_t mask = (uint64_t)1 << bit;

    if((bits & mask) == 0) {
        return false;
    }

    uint8_t rank = (uint8_t)(page->ranks >> (word * 8));
    *index = page->start + rank + mjb_unicode_popcount64(bits & (mask - 1));

    return true;
}

static bool mjb_unicode_page_bitset_lookup(const uint8_t *page_index, size_t page_count,
    const mjb_unicode_bitset_page *pages, mjb_codepoint codepoint, size_t *index) {
    size_t page = codepoint >> 8;

    if(page >= page_count) {
        return false;
    }

    uint8_t compact_page = page_index[page];

    if(compact_page == 0xFF) {
        return false;
    }

    return mjb_unicode_bitset_page_lookup(&pages[compact_page], codepoint, index);
}

#if MJB_FEATURE_SECURITY
// Wide variant for tables with 255 or more populated pages; 0xFFFF marks an empty page.
static bool mjb_unicode_page_bitset_lookup_wide(const uint16_t *page_index, size_t page_count,
    const mjb_unicode_bitset_page *pages, mjb_codepoint codepoint, size_t *index) {
    size_t page = codepoint >> 8;

    if(page >= page_count) {
        return false;
    }

    uint16_t compact_page = page_index[page];

    if(compact_page == 0xFFFF) {
        return false;
    }

    return mjb_unicode_bitset_page_lookup(&pages[compact_page], codepoint, index);
}
#endif // MJB_FEATURE_SECURITY

#if MJB_FEATURE_CHARACTER_NAMES
// Finds a name entry and the compact index of its page.
static bool mjb_unicode_name_entry_lookup(mjb_codepoint codepoint, size_t *index,
    size_t *compact_page) {
    size_t page = codepoint >> 8;

    if(page >= MJB_COUNT_OF(mjb_unicode_name_page_index)) {
        return false;
    }

    uint8_t compact = mjb_unicode_name_page_index[page];

    if(compact == 0xFF) {
        return false;
    }

    if(!mjb_unicode_bitset_page_lookup(&mjb_unicode_name_pages[compact], codepoint, index)) {
        return false;
    }

    *compact_page = compact;

    return true;
}
#endif

// The generated tables must not exceed the caller buffer size promised by unicode-tables.h.
typedef char
    mjb_unicode_sequence_max_check[MJB_UNICODE_SEQUENCE_LONGEST <= MJB_UNICODE_SEQUENCE_MAX ? 1 :
                                                                                              -1];

// Decodes codepoints from 16-bit sequence units: BMP values directly, others as surrogate pairs.
static void mjb_unicode_sequence_decode(const uint16_t *units, uint8_t count,
    mjb_codepoint *values) {
    for(uint8_t i = 0; i < count; ++i) {
        uint32_t unit = *units++;

        if((unit & 0xFC00) == 0xD800) {
            uint32_t low = *units++;
            unit = 0x10000 + ((unit - 0xD800) << 10) + (low - 0xDC00);
        }

        values[i] = (mjb_codepoint)unit;
    }
}

#if MJB_FEATURE_IDNA || MJB_FEATURE_SECURITY
// Resolves a virtual sequence offset: the core pool first, then a feature's extension pool.
static const uint16_t *mjb_unicode_sequence_units_at(const uint16_t *extension, size_t offset) {
    if(offset < MJB_COUNT_OF(mjb_unicode_sequence_units)) {
        return &mjb_unicode_sequence_units[offset];
    }

    return &extension[offset - MJB_COUNT_OF(mjb_unicode_sequence_units)];
}
#endif

static bool mjb_unicode_bitset_get(const uint8_t *data, size_t index) {
    return (data[index >> 3] & (uint8_t)(1u << (index & 7))) != 0;
}

bool mjb_unicode_name_lookup(mjb_codepoint codepoint, char *name, size_t name_size) {
#if MJB_FEATURE_CHARACTER_NAMES
    size_t entry_index = 0;
    size_t compact_page = 0;

    if(!mjb_unicode_name_entry_lookup(codepoint, &entry_index, &compact_page)) {
        return false;
    }

    if(name_size == 0) {
        return true;
    }

    // The prefix runs of the page give the local prefix of this entry, starting from the run
    // indexed by the entry's 16-entry block.
    const mjb_unicode_bitset_page *page = &mjb_unicode_name_pages[compact_page];
    size_t local = entry_index - page->start;
    size_t first_run = mjb_unicode_name_page_run_starts[compact_page];
    const uint8_t *run = &mjb_unicode_name_prefix_runs[(first_run +
                                                           mjb_unicode_name_run_block_starts
                                                               [compact_page * 16 + (local >> 4)]) *
        2];
    const uint8_t *end = &mjb_unicode_name_prefix_runs
                             [(size_t)mjb_unicode_name_page_run_starts[compact_page + 1] * 2];

    while(run + 2 < end && run[2] <= local) {
        run += 2;
    }

    const uint8_t *prefix = &mjb_unicode_name_prefix_data
                                [mjb_unicode_name_page_prefix_offsets[page->extra + run[1]]];
    size_t offset = mjb_unicode_name_offsets[entry_index] |
        ((size_t)mjb_unicode_bitset_get(mjb_unicode_name_offset_high_bits, entry_index) << 16);
    bool after_word = false;
    size_t index = mjb_unicode_name_decode(prefix, name, name_size, 0, &after_word);

    index = mjb_unicode_name_decode(&mjb_unicode_name_data[offset], name, name_size, index,
        &after_word);
    name[index] = '\0';

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

// Finds the merged property record of a codepoint: page, then block, then a short run scan.
static const uint8_t *mjb_unicode_property_record(mjb_codepoint codepoint) {
    size_t page = codepoint >> 8;

    if(page >= MJB_COUNT_OF(mjb_unicode_property_page_index)) {
        return NULL;
    }

    uint16_t compact_page = mjb_unicode_property_page_index[page];

    if(compact_page == 0xFFFF) {
        return NULL;
    }

    uint8_t codepoint_low = (uint8_t)codepoint;
    size_t page_start = mjb_unicode_property_page_starts[compact_page];
    size_t end = mjb_unicode_property_page_starts[compact_page + 1];
    size_t run = page_start +
        mjb_unicode_property_block_starts[(size_t)compact_page *
                MJB_UNICODE_PROPERTY_BLOCKS_PER_PAGE +
            (codepoint_low >> MJB_UNICODE_PROPERTY_BLOCK_SHIFT)];

    for(; run < end; ++run) {
        uint16_t range = mjb_unicode_property_runs[run];
        uint8_t start = (uint8_t)range;

        if(codepoint_low < start) {
            return NULL;
        }

        if(codepoint_low <= (uint8_t)(start + (range >> 8))) {
            return &mjb_unicode_property_records[mjb_unicode_property_run_records[run]];
        }
    }

    return NULL;
}

bool mjb_unicode_has_property(mjb_codepoint codepoint, mjb_property property, uint8_t *value) {
    const uint8_t *record = mjb_unicode_property_record(codepoint);

    if(record == NULL) {
        return false;
    }

    // Boolean ids are sorted, so the scan stops at the first larger id.
    uint8_t bool_count = *record++;

    for(uint8_t i = 0; i < bool_count; ++i) {
        uint8_t id = record[i];

        if(id == property) {
            return true;
        }

        if(id > property) {
            break;
        }
    }

    record += bool_count;
    uint8_t enum_count = *record++;

    for(uint8_t i = 0; i < enum_count; ++i) {
        uint8_t id = record[i * 2];

        if(id == property) {
            if(value != NULL) {
                *value = record[i * 2 + 1];
            }

            return true;
        }

        if(id > property) {
            break;
        }
    }

    return false;
}

bool mjb_unicode_properties(mjb_codepoint codepoint, uint8_t *buffer) {
    const uint8_t *record = mjb_unicode_property_record(codepoint);

    if(record == NULL) {
        return true;
    }

    uint8_t bool_count = *record++;

    for(uint8_t i = 0; i < bool_count; ++i) {
        buffer[record[i]] = 1;
    }

    record += bool_count;
    uint8_t enum_count = *record++;

    for(uint8_t i = 0; i < enum_count; ++i) {
        buffer[record[i * 2]] = record[i * 2 + 1];
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

    // Scan the few runs of the codepoint's 16-codepoint block; runs are sorted and disjoint.
    uint8_t codepoint_low = (uint8_t)codepoint;
    size_t page_start = mjb_unicode_n_character_pages[compact_page].start;
    size_t end = page_start + mjb_unicode_n_character_pages[compact_page].count;
    size_t run = page_start +
        mjb_unicode_n_character_block_starts[(size_t)compact_page * 16 + (codepoint_low >> 4)];

    for(; run < end; ++run) {
        uint16_t range = mjb_unicode_n_character_ranges[run];
        uint8_t start = (uint8_t)range;

        if(codepoint_low < start) {
            return false;
        }

        if(codepoint_low <= (uint8_t)(start + (range >> 8))) {
            *index = run;

            return true;
        }
    }

    return false;
}

static uint32_t mjb_unicode_n_character_entry(size_t index) {
    return mjb_unicode_n_character_values[mjb_unicode_n_character_entries[index]];
}

bool mjb_unicode_n_character_lookup(mjb_codepoint codepoint, mjb_n_character *character) {
    size_t entry_index = 0;

    if(!mjb_unicode_n_character_entry_lookup(codepoint, &entry_index)) {
        return false;
    }

    uint32_t entry = mjb_unicode_n_character_entry(entry_index);

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

    uint32_t entry = mjb_unicode_n_character_entry(entry_index);
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

    uint32_t entry = mjb_unicode_n_character_entry(entry_index);
    uint8_t bidirectional = (uint8_t)((entry >> 22) & 0x1F);

    if(bidirectional == 0 || bidirectional >= MJB_BIDI_CLASS_COUNT) {
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
           MJB_COUNT_OF(mjb_unicode_simple_case_page_index), mjb_unicode_simple_case_pages,
           codepoint, &entry_index)) {
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

    uint32_t character = mjb_unicode_n_character_entry(character_index);
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

bool mjb_unicode_special_casing_lookup(mjb_codepoint codepoint, mjb_map_case_type case_type,
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
        } else if(codepoint > entry_codepoint || case_type > entry_case_type) {
            // codepoint >= entry_codepoint here, so this is the greater-than case.
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

#if MJB_FEATURE_SECURITY
bool mjb_unicode_confusable_lookup(mjb_codepoint codepoint, mjb_codepoint *values,
    uint8_t *length) {
    size_t entry_index = 0;

    if(!mjb_unicode_page_bitset_lookup_wide(mjb_unicode_confusable_page_index,
           MJB_COUNT_OF(mjb_unicode_confusable_page_index), mjb_unicode_confusable_pages, codepoint,
           &entry_index)) {
        return false;
    }

    uint8_t mapping_length = (uint8_t)((mjb_unicode_confusable_lengths[entry_index >> 1] >>
                                           ((entry_index & 1) * 4)) &
        0xF);

    // Lengths of 15 and above are rare enough to live in a tiny exception table.
    if(mapping_length == 0xF) {
        for(size_t i = 0; i < MJB_UNICODE_CONFUSABLE_EXCEPTION_COUNT; ++i) {
            if(mjb_unicode_confusable_exception_indices[i] == entry_index) {
                mapping_length = mjb_unicode_confusable_exception_lengths[i];
                break;
            }
        }
    }

    mjb_unicode_sequence_decode(mjb_unicode_sequence_units_at(mjb_unicode_confusable_sequence_units,
                                    mjb_unicode_confusables[entry_index]),
        mapping_length, values);
    *length = mapping_length;

    return true;
}
#endif // MJB_FEATURE_SECURITY

#if MJB_FEATURE_IDNA
// Every codepoint has an IDNA range, so ranges store only their start and a lookup finds the last
// range starting at or before the codepoint.
static bool mjb_unicode_idna_range_lookup(mjb_codepoint codepoint, uint8_t *status,
    uint16_t *mapping_id) {
    size_t low = 0;

    if(codepoint < MJB_UNICODE_IDNA_SUPPLEMENTARY_START) {
        // The page index narrows the search to the ranges touching one 256-codepoint page: from
        // the range covering the page start to the one covering the next page start.
        size_t page = codepoint >> 8;
        low = mjb_unicode_idna_page_firsts[page] + 1;
        size_t high = (size_t)mjb_unicode_idna_page_firsts[page + 1] + 1;

        while(low < high) {
            size_t mid = low + (high - low) / 2;

            if((mjb_unicode_idna_ranges[mid] & 0x1FFFF) <= codepoint) {
                low = mid + 1;
            } else {
                high = mid;
            }
        }

        uint32_t entry = mjb_unicode_idna_ranges[low - 1];
        *status = (uint8_t)((entry >> 17) & 0x7);
        *mapping_id = (uint16_t)(entry >> 20);

        return true;
    }

    size_t high = MJB_COUNT_OF(mjb_unicode_idna_supplementary_ranges);

    while(low < high) {
        size_t mid = low + (high - low) / 2;

        if((mjb_unicode_idna_supplementary_ranges[mid] & 0x1FFFFF) <= codepoint) {
            low = mid + 1;
        } else {
            high = mid;
        }
    }

    if(low == 0) {
        return false;
    }

    uint64_t entry = mjb_unicode_idna_supplementary_ranges[low - 1];
    *status = (uint8_t)((entry >> 21) & 0x7);
    *mapping_id = (uint16_t)((entry >> 24) & 0x1FFF);

    return true;
}

bool mjb_unicode_idna_lookup(mjb_codepoint codepoint, mjb_unicode_idna_status *status,
    mjb_codepoint *mapping, uint8_t *length) {
    uint8_t entry_status = 0;
    uint16_t mapping_id = 0;

    if(codepoint > MJB_CODEPOINT_MAX ||
        !mjb_unicode_idna_range_lookup(codepoint, &entry_status, &mapping_id) ||
        entry_status == MJB_UNICODE_IDNA_NONE) {
        return false;
    }

    *status = (mjb_unicode_idna_status)entry_status;
    *length = 0;

    if(mapping_id != 0) {
        uint8_t mapping_length = mjb_unicode_idna_mapping_lengths[mapping_id - 1];

        mjb_unicode_sequence_decode(mjb_unicode_sequence_units_at(mjb_unicode_idna_sequence_units,
                                        mjb_unicode_idna_mapping_offsets[mapping_id - 1]),
            mapping_length, mapping);
        *length = mapping_length;
    }

    return true;
}
#endif

#if MJB_FEATURE_COLLATION
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

// Rare secondary/tertiary combinations escape to a small table sorted by entry index.
static uint32_t mjb_unicode_collation_combo_exception(size_t entry_index) {
    size_t low = 0;
    size_t high = MJB_UNICODE_COLLATION_COMBO_EXCEPTION_COUNT;

    while(low < high) {
        size_t mid = low + (high - low) / 2;
        size_t mid_index = mjb_unicode_collation_combo_exception_indices[mid];

        if(entry_index < mid_index) {
            high = mid;
        } else if(entry_index > mid_index) {
            low = mid + 1;
        } else {
            return mjb_unicode_collation_combo_exception_values[mid];
        }
    }

    return 0;
}

bool mjb_unicode_collation_entry_lookup(mjb_codepoint codepoint, uint32_t *first_weight,
    const uint32_t **expansion) {
    size_t entry_index = 0;

    if(!mjb_unicode_page_bitset_lookup(mjb_unicode_collation_page_index,
           MJB_COUNT_OF(mjb_unicode_collation_page_index), mjb_unicode_collation_pages, codepoint,
           &entry_index)) {
        return false;
    }

    const uint8_t *entry = &mjb_unicode_collation_entries[entry_index * 3];
    uint8_t combo_code = (uint8_t)(entry[2] & ~MJB_UNICODE_COLLATION_COMBO_EXPANSION_FLAG);
    uint32_t combo = combo_code != MJB_UNICODE_COLLATION_COMBO_ESCAPE ?
        mjb_unicode_collation_combo_values[combo_code] :
        mjb_unicode_collation_combo_exception(entry_index);
    uint32_t weight = (uint32_t)entry[0] | ((uint32_t)entry[1] << 8) | (combo << 16);

    if((entry[2] & MJB_UNICODE_COLLATION_COMBO_EXPANSION_FLAG) != 0) {
        uint64_t bits = mjb_unicode_collation_expansion_bits[entry_index >> 6];
        uint64_t mask = (uint64_t)1 << (entry_index & 0x3F);
        size_t rank = mjb_unicode_collation_expansion_ranks[entry_index >> 6] +
            mjb_unicode_popcount64(bits & (mask - 1));

        *expansion = &mjb_unicode_collation_expansion_weights
                         [mjb_unicode_collation_expansion_offsets[rank]];
    } else {
        // Bit 31 marks a single-element entry, matching the expansion tail terminator.
        weight |= UINT32_C(0x80000000);
        *expansion = NULL;
    }

    *first_weight = weight;

    return true;
}

// Rejects codepoints that never start a contraction with one bitset test.
static bool mjb_unicode_collation_contraction_may_start(mjb_codepoint first_codepoint) {
    if(first_codepoint < MJB_UNICODE_COLLATION_CONTRACTION_LOW_LIMIT) {
        return (mjb_unicode_collation_contraction_start_low_bits[first_codepoint >> 6] &
                   ((uint64_t)1 << (first_codepoint & 0x3F))) != 0;
    }

    size_t page = first_codepoint >> 8;

    if((page >> 6) >= MJB_COUNT_OF(mjb_unicode_collation_contraction_start_page_bits)) {
        return false;
    }

    return (mjb_unicode_collation_contraction_start_page_bits[page >> 6] &
               ((uint64_t)1 << (page & 0x3F))) != 0;
}

bool mjb_unicode_collation_contraction_range(mjb_codepoint first_codepoint,
    const mjb_unicode_collation_contraction_entry **entries, size_t *count) {
    size_t low = 0;
    size_t high = MJB_COUNT_OF(mjb_unicode_collation_contraction_first_codepoints);

    if(!mjb_unicode_collation_contraction_may_start(first_codepoint)) {
        *entries = NULL;
        *count = 0;

        return false;
    }

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
#endif // MJB_FEATURE_COLLATION

static bool mjb_unicode_decomposition_table_lookup(const uint8_t *page_index, size_t page_count,
    const mjb_unicode_bitset_page *pages, const uint16_t *mappings,
    const uint16_t *exception_indices, const uint8_t *exception_lengths, size_t exception_count,
    mjb_codepoint codepoint, mjb_codepoint *values, uint8_t *length) {
    size_t entry_index = 0;

    if(!mjb_unicode_page_bitset_lookup(page_index, page_count, pages, codepoint, &entry_index)) {
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

    mjb_unicode_sequence_decode(&mjb_unicode_sequence_units[mapping & 0x1FFF], mapping_length,
        values);
    *length = mapping_length;

    return true;
}

bool mjb_unicode_decomposition_lookup(mjb_codepoint codepoint, bool compatibility,
    mjb_codepoint *values, uint8_t *length) {
    if(compatibility) {
        return mjb_unicode_decomposition_table_lookup(
            mjb_unicode_compatibility_decomposition_page_index,
            MJB_COUNT_OF(mjb_unicode_compatibility_decomposition_page_index),
            mjb_unicode_compatibility_decomposition_pages, mjb_unicode_compatibility_decompositions,
            mjb_unicode_compatibility_decomposition_exception_indices,
            mjb_unicode_compatibility_decomposition_exception_lengths,
            MJB_UNICODE_COMPATIBILITY_DECOMPOSITION_EXCEPTION_COUNT, codepoint, values, length);
    }

    return mjb_unicode_decomposition_table_lookup(mjb_unicode_canonical_decomposition_page_index,
        MJB_COUNT_OF(mjb_unicode_canonical_decomposition_page_index),
        mjb_unicode_canonical_decomposition_pages, mjb_unicode_canonical_decompositions,
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
        } else if(starter > entry_starter || combining > entry_combining) {
            // starter >= entry_starter here, so this is the greater-than case.
            low = mid + 1;
        } else {
            return entry_composite;
        }
    }

    return MJB_CODEPOINT_NOT_VALID;
}
