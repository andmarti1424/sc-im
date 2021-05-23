/*******************************************************************************
 * Copyright (c) 2013-2021, Andrés Martinelli <andmarti@gmail.com>             *
 * All rights reserved.                                                        *
 *                                                                             *
 * This file is a part of SC-IM                                                *
 *                                                                             *
 * SC-IM is a spreadsheet program that is based on SC. The original authors    *
 * of SC are James Gosling and Mark Weiser, and mods were later added by       *
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
 * \file cmds.h
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 2021-05-02
 * \brief Header file for cmds.c
 */

#include "sc.h"
#include "macros.h"
#include <wchar.h>

extern char insert_edit_submode;       // insert or edit submode
extern wchar_t inputline[BUFFERSIZE];
extern int inputline_pos;
extern int real_inputline_pos;
extern struct block * lastcmd_buffer;

int is_single_command (struct block * buf, long timeout);
void enter_cell_content(int r, int c, char * submode,  wchar_t * content);
void send_to_interp(wchar_t * oper);   // Send command to interpreter
void send_to_interpp(char * oper);
void chg_mode(char strcmd);            // Change mode function
int modcheck();                        // Verify if open file has been modified
int savefile();                        // Save open file
void copyent(struct ent * n, struct ent * p, int dr, int dc, int r1, int c1, int r2, int c2, int special);
void flush_saved();
void insert_row(int after);
void insert_col(int after);
void deleterow(int row, int mult);
void int_deleterow(int row, int multi);
void deletecol();
void int_deletecol(int col, int mult);
void formatcol(int c);
void del_selected_cells();
struct ent * lookat(struct sheet * sh, int row, int col); // return pointer to 'ent' of cell. Create it if it doesn't exist
void cleanent(struct ent * p);         // Initialize 'ent' to zero. Won't free memory
void clearent(struct ent * v);         // free 'ent' memory.
int locked_cell(int r, int c);
int any_locked_cells(int r1, int c1, int r2, int c2);
void scroll_left (int n);
void scroll_right (int n);
void scroll_down(int n);
void scroll_up(int n);
struct ent * left_limit();
struct ent * right_limit(int row);
struct ent * goto_top();
struct ent * goto_bottom();
struct ent * tick(char c);             // 'tick' ( ' ) command
struct ent * forw_row(int arg);
struct ent * back_row(int arg);
struct ent * forw_col(int arg);
struct ent * back_col(int arg);
struct ent * go_home();
struct ent * go_end();
struct ent * go_forward();
struct ent * go_backward();
struct ent * vert_top();
struct ent * vert_middle();
struct ent * vert_bottom();
struct ent * go_bol();
struct ent * go_eol();
struct ent * horiz_middle();
struct ent * goto_last_col();
void select_inner_range(int * vir_tlrow, int * vir_tlcol, int * vir_brrow, int * vir_brcol);
void ljustify(int sr, int sc, int er, int ec);
void rjustify(int sr, int sc, int er, int ec);
void center(int sr, int sc, int er, int ec);
void doformat(int c1, int c2, int w, int p, int r);
void dorowformat(int r, unsigned char size);
int etype(struct enode *e);
void erase_area(int sr, int sc, int er, int ec, int ignorelock, int mark_ent_as_deleted);
void auto_fit(int ci, int cf, int min);
void valueize_area(int sr, int sc, int er, int ec);
void sync_refs();
void syncref(struct enode * e);
int fcopy(char * action);
int fsum();
int pad(int n, int r1, int c1, int r2, int c2);
void fix_row_hidden(int deltar, int ri, int rf);
void fix_col_hidden(int deltac, int ci, int cf);
void fix_row_frozen(int deltar, int ri, int rf);
void fix_col_frozen(int deltac, int ci, int cf);

void mark_ent_as_deleted(struct ent * p, int del);
int calc_mobile_rows(int *);
int calc_mobile_cols(int *);
void pad_and_align (char * str_value, char * numeric_value, int col_width, int align, int padding, wchar_t * str_out, int rowfmt);
