/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdlib.h>
#include <string.h>

#include "mojibake-internal.h"
#include "unicode-tables.h"
#include "unicode.h"
#include "utf.h"

// Collation element
typedef struct {
    uint16_t primary;    // 0x0000–0x73C2; 0 = ignorable at L1
    uint16_t secondary;  // 0x0000–0x0127; 0 = ignorable at L2
    uint16_t tertiary;   // 0x0000–0x001F; 0 = ignorable at L3
    uint16_t quaternary; // Filled only in SHIFTED mode
    bool variable;       // Originally marked * in DUCET
} mjb_ce;

// Dynamic collation element array
typedef struct {
    mjb_ce *data;
    size_t count;
    size_t cap;
} mjb_cea;

static bool cea_grow(mjb_cea *a, size_t need) {
    if(need > SIZE_MAX - a->count) {
        return false;
    }

    size_t required = a->count + need;

    if(required <= a->cap) {
        return true;
    }

    size_t new_cap = a->cap == 0 ? 16 : a->cap;

    while(new_cap < required) {
        if(new_cap > SIZE_MAX / 2) {
            return false;
        }

        new_cap *= 2;
    }

    if(new_cap > SIZE_MAX / sizeof(mjb_ce)) {
        return false;
    }

    mjb_ce *p = (mjb_ce*)mjb_realloc(a->data, new_cap * sizeof(mjb_ce));

    if(!p) {
        return false;
    }

    a->data = p;
    a->cap  = new_cap;

    return true;
}

static bool cea_push(mjb_cea *a, uint16_t p, uint16_t s, uint16_t t, bool var) {
    if(!cea_grow(a, 1)) {
        return false;
    }

    a->data[a->count].primary = p;
    a->data[a->count].secondary = s;
    a->data[a->count].tertiary = t;
    a->data[a->count].quaternary = 0;
    a->data[a->count].variable = var;

    ++a->count;

    return true;
}

static void cea_free(mjb_cea *a) {
    mjb_free(a->data);
    a->data  = NULL;
    a->count = 0;
    a->cap   = 0;
}

// Dynamic sort-key array (uint16_t)
typedef struct {
    uint16_t *data;
    size_t    count;
    size_t    cap;
} mjb_sort_key;

static bool sk_push(mjb_sort_key *sk, uint16_t w) {
    if(sk->count >= sk->cap) {
        if(sk->cap > SIZE_MAX / 2) {
            return false;
        }

        size_t new_cap = sk->cap == 0 ? 64 : sk->cap * 2;

        if(new_cap > SIZE_MAX / sizeof(uint16_t)) {
            return false;
        }

        uint16_t *p = (uint16_t*)mjb_realloc(sk->data, new_cap * sizeof(uint16_t));

        if(!p) {
            return false;
        }

        sk->data = p;
        sk->cap  = new_cap;
    }

    sk->data[sk->count++] = w;

    return true;
}

static void sk_free(mjb_sort_key *sk) {
    mjb_free(sk->data);

    sk->data  = NULL;
    sk->count = 0;
    sk->cap   = 0;
}

// @implicitweights ranges from allkeys.txt
static const struct {
    uint32_t start;
    uint32_t end;
    uint16_t base;
} implicit_ranges[] = {
    { 0x17000, 0x187FF, 0xFB00 }, // Tangut
    { 0x18800, 0x18AFF, 0xFB01 }, // Tangut Components
    { 0x18D00, 0x18D7F, 0xFB00 }, // Tangut Supplement
    { 0x18D80, 0x18DFF, 0xFB01 }, // Tangut Components Supplement
    { 0x1B170, 0x1B2FF, 0xFB02 }, // Nushu
    { 0x18B00, 0x18CFF, 0xFB03 }, // Khitan Small Script
};

#define IMPLICIT_RANGES_COUNT ((int)(sizeof(implicit_ranges) / sizeof(implicit_ranges[0])))

