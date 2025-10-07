/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "../mojibake.h"
#include "../breaking.h"
#include "../east-asian-width.h"

const char *category_name(mjb_category category);
char *ccc_name(mjb_canonical_combining_class ccc);
const char *bidi_name(mjb_bidi_categories bidi);
const char *decomposition_name(mjb_decomposition decomposition);
const char *line_breaking_class_name(mjb_line_breaking_class line_breaking_class);
const char *east_asian_width_name(mjb_east_asian_width east_asian_width);
