/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

 #include "mojibake.h"

 /**
  * Normalize a string
  * See: https://unicode.org/reports/tr15/#Detecting_Normalization_Forms
  */
 MJB_EXPORT mjb_quick_check_result mjb_string_is_normalized(const char *buffer, size_t size, mjb_encoding encoding, mjb_normalization form) {
    if(encoding != MJB_ENCODING_UTF_8) {
        return MJB_QUICK_CHECK_NO;
    }

    if(form != MJB_NORMALIZATION_NFD && form != MJB_NORMALIZATION_NFKD &&
        form != MJB_NORMALIZATION_NFC && form != MJB_NORMALIZATION_NFKC) {
        return MJB_QUICK_CHECK_NO;
    }

    if(size == 0) {
        return MJB_QUICK_CHECK_YES;
    }

    const char *index = buffer;
    mjb_quick_check_result result = MJB_QUICK_CHECK_YES;
    uint8_t state = MJB_UTF8_ACCEPT;
    mjb_codepoint current_codepoint;

    // Scan a string to check if it is already well-formed.
    // See: https://unicode.org/reports/tr15/#Description_Norm
    for(; *index; ++index) {
        // Find next codepoint.
        state = mjb_utf8_decode_step(state, *index, &current_codepoint);

        if(state == MJB_UTF8_REJECT) {
            result = MJB_QUICK_CHECK_NO;
            break;
        }

        if(state != MJB_UTF8_ACCEPT) {
            continue;
        }

        // Text exclusively containing ASCII characters (U+0000..U+007F) is left unaffected by all
        // of the Normalization Forms.
        if(current_codepoint < 0x80) {
            continue;
        }

        // Text exclusively containing Latin-1 characters (U+0000..U+00FF) is left unaffected by NFC.
        if(current_codepoint < 0x100 && form == MJB_NORMALIZATION_NFC) {
            continue;
        }

        // The string is not well-formed.
        result = MJB_QUICK_CHECK_NO;
        break;
    }

    return result;
 }