/**
 * Append implicit-weight CEs for a codepoint not in the DUCET (UTS#10 §10.1.3).
 *
 * The formula is the same for all implicit types: AAAA = base + (cp >> 15) BBBB = (cp & 0x7FFF) | 0x8000
 *
 * Where `base` is (UTS#10 Table 16):
 *   @implicitweights ranges -> specified base value (FB00, FB01, ...)
 *   CJK Unified Main (4E00-9FFF) -> 0xFB40
 *   CJK Extensions A-J -> 0xFB80  (NOTE: different from main!)
 *   Everything else -> 0xFBC0
 */
static bool cea_append_implicit(mjb_cea *cea, mjb_codepoint cp) {
    uint16_t base = 0xFBC0;
    bool found = false;

    for(int i = 0; i < IMPLICIT_RANGES_COUNT; ++i) {
        if(cp >= implicit_ranges[i].start && cp <= implicit_ranges[i].end) {
            base  = implicit_ranges[i].base;
            found = true;
            break;
        }
    }

    if(!found) {
        // CJK Unified Ideographs main block only -> AAAA = 0xFB40 + (cp >> 15)
        // UTS#10 Table 16: only the 4E00-9FFF block uses base FB40.
        if(cp >= MJB_CJK_IDEOGRAPH_START && cp <= MJB_CJK_IDEOGRAPH_END) {
            base = 0xFB40;
        } else if(mjb_codepoint_is_cjk_ext(cp)) { // UTS#10 Table 16: all extensions use base FB80 (NOT FB40).
            base = 0xFB80;
        } else {
            base = 0xFBC0;
        }
    }

    uint16_t aaaa = (uint16_t)(base + (cp >> 15));
    uint16_t bbbb = (uint16_t)((cp & 0x7FFF) | 0x8000);

    // CE1: [AAAA.0020.0002] non-variable
    if(!cea_push(cea, aaaa, 0x0020, 0x0002, false)) {
        return false;
    }

    // CE2: [BBBB.0000.0000]
    return cea_push(cea, bbbb, 0x0000, 0x0000, false);
}

/**
 * Decode a weight BLOB and append CEs to cea.
 * Format: 6 bytes/element [P_hi P_lo S_hi S_lo T_hi T_lo]
 * Bit 15 of TERTIARY holds the variable flag (primary can reach 0xFBC2).
 */
static bool cea_append_blob(mjb_cea *cea, const uint8_t *blob, int blob_bytes) {
    int n = blob_bytes / 6;

    if(n <= 0 || !cea_grow(cea, (size_t)n)) {
        return n <= 0;
    }

    for(int i = 0; i < n; ++i) {
        uint16_t p = (uint16_t)(((uint16_t)blob[i * 6]     << 8) | blob[i * 6 + 1]);
        uint16_t s = (uint16_t)(((uint16_t)blob[i * 6 + 2] << 8) | blob[i * 6 + 3]);
        uint16_t t = (uint16_t)(((uint16_t)blob[i * 6 + 4] << 8) | blob[i * 6 + 5]);
        bool var = (t & 0x8000) != 0;

        if(var) {
            t &= 0x7FFF;
        }

        cea->data[cea->count].primary = p;
        cea->data[cea->count].secondary = s;
        cea->data[cea->count].tertiary = t;
        cea->data[cea->count].quaternary = 0;
        cea->data[cea->count].variable = var;

        ++cea->count;
    }

    return true;
}

// Decode UTF-8 to a codepoint array

/**
 * Check whether the bytes at buf[i..i+2] are a CESU-8 encoded surrogate (U+D800–U+DFFF). The
 * Unicode collation conformance tests include these codepoints, so decode them back for distinct
 * implicit weights.
 *
 * Pattern: 0xED [0xA0–0xBF] [0x80–0xBF] -> U+D800–U+DFFF
 */
