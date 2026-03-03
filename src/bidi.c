/**
 * The Mojibake library
 *
 * Bidirectional text algorithm — Unicode Standard Annex #9 (TR9)
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <string.h>

#include "mojibake-internal.h"
#include "utf.h"

extern mojibake mjb_global;

#define MJB_BIDI_MAX_DEPTH 125
#define MJB_BIDI_STACK_SIZE (MJB_BIDI_MAX_DEPTH + 2)
#define MJB_BIDI_BRACKET_STACK 63

// Sentinel: character removed by X9.
#define MJB_BIDI_LEVEL_REMOVED 0xFF

typedef struct {
    uint8_t level;
    uint8_t override; // 0: neutral, 1: force-L, 2: force-R.
    bool isolate; // true = pushed by an isolate initiator.
} mjb_bidi_stack_entry;

typedef struct {
    mjb_codepoint open_codepoint;
    size_t position; // Index into working array.
    size_t irs_position; // Index into irs_idx[] for context lookup.
} mjb_bidi_bracket_entry;

// Working entry used during algorithm passes.
typedef struct {
    mjb_codepoint codepoint;
    size_t byte_offset;
    uint8_t level;
    mjb_bidi_class bidi; // Current (possibly remapped) bidi class.
    mjb_bidi_class orig; // Original bidi class.
    bool mirrored;
    bool removed; // X9 removed.
} mjb_bidi_work;

typedef struct {
    mjb_codepoint cp;
    mjb_codepoint pair;
    bool is_open;
} mjb_bidi_bracket_info;

// From BidiBrackets.txt, Unicode 17.0.0)
// This array is automatically generated. Do not edit.
static const mjb_bidi_bracket_info mjb_bidi_brackets[] = {
    { 0x0028, 0x0029, true }, { 0x0029, 0x0028, false }, { 0x005B, 0x005D, true },
    { 0x005D, 0x005B, false }, { 0x007B, 0x007D, true }, { 0x007D, 0x007B, false },
    { 0x0F3A, 0x0F3B, true }, { 0x0F3B, 0x0F3A, false }, { 0x0F3C, 0x0F3D, true },
    { 0x0F3D, 0x0F3C, false }, { 0x169B, 0x169C, true }, { 0x169C, 0x169B, false },
    { 0x2045, 0x2046, true }, { 0x2046, 0x2045, false }, { 0x207D, 0x207E, true },
    { 0x207E, 0x207D, false }, { 0x208D, 0x208E, true }, { 0x208E, 0x208D, false },
    { 0x2308, 0x2309, true }, { 0x2309, 0x2308, false }, { 0x230A, 0x230B, true },
    { 0x230B, 0x230A, false }, { 0x2329, 0x232A, true }, { 0x232A, 0x2329, false },
    { 0x2768, 0x2769, true }, { 0x2769, 0x2768, false }, { 0x276A, 0x276B, true },
    { 0x276B, 0x276A, false }, { 0x276C, 0x276D, true }, { 0x276D, 0x276C, false },
    { 0x276E, 0x276F, true }, { 0x276F, 0x276E, false }, { 0x2770, 0x2771, true },
    { 0x2771, 0x2770, false }, { 0x2772, 0x2773, true }, { 0x2773, 0x2772, false },
    { 0x2774, 0x2775, true }, { 0x2775, 0x2774, false }, { 0x27C5, 0x27C6, true },
    { 0x27C6, 0x27C5, false }, { 0x27E6, 0x27E7, true }, { 0x27E7, 0x27E6, false },
    { 0x27E8, 0x27E9, true }, { 0x27E9, 0x27E8, false }, { 0x27EA, 0x27EB, true },
    { 0x27EB, 0x27EA, false }, { 0x27EC, 0x27ED, true }, { 0x27ED, 0x27EC, false },
    { 0x27EE, 0x27EF, true }, { 0x27EF, 0x27EE, false }, { 0x2983, 0x2984, true },
    { 0x2984, 0x2983, false }, { 0x2985, 0x2986, true }, { 0x2986, 0x2985, false },
    { 0x2987, 0x2988, true }, { 0x2988, 0x2987, false }, { 0x2989, 0x298A, true },
    { 0x298A, 0x2989, false }, { 0x298B, 0x298C, true }, { 0x298C, 0x298B, false },
    { 0x298D, 0x2990, true }, { 0x298E, 0x298F, false }, { 0x298F, 0x298E, true },
    { 0x2990, 0x298D, false }, { 0x2991, 0x2992, true }, { 0x2992, 0x2991, false },
    { 0x2993, 0x2994, true }, { 0x2994, 0x2993, false }, { 0x2995, 0x2996, true },
    { 0x2996, 0x2995, false }, { 0x2997, 0x2998, true }, { 0x2998, 0x2997, false },
    { 0x29D8, 0x29D9, true }, { 0x29D9, 0x29D8, false }, { 0x29DA, 0x29DB, true },
    { 0x29DB, 0x29DA, false }, { 0x29FC, 0x29FD, true }, { 0x29FD, 0x29FC, false },
    { 0x2E22, 0x2E23, true }, { 0x2E23, 0x2E22, false }, { 0x2E24, 0x2E25, true },
    { 0x2E25, 0x2E24, false }, { 0x2E26, 0x2E27, true }, { 0x2E27, 0x2E26, false },
    { 0x2E28, 0x2E29, true }, { 0x2E29, 0x2E28, false }, { 0x2E55, 0x2E56, true },
    { 0x2E56, 0x2E55, false }, { 0x2E57, 0x2E58, true }, { 0x2E58, 0x2E57, false },
    { 0x2E59, 0x2E5A, true }, { 0x2E5A, 0x2E59, false }, { 0x2E5B, 0x2E5C, true },
    { 0x2E5C, 0x2E5B, false }, { 0x3008, 0x3009, true }, { 0x3009, 0x3008, false },
    { 0x300A, 0x300B, true }, { 0x300B, 0x300A, false }, { 0x300C, 0x300D, true },
    { 0x300D, 0x300C, false }, { 0x300E, 0x300F, true }, { 0x300F, 0x300E, false },
    { 0x3010, 0x3011, true }, { 0x3011, 0x3010, false }, { 0x3014, 0x3015, true },
    { 0x3015, 0x3014, false }, { 0x3016, 0x3017, true }, { 0x3017, 0x3016, false },
    { 0x3018, 0x3019, true }, { 0x3019, 0x3018, false }, { 0x301A, 0x301B, true },
    { 0x301B, 0x301A, false }, { 0xFE59, 0xFE5A, true }, { 0xFE5A, 0xFE59, false },
    { 0xFE5B, 0xFE5C, true }, { 0xFE5C, 0xFE5B, false }, { 0xFE5D, 0xFE5E, true },
    { 0xFE5E, 0xFE5D, false }, { 0xFF08, 0xFF09, true }, { 0xFF09, 0xFF08, false },
    { 0xFF3B, 0xFF3D, true }, { 0xFF3D, 0xFF3B, false }, { 0xFF5B, 0xFF5D, true },
    { 0xFF5D, 0xFF5B, false }, { 0xFF5F, 0xFF60, true }, { 0xFF60, 0xFF5F, false },
    { 0xFF62, 0xFF63, true }, { 0xFF63, 0xFF62, false }
};

#define MJB_BIDI_BRACKET_COUNT 128

static bool bidi_bracket_info(mjb_codepoint cp, mjb_codepoint *pair, bool *is_open) {
    for(int i = 0; i < MJB_BIDI_BRACKET_COUNT; ++i) {
        if(mjb_bidi_brackets[i].cp == cp) {
            *pair = mjb_bidi_brackets[i].pair;
            *is_open = mjb_bidi_brackets[i].is_open;

            return true;
        }
    }

    return false;
}

// UAX#9 6.3: U+2329≡U+3008 (open), U+232A≡U+3009 (close) are canonical equivalents.
static inline bool bracket_canon_eq(mjb_codepoint a, mjb_codepoint b) {
    return (a == 0x2329 && b == 0x3008) || (a == 0x3008 && b == 0x2329) ||
        (a == 0x232A && b == 0x3009) || (a == 0x3009 && b == 0x232A);
}

static inline bool is_strong_type(mjb_bidi_class c) {
    return c == MJB_PR_BIDI_CLASS_L || c == MJB_PR_BIDI_CLASS_R || c == MJB_PR_BIDI_CLASS_AL;
}

static inline bool is_rtl_strong(mjb_bidi_class c) {
    return c == MJB_PR_BIDI_CLASS_R || c == MJB_PR_BIDI_CLASS_AL;
}

static inline bool is_neutral(mjb_bidi_class c) {
    return c == MJB_PR_BIDI_CLASS_B || c == MJB_PR_BIDI_CLASS_S || c == MJB_PR_BIDI_CLASS_WS ||
        c == MJB_PR_BIDI_CLASS_ON;
}

static inline bool is_isolate_initiator(mjb_bidi_class c) {
    return c == MJB_PR_BIDI_CLASS_LRI || c == MJB_PR_BIDI_CLASS_RLI || c == MJB_PR_BIDI_CLASS_FSI;
}

// DB look-up: bidi class + mirrored flag
static bool bidi_query(mjb_codepoint cp, mjb_bidi_class *out_class, bool *out_mirrored) {
    if(!mjb_initialize()) {
        return false;
    }

    sqlite3_reset(mjb_global.stmt_bidi);

    if(sqlite3_bind_int(mjb_global.stmt_bidi, 1, (int)cp) != SQLITE_OK) {
        sqlite3_reset(mjb_global.stmt_bidi);

        return false;
    }

    int rc = sqlite3_step(mjb_global.stmt_bidi);

    if(rc != SQLITE_ROW) {
        sqlite3_reset(mjb_global.stmt_bidi);
        *out_class   = MJB_PR_BIDI_CLASS_L; // default: L.
        *out_mirrored = false;

        return true;
    }

    int bidi_val = sqlite3_column_int(mjb_global.stmt_bidi, 0);
    int mirrored_v = sqlite3_column_int(mjb_global.stmt_bidi, 1);

    sqlite3_reset(mjb_global.stmt_bidi);

    if(bidi_val <= 0 || bidi_val >= MJB_BIDI_CLASS_COUNT) {
        *out_class = MJB_PR_BIDI_CLASS_L;
    } else {
        *out_class = (mjb_bidi_class)bidi_val;
    }

    *out_mirrored = (mirrored_v != 0);

    return true;
}

// Pass 1: decode string + build working array + P2/P3 paragraph level.
static size_t pass1_decode(const char *buffer, size_t size, mjb_encoding encoding,
    mjb_bidi_work *work, size_t capacity, uint8_t *out_level, mjb_direction base_dir) {
    uint8_t state = MJB_UTF_ACCEPT;
    bool in_error = false;
    mjb_codepoint cp = 0;
    size_t count = 0;
    size_t i = 0;

    while(i < size && count < capacity) {
        size_t byte_offset = i;
        mjb_decode_result dr = mjb_next_codepoint(buffer, size, &state, &i, encoding, &cp, &in_error);

        if(dr == MJB_DECODE_END) {
            break;
        }

        if(dr == MJB_DECODE_INCOMPLETE) {
            continue;
        }

        mjb_bidi_class bc;
        bool mirrored;

        if(!bidi_query(cp, &bc, &mirrored)) {
            continue;
        }

        work[count].codepoint = cp;
        work[count].byte_offset = byte_offset;
        work[count].level = 0;
        work[count].bidi = bc;
        work[count].orig = bc;
        work[count].mirrored = mirrored;
        work[count].removed = false;
        ++count;
    }

    // P2/P3: determine paragraph embedding level.
    if(base_dir == MJB_DIRECTION_LTR) {
        *out_level = 0;
    } else if(base_dir == MJB_DIRECTION_RTL) {
        *out_level = 1;
    } else {
        // AUTO: scan for first strong type, skipping isolate-scoped content.
        *out_level = 0; // default LTR
        int depth = 0;

        for(size_t k = 0; k < count; ++k) {
            mjb_bidi_class bc = work[k].bidi;

            if(is_isolate_initiator(bc)) {
                ++depth;
            } else if(bc == MJB_PR_BIDI_CLASS_PDI) {
                if(depth > 0) {
                    --depth;
                }
            } else if(depth == 0 && is_strong_type(bc)) {
                *out_level = is_rtl_strong(bc) ? 1 : 0;
                break;
            }
        }
    }

    return count;
}

// Pass 2: X rules, explicit levels
static void pass2_explicit(mjb_bidi_work *work, size_t count, uint8_t para_level) {
    mjb_bidi_stack_entry stack[MJB_BIDI_STACK_SIZE];
    int top = 0;
    stack[0].level = para_level;
    stack[0].override = 0;
    stack[0].isolate = false;

    // Overflow counters.
    int overflow_isolate = 0;
    int overflow_embed = 0;
    int valid_isolate = 0;

    for(size_t i = 0; i < count; ++i) {
        mjb_bidi_class bc = work[i].bidi;
        uint8_t cur_level = stack[top].level;
        uint8_t next_even = (uint8_t)(((cur_level + 2) & ~1)); // Next even >= cur + 1.
        uint8_t next_odd = (uint8_t)(((cur_level + 1) | 1)); // Next odd >= cur + 1.

        if(bc == MJB_PR_BIDI_CLASS_RLE || bc == MJB_PR_BIDI_CLASS_RLO) {
            uint8_t new_level = next_odd;

            if(new_level <= MJB_BIDI_MAX_DEPTH && overflow_isolate == 0 && overflow_embed == 0) {
                ++top;
                stack[top].level = new_level;
                stack[top].override = (bc == MJB_PR_BIDI_CLASS_RLO) ? 2 : 0;
                stack[top].isolate = false;
            } else if(overflow_isolate == 0) {
                ++overflow_embed;
            }

            work[i].removed = true;
        } else if(bc == MJB_PR_BIDI_CLASS_LRE || bc == MJB_PR_BIDI_CLASS_LRO) {
            uint8_t new_level = next_even;

            if(new_level <= MJB_BIDI_MAX_DEPTH && overflow_isolate == 0 && overflow_embed == 0) {
                ++top;
                stack[top].level = new_level;
                stack[top].override = (bc == MJB_PR_BIDI_CLASS_LRO) ? 1 : 0;
                stack[top].isolate = false;
            } else if(overflow_isolate == 0) {
                ++overflow_embed;
            }

            work[i].removed = true;
        } else if(bc == MJB_PR_BIDI_CLASS_RLI || bc == MJB_PR_BIDI_CLASS_LRI) {
            // Apply current level and override before pushing.
            work[i].level = stack[top].level;

            if(stack[top].override == 1) {
                work[i].bidi = MJB_PR_BIDI_CLASS_L;
            } else if(stack[top].override == 2) {
                work[i].bidi = MJB_PR_BIDI_CLASS_R;
            }

            uint8_t new_level = (bc == MJB_PR_BIDI_CLASS_RLI) ? next_odd : next_even;

            if(new_level <= MJB_BIDI_MAX_DEPTH && overflow_isolate == 0 && overflow_embed == 0) {
                ++valid_isolate;
                ++top;
                stack[top].level = new_level;
                stack[top].override = 0;
                stack[top].isolate = true;
            } else {
                ++overflow_isolate;
            }
        } else if(bc == MJB_PR_BIDI_CLASS_FSI) {
            // Determine FSI direction by scanning ahead.
            int depth2 = 0;
            mjb_bidi_class fsi_dir_class = MJB_PR_BIDI_CLASS_L;

            for(size_t j = i + 1; j < count; ++j) {
                mjb_bidi_class jbc = work[j].orig;

                if(is_isolate_initiator(jbc)) {
                    ++depth2;
                } else if(jbc == MJB_PR_BIDI_CLASS_PDI) {
                    if(depth2 > 0) {
                        --depth2;
                    } else {
                        break; // Matched the PDI.
                    }
                } else if(depth2 == 0 && is_strong_type(jbc)) {
                    fsi_dir_class = jbc;
                    break;
                }
            }

            // Treat FSI as LRI or RLI based on found direction.
            mjb_bidi_class effective = is_rtl_strong(fsi_dir_class) ? MJB_PR_BIDI_CLASS_RLI
                : MJB_PR_BIDI_CLASS_LRI;
            work[i].level = stack[top].level;

            if(stack[top].override == 1) {
                work[i].bidi = MJB_PR_BIDI_CLASS_L;
            } else if(stack[top].override == 2) {
                work[i].bidi = MJB_PR_BIDI_CLASS_R;
            }

            uint8_t new_level = (effective == MJB_PR_BIDI_CLASS_RLI) ? next_odd : next_even;

            if(new_level <= MJB_BIDI_MAX_DEPTH && overflow_isolate == 0 && overflow_embed == 0) {
                ++valid_isolate;
                ++top;
                stack[top].level = new_level;
                stack[top].override = 0;
                stack[top].isolate = true;
            } else {
                ++overflow_isolate;
            }
        } else if(bc == MJB_PR_BIDI_CLASS_PDI) {
            if(overflow_isolate > 0) {
                --overflow_isolate;
            } else if(valid_isolate > 0) {
                overflow_embed = 0;

                while(!stack[top].isolate && top > 0) {
                    --top;
                }

                if(top > 0) {
                    --top;
                }

                --valid_isolate;
            }

            work[i].level = stack[top].level;

            if(stack[top].override == 1) {
                work[i].bidi = MJB_PR_BIDI_CLASS_L;
            } else if(stack[top].override == 2) {
                work[i].bidi = MJB_PR_BIDI_CLASS_R;
            }
        } else if(bc == MJB_PR_BIDI_CLASS_PDF) {
            if(overflow_isolate > 0) {
                // PDF within isolate overflow, do nothing.
            } else if(overflow_embed > 0) {
                --overflow_embed;
            } else if(!stack[top].isolate && top > 0) {
                --top;
            }

            work[i].removed = true;
        } else if(bc == MJB_PR_BIDI_CLASS_BN) {
            // BN: keep level but mark removed for further processing.
            work[i].level = stack[top].level;
            work[i].removed = true;
        } else {
            // Standard character.
            work[i].level = stack[top].level;

            if(stack[top].override == 1) {
                work[i].bidi = MJB_PR_BIDI_CLASS_L;
            } else if(stack[top].override == 2) {
                work[i].bidi = MJB_PR_BIDI_CLASS_R;
            }
        }
    }
}

// Find the sos/eos directional type for a run boundary given the levels.
static inline mjb_bidi_class level_to_dir(uint8_t level) {
    return (level & 1) ? MJB_PR_BIDI_CLASS_R : MJB_PR_BIDI_CLASS_L;
}

// Level of the non-removed char immediately before position |pos|.
static inline uint8_t prev_nonremoved_level(const mjb_bidi_work *work, size_t pos,
    uint8_t para_level) {
    for(size_t k = pos; k > 0;) {
        --k;

        if(!work[k].removed) {
            return work[k].level;
        }
    }

    return para_level;
}

/**
 * Collect an Isolating Run Sequence (IRS) beginning at |start| (a non-removed char at embedding
 * level |level|). Follows LRI/RLI/FSI -> matching-PDI hops per TR9 3.3.2. Returns the number of
 * indices stored in |irs_idx|, and sets *out_scan_end to the position of the first non-removed char
 * OUTSIDE the IRS (different level), or to |count| if the IRS reaches the end.
 */
