/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "../../src/mojibake.h"

int character_command(int argc, char * const argv[]);
int normalize_command(int argc, char * const argv[], mjb_normalization form);
int normalize_string_command(int argc, char * const argv[], mjb_normalization form);
