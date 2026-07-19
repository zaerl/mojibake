/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#ifndef MJB_SHELL_CHARACTERS_H
#define MJB_SHELL_CHARACTERS_H

#include "../mojibake.h"

bool mjbsh_next_character(mjb_character *character, mjb_character_position type);
bool mjbsh_next_array_character(mjb_character *character, mjb_character_position type);
bool mjbsh_next_string_character(mjb_character *character, mjb_character_position type);
bool mjbsh_next_escaped_character(mjb_character *character, mjb_character_position type);

#endif // MJB_SHELL_CHARACTERS_H