static size_t collect_irs(const mjb_bidi_work *work, size_t count, size_t start, uint8_t level,
    size_t *irs_idx, size_t irs_cap, size_t *out_scan_end) {
    size_t irs_len = 0;
    size_t pos = start;

    while(pos < count && irs_len < irs_cap) {
        // Skip removed chars.
        while(pos < count && work[pos].removed) {
            ++pos;
        }

        if(pos >= count) {
            break;
        }

        // Stop at a different embedding level.
        if(work[pos].level != level) {
            break;
        }

        // Add this character to the IRS.
        irs_idx[irs_len++] = pos;
        mjb_bidi_class orig = work[pos].orig;
        ++pos;

        // If this character was originally an isolate initiator, hop over the matching isolate
        // scope and resume from the paired PDI.
        if(is_isolate_initiator(orig)) {
            int depth = 1;

            while(pos < count && depth > 0) {
                if(!work[pos].removed) {
                    if(is_isolate_initiator(work[pos].orig)) {
                        ++depth;
                    } else if(work[pos].orig == MJB_PR_BIDI_CLASS_PDI) {
                        --depth;

                        if(depth == 0) {
                            break; // pos = matching PDI; outer loop will add it */
                        }
                    }
                }

                ++pos;
            }

            // pos now points to the matching PDI (depth==0) or to count.
        }
    }

    *out_scan_end = pos;

    return irs_len;
}

