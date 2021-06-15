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
#include "macros.h"
#include "color.h"
#include "cmds/cmds.h"
#include "conf.h"
#include "yank.h"
#include "graph.h"
#include "xmalloc.h" // for scxfree

#ifdef UNDO
#include "undo.h"
#endif

extern struct ent * forw_row(struct sheet * sh, int arg);
extern struct ent * back_row(struct sheet * sh, int arg);
extern struct ent * forw_col(struct sheet * sh, int arg);
extern struct ent * back_col(struct sheet * sh, int arg);
extern struct session * session;

int yank_arg;                 // number of rows and columns yanked. Used for commands like `4yr`
char type_of_yank;            // yank type. c=col, r=row, a=range, e=cell, '\0'=no yanking
static struct ent_ptr * yanklist;
struct ent_ptr * yanklist_tail;   // so we can always add ents at the end of the list easily
unsigned int yanked_cells = 0;// keeping this helps performance


/**
 * \brief init_yanklist()
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
 * \return struct ent *
 */
struct ent_ptr * get_yanklist() {
    return yanklist;
}


/**
 * \brief Remove elements from the yanklist and free its memory
 * \return none
 */
void free_yanklist () {
    if (yanklist == NULL) return;

    // free each ent content
    struct ent_ptr * r = yanklist;
    struct ent_ptr * e;
    while (r != NULL) {
        e = r->next;
        clearent(r->vp);
        r = e;
    }
    free(yanklist->vp); // free ents
    yanklist->vp = NULL;

    // free yanklist ent_ptr
    free(yanklist);
    yanked_cells = 0;
    yanklist = NULL;
    yanklist_tail = NULL;
    return;
}

/**
 * \brief Add an already alloc'ed 'ent_ptr' element to the yanklist
 * yanklist_tail is pointer to the last ent in the list
 * \param[in] item 'ent' element to add to the yanklist
 * \return none
 */
void add_ent_to_yanklist(struct ent_ptr * item) {
    yanked_cells++;

   // If yanklist is empty, insert at the beginning
    if (yanklist == NULL) {
        yanklist = item;
        yanklist_tail = item;
        return;
    }

    // If yanklist is NOT empty, insert it at the end
    yanklist_tail->next = item;
    yanklist_tail = item;
    return;
}


/**
 * \brief Yank a range of ents of a given range of a sheet
 * \param[in] struct sheet * sh
 * \param[in] tlrow
 * \param[in] tlcol
 * \param[in] brrow
 * \param[in] brcol
 * \param[in] type yank type. c=col, r=row, a=range, e=cell. '\0'=no yanking,
 * 's' sort. Used for pasting.
 * \param[in] arg number of rows or columns yanked. Used in commands like
 * '4yr'. Used for pasting.
 * \return none
 */
void yank_area(struct sheet * sh, int tlrow, int tlcol, int brrow, int brcol, char type, int arg) {
    int r,c;
    type_of_yank = type;
    yank_arg = arg;
    free_yanklist();

    struct ent * e_ori;
    // ask for memory to keep struct ent_ptr * and struct ent * for the whole range
    struct ent_ptr * y_cells = (struct ent_ptr *) calloc((brrow-tlrow+1)*(brcol-tlcol+1), sizeof(struct ent_ptr));
    struct ent * y_cells_vp = (struct ent *) calloc((brrow-tlrow+1)*(brcol-tlcol+1), sizeof(struct ent));

    for (r = tlrow; r <= brrow; r++)
        for (c = tlcol; c <= brcol; c++) {
            e_ori = *ATBL(sh, sh->tbl, r, c);
            if (e_ori == NULL) continue;

            // initialize the 'ent'
            y_cells->vp = y_cells_vp++;
            cleanent(y_cells->vp);

            // Copy 'e_ori' contents to 'y_cells' ent
            y_cells->sheet = sh;
            copyent(y_cells->vp, sh, e_ori, 0, 0, 0, 0, 0, 0, 0);

            // Important: each 'ent' element keeps the corresponding row and col
            (y_cells)->vp->row = e_ori->row;
            (y_cells)->vp->col = e_ori->col;

            add_ent_to_yanklist(y_cells++);
        }
    // this takes care of a potential memory leak if no ent was added to yanklist
    // for instance when deleting empty row
    if (! yanked_cells) {
        free(y_cells);
        free(y_cells_vp);
    }
    return;
}


