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
 * \file undo.c
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 06/06/2021
 * \brief This file contains the main functions to support the undo/redo feature.
 */

/*
 * UNDO and REDO feature works with an 'undo' struct list.
 * Which contains:
 *       p_ant: pointer to 'undo' struct. If NULL, this node is the first change
 *              for the session.
 *
 *       struct ent_ptr * added: 'ent' elements added by the change
 *
 *       struct ent_ptr * removed: 'ent' elements removed by the change
 *
 *       struct allocation_list * allocations: since we alloc over added and removed
 *              list in batches. we need to keep the first position in memory of each calloc.
 *
 *       struct undo_range_shift * range_shift: range shifted by change
 *
 *       row_hidded: integers list (int *) hidden rows on screen
 *
 *       row_showed: integers list (int *) visible rows on screen
 *
 *       row_frozed: integers list (int *) frozen rows on screen
 *
 *       row_unfrozed: integers list (int *) unfrozen rows on screen
 *
 *       col_hidded: integers list (int *) hidden columns on screen
 *
 *       col_showed: integers list (int *) visible columns on screen
 *              NOTE: the first position of the lists contains (number of elements - 1) in the list
 *
 *       col_frozed: integers list (int *) frozen cols on screen
 *
 *       col_unfrozed: integers list (int *) unfrozen cols on screen
 *
 *       struct undo_cols_format * cols_format: list of 'undo_col_info' elements used for
 *              undoing / redoing changes in columns format (fwidth, precision y realfmt)
 *
 *       struct undo_rows_format * rows_format: list of 'undo_row_info' elements used for
 *              undoing / redoing changes in rows format (height)
 *
 *       p_sig: pointer to 'undo' struct, If NULL, this node is the last change in
 *              the session.
 *
 *       modflg_bef: document modflg before the change
 *       maxrow_bef: sheet maxrow before the change
 *       maxcol_bef: sheet maxcol before the change
 *       modflg_aft: document modflg after the change
 *       maxrow_aft: sheet maxrow after the change
 *       maxcol_aft: sheet maxcol after the change
 *
 * Follows one level UNDO/REDO scheme. A change (C1) is made, then an UNDO operation, and
 * another change (C2). From there later changes are removed.
 * Scheme:
 *
 * + C1 -> + -> UNDO -
 * ^                 \
 * |_                |
 *   \---------------/
 * |
 * |
 * |
 *  \-> C2 --> + ...
 *
 * undo_shift_range struct contains:
 *     int delta_rows: delta rows for the range shift
 *     int delta_cols: delta columns for the range shift
 *     int tlrow:      Upper left row defining the range of the shift
 *                     (As if a cell range shift is made)
 *     int tlcol:      Upper left column defining the range of the shift
 *                     (As if a cell range shift is made)
 *     int brrow:      Lower right row defining the range of the shift
 *                     (As if a cell range shift is made)
 *     int brcol:      Lower right column defining the range of the shift
 *                     (As if a cell range shift is made)
 *
 * Implemented actions for UNDO/REDO:
 * 1. Remove content from cell or range
 * 2. Input content into a cell
 * 3. Edit a cell
 * 4. Change alginment of range or cell
 * 5. Paste range or cell
 * 6. Shift range or cell with sh, sj, sk, sl
 * 7. Insert row or column
 * 8. Delete row or column
 * 9. Paste row or column
 * 10. Hide/show rows and columns
 * 11. Sort of a range
 * 12. Change in the format of a range or cell
 * 13. '-' and '+' commands in normal mode
 * 14. Lock and unlock of cells
 * 15. datefmt command
 * 16. the cellcolor command
 * 17. Change in format of a column as a result of the 'f' command
 * 18. Change in format of a column as a result of auto_jus
 * 19. Change format of columns as a result of ic dc
 * 20. fill command
 * 21. unformat
 * 22. change in the format of rows
 * 23. undo of freeze / unfreeze commands
 *
 */

#ifdef UNDO

#include <stdlib.h>
#include "sc.h"
#include "undo.h"
#include "macros.h"
#include "color.h"
#include "curses.h"
#include "conf.h"
#include "cmds/cmds.h"
#include "marks.h"
#include "actions/shift.h"
#include "graph.h"

extern struct session * session;

// undolist
static struct undo * undo_list = NULL;

// current position in the list
static int undo_list_pos = 0;

// Number of elements in the list
static int undo_list_len = 0;

// Temporal variable
static struct undo undo_item;


/**
 * \brief Init 'undo_item'
 * \return none
 */
void create_undo_action() {
    undo_item.added       = NULL;
    undo_item.removed     = NULL;
    undo_item.allocations = NULL;
    undo_item.p_ant       = NULL;
    undo_item.p_sig       = NULL;
    undo_item.range_shift = NULL;
    undo_item.cols_format = NULL;
    undo_item.rows_format = NULL;
    undo_item.sheet = session->cur_doc->cur_sh; // we should keep the current sheet reference

    undo_item.row_hidded  = NULL;
    undo_item.row_showed  = NULL;
    undo_item.row_frozed  = NULL;
    undo_item.row_unfrozed  = NULL;
    undo_item.col_hidded  = NULL;
    undo_item.col_showed  = NULL;
    undo_item.col_frozed  = NULL;
    undo_item.col_unfrozed  = NULL;
    undo_item.modflg_bef = session->cur_doc->modflg;
    undo_item.maxrow_bef = undo_item.sheet->maxrow;
    undo_item.maxcol_bef = undo_item.sheet->maxcol;
    return;
}


/**
 * @brief end_undo_action()
 *
 * \details Save undo_item copy with 'ent' elements modified, and the
 * undo range shift struct into the undolist.
 *
 * \return none
 */