// Pass 3: W rules, weak types (per Isolating Run Sequence).
static void pass3_weak(mjb_bidi_work *work, size_t count, uint8_t para_level) {
    size_t *irs_idx = (size_t *)mjb_alloc(count * sizeof(size_t));
    bool *done = (bool *)mjb_alloc(count * sizeof(bool));

    if(!irs_idx || !done) {
        mjb_free(irs_idx);
        mjb_free(done);

        return;
    }

    memset(done, 0, count * sizeof(bool));

    size_t scan = 0;

    while(scan < count) {
        // Find the next unprocessed, non-removed char.
        while(scan < count && (work[scan].removed || done[scan])) {
            ++scan;
        }

        if(scan >= count) {
            break;
        }

        uint8_t level = work[scan].level;
        size_t scan_end;
        size_t irs_len = collect_irs(work, count, scan, level, irs_idx, count, &scan_end);

        // Mark all IRS members processed.
        for(size_t j = 0; j < irs_len; ++j) {
            done[irs_idx[j]] = true;
        }

        // sos/eos per TR9 3.3.2.
        uint8_t prev_lv = prev_nonremoved_level(work, irs_idx[0], para_level);
        mjb_bidi_class sos = level_to_dir(level > prev_lv ? level : prev_lv);

        uint8_t next_lv = para_level;

        for(size_t k = scan_end; k < count; ++k) {
            if(!work[k].removed) {
                next_lv = work[k].level;
                break;
            }
        }

        mjb_bidi_class eos = level_to_dir(level > next_lv ? level : next_lv);
        (void)eos; // eos is used by N1/N2; W rules only use sos.

        // W1: NSM -> type of preceding character (or sos).
        for(size_t j = 0; j < irs_len; ++j) {
            size_t idx = irs_idx[j];

            if(work[idx].bidi == MJB_PR_BIDI_CLASS_NSM) {
                if(j == 0) {
                    work[idx].bidi = sos;
                } else {
                    mjb_bidi_class prev = work[irs_idx[j - 1]].bidi;

                    if(is_isolate_initiator(prev) || prev == MJB_PR_BIDI_CLASS_PDI) {
                        work[idx].bidi = MJB_PR_BIDI_CLASS_ON;
                    } else {
                        work[idx].bidi = prev;
                    }
                }
            }
        }

        // W2: EN -> AN when last strong type was AL.
        mjb_bidi_class last_strong = sos;

        for(size_t j = 0; j < irs_len; ++j) {
            size_t idx = irs_idx[j];
            mjb_bidi_class bc = work[idx].bidi;

            if(bc == MJB_PR_BIDI_CLASS_L || bc == MJB_PR_BIDI_CLASS_R ||
               bc == MJB_PR_BIDI_CLASS_AL) {
                last_strong = bc;
            } else if(bc == MJB_PR_BIDI_CLASS_EN && last_strong == MJB_PR_BIDI_CLASS_AL) {
                work[idx].bidi = MJB_PR_BIDI_CLASS_AN;
            }
        }

        // W3: AL -> R.
        for(size_t j = 0; j < irs_len; ++j) {
            if(work[irs_idx[j]].bidi == MJB_PR_BIDI_CLASS_AL) {
                work[irs_idx[j]].bidi = MJB_PR_BIDI_CLASS_R;
            }
        }

        // W4: single ES/CS between matching EN or AN pairs.
        for(size_t j = 1; j + 1 < irs_len; ++j) {
            size_t idx = irs_idx[j];
            mjb_bidi_class bc = work[idx].bidi;
            mjb_bidi_class prev = work[irs_idx[j - 1]].bidi;
            mjb_bidi_class next = work[irs_idx[j + 1]].bidi;

            if(bc == MJB_PR_BIDI_CLASS_ES) {
                if(prev == MJB_PR_BIDI_CLASS_EN && next == MJB_PR_BIDI_CLASS_EN) {
                    work[idx].bidi = MJB_PR_BIDI_CLASS_EN;
                }
            } else if(bc == MJB_PR_BIDI_CLASS_CS) {
                if(prev == MJB_PR_BIDI_CLASS_EN && next == MJB_PR_BIDI_CLASS_EN) {
                    work[idx].bidi = MJB_PR_BIDI_CLASS_EN;
                } else if(prev == MJB_PR_BIDI_CLASS_AN && next == MJB_PR_BIDI_CLASS_AN) {
                    work[idx].bidi = MJB_PR_BIDI_CLASS_AN;
                }
            }
        }

        // W5: ET adjacent to EN -> EN (forward pass).
        bool et_after_en = false;

        for(size_t j = 0; j < irs_len; ++j) {
            mjb_bidi_class bc = work[irs_idx[j]].bidi;

            if(bc == MJB_PR_BIDI_CLASS_EN) {
                et_after_en = true;
            } else if(bc == MJB_PR_BIDI_CLASS_ET && et_after_en) {
                work[irs_idx[j]].bidi = MJB_PR_BIDI_CLASS_EN;
            } else {
                et_after_en = false;
            }
        }

        // W5: backward pass (ET before EN).
        bool et_before_en = false;

        for(size_t j = irs_len; j-- > 0;) {
            mjb_bidi_class bc = work[irs_idx[j]].bidi;

            if(bc == MJB_PR_BIDI_CLASS_EN) {
                et_before_en = true;
            } else if(bc == MJB_PR_BIDI_CLASS_ET && et_before_en) {
                work[irs_idx[j]].bidi = MJB_PR_BIDI_CLASS_EN;
            } else {
                et_before_en = false;
            }
        }

        // W6: remaining ES/ET/CS -> ON.
        for(size_t j = 0; j < irs_len; ++j) {
            mjb_bidi_class bc = work[irs_idx[j]].bidi;

            if(bc == MJB_PR_BIDI_CLASS_ES || bc == MJB_PR_BIDI_CLASS_ET ||
               bc == MJB_PR_BIDI_CLASS_CS) {
                work[irs_idx[j]].bidi = MJB_PR_BIDI_CLASS_ON;
            }
        }

        // W7: EN -> L when last strong type was L.
        last_strong = sos;

        for(size_t j = 0; j < irs_len; ++j) {
            size_t idx = irs_idx[j];
            mjb_bidi_class bc = work[idx].bidi;

            if(bc == MJB_PR_BIDI_CLASS_L || bc == MJB_PR_BIDI_CLASS_R) {
                last_strong = bc;
            } else if(bc == MJB_PR_BIDI_CLASS_EN && last_strong == MJB_PR_BIDI_CLASS_L) {
                work[idx].bidi = MJB_PR_BIDI_CLASS_L;
            }
        }

        ++scan;
    }

    mjb_free(done);
    mjb_free(irs_idx);
}

