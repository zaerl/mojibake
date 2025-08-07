/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

 #include "buffer.h"
 #include "mojibake.h"

 extern struct mojibake mjb_global;

 static bool mjb_codepoint_is_allowed(mjb_codepoint codepoint) {
    return codepoint >= 0x0 && codepoint <= 0x10FFFF;
 }

 /**
  * Normalize a string
  * See: https://unicode.org/reports/tr15/#Detecting_Normalization_Forms
  */
 MJB_EXPORT mjb_quick_check_result mjb_string_is_normalized(const char *buffer, size_t size, mjb_encoding encoding, mjb_normalization form) {
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

    const char *index = buffer;
    mjb_quick_check_result result = MJB_QC_NO;
    uint8_t state = MJB_UTF8_ACCEPT;
    mjb_codepoint current_codepoint;
    bool is_ascii = false;
    bool is_latin_1 = false;

    // Scan a ASCII or latin-1 string to check if it is already well-formed.
    // See: https://unicode.org/reports/tr15/#Description_Norm
    for(; *index; ++index) {
        // Find next codepoint.
        state = mjb_utf8_decode_step(state, *index, &current_codepoint);

        if(state == MJB_UTF8_REJECT) {
            return MJB_QC_NO;
        }

        if(state != MJB_UTF8_ACCEPT) {
            continue;
        }

        // Text exclusively containing ASCII characters (U+0000..U+007F) is left unaffected by all
        // of the Normalization Forms.
        if(current_codepoint < 0x80) {
            is_ascii = true;
            continue;
        }

        // Text exclusively containing Latin-1 characters (U+0000..U+00FF) is left unaffected by NFC.
        if(current_codepoint < 0x100 && form == MJB_NORMALIZATION_NFC) {
            is_latin_1 = true;
            continue;
        }

        is_ascii = false;
        is_latin_1 = false;
        break;
    }

    if(is_ascii || is_latin_1) {
        return MJB_QC_YES;
    }

    index = buffer;
    mjb_canonical_combining_class last_canonical_class = MJB_CCC_NOT_REORDERED;
    mjb_normalization_character current_character;
    result = MJB_QC_NO;

    /*result = MJB_QC_YES;

    /*for(; *index; ++index) {
        // Find next codepoint.
        state = mjb_utf8_decode_step(state, *index, &current_codepoint);

        if(state != MJB_UTF8_ACCEPT) {
            continue;
        }

        // The codepoint is in the supplementary character range.
        if(current_codepoint >= 0x10000 && current_codepoint <= MJB_CODEPOINT_MAX) {
            ++index;
        }

        // Get current character.
        if(!mjb_get_buffer_character(&current_character, current_codepoint)) {
            continue;
        }

        if(last_canonical_class > current_character.combining && current_character.combining != MJB_CCC_NOT_REORDERED) {
            return MJB_QC_NO;
        }

        if(current_character.decomposition != MJB_DECOMPOSITION_NONE) {
            continue;
        }

        last_canonical_class = current_character.combining;
    }*/

    return result;
 }
