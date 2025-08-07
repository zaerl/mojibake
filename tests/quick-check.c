/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

 #include <stdio.h>
 #include <string.h>

 #include "test.h"

 void *test_quick_check(void *arg) {
    mjb_encoding enc = MJB_ENCODING_UTF_8;

    ATT_ASSERT(mjb_string_is_normalized("", 0, enc, MJB_NORMALIZATION_NFC), MJB_QC_YES, "Empty string is normalized")
    ATT_ASSERT(mjb_string_is_normalized("", 0, enc, MJB_NORMALIZATION_NFD), MJB_QC_YES, "Empty string is normalized")
    ATT_ASSERT(mjb_string_is_normalized("", 0, enc, MJB_NORMALIZATION_NFKC), MJB_QC_YES, "Empty string is normalized")
    ATT_ASSERT(mjb_string_is_normalized("", 0, enc, MJB_NORMALIZATION_NFKD), MJB_QC_YES, "Empty string is normalized")

    ATT_ASSERT(mjb_string_is_normalized("abc", 3, enc, MJB_NORMALIZATION_NFC), MJB_QC_YES, "ASCII string is NFC normalized")
    ATT_ASSERT(mjb_string_is_normalized("def", 3, enc, MJB_NORMALIZATION_NFD), MJB_QC_YES, "ASCII string is NFD normalized")
    ATT_ASSERT(mjb_string_is_normalized("ghi", 3, enc, MJB_NORMALIZATION_NFKC), MJB_QC_YES, "ASCII string is NFKC normalized")
    ATT_ASSERT(mjb_string_is_normalized("jkl", 3, enc, MJB_NORMALIZATION_NFKD), MJB_QC_YES, "ASCII string is NFKD normalized")

    ATT_ASSERT(mjb_string_is_normalized("áéíóú", 10, enc, MJB_NORMALIZATION_NFC), MJB_QC_YES, "Latin-1 string is NFC normalized")

    return NULL;
 }