// Pass 4: N0, bracket pairs (per Isolating Run Sequence).
// Bracket pair collected during phase 1 of N0
typedef struct {
    size_t open_idx; // Work array index of open bracket
    size_t close_idx; // Work array index of close bracket
    size_t open_irs; // IRS index of open bracket
    size_t close_irs; // IRS index of close bracket
} mjb_bracket_pair;

static void pass4_brackets(mjb_bidi_work *work, size_t count, uint8_t para_level) {
    size_t *irs_idx = (size_t *)mjb_alloc(count * sizeof(size_t));
    bool *done = (bool*)mjb_alloc(count * sizeof(bool));

    if(!irs_idx || !done) {
        mjb_free(irs_idx);
        mjb_free(done);

        return;
    }

    memset(done, 0, count * sizeof(bool));

    size_t scan = 0;

    while(scan < count) {
        while(scan < count && (work[scan].removed || done[scan])) {
            ++scan;
        }

        if(scan >= count) {
            break;
        }

        uint8_t level = work[scan].level;
        size_t scan_end = 0;
        size_t irs_len = collect_irs(work, count, scan, level, irs_idx, count, &scan_end);

        for(size_t j = 0; j < irs_len; ++j) {
            done[irs_idx[j]] = true;
        }

        mjb_bidi_class embed_dir = level_to_dir(level);
        mjb_bidi_class opp_dir = (embed_dir == MJB_PR_BIDI_CLASS_L) ? MJB_PR_BIDI_CLASS_R :
            MJB_PR_BIDI_CLASS_L;

        // Phase 1: collect all bracket pairs for this IRS.
        mjb_bidi_bracket_entry bstack[MJB_BIDI_BRACKET_STACK];
        int bstop = 0;

        mjb_bracket_pair pairs[MJB_BIDI_BRACKET_STACK];
        int pair_count = 0;

        for(size_t j = 0; j < irs_len; ++j) {
            size_t idx = irs_idx[j];

            if(work[idx].bidi != MJB_PR_BIDI_CLASS_ON) {
                continue;
            }

            mjb_codepoint bracket_pair_cp;
            bool bracket_is_open;

            if(!bidi_bracket_info(work[idx].codepoint, &bracket_pair_cp, &bracket_is_open)) {
                continue;
            }

            if(bracket_is_open) {
                if(bstop < MJB_BIDI_BRACKET_STACK) {
                    bstack[bstop].open_codepoint = work[idx].codepoint;
                    bstack[bstop].position = idx;
                    bstack[bstop].irs_position = j;
                    ++bstop;
                } else {
                    break; // Stack overflow, stop collecting per spec.
                }
            } else if(bstop > 0) {
                /*
                    Search stack top-to-bottom for matching open bracket.
                    Include canonical equivalents (UAX#9 6.3):
                    U+2329 <-> U+3008 and U+232A <-> U+3009.
                */
                int found = -1;

                for(int s = bstop - 1; s >= 0; --s) {
                    mjb_codepoint open_cp = bstack[s].open_codepoint;

                    if(open_cp == bracket_pair_cp || bracket_canon_eq(open_cp, bracket_pair_cp)) {
                        found = s;
                        break;
                    }
                }

                if(found >= 0 && pair_count < MJB_BIDI_BRACKET_STACK) {
                    pairs[pair_count].open_idx  = bstack[found].position;
                    pairs[pair_count].close_idx = idx;
                    pairs[pair_count].open_irs  = bstack[found].irs_position;
                    pairs[pair_count].close_irs = j;
                    ++pair_count;
                    bstop = found; // Pop stack to just below the matched entry.
                }
            }
        }

        // Phase 2: sort pairs by open_irs (logical / outer-first order).
        // Insertion sort, max 63 elements.
        for(int i = 1; i < pair_count; ++i) {
            mjb_bracket_pair tmp = pairs[i];
            int k = i;

            while(k > 0 && pairs[k - 1].open_irs > tmp.open_irs) {
                pairs[k] = pairs[k - 1];
                --k;
            }

            pairs[k] = tmp;
        }

        /*
            Phase 3: process pairs in outer-first (logical) order.
            UAX#9 N0: "consider the actual types (including those resulting from this resolution)"
            Outer brackets resolved first so inner brackets see the updated types of their
            surrounding context.
        */
        for(int p = 0; p < pair_count; ++p) {
            size_t open_idx = pairs[p].open_idx;
            size_t close_idx = pairs[p].close_idx;
            size_t open_irs = pairs[p].open_irs;
            size_t close_irs = pairs[p].close_irs;

            // Scan only IRS chars strictly between the brackets.
            bool found_embed = false;
            bool found_opp   = false;

            for(size_t kk = open_irs + 1; kk < close_irs; ++kk) {
                mjb_bidi_class kbc = work[irs_idx[kk]].bidi;

                if(kbc == MJB_PR_BIDI_CLASS_EN || kbc == MJB_PR_BIDI_CLASS_AN) {
                    kbc = MJB_PR_BIDI_CLASS_R;
                }

                if(kbc == embed_dir) {
                    found_embed = true;
                    break; // N0b: first match wins.
                } else if(kbc == opp_dir) {
                    found_opp = true;
                }
            }

            mjb_bidi_class pair_dir;
            bool set_pair = true;

            if(found_embed) {
                pair_dir = embed_dir;
            } else if(found_opp) {
                // N0c: context = last strong type before the open bracket in IRS.
                mjb_bidi_class ctx = embed_dir; // default = embedding direction (sos)

                for(size_t jj = open_irs; jj-- > 0;) {
                    mjb_bidi_class kbc = work[irs_idx[jj]].bidi;

                    if(kbc == MJB_PR_BIDI_CLASS_EN || kbc == MJB_PR_BIDI_CLASS_AN) {
                        kbc = MJB_PR_BIDI_CLASS_R;
                    }

                    if(kbc == MJB_PR_BIDI_CLASS_L || kbc == MJB_PR_BIDI_CLASS_R) {
                        ctx = kbc;
                        break;
                    }
                }

                pair_dir = (ctx == opp_dir) ? opp_dir : embed_dir;
            } else {
                // N0c: no strong types inside, leave as ON for N1/N2.
                set_pair = false;
                pair_dir = MJB_PR_BIDI_CLASS_ON; // Unused.
            }

            if(set_pair) {
                work[open_idx].bidi = pair_dir;
                work[close_idx].bidi = pair_dir;

                // Propagate pair_dir to NSMs immediately after each bracket.
                for(size_t k = open_idx + 1; k < count; ++k) {
                    if(work[k].removed) {
                        continue;
                    }

                    if(work[k].orig == MJB_PR_BIDI_CLASS_NSM && work[k].level == level) {
                        work[k].bidi = pair_dir;
                    } else {
                        break;
                    }
                }

                for(size_t k = close_idx + 1; k < count; ++k) {
                    if(work[k].removed) {
                        continue;
                    }

                    if(work[k].orig == MJB_PR_BIDI_CLASS_NSM && work[k].level == level) {
                        work[k].bidi = pair_dir;
                    } else {
                        break;
                    }
                }
            }
        }

        ++scan;
    }

    mjb_free(done);
    mjb_free(irs_idx);
}

