/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "../mojibake.h"

bool mjbsh_next_character(mjb_character *character, mjb_next_character_type type);
bool mjbsh_next_array_character(mjb_character *character, mjb_next_character_type type);
bool mjbsh_next_string_character(mjb_character *character, mjb_next_character_type type);
bool mjbsh_next_escaped_character(mjb_character *character, mjb_next_character_type type);
bool mjbsh_next_current_character(mjb_character *character, mjb_next_character_type type);