/**
 * \brief paste_yanked_ents()
 *
 * \details This function is used for pasting ents that were yanked
 * with yr, yc, dr, or dc. It is also used for sorting.
 * If above == 1, paste is done above current row or the
 * right of the current column. Ents that were yanked using yy or yanked
 * ents of a range, are always pasted over currow and curcol positions
 * of the given sheet.
 * diffr: difference between current rows and the yanked 'ent'
 * diffc: difference between current columns and the yanked 'ent'
 * When sorting, rwo and col values can vary from yank to paste
 * time, so diffr should be zero.
 * When implementing column sorting, diffc should be zero as well!
 * type indicates if pasting format only, valuue only for the
 * whole content.
 * yank type: c=col, r=row, a=range, e=cell, '\0'=no yanking.
 *
 * \param[in] struct sheet * sh
 * \param[in] above
 * \param[in] type_paste
 *
 * \return -1 if locked cells are found
 * \return 0 otherwise
 */
int paste_yanked_ents(struct sheet * sh, int above, int type_paste) {
    struct roman * roman = session->cur_doc;
    if (yanklist == NULL) return 0;

    struct ent_ptr * yl = yanklist;
    struct ent_ptr * yll = yl;
    int diffr = 0, diffc = 0 , ignorelock = 0;

    extern struct ent_ptr * deps;
    //FIXME:
    if (yl->sheet == NULL) yl->sheet = roman->cur_sh;

#ifdef UNDO
    create_undo_action();
#endif

    if (type_of_yank == YANK_SORT) {                   // paste a range that was yanked in the sort function
        diffr = 0;
        diffc = sh->curcol - yl->vp->col;
        ignorelock = 1;

    } else if (type_of_yank == YANK_RANGE || type_of_yank == YANK_CELL) { // paste cell or range
        diffr = sh->currow - yl->vp->row;
        diffc = sh->curcol - yl->vp->col;

    } else if (type_of_yank == YANK_ROW && sh == yl->sheet) {             // paste row
        int c = yank_arg;
#ifdef UNDO
        copy_to_undostruct(sh, sh->currow + ! above, 0, sh->currow + ! above - 1 + yank_arg, sh->maxcol, UNDO_DEL, IGNORE_DEPS, NULL);
#endif
        while (c--) above ? insert_row(sh, 0) : insert_row(sh, 1);
        if (! above) sh->currow = forw_row(sh, 1)->row;                   // paste below
        diffr = sh->currow - yl->vp->row;
        diffc = yl->vp->col;
        fix_marks(sh, yank_arg, 0, sh->currow, sh->maxrow, 0, sh->maxcol);
#ifdef UNDO
        save_undo_range_shift(yank_arg, 0, sh->currow, 0, sh->currow - 1 + yank_arg, sh->maxcol);
#endif

    } else if (type_of_yank == YANK_COL) {             // paste col
        int c = yank_arg;
#ifdef UNDO
        copy_to_undostruct(sh, 0, sh->curcol + above, sh->maxrow, sh->curcol + above - 1 + yank_arg, UNDO_DEL, IGNORE_DEPS, NULL);
#endif
        while (c--) above ? insert_col(sh, 1) : insert_col(sh, 0);        // insert cols to the right if above or to the left
        diffr = yl->vp->row;
        diffc = sh->curcol - yl->vp->col;
        fix_marks(sh, 0, yank_arg, 0, sh->maxrow, sh->curcol, sh->maxcol);
#ifdef UNDO
        save_undo_range_shift(0, yank_arg, 0, sh->curcol, sh->maxrow, sh->curcol - 1 + yank_arg);
#endif
    }

    // paste cell or range
    if (type_of_yank == YANK_RANGE || type_of_yank == YANK_CELL) {
        // first check if there are any locked cells over destination
        // if so, just return
        while (yll != NULL) {
            int r = yll->vp->row + diffr;
            int c = yll->vp->col + diffc;
            checkbounds(sh, &r, &c);
            if (any_locked_cells(sh, yll->vp->row + diffr, yll->vp->col + diffc, yll->vp->row + diffr, yll->vp->col + diffc)) {
#ifdef UNDO
                dismiss_undo_item(NULL);
#endif
                return -1;
            }
            yll = yll->next;
        }
    }
    // its really needed to save deps of yanked ents?
    ents_that_depends_on_list(yl, diffr, diffc);

#ifdef UNDO
    // ask for memory to save the entire yanklist (and its dependencies) in the undo struct
    // note that the yanked ents could share dependencies.
    struct ent_ptr * y_cells = (struct ent_ptr *) calloc(2*(yanked_cells + (deps != NULL ? deps->vf : 0)), sizeof(struct ent_ptr));
    save_yl_pointer_after_calloc(y_cells);

    // save in undo the dependent ents before the paste
    copy_to_undostruct(sh, 0, 0, -1, -1, UNDO_DEL, HANDLE_DEPS, &y_cells);
#endif

    // paste each ent in yank list
    while (yl != NULL) {
        //FIXME:
        if (yl->sheet == NULL) yl->sheet = roman->cur_sh;

#ifdef UNDO
        copy_cell_to_undostruct(y_cells++, sh, lookat(sh, yl->vp->row + diffr, yl->vp->col + diffc), UNDO_DEL);
#endif

        // here we delete current content of "destino" ent.
        if (type_paste == YANK_RANGE || type_paste == YANK_SORT)
            erase_area(sh, yl->vp->row + diffr, yl->vp->col + diffc, yl->vp->row + diffr, yl->vp->col + diffc, ignorelock, 0);

        struct ent * destino = lookat(sh, yl->vp->row + diffr, yl->vp->col + diffc);

        if (type_paste == YANK_RANGE || type_paste == YANK_SORT) {
            (void) copyent(destino, sh, yl->vp, 0, 0, 0, 0, 0, 0, 0);
        } else if (type_paste == YANK_FORMAT) {
            (void) copyent(destino, sh, yl->vp, 0, 0, 0, 0, 0, 0, 'f');
        } else if (type_paste == YANK_VALUE) {
            (void) copyent(destino, sh, yl->vp, 0, 0, 0, 0, 0, 0, 'v');
            if (yl->sheet == sh && yl->vp->row == destino->row && yl->vp->col == destino->col) {
                efree(destino->expr);
                destino->expr = NULL;
            }
        } else if (type_paste == YANK_REF) {
            (void) copyent(destino, sh, yl->vp, diffr, diffc, 0, 0, sh->maxrows, sh->maxcols, 'c');
        }
        destino->row += diffr;
        destino->col += diffc;

        /******************** this might be put outside the loop  */
        // if so, use EvalRange
        // sync and then eval.
        //sync_refs(sh);

        if (destino->expr) {
            syncref(sh, destino->expr);
            if (get_conf_int("autocalc")) EvalJustOneVertex(sh, destino, 1);
            //EvalRange(destino->row, destino->col, destino->row, destino->col);
        }

        int i;
        for (i = 0; deps != NULL && i < deps->vf; i++) {
            syncref(sh, deps[i].vp->expr);
            if (get_conf_int("autocalc") && deps[i].vp->expr) EvalJustOneVertex(sh, deps[i].vp, 0);
        }
        /*******************/
#ifdef UNDO
        copy_cell_to_undostruct(y_cells++, sh, lookat(sh, yl->vp->row + diffr, yl->vp->col + diffc), UNDO_ADD);
#endif
        yl = yl->next;
    }

#ifdef UNDO
    // save in undo the dependent ents after the paste
    copy_to_undostruct(sh, 0, 0, -1, -1, UNDO_ADD, HANDLE_DEPS, &y_cells);
#endif

    //rebuild_graph();
    //if (get_conf_int("autocalc")) EvalAll();
    roman->modflg++;

#ifdef UNDO
    end_undo_action();
    if (deps != NULL) free(deps);
    deps = NULL;
#endif
    return 0;
}
