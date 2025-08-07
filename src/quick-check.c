/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

 #include "mojibake.h"

 /**
  * Normalize a string
  */
 MJB_EXPORT bool mjb_string_is_normalized(const char *buffer, size_t size, mjb_encoding encoding, mjb_normalization form) {
    if(encoding != MJB_ENCODING_UTF_8) {
        return false;
    }

    if(form != MJB_NORMALIZATION_NFD && form != MJB_NORMALIZATION_NFKD &&
        form != MJB_NORMALIZATION_NFC && form != MJB_NORMALIZATION_NFKC) {
        return false;
    }

    if(size == 0) {
        return true;
    }

    const char *index = buffer;
    bool well_formed = true;
    uint8_t state = MJB_UTF8_ACCEPT;
    mjb_codepoint current_codepoint;

    // Scan a string to check if it is already well-formed.
    // See: https://unicode.org/reports/tr15/#Description_Norm
    for(; *index; ++index) {
        // Find next codepoint.
        state = mjb_utf8_decode_step(state, *index, &current_codepoint);

        if(state == MJB_UTF8_REJECT) {
            well_formed = false;
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
        well_formed = false;
        break;
    }

    return well_formed;
 }