void end_undo_action() {
    struct roman * roman = session->cur_doc;
    undo_item.modflg_aft = roman->modflg;
    undo_item.maxrow_aft = undo_item.sheet->maxrow;
    undo_item.maxcol_aft = undo_item.sheet->maxcol;
    add_to_undolist(undo_item);

    // just check if we need to dismiss this undo_item!
    if ((undo_item.added      == NULL && undo_item.allocations == NULL &&
        undo_item.removed     == NULL && undo_item.range_shift == NULL &&
        undo_item.row_hidded  == NULL && undo_item.row_showed  == NULL &&
        undo_item.row_frozed  == NULL && undo_item.col_frozed  == NULL &&
        undo_item.row_unfrozed  == NULL && undo_item.col_unfrozed  == NULL &&
        undo_item.cols_format == NULL && undo_item.rows_format == NULL &&
        undo_item.col_hidded  == NULL && undo_item.col_showed  == NULL) || roman->loading) {
        if (undo_list->p_ant != NULL) undo_list = undo_list->p_ant;
        undo_list_pos--;
        clear_from_current_pos();
    }

    return;
}


/**
 * \brief add_to_undolist()
 *
 * \details Add an undo node to the undolist. Allocate memory for
 * undo struct. Fill variable with undo_item value and append it
 * to the list.
 *
 * \param[in] u
 *
 * \return none
 */
void add_to_undolist(struct undo u) {
    // If not at the end of the list, remove from the end
    if (undo_list != NULL && undo_list_pos != len_undo_list()) clear_from_current_pos();

    struct undo * ul = (struct undo *) malloc (sizeof(struct undo)) ;
    ul->p_sig = NULL;

    // Add 'ent' elements
    ul->added = u.added;
    ul->removed = u.removed;
    ul->sheet = u.sheet;
    ul->allocations = u.allocations;
    ul->range_shift = u.range_shift;
    ul->cols_format = u.cols_format;
    ul->rows_format = u.rows_format;
    ul->row_hidded = u.row_hidded;
    ul->col_hidded = u.col_hidded;
    ul->row_showed = u.row_showed;
    ul->col_showed = u.col_showed;
    ul->row_frozed = u.row_frozed;
    ul->col_frozed = u.col_frozed;
    ul->row_unfrozed = u.row_unfrozed;
    ul->col_unfrozed = u.col_unfrozed;

    if (undo_list == NULL) {
        ul->p_ant = NULL;
        undo_list = ul;
    } else {
        ul->p_ant = undo_list;

        // go to end of list
        // TODO we can improve this by keeping always the last pointer at hand
        while (undo_list->p_sig != NULL) undo_list = undo_list->p_sig;

        undo_list->p_sig = ul;
        undo_list = undo_list->p_sig;
    }
    undo_list_pos++;
    undo_list_len++;
    return;
}


/**
 * \brief Dismiss current undo_item
 *
 * \details This function frees memory of a struct undo item.
 * It is internally used by free_undo_node(). But as well, this function
 * shall be called instead of end_undo_action in case we want to cancel
 * a previous create_undo_action.
 * If called for this purpose, argument shall be NULL.
 *
 * \param[in] ul
 *
 * \return none
 */
void dismiss_undo_item(struct undo * ul) {

    if (ul == NULL) ul = &undo_item;

    // first free inside each added and removed ents
    // (their labels, expressions, etc.
    struct ent_ptr * en;
    struct ent_ptr * de;
    en = ul->added;     // free added
    while (en != NULL) {
        de = en->next;
        clearent(en->vp);
        free(en->vp);
        en->vp = NULL;
        en = de;
    }

    en = ul->removed;   // free removed
    while (en != NULL) {
        de = en->next;
        clearent(en->vp);
        free(en->vp);
        en->vp = NULL;
        en = de;
    }

    // now free added and removed lists
    // in the way they were alloc'ed (as batches)
    int i, size = ul->allocations != NULL ? ul->allocations->size : 0;
    struct ent_ptr ** alls = ul->allocations != NULL ? ul->allocations->items : NULL;
    for (i = 0; i < size; i++) {
        free(*alls); // each ent_ptr
        *alls = NULL;
        alls++;
    }
    if (ul->allocations != NULL) {
        if (ul->allocations->items != NULL) {
            free(ul->allocations->items);
            ul->allocations->items = NULL;
        }
        free(ul->allocations);
        ul->allocations = NULL;
    }

    if (ul->range_shift != NULL) free(ul->range_shift);    // Free undo_range_shift memory
    if (ul->cols_format != NULL) {                         // Free cols_format memory
        free(ul->cols_format->cols);
        free(ul->cols_format);
    }
    if (ul->rows_format != NULL) {                         // Free rows_format memory
        free(ul->rows_format->rows);
        free(ul->rows_format);
    }
    if (ul->row_hidded  != NULL) free(ul->row_hidded);     // Free hidden row memory
    if (ul->col_hidded  != NULL) free(ul->col_hidded);     // Free hidden col memory
    if (ul->row_showed  != NULL) free(ul->row_showed);     // Free showed row memory
    if (ul->row_frozed  != NULL) free(ul->row_frozed);     // Free frozed row memory
    if (ul->col_showed  != NULL) free(ul->col_showed);     // Free showed col memory
    if (ul->col_frozed  != NULL) free(ul->col_frozed);     // Free frozed col memory
    if (ul->row_unfrozed  != NULL) free(ul->row_unfrozed); // Free unfrozed row memory
    if (ul->col_unfrozed  != NULL) free(ul->col_unfrozed); // Free unfrozed col memory

    return;
}


/**
 * \brief Cascade free UNDO node memory
 *
 * \param[in] ul
 *
 * \return none
 */
void free_undo_node(struct undo * ul) {
    struct undo * e;

    // Remove from current position
    while (ul != NULL) {
        dismiss_undo_item(ul);

        e = ul->p_sig;
        free(ul);
        undo_list_len--;
        ul = e;
    }
    return;
}


/**
 * \brief Remove nodes below the current position from the undolist
 * \return none
 */
