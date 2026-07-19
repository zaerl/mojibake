/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#ifndef MJB_SHELL_MAPS_H
#define MJB_SHELL_MAPS_H

#include "../mojibake.h"

const char *mjbsh_category_name(mjb_category category);
char *mjbsh_ccc_name(mjb_canonical_combining_class ccc);
const char *mjbsh_bidi_name(mjb_bidi_class bidi);
const char *mjbsh_decomposition_name(mjb_decomposition decomposition);
// const char *mjbsh_line_breaking_class_name(mjb_line_breaking_class line_breaking_class);
const char *mjbsh_east_asian_width_name(mjb_east_asian_width east_asian_width);

#endif // MJB_SHELL_MAPS_H
