/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "../../test.h"
#include "../../../src/cpp/mojibake.hpp"

int test_cpp_normalization(void *arg) {
    ATT_ASSERT(mjb::nfc(""), std::string(""), "nfc(\"\")")
    ATT_ASSERT(mjb::nfd(""), std::string(""), "nfd(\"\")")
    ATT_ASSERT(mjb::nfkc(""), std::string(""), "nfkc(\"\")")
    ATT_ASSERT(mjb::nfkd(""), std::string(""), "nfkd(\"\")")

    ATT_ASSERT(mjb::nfc("a"), std::string("a"), "nfc(\"a\")")
    ATT_ASSERT(mjb::nfd("a"), std::string("a"), "nfd(\"a\")")
    ATT_ASSERT(mjb::nfkc("a"), std::string("a"), "nfkc(\"a\")")
    ATT_ASSERT(mjb::nfkd("a"), std::string("a"), "nfkd(\"a\")")
    ATT_ASSERT(mjb::nfkc_casefold("Stra\xC3\x9F" "e\xC2\xAD"), std::string("strasse"),
        "nfkc_casefold")

    return 0;
}