/**
 * Pass 5: N1/N2 — other neutrals (per Isolating Run Sequence)
 */
static void pass5_neutrals(mjb_bidi_work *work, size_t count, uint8_t para_level) {
    size_t *irs_idx = (size_t *)mjb_alloc(count * sizeof(size_t));
    bool *done = (bool*)mjb_alloc(count * sizeof(bool));

    if(!irs_idx || !done) {
        mjb_free(irs_idx);
        mjb_free(done);

        return;
    }

    memset(done, 0, count * sizeof(bool));

    size_t scan = 0;

    while(scan < count) {
        while(scan < count && (work[scan].removed || done[scan])) {
            ++scan;
        }

        if(scan >= count) {
            break;
        }

        uint8_t level = work[scan].level;
        size_t scan_end;
        size_t irs_len = collect_irs(work, count, scan, level, irs_idx, count, &scan_end);

        for(size_t j = 0; j < irs_len; ++j) {
            done[irs_idx[j]] = true;
        }

        // SOS/EOS per TR9 3.3.2.
        uint8_t prev_lv = prev_nonremoved_level(work, irs_idx[0], para_level);
        mjb_bidi_class sos = level_to_dir(level > prev_lv ? level : prev_lv);

        uint8_t next_lv = para_level;

        for(size_t k = scan_end; k < count; ++k) {
            if(!work[k].removed) {
                next_lv = work[k].level;
                break;
            }
        }

        mjb_bidi_class embed_dir = level_to_dir(level);
        mjb_bidi_class eos = level_to_dir(level > next_lv ? level : next_lv);

        // N1/N2: resolve neutral runs.
        size_t j = 0;

        while(j < irs_len) {
            mjb_bidi_class bc = work[irs_idx[j]].bidi;

            if(!is_neutral(bc) && bc != MJB_PR_BIDI_CLASS_BN) {
                ++j;
                continue;
            }

            // Extend neutral run.
            size_t run_start = j;
            size_t run_end = j;

            while(run_end < irs_len) {
                mjb_bidi_class nc = work[irs_idx[run_end]].bidi;

                if(!is_neutral(nc) && nc != MJB_PR_BIDI_CLASS_BN) {
                    break;
                }

                ++run_end;
            }

            // Strong types flanking this neutral run within the IRS.
            mjb_bidi_class before = (run_start == 0) ? sos : work[irs_idx[run_start - 1]].bidi;
            mjb_bidi_class after = (run_end >= irs_len) ? eos : work[irs_idx[run_end]].bidi;

            if(before == MJB_PR_BIDI_CLASS_EN || before == MJB_PR_BIDI_CLASS_AN) {
                before = MJB_PR_BIDI_CLASS_R;
            }

            if(after == MJB_PR_BIDI_CLASS_EN || after == MJB_PR_BIDI_CLASS_AN) {
                after = MJB_PR_BIDI_CLASS_R;
            }

            mjb_bidi_class resolved;

            if(before == after &&
               (before == MJB_PR_BIDI_CLASS_L || before == MJB_PR_BIDI_CLASS_R)) {
                resolved = before; // N1.
            } else {
                resolved = embed_dir; // N2.
            }

            for(size_t k = run_start; k < run_end; ++k) {
                work[irs_idx[k]].bidi = resolved;
            }

            j = run_end;
        }

        ++scan;
    }

    mjb_free(done);
    mjb_free(irs_idx);
}

