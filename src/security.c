/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <string.h>

#include "mojibake-internal.h"
#include "utf.h"

extern mojibake mjb_global;

// Maximum number of codepoints a single codepoint's skeleton can expand to.
#define MJB_SKELETON_MAX_EXPAND 8

// Look up the confusable skeleton for a codepoint.
// Returns the number of skeleton codepoints written to result (0 if no mapping exists).
static size_t confusable_lookup(mjb_codepoint codepoint, mjb_codepoint *result,
    size_t result_size) {
    sqlite3_bind_int(mjb_global.stmt_confusable, 1, (int)codepoint);

    if(sqlite3_step(mjb_global.stmt_confusable) == SQLITE_ROW) {
        const uint8_t *blob = (const uint8_t *)sqlite3_column_blob(mjb_global.stmt_confusable, 0);
        int bytes = sqlite3_column_bytes(mjb_global.stmt_confusable, 0);
        size_t count = (size_t)(bytes / 4);

        if(count > result_size) {
            count = result_size;
        }

        for(size_t i = 0; i < count; ++i) {
            result[i] = ((uint32_t)blob[i * 4] << 24) |
                ((uint32_t)blob[i * 4 + 1] << 16) |
                ((uint32_t)blob[i * 4 + 2] <<  8) |
                ((uint32_t)blob[i * 4 + 3]);
        }

        sqlite3_reset(mjb_global.stmt_confusable);

        return count;
    }

    sqlite3_reset(mjb_global.stmt_confusable);

    return 0;
}

// Compute the Unicode security skeleton of a string (UTS#39 §4).
// Algorithm: NFD(input) -> per-codepoint skeleton substitution -> NFD.
static bool mjb_string_skeleton(const char *buffer, size_t size, mjb_encoding encoding,
    mjb_result *result) {
    if(size == 0) {
        result->output = (char *)buffer;
        result->output_size = 0;
        result->transformed = false;

        return true;
    }

    if(!mjb_initialize()) {
        return false;
    }

    // Step 1: NFD the input.
    mjb_result nfd;

    if(!mjb_normalize(buffer, size, encoding, MJB_NORMALIZATION_NFD, MJB_ENCODING_UTF_8, &nfd)) {
        return false;
    }

    // Step 2: Replace each codepoint with its skeleton mapping.
    // Worst case: each byte expands to MJB_SKELETON_MAX_EXPAND codepoints of 4 UTF-8 bytes each.
    size_t mid_size = nfd.output_size * MJB_SKELETON_MAX_EXPAND * 4 + 4;
    char *mid = (char *)mjb_alloc(mid_size);

    if(!mid) {
        if(nfd.transformed) {
            mjb_free(nfd.output);
        }

        return false;
    }

    size_t mid_index = 0;
    uint8_t state = MJB_UTF_ACCEPT;
    mjb_codepoint cp;
    bool in_error = false;

    for(size_t i = 0; i < nfd.output_size;) {
        mjb_decode_result dr = mjb_next_codepoint(nfd.output, nfd.output_size, &state, &i,
            MJB_ENCODING_UTF_8, &cp, &in_error);

        if(dr == MJB_DECODE_END) {
            break;
        }

        if(dr == MJB_DECODE_INCOMPLETE) {
            continue;
        }

        if(dr == MJB_DECODE_ERROR) {
            if(nfd.transformed) {
                mjb_free(nfd.output);
            }

            mjb_free(mid);

            return false;
        }

        mjb_codepoint skeleton[MJB_SKELETON_MAX_EXPAND];
        size_t skel_count = confusable_lookup(cp, skeleton, MJB_SKELETON_MAX_EXPAND);

        if(skel_count == 0) {
            skeleton[0] = cp;
            skel_count = 1;
        }

        for(size_t j = 0; j < skel_count; ++j) {
            unsigned int encoded = mjb_codepoint_encode(skeleton[j], mid + mid_index,
                mid_size - mid_index, MJB_ENCODING_UTF_8);

            if(encoded == 0) {
                if(nfd.transformed) {
                    mjb_free(nfd.output);
                }

                mjb_free(mid);

                return false;
            }

            mid_index += encoded;
        }
    }

    if(nfd.transformed) {
        mjb_free(nfd.output);
    }

    // Step 3: NFD the intermediate string.
    bool ok = mjb_normalize(mid, mid_index, MJB_ENCODING_UTF_8, MJB_NORMALIZATION_NFD,
        MJB_ENCODING_UTF_8, result);

    // mjb_normalize may return result->output == mid when the string is already NFD.
    // In that case transfer ownership so the caller can free it via mjb_free(result->output).
    if(ok && !result->transformed) {
        result->transformed = true;
    } else {
        mjb_free(mid);
    }

    return ok;
}

// Return true if two strings are visually confusable (UTS#39 §4).
// Two strings are confusable if their skeletons are byte-identical.
MJB_EXPORT bool mjb_string_is_confusable(const char *s1, size_t s1_size,
    const char *s2, size_t s2_size, mjb_encoding encoding) {
    mjb_result skel1;
    mjb_result skel2;

    if(!mjb_string_skeleton(s1, s1_size, encoding, &skel1)) {
        return false;
    }

    if(!mjb_string_skeleton(s2, s2_size, encoding, &skel2)) {
        if(skel1.transformed) {
            mjb_free(skel1.output);
        }

        return false;
    }

    bool confusable = (skel1.output_size == skel2.output_size) && skel1.output_size > 0 &&
        (memcmp(skel1.output, skel2.output, skel1.output_size) == 0);

    if(skel1.transformed) {
        mjb_free(skel1.output);
    }

    if(skel2.transformed) {
        mjb_free(skel2.output);
    }

    return confusable;
}
