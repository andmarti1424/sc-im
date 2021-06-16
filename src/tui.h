/*******************************************************************************
 * Copyright (c) 2013-2021, Andrés Martinelli <andmarti@gmail.com>             *
 * All rights reserved.                                                        *
 *                                                                             *
 * This file is a part of sc-im                                                *
 *                                                                             *
 * sc-im is a spreadsheet program that is based on sc. The original authors    *
 * of sc are James Gosling and Mark Weiser, and mods were later added by       *
 * Chuck Martin.                                                               *
 *                                                                             *
 * Redistribution and use in source and binary forms, with or without          *
 * modification, are permitted provided that the following conditions are met: *
 * 1. Redistributions of source code must retain the above copyright           *
 *    notice, this list of conditions and the following disclaimer.            *
 * 2. Redistributions in binary form must reproduce the above copyright        *
 *    notice, this list of conditions and the following disclaimer in the      *
 *    documentation and/or other materials provided with the distribution.     *
 * 3. All advertising materials mentioning features or use of this software    *
 *    must display the following acknowledgement:                              *
 *    This product includes software developed by Andrés Martinelli            *
 *    <andmarti@gmail.com>.                                                    *
 * 4. Neither the name of the Andrés Martinelli nor the                        *
 *   names of other contributors may be used to endorse or promote products    *
 *   derived from this software without specific prior written permission.     *
 *                                                                             *
 * THIS SOFTWARE IS PROVIDED BY ANDRES MARTINELLI ''AS IS'' AND ANY            *
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED   *
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE      *
 * DISCLAIMED. IN NO EVENT SHALL ANDRES MARTINELLI BE LIABLE FOR ANY           *
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES  *
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;*
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND *
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT  *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE       *
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.           *
 *******************************************************************************/

/**
 * \file tui.h
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 2017-07-18
 * \brief Header file for tui.c
 */

#include <ncurses.h>
#include <wchar.h>
#ifdef XLUA
#include <lua.h>
#endif
#include "color.h"

#define NONE_COLOR        -2
#define DEFAULT_COLOR     -1
#define BLACK             COLOR_BLACK
#define RED               COLOR_RED
#define GREEN             COLOR_GREEN
#define YELLOW            COLOR_YELLOW
#define BLUE              COLOR_BLUE
#define MAGENTA           COLOR_MAGENTA
#define CYAN              COLOR_CYAN
#define WHITE             COLOR_WHITE

extern int offscr_sc_rows, offscr_sc_cols;
extern int nb_frozen_rows, nb_frozen_cols;

extern unsigned int curmode;
extern struct srange * ranges;
extern struct ent ** p;

void ui_start_screen();
void ui_stop_screen();
int ui_getch(wint_t * wd);
int ui_getch_b(wint_t * wd);
void ui_clr_header(int row);
void ui_refresh_pad(int scroll);
void ui_print_mult_pend();
void ui_show_header();
void ui_show_celldetails();
void ui_print_mode();
void ui_do_welcome();
void ui_update(int header);
int ui_get_formated_value(struct ent ** p, int col, char * value);
void ui_handle_cursor();
void yyerror(char *err);               // error routine for yacc (gram.y)
void ui_show_text(char * val);
#ifdef XLUA
void ui_bail(lua_State *L, char * msg);
#endif
char * ui_query(char * initial_msg);
void ui_start_colors();
void ui_sc_msg(char * s, int type, ...);

void ui_set_ucolor(WINDOW * w, struct ucolor * uc, int bg_override);
int ui_show_content(WINDOW * win, int mxrow, int mxcol);
void ui_show_sc_row_headings(WINDOW * win, int mxrow);
void ui_show_sc_col_headings(WINDOW * win, int mxcol);
void ui_add_cell_detail(char * d, struct ent * p1);
void ui_write_j(WINDOW * win, const char * word, const unsigned int row, const unsigned int justif);
void ui_show_cursor(WINDOW * win);
void ui_pause();
void ui_resume();
wchar_t ui_query_opt(wchar_t * initial_msg, wchar_t * valid);
void ui_mv_bottom_bar();
#ifdef MOUSE
void ui_handle_mouse(MEVENT event);
#endif