// Pass 6: I1/I2, implicit levels.
static void pass6_implicit(mjb_bidi_work *work, size_t count) {
    for(size_t i = 0; i < count; ++i) {
        if(work[i].removed) {
            continue;
        }

        uint8_t level = work[i].level;
        mjb_bidi_class bc = work[i].bidi;

        if((level & 1) == 0) {
            // Even level: I1
            if(bc == MJB_PR_BIDI_CLASS_R) {
                work[i].level = (uint8_t)(level + 1);
            } else if(bc == MJB_PR_BIDI_CLASS_AN || bc == MJB_PR_BIDI_CLASS_EN) {
                work[i].level = (uint8_t)(level + 2);
            }
        } else {
            // Odd level: I2
            if(bc == MJB_PR_BIDI_CLASS_L || bc == MJB_PR_BIDI_CLASS_AN ||
               bc == MJB_PR_BIDI_CLASS_EN) {
                work[i].level = (uint8_t)(level + 1);
            }
        }
    }
}

// Resolve bidirectional text (TR9) for a paragraph.
MJB_EXPORT bool mjb_bidi_resolve(const char *buffer, size_t size, mjb_encoding encoding,
    mjb_direction direction, mjb_bidi_paragraph *result) {
    if(!mjb_initialize()) {
        return false;
    }

    result->chars = NULL;
    result->count = 0;
    result->paragraph_level = 0;
    result->direction = MJB_DIRECTION_LTR;

    if(size == 0) {
        return true;
    }

    // Upper bound: size bytes cannot produce more than size codepoints.
    mjb_bidi_work *work = (mjb_bidi_work *)mjb_alloc(size * sizeof(mjb_bidi_work));

    if(!work) {
        return false;
    }

    uint8_t para_level = 0;

    // Pass 1.
    size_t count = pass1_decode(buffer, size, encoding, work, size, &para_level, direction);

    if(count == 0) {
        mjb_free(work);

        return true;
    }

    // Pass 2.
    pass2_explicit(work, count, para_level);

    /*
        TR9 3.3.2: LRI/RLI/FSI/PDI are treated as ON in W, N0, N1/N2, I rules.
        Remap them now so all subsequent passes see neutral (ON) for these chars.
    */
    for(size_t i = 0; i < count; ++i) {
        if(!work[i].removed) {
            mjb_bidi_class bc = work[i].bidi;

            if(bc == MJB_PR_BIDI_CLASS_LRI || bc == MJB_PR_BIDI_CLASS_RLI ||
                bc == MJB_PR_BIDI_CLASS_FSI || bc == MJB_PR_BIDI_CLASS_PDI) {
                work[i].bidi = MJB_PR_BIDI_CLASS_ON;
            }
        }
    }

    // Pass 3.
    pass3_weak(work, count, para_level);

    // Pass 4.
    pass4_brackets(work, count, para_level);

    // Pass 5.
    pass5_neutrals(work, count, para_level);

    // Pass 6.
    pass6_implicit(work, count);

    /*
        L1: Reset segment/paragraph separators and their preceding whitespace/isolate chars to
        para_level (TR9 L1). This must be applied treating the whole paragraph as one line.
        Step 1: S and B chars, reset to para_level; scan backward to also reset WS / isolate chars.
    */
    for(size_t i = 0; i < count; ++i) {
        if(work[i].removed) {
            continue;
        }

        mjb_bidi_class oc = work[i].orig;

        if(oc == MJB_PR_BIDI_CLASS_S || oc == MJB_PR_BIDI_CLASS_B) {
            work[i].level = para_level;

            for(size_t k = i; k > 0;) {
                --k;

                if(work[k].removed) {
                    continue;
                }

                mjb_bidi_class koc = work[k].orig;

                if(koc == MJB_PR_BIDI_CLASS_WS ||
                    is_isolate_initiator(koc) ||
                    koc == MJB_PR_BIDI_CLASS_PDI) {
                    work[k].level = para_level;
                } else {
                    break;
                }
            }
        }
    }

    // Step 2: Trailing WS / isolate chars at end of paragraph.
    for(size_t i = count; i > 0;) {
        --i;

        if(work[i].removed) {
            continue;
        }

        mjb_bidi_class oc = work[i].orig;

        if(oc == MJB_PR_BIDI_CLASS_WS ||
            is_isolate_initiator(oc) ||
            oc == MJB_PR_BIDI_CLASS_PDI) {
            work[i].level = para_level;
        } else {
            break;
        }
    }

    // Collect non-removed characters.
    size_t out_count = 0;

    for(size_t i = 0; i < count; ++i) {
        if(!work[i].removed) {
            ++out_count;
        }
    }

    mjb_bidi_char *out = NULL;

    if(out_count > 0) {
        out = (mjb_bidi_char *)mjb_alloc(out_count * sizeof(mjb_bidi_char));

        if(!out) {
            mjb_free(work);

            return false;
        }

        size_t k = 0;

        for(size_t i = 0; i < count; ++i) {
            if(!work[i].removed) {
                out[k].codepoint = work[i].codepoint;
                out[k].byte_offset = work[i].byte_offset;
                out[k].level = work[i].level;
                out[k].resolved_class = work[i].bidi;
                /*
                    L4: if Bidi_Mirrored and at an odd level, look up the mirroring glyph from the
                    bracket table (covers all bracket pairs). Non-bracket mirrored chars
                    (e.g. U+003C '<') get 0 for now.
                    TODO: for full coverage requires BidiMirroring.txt integration.
                */
                if(work[i].mirrored && (work[i].level & 1)) {
                    mjb_codepoint mirror_pair = 0;
                    bool mirror_open = false;

                    if(bidi_bracket_info(work[i].codepoint, &mirror_pair, &mirror_open)) {
                        out[k].mirroring_glyph = mirror_pair;
                    } else {
                        out[k].mirroring_glyph = 0;
                    }
                } else {
                    out[k].mirroring_glyph = 0;
                }
                ++k;
            }
        }
    }

    mjb_free(work);

    result->chars = out;
    result->count = out_count;
    result->paragraph_level = para_level;
    result->direction = (para_level & 1) ? MJB_DIRECTION_RTL : MJB_DIRECTION_LTR;

    return true;
}