void clear_from_current_pos() {
    if (undo_list == NULL) return;

    if (undo_list->p_ant == NULL) {
        free_undo_node(undo_list);
        undo_list = NULL;
    } else {
        struct undo * ul = undo_list->p_sig; // Previous
        free_undo_node(ul);
        undo_list->p_sig = NULL;
    }

    return;
}


/**
 * \brief Remove undolist content
 * \return none
 */
void clear_undo_list() {
    if (undo_list == NULL) return;

    // Go to the beginning of the list
    while (undo_list->p_ant != NULL ) {
        undo_list = undo_list->p_ant;
    }

    struct undo * ul = undo_list;

    free_undo_node(ul);

    undo_list = NULL;
    undo_list_pos = 0;
    return;
}


/**
 * \brief Return the length of the undo list
 * \return length of undolist
 */
int len_undo_list() {
    return undo_list_len;
}

/*
 * \brief ent_ptr_exists_on_list
 * \details check if an ent exists on ent_ptr list
 * (used for added / removed lists of undo struct)
 * return 0 if not exists or 1 if exists.
 */
int ent_ptr_exists_on_list(struct ent_ptr * list, struct ent_ptr * ep) {
    int repeated = 0;
    if (ep == NULL) return repeated;
    while (list != NULL && list->vp != NULL) {
        if (list->sheet == ep->sheet && list->vp->row == ep->vp->row && list->vp->col == ep->vp->col) {
            repeated = 1;
            break;
        }
        list = list->next;
    }
    return repeated;
}

/**
 * \brief copy_to_undostruct()
 *
 * \details Take a range of 'ent' elements of a sheet, and create ent copies to keep in undo structs lists
 * such as the 'added' or 'removed' lists.
 *
 * char type: indicates UNDO_ADD ('a') for added list. or UNDO_DEL ('d') for the 'removed' list.
 *
 * handle_deps: if set to HANDLE_DEPS it will store the dependencies of the specified range as well.
 * remember deps is a global variable.
 *
 * destination: struct ent_ptr ** to use in the copy. if none was given, just malloc one.
 * Note that if destination is not null, the struct ent * vp inside the struct ent_ptr is already malloc'ed
 * returns: none
 */
void copy_to_undostruct (struct sheet * sh, int ri, int ci, int rf, int cf, char type, short handle_deps, struct ent_ptr ** destination) {
    int i, c, r;
    struct ent * p;
    extern struct ent_ptr * deps;

    // ask for memory to keep struct ent * for the whole range
    // and only if no destination pointer was given
    struct ent_ptr * y_cells = destination == NULL ? NULL : *destination;
    if (y_cells == NULL && handle_deps == HANDLE_DEPS && deps != NULL)
        y_cells = (struct ent_ptr *) calloc((rf-ri+1)*(cf-ci+1) + deps->vf, sizeof(struct ent_ptr));
    else if (y_cells == NULL)
        y_cells = (struct ent_ptr *) calloc((rf-ri+1)*(cf-ci+1), sizeof(struct ent_ptr));

    // if no destination pointer was given
    // we save the pointer for future free
    if (destination == NULL) save_yl_pointer_after_calloc(y_cells);

    for (r = ri; r <= rf; r++)
        for (c = ci; c <= cf; c++) {
            p = *ATBL(sh, sh->tbl, r, c);
            if (p == NULL) continue;

            /* here check that ent to add is not already in the list
             * if so, avoid to add a duplicate ent
             */
            struct ent_ptr * lista = type == 'a' ? undo_item.added : undo_item.removed;
            struct ent_ptr e;
            e.sheet = sh;
            e.vp = p;
            if (ent_ptr_exists_on_list(lista, &e)) continue;

            // initialize the 'ent'
            //if (destination == NULL) y_cells->vp = malloc(sizeof(struct ent));
            y_cells->vp = malloc(sizeof(struct ent));
            cleanent(y_cells->vp);
            y_cells->sheet = sh;

            // Copy cell at 'r, c' contents to 'y_cells' ent
            y_cells->vp->expr = NULL;
            copyent(y_cells->vp, sh, lookat(sh, r, c), 0, 0, 0, 0, 0, 0, 'u');

            // Append 'ent' element at the beginning
            //TODO: add it ordered?
            if (type == UNDO_ADD) {
                y_cells->next = undo_item.added;
                undo_item.added = y_cells;
            } else {
                y_cells->next = undo_item.removed;
                undo_item.removed = y_cells;
            }

            // increase pointer!
            if (destination == NULL) y_cells++;
            else *destination = ++y_cells;
        }

    // do the same for dependencies
    if (handle_deps == HANDLE_DEPS)
        for (i = 0; deps != NULL && i < deps->vf; i++) {
            p = *ATBL(deps[i].sheet, deps[i].sheet->tbl, deps[i].vp->row, deps[i].vp->col);
            if (p == NULL) continue;

            // initialize the 'ent'
            //if (destination == NULL) y_cells->vp = malloc(sizeof(struct ent));
            y_cells->vp = malloc(sizeof(struct ent));
            cleanent(y_cells->vp);
            y_cells->sheet = deps[i].sheet;

            // Copy cell at deps[i].vp->row, deps[i].vp->col contents to 'y_cells' ent
            copyent(y_cells->vp, deps[i].sheet, lookat(deps[i].sheet, deps[i].vp->row, deps[i].vp->col), 0, 0, 0, 0, 0, 0, 'u');

            //sc_debug("copy_to_undostruct a undo %d %d", deps[i].vp->row, deps[i].vp->col);
            // Append 'ent' element at the beginning
            //TODO: add it ordered?
            if (type == UNDO_ADD) {
                y_cells->next = undo_item.added;
                undo_item.added = y_cells;
            } else {
                y_cells->next = undo_item.removed;
                undo_item.removed = y_cells;
            }
            if (destination == NULL) y_cells++;
            else *destination = ++y_cells;
        }
    return;
}