static bool try_cesu8_surrogate(const char *buf, size_t len, size_t i,
    size_t *out_advance, mjb_codepoint *out_cp) {
    if(i + 2 >= len) {
        return false;
    }

    uint8_t b0 = (uint8_t)buf[i];
    uint8_t b1 = (uint8_t)buf[i + 1];
    uint8_t b2 = (uint8_t)buf[i + 2];

    if(b0 != 0xED) {
        return false;
    }

    if(b1 < 0xA0 || b1 > 0xBF) {
        return false;
    }

    if(b2 < 0x80 || b2 > 0xBF) {
        return false;
    }

    *out_cp = (mjb_codepoint)(((uint32_t)(b0 & 0x0F) << 12) |
        ((uint32_t)(b1 & 0x3F) <<  6) |
        (uint32_t)(b2 & 0x3F));
    *out_advance = 3;

    return true;
}

static bool utf8_to_codepoints(const char *buf, size_t len, mjb_codepoint **out_codepoints,
    size_t *out_count) {
    *out_codepoints = NULL;
    *out_count = 0;

    // First pass: count
    size_t count = 0;
    size_t i = 0;
    uint8_t state = MJB_UTF_ACCEPT;
    bool in_error = false;
    mjb_codepoint cp;

    while(i < len) {
        // Intercept CESU-8 encoded surrogates before the regular decoder. See test 229206.
        if(state == MJB_UTF_ACCEPT) {
            size_t adv = 0;
            mjb_codepoint surr = 0;

            if(try_cesu8_surrogate(buf, len, i, &adv, &surr)) {
                i += adv;
                ++count;

                continue;
            }
        }

        mjb_decode_result dr = mjb_next_codepoint(buf, len, &state, &i, MJB_ENC_UTF_8, &cp,
            &in_error);

        if(dr == MJB_DECODE_END) {
            break;
        }

        if(dr != MJB_DECODE_INCOMPLETE) {
            ++count;
        }
    }

    if(count == 0) {
        return true;
    }

    if(count > SIZE_MAX / sizeof(mjb_codepoint)) {
        return false;
    }

    mjb_codepoint *arr = (mjb_codepoint*)mjb_alloc(count * sizeof(mjb_codepoint));

    if(!arr) {
        return false;
    }

    // Second pass: fill
    i = 0;
    state = MJB_UTF_ACCEPT;
    in_error = false;
    size_t j = 0;

    while(i < len && j < count) {
        if(state == MJB_UTF_ACCEPT) {
            size_t adv = 0;
            mjb_codepoint surr = 0;

            if(try_cesu8_surrogate(buf, len, i, &adv, &surr)) {
                i += adv;
                arr[j++] = surr;

                continue;
            }
        }

        mjb_decode_result dr = mjb_next_codepoint(buf, len, &state, &i, MJB_ENC_UTF_8, &cp,
            &in_error);

        if(dr == MJB_DECODE_END) {
            break;
        }

        if(dr != MJB_DECODE_INCOMPLETE) {
            arr[j++] = cp;
        }
    }

    *out_count = j;
    *out_codepoints = arr;

    return true;
}

// Build CEA from NFD codepoint array

// Canonical Combining Class for a codepoint.
static uint8_t ccc_of(mjb_codepoint cp) {
    mjb_n_character character;

    if(mjb_unicode_n_character_lookup(cp, &character)) {
        return (uint8_t)character.combining;
    }

    return 0;
}

// Look up or synthesize CEs for a single codepoint and append to cea.
static bool cea_lookup_or_implicit(mjb_cea *cea, mjb_codepoint cp) {
    const uint8_t *weights = NULL;
    uint8_t length = 0;

    if(mjb_unicode_collation_entry_lookup(cp, &weights, &length)) {
        return cea_append_blob(cea, weights, length);
    } else {
        return cea_append_implicit(cea, cp);
    }
}

/**
 * UTS#10 S2.1 – find the longest CONSECUTIVE contraction starting at cps[pos].
 * Only examines positions pos, pos+1, pos+2, ... (no skip).
 * Returns true and fills out_* if a multi-cp match is found (seq_len >= 2).
 */
