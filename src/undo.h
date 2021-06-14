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
 * \file undo.h
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 2017-07-18
 * \brief Header file for undo.c
 */

#define UNDO_ADD 'a'
#define UNDO_DEL 'd'
#define HANDLE_DEPS 1
#define IGNORE_DEPS 0

struct undo {
    struct undo * p_ant;
    struct ent_ptr * added;
    struct ent_ptr * removed;
    struct allocation_list * allocations;
    struct sheet * sheet; // the sheet where the action took place
    struct undo_range_shift * range_shift;
    struct undo_cols_format * cols_format;
    struct undo_rows_format * rows_format;
    struct undo * p_sig;
    int * row_hidded;
    int * row_showed;
    int * col_hidded;
    int * col_showed;
    int * row_frozed;
    int * row_unfrozed;
    int * col_frozed;
    int * col_unfrozed;
    /* keep modflg, maxrow and maxcol status before and after changes */
    int modflg_bef;
    int modflg_aft;
    int maxrow_bef;
    int maxrow_aft;
    int maxcol_bef;
    int maxcol_aft;
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

//These two structures are for undo / redo changes in row format
struct undo_row_info {
    char type;       // a row can be 'R' (removed) or 'A' (added) because of change
    int row;
    unsigned char format;      // 1 to n
};

struct undo_rows_format {
    size_t length;   // keep the number of elements (rows)
    struct undo_row_info * rows;
};

void create_undo_action();
void end_undo_action();
void copy_to_undostruct (struct sheet * sh, int ri, int ci, int rf, int cf, char type, short handle_deps, struct ent_ptr ** destination);
void save_undo_range_shift(int delta_rows, int delta_cols, int tlrow, int tlcol, int brrow, int brcol);
void undo_hide_show(int row, int col, char type, int arg);
void undo_freeze_unfreeze(int row, int col, char type, int arg);

void add_undo_col_format(int col, int type, int fwidth, int precision, int realfmt);
void add_undo_row_format(int row, int type, unsigned char format);

void add_to_undolist(struct undo u);
void do_undo();
void do_redo();

void clear_undo_list ();
void clear_from_current_pos();
int len_undo_list();
void free_undo_node(struct undo * ul);
void dismiss_undo_item(struct undo * ul);
void copy_cell_to_undostruct(struct ent_ptr * e_ptr, struct sheet * sh_ori, struct ent * ori, char type);
void save_yl_pointer_after_calloc(struct ent_ptr * e);
int ent_ptr_exists_on_list(struct ent_ptr * list, struct ent_ptr * ep);