/**
 * \brief save_yl_pointer_after_calloc()
 *
 * \details This function keeps in a pointer array every pointer that was returned by calloc
 * so we can free them
 *
 * \param[in] struct ent e
 * \return void
 */

void save_yl_pointer_after_calloc(struct ent_ptr * e) {
    if (undo_item.allocations == NULL) {
        undo_item.allocations = malloc(sizeof(struct allocation_list));
        undo_item.allocations->size = 0;
        undo_item.allocations->items = NULL;
    }
    undo_item.allocations->items = (struct ent_ptr **) realloc(undo_item.allocations->items, sizeof(struct ent_ptr *) * (++(undo_item.allocations->size)));
    undo_item.allocations->items[undo_item.allocations->size-1] = e; // keep the pointer so later can be freed
}


/*
 * \brief copy_cell_to_undostruct()
 *
 * \details This function adds an struct ent * (new) to undo struct lists.
 * its contents are based on the struct ent * (ori).
 * could be added list or deleted list depending on the type.
 * \param[in] struct ent_ptr * e_ptr
 * \param[in] struct sheet * sh_ori
 * \param[in] struct ent * ori
 * \param[in] char type: indicates UNDO_ADD ('a') for added list. or UNDO_DEL ('d') for the 'removed' list.
 *
 * the struct ent_ptr * e_ptr should be already alloc'ed
 * \return void
 * NOTE: used on yank.c
 *
 */
void copy_cell_to_undostruct (struct ent_ptr * e_ptr, struct sheet * sh_ori, struct ent * ori, char type) {
    /* here check that ent to add is not already in the list
     * if so, avoid to add a duplicate ent
     */
    struct ent_ptr * lista = type == 'a' ? undo_item.added : undo_item.removed;
    struct ent_ptr e;
    e.sheet = sh_ori;
    e.vp = ori;
    if (ent_ptr_exists_on_list(lista, &e)) return;
    // in case of returning because ent exists on list, make sure struct ent_ptr gets freed later.

    // if reached here, malloc and add to undolist
    struct ent_ptr * new_ptr = e_ptr;
    struct ent * new = malloc(sizeof(struct ent));

    // initialize the 'ent'
    cleanent(new);

    // Copy 'ori' cell contents to 'new' ent
    copyent(new, sh_ori, ori, 0, 0, 0, 0, 0, 0, 'u');

    new_ptr->sheet = sh_ori;
    new_ptr->vp = new;

    // Append 'ent' element at the beginning
    //TODO: add it ordered?
    if (type == UNDO_ADD) {
        new_ptr->next = undo_item.added;
        undo_item.added = new_ptr;
    } else {
        new_ptr->next = undo_item.removed;
        undo_item.removed = new_ptr;
    }
    return;
}


/**
 * \brief add_undo_col_format()
 *
 * \param[in] col
 * \param[in] type
 * \param[in] fwidth
 * \param[in] precision
 * \param[in] realfmt
 *
 * \return none
 */
void add_undo_col_format(int col, int type, int fwidth, int precision, int realfmt) {
    if (undo_item.cols_format == NULL) {
        undo_item.cols_format = (struct undo_cols_format *) malloc( sizeof(struct undo_cols_format));
        undo_item.cols_format->length = 1;
        undo_item.cols_format->cols = (struct undo_col_info *) malloc( sizeof(struct undo_col_info));
    } else {
        undo_item.cols_format->length++;
        undo_item.cols_format->cols = (struct undo_col_info *) realloc(undo_item.cols_format->cols, undo_item.cols_format->length * sizeof(struct undo_col_info));
    }
    undo_item.cols_format->cols[ undo_item.cols_format->length - 1].type = type;
    undo_item.cols_format->cols[ undo_item.cols_format->length - 1].col = col;
    undo_item.cols_format->cols[ undo_item.cols_format->length - 1].fwidth = fwidth;
    undo_item.cols_format->cols[ undo_item.cols_format->length - 1].precision = precision;
    undo_item.cols_format->cols[ undo_item.cols_format->length - 1].realfmt = realfmt;
    return;
}


/**
 * \brief add_undo_row_format()
 *
 * \param[in] row
 * \param[in] type
 * \param[in] format
 *
 * \return none
 */
void add_undo_row_format(int row, int type, unsigned char format) {
    if (undo_item.rows_format == NULL) {
        undo_item.rows_format = (struct undo_rows_format *) malloc( sizeof(struct undo_rows_format));
        undo_item.rows_format->length = 1;
        undo_item.rows_format->rows = (struct undo_row_info *) malloc( sizeof(struct undo_row_info));
    } else {
        undo_item.rows_format->length++;
        undo_item.rows_format->rows = (struct undo_row_info *) realloc(undo_item.rows_format->rows, undo_item.rows_format->length * sizeof(struct undo_row_info));
    }
    undo_item.rows_format->rows[ undo_item.rows_format->length - 1].type = type;
    undo_item.rows_format->rows[ undo_item.rows_format->length - 1].row = row;
    undo_item.rows_format->rows[ undo_item.rows_format->length - 1].format = format;
    return;
}


/**
 * \brief save_undo_range_shift()
 *
 * \details Take a range, a rows and columns delta and save them into
 * they undo struct. Used to shift ranges when UNDO or REDO without
 * duplicating 'ent'elements.
 *
 * \param[in] delta_rows
 * \param[in] delta_cols
 * \param[in] tlrow
 * \param[in] tlcol
 * \param[in] brrow
 * \param[in] brcol
 *
 * \return none
 */
void save_undo_range_shift(int delta_rows, int delta_cols, int tlrow, int tlcol, int brrow, int brcol) {
    struct undo_range_shift * urs = (struct undo_range_shift *) malloc( (unsigned) sizeof(struct undo_range_shift ) );
    urs->delta_rows = delta_rows;
    urs->delta_cols = delta_cols;
    urs->tlrow = tlrow;
    urs->tlcol = tlcol;
    urs->brrow = brrow;
    urs->brcol = brcol;
    undo_item.range_shift = urs;
    return;
}