static bool consecutive_contraction(const mjb_codepoint *cps, size_t pos, size_t total,
    uint8_t *out_weights, int *out_bytes, size_t *out_advance) {
    const mjb_unicode_collation_contraction_entry *entries = NULL;
    size_t entry_count = 0;

    *out_advance = 0;

    if(!mjb_unicode_collation_contraction_range(cps[pos], &entries, &entry_count)) {
        return false;
    }

    for(size_t entry_index = 0; entry_index < entry_count; ++entry_index) {
        uint8_t sl = 0;
        const mjb_codepoint *seq =
            mjb_unicode_collation_contraction_sequence(&entries[entry_index], &sl);

        if(sl < 2 || pos + (size_t)sl > total) {
            continue;
        }

        bool match = true;

        for(int k = 0; k < sl; ++k) {
            if(cps[pos + k] != seq[k]) {
                match = false;
                break;
            }
        }

        if(match && (size_t)sl > *out_advance) {
            *out_advance = (size_t)sl;
            uint8_t weights_length = 0;
            const uint8_t *weights =
            mjb_unicode_collation_contraction_weights(&entries[entry_index],
                    &weights_length);

            *out_bytes = weights_length;

            if(*out_bytes <= 18 * 6) {
                memcpy(out_weights, weights, (size_t)*out_bytes);
            } else {
                *out_bytes = 0;
            }
        }
    }

    return *out_advance >= 2;
}

/**
 * UTS#10 S2.1.1-S2.1.3 – check if the exact sequence `seq[0..seq_len-1]`
 * has an entry in the DB.  seq_len >= 2 (caller guarantees).
 */
static bool lookup_sequence(const mjb_codepoint *seq, int seq_len,
    uint8_t *out_weights, int *out_bytes) {
    const mjb_unicode_collation_contraction_entry *entries = NULL;
    size_t entry_count = 0;

    if(!mjb_unicode_collation_contraction_range(seq[0], &entries, &entry_count)) {
        return false;
    }

    for(size_t entry_index = 0; entry_index < entry_count; ++entry_index) {
        uint8_t db_len = 0;
        const mjb_codepoint *db_seq =
            mjb_unicode_collation_contraction_sequence(&entries[entry_index], &db_len);

        if(db_len != seq_len) {
            continue;
        }

        bool match = true;

        for(int k = 0; k < seq_len; ++k) {
            if(seq[k] != db_seq[k]) {
                match = false;
                break;
            }
        }

        if(match) {
            uint8_t weights_length = 0;
            const uint8_t *weights =
                mjb_unicode_collation_contraction_weights(&entries[entry_index],
                    &weights_length);

            *out_bytes = weights_length;

            if(*out_bytes <= 18 * 6) {
                memcpy(out_weights, weights, (size_t)*out_bytes);
            } else {
                *out_bytes = 0;
            }

            return true;
        }
    }

    return false;
}

/**
 * Build the Collation Element Array from NFD codepoints.
 *
 * Implements UTS#10 S2.1 (longest consecutive match) followed by S2.1.1-S2.1.3 (discontiguous
 * extension via unblocked non-starters).
 *
 * Key rule: even when a consecutive contraction is found, we MUST still try to extend it further
 * via unblocked non-starters. For example: [0DD9, 0DCF, 0334, 0DCA]: consecutive [0DD9,0DCF] ->
 * weight 3133, but 0DCA is unblocked by 0334 (CCC 1 < 9), so we extend to [0DD9,0DCF,0DCA] ->
 * weight 3134.
 *
 * `used[]` tracks positions consumed by discontiguous extension so that the main loop skips them
 * (they were already incorporated into a prior CE). `cons_advance` tracks how many consecutive
 * positions to skip after a match.
 */
