/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>

#include "test.h"

int test_quick_check(void *arg) {
    mjb_encoding enc = MJB_ENC_UTF_8;

    ATT_ASSERT((unsigned int)mjb_normalization_quick_check(NULL, 1, enc, MJB_NORMALIZATION_NFC),
        (unsigned int)MJB_QC_NO, "NULL string is not normalized")
    ATT_ASSERT((unsigned int)mjb_normalization_quick_check("", 0, enc, MJB_NORMALIZATION_NFC),
        (unsigned int)MJB_QC_YES, "Empty string is normalized")
    ATT_ASSERT((unsigned int)mjb_normalization_quick_check("", 0, enc, MJB_NORMALIZATION_NFD),
        (unsigned int)MJB_QC_YES, "Empty string is normalized")
    ATT_ASSERT((unsigned int)mjb_normalization_quick_check("", 0, enc, MJB_NORMALIZATION_NFKC),
        (unsigned int)MJB_QC_YES, "Empty string is normalized")
    ATT_ASSERT((unsigned int)mjb_normalization_quick_check("", 0, enc, MJB_NORMALIZATION_NFKD),
        (unsigned int)MJB_QC_YES, "Empty string is normalized")

    ATT_ASSERT((unsigned int)mjb_normalization_quick_check("abc", 3, enc, MJB_NORMALIZATION_NFC),
        (unsigned int)MJB_QC_YES, "ASCII string is NFC normalized")
    ATT_ASSERT((unsigned int)mjb_normalization_quick_check("def", 3, enc, MJB_NORMALIZATION_NFD),
        (unsigned int)MJB_QC_YES, "ASCII string is NFD normalized")
    ATT_ASSERT((unsigned int)mjb_normalization_quick_check("ghi", 3, enc, MJB_NORMALIZATION_NFKC),
        (unsigned int)MJB_QC_YES, "ASCII string is NFKC normalized 1")
    ATT_ASSERT((unsigned int)mjb_normalization_quick_check("jkl", 3, enc, MJB_NORMALIZATION_NFKD),
        (unsigned int)MJB_QC_YES, "ASCII string is NFKD normalized 2")

    ATT_ASSERT((unsigned int)mjb_normalization_quick_check("áéíóú", 10, enc, MJB_NORMALIZATION_NFC),
        (unsigned int)MJB_QC_YES, "Latin-1 string is NFC normalized")

    return 0;
}
