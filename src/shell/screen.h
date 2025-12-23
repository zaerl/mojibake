/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#pragma once

#ifndef MJB_SCREEN_H
#define MJB_SCREEN_H

typedef void (*mjbsh_screen_fn)(const char* input);

void mjbsh_clear_screen(void);
void mjbsh_screen_mode(mjbsh_screen_fn fn);

void mjbsh_table_top(void);
void mjbsh_table_bottom(void);

#endif // MJB_SCREEN_H