static bool build_cea(const mjb_codepoint *cps, size_t len, mjb_cea *cea) {
    cea->data = NULL;
    cea->count = 0;
    cea->cap = 0;

    if(len == 0) {
        return true;
    }

    bool *used = (bool*)mjb_alloc(len * sizeof(bool));

    if(!used) {
        return false;
    }

    size_t i = 0;

    while(i < len) {
        if(used[i]) {
            ++i;

            continue;
        }

        // Build S: start with the longest consecutive match
        mjb_codepoint cur_seq[18];
        cur_seq[0] = cps[i];
        int cur_len = 1;
        size_t last_pos = i; // Position of last char in S
        size_t cons_advance = 1; // Positions to skip after the consecutive part

        uint8_t best_w[18 * 6];
        int best_bytes = 0;
        bool have_match = false;

        // S2.1: find longest consecutive contraction starting at i
        if(i + 1 < len) {
            uint8_t cons_w[18 * 6];
            int cons_bytes = 0;
            size_t ca = 0;

            if(consecutive_contraction(cps, i, len, cons_w, &cons_bytes, &ca)) {
                for(size_t k = 1; k < ca && cur_len < 18; ++k) {
                    cur_seq[cur_len++] = cps[i + k];
                }

                last_pos = i + ca - 1;
                cons_advance = ca;

                memcpy(best_w, cons_w, (size_t)cons_bytes);

                best_bytes = cons_bytes;
                have_match = true;
            }
        }

        // S2.1.1 – S2.1.3: extend S via unblocked non-starters
        bool keep_extending = true;

        while(keep_extending) {
            keep_extending = false;

            for(size_t j = last_pos + 1; j < len; ++j) {
                if(used[j]) {
                    continue;
                }

                uint8_t cj_ccc = ccc_of(cps[j]);

                // A starter ends the non-starter run
                if(cj_ccc == 0) {
                    break;
                }

                // S2.1.2: blocked if any non-used B in (last_pos, j) has CCC(B) >= CCC(cps[j]).
                bool blocked = false;

                for(size_t k = last_pos + 1; k < j; ++k) {
                    if(used[k]) {
                        continue;
                    }

                    if(ccc_of(cps[k]) >= cj_ccc) {
                        blocked = true;
                        break;
                    }
                }

                if(blocked) {
                    continue;
                }

                // S2.1.3: does S + cps[j] have a table entry?
                if(cur_len >= 18) {
                    break;
                }

                cur_seq[cur_len] = cps[j];

                uint8_t tmp_w[18 * 6];
                int tmp_bytes = 0;

                if(lookup_sequence(cur_seq, cur_len + 1, tmp_w, &tmp_bytes)) {
                    ++cur_len;
                    last_pos       = j;
                    used[j]        = true;
                    memcpy(best_w, tmp_w, (size_t)tmp_bytes);
                    best_bytes     = tmp_bytes;
                    have_match     = true;
                    keep_extending = true;

                    // Restart S2.1.1 from new last_pos.
                    break;
                }
            }
        }

        if(have_match) {
            if(!cea_append_blob(cea, best_w, best_bytes)) {
                mjb_free(used);

                return false;
            }
        } else if(!cea_lookup_or_implicit(cea, cps[i])) {
            mjb_free(used);

            return false;
        }

        i += cons_advance;
    }

    mjb_free(used);

    return true;
}

// Sort key construction
static bool build_sort_key_non_ignorable(const mjb_cea *cea, mjb_sort_key *sk) {
    // L1: non-zero primary weights
    for(size_t i = 0; i < cea->count; ++i) {
        if(cea->data[i].primary != 0 && !sk_push(sk, cea->data[i].primary)) {
            return false;
        }
    }

    if(!sk_push(sk, 0x0000)) { // Level separator
        return false;
    }

    // L2: non-zero secondary weights
    for(size_t i = 0; i < cea->count; ++i) {
        if(cea->data[i].secondary != 0 && !sk_push(sk, cea->data[i].secondary)) {
            return false;
        }
    }

    if(!sk_push(sk, 0x0000)) {
        return false;
    }

    // L3: non-zero tertiary weights
    for(size_t i = 0; i < cea->count; ++i) {
        if(cea->data[i].tertiary != 0 && !sk_push(sk, cea->data[i].tertiary)) {
            return false;
        }
    }

    if(!sk_push(sk, 0x0000)) {
        return false;
    }

    return true;
}