/*
 * \brief undo_hide_show()
 * This function is used for undoing and redoing
 * changes caused by commands that hide/show rows/columns of screen
 * such as Zr Zc Sc Sr commands.
 * it stores in four different lists (int * list) the row or columns numbers
 * that are showed or hidden because of a change.
 * As these lists are dynamically built, in the first position of every list,
 * we always store the number of elements that the list has.
 *
 * \details this function is used for undoint and redoing changes
 * caused by commands that hide/show rows/columns of screen such
 * as zr, zc, sc, and sr commands.
 *
 * it stores in four different lists (int * list) the row or column numbers
 * that are shown or hidden because of a change. as these lists are
 * dynamically built, in the first position of every list, we always store
 * the number of elements that the list has.
 *
 * \param[in] row
 * \param[in] col
 * \param[in] type
 * \param[in] arg
 *
 * \return none
 */
void undo_hide_show(int row, int col, char type, int arg) {
    int i;
    if (type == 'h') {
        if (row > -1) {        // hide row
            if (undo_item.row_hidded == NULL) {
                undo_item.row_hidded = (int *) malloc(sizeof(int) * (arg + 1));
                undo_item.row_hidded[0] = 0;
            } else
                undo_item.row_hidded = (int *) realloc(undo_item.row_hidded, sizeof(int) * (undo_item.row_hidded[0] + arg + 1));

            for (i=0; i < arg; i++)
                undo_item.row_hidded[undo_item.row_hidded[0] + i + 1] = row + i;

            undo_item.row_hidded[0] += arg; // keep in first position the number of elements (rows)

        } else if (col > -1) { // hide col
            if (undo_item.col_hidded == NULL) {
                undo_item.col_hidded = (int *) malloc(sizeof(int) * (arg + 1));
                undo_item.col_hidded[0] = 0;
            } else
                undo_item.col_hidded = (int *) realloc(undo_item.col_hidded, sizeof(int) * (undo_item.col_hidded[0] + arg + 1));

            for (i=0; i < arg; i++)
                undo_item.col_hidded[undo_item.col_hidded[0] + i + 1] = col + i;

            undo_item.col_hidded[0] += arg; // keep in first position the number of elements (cols)
        }

    } else if (type == 's') {
        if (row > -1) {        // show row
            if (undo_item.row_showed == NULL) {
                undo_item.row_showed = (int *) malloc(sizeof(int) * (arg + 1));
                undo_item.row_showed[0] = 0;
            } else
                undo_item.row_showed = (int *) realloc(undo_item.row_showed, sizeof(int) * (undo_item.row_showed[0] + arg + 1));

            for (i=0; i < arg; i++)
                undo_item.row_showed[undo_item.row_showed[0] + i + 1] = row + i;

            undo_item.row_showed[0] += arg; // keep in first position the number of elements (rows)

        } else if (col > -1) { // show col
            if (undo_item.col_showed == NULL) {
                undo_item.col_showed = (int *) malloc(sizeof(int) * (arg + 1));
                undo_item.col_showed[0] = 0;
            } else
                undo_item.col_showed = (int *) realloc(undo_item.col_showed, sizeof(int) * (undo_item.col_showed[0] + arg + 1));

            for (i=0; i < arg; i++)
                undo_item.col_showed[undo_item.col_showed[0] + i + 1] = col + i;

            undo_item.col_showed[0] += arg; // keep in first position the number of elements (cols)

        }
    }
    return;
}


/**
 * \brief undo_freeze_unfreeze()
 *
 * \details this function is used for undoint and redoing changes
 * caused by freeze row/col and unfreeze row/col commands
 *
 * \param[in] row
 * \param[in] col
 * \param[in] type 'f' or 'u'
 * \param[in] arg
 *
 * \return none
 */
void undo_freeze_unfreeze(int row, int col, char type, int arg) {
    int i;
    if (type == 'f') {
        if (row > -1) {        // hide row
            if (undo_item.row_frozed == NULL) {
                undo_item.row_frozed = (int *) malloc(sizeof(int) * (arg + 1));
                undo_item.row_frozed[0] = 0;
            } else
                undo_item.row_frozed = (int *) realloc(undo_item.row_frozed, sizeof(int) * (undo_item.row_frozed[0] + arg + 1));

            for (i=0; i < arg; i++)
                undo_item.row_frozed[undo_item.row_frozed[0] + i + 1] = row + i;

            undo_item.row_frozed[0] += arg; // keep in first position the number of elements (rows)

        } else if (col > -1) { // hide col
            if (undo_item.col_frozed == NULL) {
                undo_item.col_frozed = (int *) malloc(sizeof(int) * (arg + 1));
                undo_item.col_frozed[0] = 0;
            } else
                undo_item.col_frozed = (int *) realloc(undo_item.col_frozed, sizeof(int) * (undo_item.col_frozed[0] + arg + 1));

            for (i=0; i < arg; i++)
                undo_item.col_frozed[undo_item.col_frozed[0] + i + 1] = col + i;

            undo_item.col_frozed[0] += arg; // keep in first position the number of elements (cols)
        }
    } else if (type == 'u') {
        if (row > -1) {        // unfreeze row
            if (undo_item.row_unfrozed == NULL) {
                undo_item.row_unfrozed = (int *) malloc(sizeof(int) * (arg + 1));
                undo_item.row_unfrozed[0] = 0;
            } else
                undo_item.row_unfrozed = (int *) realloc(undo_item.row_unfrozed, sizeof(int) * (undo_item.row_unfrozed[0] + arg + 1));

            for (i=0; i < arg; i++)
                undo_item.row_unfrozed[undo_item.row_unfrozed[0] + i + 1] = row + i;

            undo_item.row_unfrozed[0] += arg; // keep in first position the number of elements (rows)

        } else if (col > -1) { // unfreeze col
            if (undo_item.col_unfrozed == NULL) {
                undo_item.col_unfrozed = (int *) malloc(sizeof(int) * (arg + 1));
                undo_item.col_unfrozed[0] = 0;
            } else
                undo_item.col_unfrozed = (int *) realloc(undo_item.col_unfrozed, sizeof(int) * (undo_item.col_unfrozed[0] + arg + 1));

            for (i=0; i < arg; i++)
                undo_item.col_unfrozed[undo_item.col_unfrozed[0] + i + 1] = col + i;

            undo_item.col_unfrozed[0] += arg; // keep in first position the number of elements (cols)

        }
    }
    return;
}