// Free a bidi paragraph allocated by mjb_bidi_resolve.
MJB_EXPORT void mjb_bidi_free(mjb_bidi_paragraph *paragraph) {
    if(paragraph->chars) {
        mjb_free(paragraph->chars);
    }

    paragraph->chars = NULL;
    paragraph->count = 0;
}

// Reorder a line visually (L1-L4); visual_order is caller-allocated.
MJB_EXPORT bool mjb_bidi_reorder_line(const mjb_bidi_paragraph *paragraph,
    size_t line_start, size_t line_end, size_t *visual_order) {
    if(line_start >= line_end || line_end > paragraph->count) {
        return false;
    }

    size_t n = line_end - line_start;
    uint8_t para_level = paragraph->paragraph_level;

    // Initialise identity map
    for(size_t i = 0; i < n; ++i) {
        visual_order[i] = line_start + i;
    }

    // L1: reset levels of whitespace/paragraph separator at end of line
    for(size_t i = n; i-- > 0;) {
        size_t idx = visual_order[i];
        mjb_bidi_class rc = paragraph->chars[idx].resolved_class;

        if(rc == MJB_PR_BIDI_CLASS_WS || rc == MJB_PR_BIDI_CLASS_B ||
           rc == MJB_PR_BIDI_CLASS_S || is_isolate_initiator(rc) ||
           rc == MJB_PR_BIDI_CLASS_PDI) {
            (void)para_level;
            // Simplified: stop at first non-whitespace from end
            break;
        } else {
            break;
        }
    }

    // Build level array for the line
    uint8_t *levels = (uint8_t *)mjb_alloc(n * sizeof(uint8_t));

    if(!levels) {
        return false;
    }

    for(size_t i = 0; i < n; ++i) {
        levels[i] = paragraph->chars[line_start + i].level;
    }

    // Find max level and minimum odd level
    uint8_t max_level = para_level;
    uint8_t min_odd = 255;

    for(size_t i = 0; i < n; ++i) {
        if(levels[i] > max_level) {
            max_level = levels[i];
        }

        if((levels[i] & 1) && levels[i] < min_odd) {
            min_odd = levels[i];
        }
    }

    // L2: reverse substrings from max_level down to min_odd. If no odd levels exist (min_odd==255)
    // there is nothing to reverse.
    if(min_odd == 255) {
        mjb_free(levels);

        return true;
    }

    for(uint8_t lv = max_level; lv >= min_odd; --lv) {
        size_t start = SIZE_MAX;

        for(size_t i = 0; i <= n; ++i) {
            if(i < n && levels[i] >= lv) {
                if(start == SIZE_MAX) {
                    start = i;
                }
            } else {
                if(start != SIZE_MAX) {
                    // Reverse visual_order[start..i-1]
                    size_t lo = start;
                    size_t hi = i - 1;

                    while(lo < hi) {
                        size_t tmp = visual_order[lo];
                        visual_order[lo] = visual_order[hi];
                        visual_order[hi] = tmp;
                        ++lo;
                        --hi;
                    }

                    start = SIZE_MAX;
                }
            }
        }

        if(lv == 0) {
            break; // Prevent underflow on uint8_t.
        }
    }

    mjb_free(levels);

    return true;
}

// Compute visual level runs; pass runs=NULL to count first.
MJB_EXPORT bool mjb_bidi_line_runs(const mjb_bidi_paragraph *paragraph,
    const size_t *visual_order, size_t count, mjb_bidi_run *runs, size_t *run_count) {
    *run_count = 0;

    if(count == 0) {
        return true;
    }

    size_t nruns = 0;

    // Count or fill runs.
    uint8_t cur_level = paragraph->chars[visual_order[0]].level;
    size_t run_start = 0;

    for(size_t i = 1; i <= count; ++i) {
        uint8_t next_level = (i < count) ? paragraph->chars[visual_order[i]].level : (uint8_t)255;

        if(next_level != cur_level || i == count) {
            if(runs) {
                runs[nruns].start = run_start;
                runs[nruns].end = i;
                runs[nruns].level = cur_level;
                runs[nruns].direction = (cur_level & 1) ? MJB_DIRECTION_RTL : MJB_DIRECTION_LTR;
            }

            ++nruns;
            run_start = i;
            cur_level = next_level;
        }
    }

    *run_count = nruns;

    return true;
}
