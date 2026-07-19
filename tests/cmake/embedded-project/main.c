/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <mojibake.h>

#ifndef MJB_PARENT_FLAGS_PRESERVED
#error "The parent project's C flags were not preserved"
#endif

int main(void) {
    return mjb_version_number() > 0 ? 0 : 1;
}