/**
 * \brief Do UNDO operation
 *
 * Shift a range of an undo shift range to the original position, if any,
 * append 'ent' elements 'removed' and remove those from 'added'.
 *
 * \return none
 */
void do_undo() {
    struct roman * roman = session->cur_doc;
    if (undo_list == NULL || undo_list_pos == 0) {
        sc_error("No UNDO's left.");
        return;
    }

    // move to the according sheet
    struct sheet * sh = undo_list->sheet;
    roman->cur_sh = sh;

    int ori_currow = sh->currow;
    int ori_curcol = sh->curcol;
    //int mf = roman->modflg; // save modflag status

    struct undo * ul = undo_list;

    // removed added ents
    struct ent_ptr * i = ul->added;
    while (i != NULL) {
        erase_area(i->sheet, i->vp->row, i->vp->col, i->vp->row, i->vp->col, 1, 0);
        i = i->next;
    }

    // Make undo shift, if any
    if (ul->range_shift != NULL) {
        int deltarows = ul->range_shift->delta_rows;
        int deltacols = ul->range_shift->delta_cols;
        int brrow = ul->range_shift->brrow;
        int tlrow = ul->range_shift->tlrow;
        int brcol = ul->range_shift->brcol;
        int tlcol = ul->range_shift->tlcol;
        // fix marks for rows
        if (deltarows > 0)      // sj
            fix_marks(sh, -(brrow - tlrow + 1), 0, tlrow, sh->maxrow, tlcol, brcol);
        else if (deltarows < 0) // sk
            fix_marks(sh,  (brrow - tlrow + 1), 0, tlrow, sh->maxrow, tlcol, brcol);

        // handle row_hidden
        fix_row_hidden(sh, deltarows, tlrow, sh->maxrow);

        // fix marks for cols
        if (deltacols > 0)      // sl
            fix_marks(sh, 0, -(brcol - tlcol + 1), tlrow, brrow, tlcol, sh->maxcol);
        else if (deltacols < 0) // sh
            fix_marks(sh, 0,  (brcol - tlcol + 1), tlrow, brrow, tlcol, sh->maxcol);

        // handle col_hidden
        fix_col_hidden(sh, deltacols, tlcol, sh->maxcol);

        // handle row_frozen
        fix_row_frozen(sh, deltarows, tlrow, sh->maxrow);

        // handle col_frozen
        fix_col_frozen(sh, deltacols, tlcol, sh->maxcol);

        // shift range now
        shift_range(sh, - deltarows, - deltacols, tlrow, tlcol, brrow, brcol);

        // shift col_formats here.
        if (tlcol >= 0 && tlrow == 0 && brrow == sh->maxrows) {
            //sh->maxcols -= deltacols;
            int i;
            if (deltacols > 0)
            for (i = brcol + deltacols; i <= sh->maxcols; i++) {
                sh->fwidth[i - deltacols] = sh->fwidth[i];
                sh->precision[i - deltacols] = sh->precision[i];
                sh->realfmt[i - deltacols] = sh->realfmt[i];
            }
            else
            for (i = sh->maxcols; i >= tlcol - deltacols; i--) {
                 sh->fwidth[i] = sh->fwidth[i + deltacols];
                 sh->precision[i] = sh->precision[i + deltacols];
                 sh->realfmt[i] = sh->realfmt[i + deltacols];
            }
        }
        // do the same for rows here.
        if (tlrow >= 0 && tlcol == 0 && brcol == sh->maxcols) {
            //sh->maxrows -= deltarows;
            int i;
            if (deltarows > 0)
                for (i = brrow + deltarows; i <= sh->maxrows; i++)
                    sh->row_format[i - deltarows] = sh->row_format[i];
            else
                for (i = sh->maxrow; i >= tlrow - deltarows; i--)
                    sh->row_format[i] = sh->row_format[i + deltarows];
        }
    }

    // Change cursor position
    //if (ul->removed != NULL) {
    //    currow = ul->removed->row;
    //    curcol = ul->removed->col;
    //}

    // Append 'ent' elements from the removed ones
    struct ent_ptr * j = ul->removed;
    while (j != NULL) {
        struct ent * h;
        if ((h = *ATBL(j->sheet, j->sheet->tbl, j->vp->row, j->vp->col))) clearent(h);
        struct ent * e_now = lookat(j->sheet, j->vp->row, j->vp->col);
        (void) copyent(e_now, j->sheet, j->vp, 0, 0, 0, 0, 0, 0, 0);
        j = j->next;
    }

    // Show hidden cols and rows
    // Hide visible cols and rows
    if (ul->col_hidded != NULL) {
        int * pd = ul->col_hidded;
        int left = *(pd++);
        while (left--) {
            sh->col_hidden[*(pd++)] = FALSE;
        }
    }
    else if (ul->col_showed  != NULL) {
        int * pd = ul->col_showed;
        int left = *(pd++);
        while (left--) {
            sh->col_hidden[*(pd++)] = TRUE;
        }
    }
    else if (ul->row_hidded  != NULL) {
        int * pd = ul->row_hidded;
        int left = *(pd++);
        while (left--) {
            sh->row_hidden[*(pd++)] = FALSE;
        }
    }
    else if (ul->row_showed  != NULL) {
        int * pd = ul->row_showed;
        int left = *(pd++);
        while (left--) {
            sh->row_hidden[*(pd++)] = TRUE;
        }
    }

    // freeze frozen cols and rows
    // unfreeze unfrozen cols and rows
    if (ul->col_unfrozed != NULL) {
        int * pd = ul->col_unfrozed;
        int left = *(pd++);
        while (left--) {
            sh->col_frozen[*(pd++)] = TRUE;
        }
    }
    if (ul->col_frozed != NULL) {
        int * pd = ul->col_frozed;
        int left = *(pd++);
        while (left--) {
            sh->col_frozen[*(pd++)] = FALSE;
        }
    }
    if (ul->row_unfrozed  != NULL) {
        int * pd = ul->row_unfrozed;
        int left = *(pd++);
        while (left--) {
            sh->row_frozen[*(pd++)] = TRUE;
        }
    }
    if (ul->row_frozed  != NULL) {
        int * pd = ul->row_frozed;
        int left = *(pd++);
        while (left--) {
            sh->row_frozen[*(pd++)] = FALSE;
        }
    }

    // Restore previous col format
    if (ul->cols_format != NULL) {
        struct undo_cols_format * uf = ul->cols_format;
        int size = uf->length;
        int i;

        for (i=0; i < size; i++) {
           if (uf->cols[i].type == 'R') {
               sh->fwidth[uf->cols[i].col]    = uf->cols[i].fwidth;
               sh->precision[uf->cols[i].col] = uf->cols[i].precision;
               sh->realfmt[uf->cols[i].col]   = uf->cols[i].realfmt;
           }
        }
    }

    // Restore previous row format
    if (ul->rows_format != NULL) {
        struct undo_rows_format * uf = ul->rows_format;
        int size = uf->length;
        int i;

        for (i=0; i < size; i++) {
           if (uf->rows[i].type == 'R') {
               sh->row_format[uf->rows[i].row] = uf->rows[i].format;
           }
        }
    }

    // for every ent in added and removed, we reeval expression to update graph
    struct ent_ptr * ie = ul->added;
    while (ie != NULL) {
        struct ent * p;
        if ((p = *ATBL(ie->sheet, ie->sheet->tbl, ie->vp->row, ie->vp->col)) && p->expr)
            EvalJustOneVertex(ie->sheet, p, 1);
        ie = ie->next;
    }
    ie = ul->removed;
    while (ie != NULL) {
        struct ent * p;
        if ((p = *ATBL(ie->sheet, ie->sheet->tbl, ie->vp->row, ie->vp->col)) && p->expr)
            EvalJustOneVertex(ie->sheet, p, 1);
        ie = ie->next;
    }

    // Restores cursor position
    sh->currow = ori_currow;
    sh->curcol = ori_curcol;

    // decrease modflg
    //roman->modflg = mf - 1;

    // restore maxrow, maxcol and modflg status before the action
    roman->modflg = undo_item.modflg_bef;
    sh->maxrow = undo_item.maxrow_bef;
    sh->maxcol = undo_item.maxcol_bef;

    if (undo_list->p_ant != NULL) undo_list = undo_list->p_ant;
    sc_info("Change: %d of %d", --undo_list_pos, len_undo_list());
    return;
}


