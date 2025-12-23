/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "../mojibake.h"
#include "../breaking.h"
#include "../east-asian-width.h"

const char *mjbsh_category_name(mjb_category category);
char *mjbsh_ccc_name(mjb_canonical_combining_class ccc);
const char *mjbsh_bidi_name(mjb_bidi_categories bidi);
const char *mjbsh_decomposition_name(mjb_decomposition decomposition);
const char *mjbsh_line_breaking_class_name(mjb_line_breaking_class line_breaking_class);
const char *mjbsh_east_asian_width_name(mjb_east_asian_width east_asian_width);