static bool build_sort_key_shifted(const mjb_cea *cea, mjb_sort_key *sk) {
    size_t n = cea->count;

    if(n > SIZE_MAX / sizeof(uint16_t)) {
        return false;
    }

    // We need adjusted weights; avoid dynamic allocation by iterating twice. Compute L1/L2/L3 and
    // L4 in three passes: first collect into a scratch array (malloc), then emit.
    uint16_t *l1 = (uint16_t*)mjb_alloc(n * sizeof(uint16_t));
    uint16_t *l2 = (uint16_t*)mjb_alloc(n * sizeof(uint16_t));
    uint16_t *l3 = (uint16_t*)mjb_alloc(n * sizeof(uint16_t));
    uint16_t *l4 = (uint16_t*)mjb_alloc(n * sizeof(uint16_t));

    if(!l1 || !l2 || !l3 || !l4) {
        mjb_free(l1);
        mjb_free(l2);
        mjb_free(l3);
        mjb_free(l4);

        return false;
    }

    bool after_variable = false;

    for(size_t i = 0; i < n; ++i) {
        const mjb_ce *ce = &cea->data[i];

        if(ce->variable) {
            // Original primary -> L4
            l4[i] = ce->primary;
            l1[i] = 0; l2[i] = 0; l3[i] = 0;
            after_variable = true;
        } else if(ce->primary == 0 && after_variable) {
            // Completely ignorable following a variable: all levels zero
            l1[i] = 0; l2[i] = 0; l3[i] = 0; l4[i] = 0;
        } else {
            l1[i] = ce->primary;
            l2[i] = ce->secondary;
            l3[i] = ce->tertiary;
            // UTS#10 §4 SHIFTED: non-variable primary≠0 -> L4=FFFF; primary=0 -> L4=0
            l4[i] = (ce->primary != 0) ? 0xFFFF : 0x0000;

            if(ce->primary != 0) after_variable = false;
        }
    }

    // Emit L1
    for(size_t i = 0; i < n; ++i) {
        if(l1[i] != 0) {
            if(!sk_push(sk, l1[i])) {
                goto fail;
            }
        }
    }

    if(!sk_push(sk, 0x0000)) {
        goto fail;
    }

    // Emit L2
    for(size_t i = 0; i < n; ++i) {
        if(l2[i] != 0) {
            if(!sk_push(sk, l2[i])) {
                goto fail;
            }
        }
    }

    if(!sk_push(sk, 0x0000)) {
        goto fail;
    }

    // Emit L3
    for(size_t i = 0; i < n; ++i) {
        if(l3[i] != 0) {
            if(!sk_push(sk, l3[i])) {
                goto fail;
            }
        }
    }

    if(!sk_push(sk, 0x0000)) {
        goto fail;
    }

    // Emit L4: only the shifted primaries (non-zero)
    for(size_t i = 0; i < n; ++i) {
        if(l4[i] != 0) {
            if(!sk_push(sk, l4[i])) {
                goto fail;
            }
        }
    }

    mjb_free(l1);
    mjb_free(l2);
    mjb_free(l3);
    mjb_free(l4);

    return true;

fail:
    mjb_free(l1);
    mjb_free(l2);
    mjb_free(l3);
    mjb_free(l4);

    return false;
}

