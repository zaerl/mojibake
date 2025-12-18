/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#pragma once

#ifndef MJB_SCREEN_H
#define MJB_SCREEN_H

typedef void (*screen_fn)(const char* input);

void clear_screen(void);
void screen_mode(screen_fn fn);

void table_top(void);
void table_bottom(void);

#endif // MJB_SCREEN_H
