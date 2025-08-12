/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "mojibake-internal.h"
#include "buffer.h"
#include "utf8.h"

extern mojibake mjb_global;

/**
 * Normalize a string
 * See: https://unicode.org/reports/tr15/#Detecting_Normalization_Forms
 */
MJB_EXPORT mjb_quick_check_result mjb_string_is_normalized(const char *buffer, size_t size,
    mjb_encoding encoding, mjb_normalization form) {
    if(encoding != MJB_ENCODING_UTF_8) {
        return MJB_QC_NO;
    }

    if(form != MJB_NORMALIZATION_NFD && form != MJB_NORMALIZATION_NFKD &&
        form != MJB_NORMALIZATION_NFC && form != MJB_NORMALIZATION_NFKC) {
        return MJB_QC_NO;
    }

    if(size == 0) {
        return MJB_QC_YES;
    }

    mjb_quick_check_result result = MJB_QC_NO;
    uint8_t state = MJB_UTF8_ACCEPT;
    mjb_codepoint current_codepoint;
    mjb_canonical_combining_class last_canonical_class = MJB_CCC_NOT_REORDERED;
    mjb_normalization_character current_character;
    result = MJB_QC_YES;

    const char *index = buffer;
    const char *end = buffer + size;

    for(; index < end && *index; ++index) {
        // Find next codepoint.
        state = mjb_utf8_decode_step(state, *index, &current_codepoint);

        if(state != MJB_UTF8_ACCEPT) {
            continue;
        }

        // Text exclusively containing ASCII characters (U+0000..U+007F) is left unaffected by all
        // of the Normalization Forms.
        if(current_codepoint < 0x80) {
            continue;
        }

        // Text with only Latin-1 characters (U+0000..U+00FF) is left unaffected by NFC.
        if(current_codepoint < 0x100 && form == MJB_NORMALIZATION_NFC) {
            continue;
        }

        // Get current character.
        if(!mjb_get_buffer_character(&current_character, current_codepoint)) {
            continue;
        }

        if(last_canonical_class > current_character.combining && current_character.combining !=
            MJB_CCC_NOT_REORDERED) {
            return MJB_QC_NO;
        }

        if(current_character.quick_check == MJB_QC_NO) {
            return MJB_QC_NO;
        }

        bool is_hangul_syllable = mjb_codepoint_is_hangul_syllable(current_codepoint);

        switch(form) {
            case MJB_NORMALIZATION_NFC:
                if(current_character.quick_check & MJB_QC_NFC_MAYBE) {
                    result = MJB_QC_MAYBE;
                } else if(current_character.quick_check & MJB_QC_NFC_NO) {
                    return MJB_QC_NO;
                }

                break;
            case MJB_NORMALIZATION_NFKC:
                if(current_character.quick_check & MJB_QC_NFKC_MAYBE) {
                    result = MJB_QC_MAYBE;
                } else if(current_character.quick_check & MJB_QC_NFKC_NO) {
                    return MJB_QC_NO;
                }

                break;
            case MJB_NORMALIZATION_NFD:
                if(is_hangul_syllable) {
                    return MJB_QC_NO;
                }

                // There are no MAYBE values for NFD.
                if(current_character.quick_check & MJB_QC_NFD_NO) {
                    return MJB_QC_NO;
                }

                break;
            case MJB_NORMALIZATION_NFKD:
                if(is_hangul_syllable) {
                    return MJB_QC_NO;
                }

                // There are no MAYBE values for NFKD.
                if(current_character.quick_check & MJB_QC_NFKD_NO) {
                    return MJB_QC_NO;
                }

                break;
        }

        last_canonical_class = current_character.combining;
    }

    return result;
}
