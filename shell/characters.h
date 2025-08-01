/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "../src/mojibake.h"

bool next_character(mjb_character *character, mjb_next_character_type type);
bool next_array_character(mjb_character *character, mjb_next_character_type type);
bool next_string_character(mjb_character *character, mjb_next_character_type type);
bool next_escaped_character(mjb_character *character, mjb_next_character_type type);
bool next_current_character(mjb_character *character, mjb_next_character_type type);
