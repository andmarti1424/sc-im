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
 * \file yank.c
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 2017-07-18
 * \brief TODO Write a tbrief file description.
 *
 * \details Yanklist doesn't keep references to 'ent' elements, it
 * creates new nodes.Tthe yanklist is constantly cleaned out. Example: When
 * removing 'ent' elements with `dc`, the original ones are removed
 * and new elements are created in the yanklist. Important: each 'ent'
 * element should keep a row and col.
 */

#include "sc.h"
#include "stdlib.h"
#include "marks.h"
#include "cmds.h"
#include "conf.h"
#include "yank.h"
#include "dep_graph.h"
#include "xmalloc.h" // for scxfree

#ifdef UNDO
#include "undo.h"
#endif

extern struct ent * forw_row(int arg);
extern struct ent * back_row(int arg);
extern struct ent * forw_col(int arg);
extern struct ent * back_col(int arg);

int yank_arg;                 // number of rows and columns yanked. Used for commands like `4yr`
char type_of_yank;            // yank type. c=col, r=row, a=range, e=cell, '\0'=no yanking
static struct ent * yanklist;
struct ent * yanklist_tail;   // so we can always add ents at the end of the list easily
unsigned int yanked_cells = 0;// keeping this helps performance

/**
 * \brief TODO Document init_yanklist()
 *
 * \return none
 */

void init_yanklist() {
    yanked_cells = 0;
    type_of_yank = YANK_NULL;
    yanklist = NULL;
    yanklist_tail = NULL;
}

/**
 * \brief Return the yanklist
 *
 * \return yanklist
 */

struct ent * get_yanklist() {
    return yanklist;
}

/**
 * \brief Remove yank 'ent' elements and free corresponding memory
 *
 * \return none
 */

void free_yanklist () {
    if (yanklist == NULL) return;
    int c;

    // free each ent internally
    struct ent * r = yanklist;
    struct ent * e;
    while (r != NULL) {
        e = r->next;

        if (r->format) scxfree(r->format);
        if (r->label) scxfree(r->label);
        if (r->expr) efree(r->expr);
        if (r->ucolor) free(r->ucolor);

        //free(r);
        r = e;
    }

    for (c = 0; c < COLFORMATS; c++) {
        if (colformat[c] != NULL)
            scxfree(colformat[c]);
        colformat[c] = NULL;
    }

    // free yanklist
    free(yanklist);
    yanked_cells = 0;
    yanklist = NULL;
    yanklist_tail = NULL;
    return;
}

/**
 * \brief Add an already alloc'ed 'ent' element to the yanklist
 *
 * yanklist_tail is pointer to the last ent in the list
 * \param[in] item 'ent' element to add to the yanklist
 * \return none
 */

void add_ent_to_yanklist(struct ent * item) {
    yanked_cells++;

   // If yanklist is empty, insert at the beginning
    if (yanklist == NULL) {
        yanklist = item;
        yanklist_tail = item;
        return;
    }

    // If yanklist is NOT empty, insert at the end
    // insert at the end
    yanklist_tail->next = item;
    yanklist_tail = item;
    return;
}

/**
 * \brief Yank a range of ents
 *
 * \param[in] tlrow
 * \param[in] tlcol
 * \param[in] brrow
 * \param[in] brcol
 * \param[in] type yank type. c=col, r=row, a=range, e=cell. '\o'=no yanking,
 * 's' sort. Used for pasting.
 * \param[in] arg number of rows or columns yanked. Used in commands like
 * '4yr'. Used for pasting.
 *
 * \return none
 */

void yank_area(int tlrow, int tlcol, int brrow, int brcol, char type, int arg) {
    int r,c;
    type_of_yank = type;
    yank_arg = arg;
    free_yanklist();

    struct ent * e_ori;
    // ask for memory to keep struct ent * for the whole range
    struct ent * y_cells = (struct ent *) calloc((brrow-tlrow+1)*(brcol-tlcol+1), sizeof(struct ent));

    for (r = tlrow; r <= brrow; r++)
        for (c = tlcol; c <= brcol; c++) {
            e_ori = *ATBL(tbl, r, c);
            if (e_ori == NULL) continue;

            // initialize the 'ent'
            cleanent(y_cells);

            // Copy 'e_ori' contents to 'y_cells' ent
            (void) copyent(y_cells, e_ori, 0, 0, 0, 0, 0, 0, 0);

            // Important: each 'ent' element keeps the corresponding row and col
            (y_cells)->row = e_ori->row;
            (y_cells)->col = e_ori->col;

            add_ent_to_yanklist(y_cells++);
        }
    // this takes care of a potential memory leak if no ent was added to yanklist
    // for instance when deleting empty row
    if (! yanked_cells) free(y_cells);
    return;
}

/**
 * \brief Paste yanked ents
 *
 * \details This function is used for pasting ents that were yanked
 * with tr, yc, dr, or dc. It is also used for sorting.
 * \details If above == 1, paste is done above current row or the
 * right of the current column. Enst that were yanked using yy or yanked
 * ents of a range, always pasted in currow and curcol positions.
 * \details diffr: difference between current rows and the yanked 'ent'
 * \details diffc: difference between current columns and the yanked 'ent'
 * \details When sorting, rwo and col values can vary from yank to paste
 * time, so diffr should be zero.
 * \details When implementing column sorting, diffc should be zero as well!
 * \details type indicates if pasting format only, valuue only for the
 * whole content.
 * \details yank type: c=col, r=row, a=range, e=cell, '\0'=no yanking.
 *
 * \param[in] above
 * \param[in] type_paste
 *
 * \return -1 if locked cells are found
 * \return 0 otherwise
 */

