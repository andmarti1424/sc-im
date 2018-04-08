/*******************************************************************************
 * Copyright (c) 2013-2017, Andrés Martinelli <andmarti@gmail.com              *
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
 * \file undo.h
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 2017-07-18
 * \brief Header file for undo.c
 */

struct undo {
    struct undo * p_ant;
    struct ent * added;
    struct ent * removed;
    struct ent * aux_ents;          // add e_new to beginning of list
    struct undo_range_shift * range_shift;
    struct undo_cols_format * cols_format;
    struct undo * p_sig;
    int * row_hidded;
    int * row_showed;
    int * col_hidded;
    int * col_showed;
};

struct undo_range_shift {
    int delta_rows;
    int delta_cols;
    int tlrow;
    int tlcol;
    int brrow;
    int brcol;
};

//These two structures are for undo / redo changes in column format
struct undo_col_info {
    char type;       // a column can be 'R' (removed) or 'A' (added) because of change
    int col;
    int fwidth;
    int precision;
    int realfmt;
};

struct undo_cols_format {
    size_t length;   // keep the number of elements (cols)
    struct undo_col_info * cols;
};

void create_undo_action();
void end_undo_action();
void copy_to_undostruct (int row_desde, int col_desde, int row_hasta, int col_hasta, char type);
void save_undo_range_shift(int delta_rows, int delta_cols, int tlrow, int tlcol, int brrow, int brcol);
void undo_hide_show(int row, int col, char type, int arg);
void add_undo_col_format(int col, int type, int fwidth, int precision, int realfmt);

void add_to_undolist(struct undo u);
void do_undo();
void do_redo();

void clear_undo_list ();
void clear_from_current_pos();
int len_undo_list();
void free_undo_node(struct undo * ul);
void dismiss_undo_item(struct undo * ul);
struct ent * add_undo_aux_ent(struct ent * e);
