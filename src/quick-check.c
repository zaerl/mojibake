/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "mojibake-internal.h"
#include "utf.h"

extern mojibake mjb_global;

/**
 * Check if a string is normalized to NFC/NFKC/NFD/NFKD form.
 * See: https://unicode.org/reports/tr15/#Detecting_Normalization_Forms
 */
MJB_EXPORT mjb_status mjb_normalization_quick_check(const char *buffer, size_t byte_length,
    mjb_encoding encoding, mjb_normalization form, mjb_quick_check_result *quick_check) {
    if(quick_check == NULL) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    *quick_check = MJB_QC_NO;

    if(buffer == NULL && byte_length > 0) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    if(form != MJB_NORMALIZATION_NFD && form != MJB_NORMALIZATION_NFKD &&
        form != MJB_NORMALIZATION_NFC && form != MJB_NORMALIZATION_NFKC) {
        return MJB_STATUS_INVALID_FORM;
    }

    if(!mjb_encoding_is_valid_input(encoding)) {
        return MJB_STATUS_INVALID_ENCODING;
    }

    if(byte_length == 0) {
        *quick_check = MJB_QC_YES;

        return MJB_STATUS_OK;
    }

    size_t resolved_index = 0;
    mjb_encoding resolved_encoding = mjb_resolve_input_encoding(buffer, byte_length, encoding,
        &resolved_index);

    if((encoding == MJB_ENC_UTF_16 || encoding == MJB_ENC_UTF_32) &&
        resolved_encoding == encoding) {
        return MJB_STATUS_INVALID_ENCODING;
    }

    mjb_quick_check_result result = MJB_QC_YES;
    uint8_t state = MJB_UTF_ACCEPT;
    mjb_codepoint codepoint;
    mjb_canonical_combining_class last_canonical_class = MJB_CCC_NOT_REORDERED;
    mjb_n_character current_character;
    bool in_error = false;

    for(size_t i = 0; i < byte_length;) {
        // Find next codepoint.
        mjb_decode_result decode_status = mjb_next_codepoint(buffer, byte_length, &state, &i,
            encoding, &codepoint, &in_error);

        if(decode_status == MJB_DECODE_END) {
            break;
        }

        if(decode_status == MJB_DECODE_INCOMPLETE) {
            continue;
        }

        if(decode_status == MJB_DECODE_ERROR) {
            return MJB_STATUS_MALFORMED_INPUT;
        }

        // Text exclusively containing ASCII characters (U+0000..U+007F) is left unaffected by all
        // of the Normalization Forms.
        if(codepoint < 0x80) {
            continue;
        }

        // Text with only Latin-1 characters (U+0000..U+00FF) is left unaffected by NFC.
        if(codepoint < 0x100 && form == MJB_NORMALIZATION_NFC) {
            continue;
        }

        // Get current character.
        if(!mjb_n_codepoint_character(codepoint, &current_character)) {
            continue;
        }

        if(last_canonical_class > current_character.combining &&
            current_character.combining != MJB_CCC_NOT_REORDERED) {
            *quick_check = MJB_QC_NO;

            return MJB_STATUS_OK;
        }

        if(current_character.quick_check == MJB_QC_NO) {
            *quick_check = MJB_QC_NO;

            return MJB_STATUS_OK;
        }

        bool is_hangul_syllable = mjb_codepoint_is_hangul_syllable(codepoint);

        switch(form) {
            case MJB_NORMALIZATION_NFC:
                if(current_character.quick_check & MJB_QC_NFC_MAYBE) {
                    result = MJB_QC_MAYBE;
                } else if(current_character.quick_check & MJB_QC_NFC_NO) {
                    *quick_check = MJB_QC_NO;

                    return MJB_STATUS_OK;
                }

                break;
            case MJB_NORMALIZATION_NFKC:
                if(current_character.quick_check & MJB_QC_NFKC_MAYBE) {
                    result = MJB_QC_MAYBE;
                } else if(current_character.quick_check & MJB_QC_NFKC_NO) {
                    *quick_check = MJB_QC_NO;

                    return MJB_STATUS_OK;
                }

                break;
            case MJB_NORMALIZATION_NFD:
                if(is_hangul_syllable) {
                    *quick_check = MJB_QC_NO;

                    return MJB_STATUS_OK;
                }

                // There are no MAYBE values for NFD.
                if(current_character.quick_check & MJB_QC_NFD_NO) {
                    *quick_check = MJB_QC_NO;

                    return MJB_STATUS_OK;
                }

                break;
            case MJB_NORMALIZATION_NFKD:
                if(is_hangul_syllable) {
                    *quick_check = MJB_QC_NO;

                    return MJB_STATUS_OK;
                }

                // There are no MAYBE values for NFKD.
                if(current_character.quick_check & MJB_QC_NFKD_NO) {
                    *quick_check = MJB_QC_NO;

                    return MJB_STATUS_OK;
                }

                break;
        }

        last_canonical_class = (mjb_canonical_combining_class)current_character.combining;
    }

    if(mjb_utf_state_is_incomplete(state)) {
        return MJB_STATUS_MALFORMED_INPUT;
    }

    *quick_check = result;

    return MJB_STATUS_OK;
}