int paste_yanked_ents(int above, int type_paste) {
    if (yanklist == NULL) return 0;

    struct ent * yl = yanklist;
    struct ent * yll = yl;
    int diffr = 0, diffc = 0 , ignorelock = 0;

    extern struct ent_ptr * deps;
#ifdef UNDO
    create_undo_action();
#endif

    if (type_of_yank == YANK_SORT) {                              // paste a range that was yanked in the sort function
        diffr = 0;
        diffc = curcol - yl->col;
        ignorelock = 1;

    } else if (type_of_yank == YANK_RANGE || type_of_yank == YANK_CELL) { // paste cell or range
        diffr = currow - yl->row;
        diffc = curcol - yl->col;

    } else if (type_of_yank == YANK_ROW) {                        // paste row
        int c = yank_arg;
#ifdef UNDO
        copy_to_undostruct(currow + ! above, 0, currow + ! above - 1 + yank_arg, maxcol, UNDO_DEL, IGNORE_DEPS, NULL);
#endif
        while (c--) above ? insert_row(0) : insert_row(1);
        if (! above) currow = forw_row(1)->row;                   // paste below
        diffr = currow - yl->row;
        diffc = yl->col;
        fix_marks(yank_arg, 0, currow, maxrow, 0, maxcol);
#ifdef UNDO
        save_undo_range_shift(yank_arg, 0, currow, 0, currow - 1 + yank_arg, maxcol);
#endif

    } else if (type_of_yank == YANK_COL) {                        // paste col
        int c = yank_arg;
#ifdef UNDO
        copy_to_undostruct(0, curcol + above, maxrow, curcol + above - 1 + yank_arg, UNDO_DEL, IGNORE_DEPS, NULL);
#endif
        while (c--) above ? insert_col(1) : insert_col(0);        // insert cols to the right if above or to the left
        diffr = yl->row;
        diffc = curcol - yl->col;
        fix_marks(0, yank_arg, 0, maxrow, curcol, maxcol);
#ifdef UNDO
        save_undo_range_shift(0, yank_arg, 0, curcol, maxrow, curcol - 1 + yank_arg);
#endif
    }

    // first check if there are any locked cells over destination
    // if so, just return
    if (type_of_yank == YANK_RANGE || type_of_yank == YANK_CELL) {
        while (yll != NULL) {
            int r = yll->row + diffr;
            int c = yll->col + diffc;
            checkbounds(&r, &c);
            if (any_locked_cells(yll->row + diffr, yll->col + diffc, yll->row + diffr, yll->col + diffc)) {
#ifdef UNDO
                dismiss_undo_item(NULL);
#endif
                return -1;
            }
            yll = yll->next;
        }
    }

    ents_that_depends_on_list(yl, diffr, diffc);

#ifdef UNDO
    // ask for memory to save the entire yanklist (and its dependencies) in the undo struct
    struct ent * y_cells = (struct ent *) calloc(2*(yanked_cells + (deps != NULL ? deps->vf : 0)), sizeof(struct ent));
    save_pointer_after_calloc(y_cells);
#endif

    while (yl != NULL) {

#ifdef UNDO
        copy_cell_to_undostruct(y_cells++, lookat(yl->row + diffr, yl->col + diffc), UNDO_DEL);

        // Here pass struct ent ** to copy_to_undostruct
        copy_to_undostruct(0, 0, -1, -1, UNDO_DEL, HANDLE_DEPS, &y_cells);
#endif

        // here we delete current content of "destino" ent.
        if (type_paste == YANK_RANGE || type_paste == YANK_SORT)
            erase_area(yl->row + diffr, yl->col + diffc, yl->row + diffr, yl->col + diffc, ignorelock, 0);

        struct ent * destino = lookat(yl->row + diffr, yl->col + diffc);

        if (type_paste == YANK_RANGE || type_paste == YANK_SORT) {
            (void) copyent(destino, yl, 0, 0, 0, 0, 0, 0, 0);
        } else if (type_paste == YANK_FORMAT) {
            (void) copyent(destino, yl, 0, 0, 0, 0, 0, 0, 'f');
        } else if (type_paste == YANK_VALUE) {
            (void) copyent(destino, yl, 0, 0, 0, 0, 0, 0, 'v');
        } else if (type_paste == YANK_REF) {
            (void) copyent(destino, yl, diffr, diffc, 0, 0, maxrows, maxcols, 'c');
        }
        destino->row += diffr;
        destino->col += diffc;

        /******************** this might be put outside the loop  */
        // if so, use EvalRange
        // sync and then eval.
        // sync_refs();

        if (destino->expr) {
            syncref(destino->expr);
            if (get_conf_int("autocalc")) EvalJustOneVertex(destino, 1);
            //EvalRange(destino->row, destino->col, destino->row, destino->col);
        }

        int i;
        for (i = 0; deps != NULL && i < deps->vf; i++) {
            syncref(deps[i].vp->expr);
            if (get_conf_int("autocalc")) EvalJustOneVertex(deps[i].vp, 0);
        }
        /*******************/

#ifdef UNDO
        copy_cell_to_undostruct(y_cells++, lookat(yl->row + diffr, yl->col + diffc), UNDO_ADD);
        // store dependencies after the change as well
        copy_to_undostruct(0, 0, -1, -1, UNDO_ADD, HANDLE_DEPS, &y_cells);
#endif
        yl = yl->next;
    }
    //rebuild_graph();
    //if (get_conf_int("autocalc")) EvalAll();

#ifdef UNDO
    end_undo_action();
    if (deps != NULL) free(deps);
    deps = NULL;
#endif
    return 0;
}