static bool compute_sort_key(const char *buffer, size_t byte_length, mjb_encoding encoding,
    mjb_collation_mode mode, mjb_sort_key *sk) {
    sk->data = NULL;
    sk->count = 0;
    sk->cap = 0;

    if(byte_length == 0) {
        return true;
    }

    mjb_result r;
    bool ok = mjb_normalize(buffer, byte_length, MJB_NORMALIZATION_NFD, encoding, encoding, &r) ==
        MJB_STATUS_OK;

    if(!ok) {
        return false;
    }

    size_t len = 0;
    mjb_codepoint *cps = NULL;
    bool codepoints_ok = utf8_to_codepoints(r.output, r.output_size, &cps, &len);

    if(r.output && r.output != buffer) {
        mjb_free((void*)r.output);
    }

    if(!codepoints_ok) {
        return false;
    }

    mjb_cea cea = { 0, 0, 0 };

    if(len > 0) {
        if(!build_cea(cps, len, &cea)) {
            mjb_free(cps);
            cea_free(&cea);

            return false;
        }
    }

    mjb_free(cps);

    bool key_ok = false;

    if(mode == MJB_COLLATION_SHIFTED) {
        key_ok = build_sort_key_shifted(&cea, sk);
    } else {
        key_ok = build_sort_key_non_ignorable(&cea, sk);
    }

    cea_free(&cea);

    if(!key_ok) {
        sk_free(sk);

        return false;
    }

    return true;
}

static int compare_sort_keys(const mjb_sort_key *k1, const mjb_sort_key *k2) {
    size_t min_count = k1->count < k2->count ? k1->count : k2->count;

    for(size_t i = 0; i < min_count; ++i) {
        if(k1->data[i] < k2->data[i]) {
            return -1;
        }

        if(k1->data[i] > k2->data[i]) {
            return 1;
        }
    }

    if(k1->count < k2->count) {
        return -1;
    }

    if(k1->count > k2->count) {
        return 1;
    }

    return 0;
}

MJB_EXPORT mjb_status mjb_collation_key(const char *buffer, size_t byte_length, mjb_encoding encoding,
    mjb_collation_mode mode, mjb_result *result) {
    if(result == NULL || (buffer == NULL && byte_length > 0)) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    result->output = NULL;
    result->output_size = 0;
    result->transformed = false;

    mjb_sort_key sk = { 0, 0, 0 };

    if(!compute_sort_key(buffer, byte_length, encoding, mode, &sk)) {
        return MJB_STATUS_NO_MEMORY;
    }

    if(sk.count > SIZE_MAX / 2) {
        sk_free(&sk);

        return MJB_STATUS_OVERFLOW;
    }

    size_t byte_count = sk.count * 2;

    if(byte_count == 0) {
        result->transformed = true;

        return MJB_STATUS_OK;
    }

    uint8_t *bytes = (uint8_t*)mjb_alloc(byte_count);

    if(!bytes) {
        sk_free(&sk);
        return MJB_STATUS_NO_MEMORY;
    }

    for(size_t i = 0; i < sk.count; ++i) {
        bytes[i * 2] = (uint8_t)(sk.data[i] >> 8);
        bytes[i * 2 + 1] = (uint8_t)(sk.data[i] & 0xFF);
    }

    sk_free(&sk);

    result->output = (char*)bytes;
    result->output_size = byte_count;
    result->transformed = true;

    return MJB_STATUS_OK;
}

MJB_EXPORT int mjb_string_compare(const char *s1, size_t s1_length, const char *s2,
    size_t s2_length, mjb_encoding encoding, mjb_collation_mode mode) {
    if((s1 == NULL && s1_length > 0) || (s2 == NULL && s2_length > 0)) {
        return -1;
    }

    if(s1_length == 0 && s2_length == 0) {
        return 0;
    }

    if(s1_length == 0) {
        return -1;
    }

    if(s2_length == 0) {
        return 1;
    }

    mjb_sort_key sk1 = { 0, 0, 0 };
    mjb_sort_key sk2 = { 0, 0, 0 };

    if(!compute_sort_key(s1, s1_length, encoding, mode, &sk1)) {
        return -1;
    }

    if(!compute_sort_key(s2, s2_length, encoding, mode, &sk2)) {
        sk_free(&sk1);

        return -1;
    }

    int result = compare_sort_keys(&sk1, &sk2);

    sk_free(&sk1);
    sk_free(&sk2);

    return result;
}