/**
 * \brief Do REDO
 * Shift a range of an undo shift range to the original position, if any,
 * append 'ent' elements from 'added' and remove those from 'removed'.
 * \return none
 */
void do_redo() {
    struct roman * roman = session->cur_doc;

    if ( undo_list == NULL || undo_list_pos == len_undo_list()  ) {
        sc_error("No REDO's left.");
        return;
    }

    if ((undo_list->p_ant != NULL || undo_list_pos != 0)
    && (undo_list->p_sig != NULL)) undo_list = undo_list->p_sig;

    struct undo * ul = undo_list;
    struct sheet * sh = undo_list->sheet;

    int ori_currow = sh->currow;
    int ori_curcol = sh->curcol;
    //int mf = roman->modflg; // save modflag status

    // move to the according sheet
    roman->cur_sh = sh;

    // Remove 'ent' elements
    struct ent_ptr * i = ul->removed;
    while (i != NULL) {
        erase_area(i->sheet, i->vp->row, i->vp->col, i->vp->row, i->vp->col, 1, 0);
        i = i->next;
    }

    // Make undo shift, if any
    if (ul->range_shift != NULL) {
        int deltarows = ul->range_shift->delta_rows;
        int deltacols = ul->range_shift->delta_cols;
        int brrow = ul->range_shift->brrow;
        int tlrow = ul->range_shift->tlrow;
        int brcol = ul->range_shift->brcol;
        int tlcol = ul->range_shift->tlcol;
        // fix marks for rows
        if (deltarows > 0)      // sj
            fix_marks(sh,  (brrow - tlrow + 1), 0, tlrow, sh->maxrow, tlcol, brcol);
        else if (deltarows < 0) // sk
            fix_marks(sh, -(brrow - tlrow + 1), 0, tlrow, sh->maxrow, tlcol, brcol);

        // handle row_hidden
        fix_row_hidden(sh, -deltarows, tlrow, sh->maxrow);

        // fix marks for cols
        if (deltacols > 0)      // sl
            fix_marks(sh, 0,  (brcol - tlcol + 1), tlrow, brrow, tlcol, sh->maxcol);
        else if (deltacols < 0) // sh
            fix_marks(sh, 0, -(brcol - tlcol + 1), tlrow, brrow, tlcol, sh->maxcol);

        // handle col_hidden
        fix_col_hidden(sh, -deltacols, tlcol, sh->maxcol);

        // handle row_frozen
        fix_row_frozen(sh, -deltarows, tlrow, sh->maxrow);

        // handle col_frozen
        fix_col_frozen(sh, -deltacols, tlcol, sh->maxcol);

        // shift range now
        shift_range(sh, deltarows, deltacols, tlrow, tlcol, brrow, brcol);

        // shift col_formats here
        if (tlcol >= 0 && tlrow == 0 && brrow == sh->maxrow) {
            //sh->maxcols += deltacols;
            int i;
            if (deltacols > 0)
            for (i = sh->maxcols; i >= tlcol + deltacols; i--) {
                sh->fwidth[i] = sh->fwidth[i - deltacols];
                sh->precision[i] = sh->precision[i - deltacols];
                sh->realfmt[i] = sh->realfmt[i - deltacols];
            }
            else
            for (i = tlcol; i - deltacols <= sh->maxcols; i++) {
                sh->fwidth[i] = sh->fwidth[i - deltacols];
                sh->precision[i] = sh->precision[i - deltacols];
                sh->realfmt[i] = sh->realfmt[i - deltacols];
            }
        }
        // do the same for rows here
        if (tlrow >= 0 && tlcol == 0 && brcol == sh->maxcols) {
            //sh->maxrows += deltarows;
            int i;
            if (deltarows > 0)
                for (i = sh->maxrows; i >= tlrow + deltarows; i--)
                    sh->row_format[i] = sh->row_format[i - deltarows];
            else
                for (i = tlrow; i - deltarows <= sh->maxrows; i++)
                    sh->row_format[i] = sh->row_format[i - deltarows];
        }
    }

    // Change cursor position
    //if (ul->p_sig != NULL && ul->p_sig->removed != NULL) {
    //    currow = ul->p_sig->removed->row;
    //    curcol = ul->p_sig->removed->col;
    //}

    // Append 'ent' elements
    struct ent_ptr * j = ul->added;
    while (j != NULL) {
        struct ent * h;
        if ((h = *ATBL(j->sheet, j->sheet->tbl, j->vp->row, j->vp->col))) clearent(h);
        struct ent * e_now = lookat(j->sheet, j->vp->row, j->vp->col);
        (void) copyent(e_now, j->sheet, j->vp, 0, 0, 0, 0, 0, 0, 0);
        j = j->next;
    }

    // Hide previously hidden cols and rows
    // Show previously visible cols and rows
    if (ul->col_hidded != NULL) {
        int * pd = ul->col_hidded;
        int left = *(pd++);
        while (left--) {
            sh->col_hidden[*(pd++)] = TRUE;
        }
    }
    else if (ul->col_showed  != NULL) {
        int * pd = ul->col_showed;
        int left = *(pd++);
        while (left--) {
            sh->col_hidden[*(pd++)] = FALSE;
        }
    }
    else if (ul->row_hidded  != NULL) {
        int * pd = ul->row_hidded;
        int left = *(pd++);
        while (left--) {
            sh->row_hidden[*(pd++)] = TRUE;
        }
    }
    else if (ul->row_showed  != NULL) {
        int * pd = ul->row_showed;
        int left = *(pd++);
        while (left--) {
            sh->row_hidden[*(pd++)] = FALSE;
        }
    }

    // freeze frozen cols and rows
    // unfreeze unfrozen cols and rows
    if (ul->col_unfrozed != NULL) {
        int * pd = ul->col_unfrozed;
        int left = *(pd++);
        while (left--) {
            sh->col_frozen[*(pd++)] = FALSE;
        }
    }
    if (ul->col_frozed  != NULL) {
        int * pd = ul->col_frozed;
        int left = *(pd++);
        while (left--) {
            sh->col_frozen[*(pd++)] = TRUE;
        }
    }
    if (ul->row_unfrozed  != NULL) {
        int * pd = ul->row_unfrozed;
        int left = *(pd++);
        while (left--) {
            sh->row_frozen[*(pd++)] = FALSE;
        }
    }
    if (ul->row_frozed  != NULL) {
        int * pd = ul->row_frozed;
        int left = *(pd++);
        while (left--) {
            sh->row_frozen[*(pd++)] = TRUE;
        }
    }

    // Restore new col format
    if (ul->cols_format != NULL) {
        struct undo_cols_format * uf = ul->cols_format;
        int size = uf->length;
        int i;

        for (i=0; i < size; i++) {
            if (uf->cols[i].type == 'A') {
                sh->fwidth[uf->cols[i].col]    = uf->cols[i].fwidth;
                sh->precision[uf->cols[i].col] = uf->cols[i].precision;
                sh->realfmt[uf->cols[i].col]   = uf->cols[i].realfmt;
            }
        }
    }

    // Restore new row format
    if (ul->rows_format != NULL) {
        struct undo_rows_format * uf = ul->rows_format;
        int size = uf->length;
        int i;

        for (i=0; i < size; i++) {
           if (uf->rows[i].type == 'A') {
               sh->row_format[uf->rows[i].row] = uf->rows[i].format;
           }
        }
    }

    // for every ent in added and removed, we reeval expression to update graph
    struct ent_ptr * ie = ul->added;
    while (ie != NULL) {
        struct ent * p;
        if ((p = *ATBL(ie->sheet, ie->sheet->tbl, ie->vp->row, ie->vp->col)) && p->expr)
            EvalJustOneVertex(ie->sheet, p, 1);
        ie = ie->next;
    }
    ie = ul->removed;
    while (ie != NULL) {
        struct ent * p;
        if ((p = *ATBL(ie->sheet, ie->sheet->tbl, ie->vp->row, ie->vp->col)) && p->expr)
            EvalJustOneVertex(ie->sheet, p, 1);
        ie = ie->next;
    }

    // Restores cursor position
    sh->currow = ori_currow;
    sh->curcol = ori_curcol;

    // increase modflg
    //roman->modflg = mf + 1;

    // restore maxrow, maxcol and modflg status before the action
    roman->modflg = undo_item.modflg_aft;
    sh->maxrow = undo_item.maxrow_aft;
    sh->maxcol = undo_item.maxcol_aft;

    sc_info("Change: %d of %d", ++undo_list_pos, len_undo_list());
    return;
}
#endif
