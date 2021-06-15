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
 * \file shift.c
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 25/05/2021
 */

#include <stdio.h>
#include <stdlib.h>
#include "shift.h"
#include "../sc.h"
#include "../vmtbl.h"   // for growtbl
#include "../cmds/cmds.h"
#include "../graph.h"
#include "../undo.h"
#include "../marks.h"
#include "../yank.h"
#include "../conf.h"
#include "../tui.h"

extern graphADT graph;
extern int cmd_multiplier;
extern struct session * session;

/**
 * @brief Shift function - handles undo
 *
 * Shift functin - handles unto. Should also be called form gram.y.
 *
 * \param[in] r
 * \param[in] c
 * \param[in] rf
 * \param[in] cf
 * \param[in] type
 *
 * \return none
 */

void shift(struct sheet * sh, int r, int c, int rf, int cf, wchar_t type) {
    struct roman * roman = session->cur_doc;
    if (cf - 1 + cmd_multiplier >= sh->maxcols && type == L'h') {
        sc_error("current column + multiplier exceeds max column. Nothing changed");
        return;
    } else if (rf - 1 + cmd_multiplier >= sh->maxrows && type == L'k') {
        sc_error("current row + multiplier exceeds max row. Nothing changed");
        return;
    } else if (any_locked_cells(sh, r, c, rf, cf) && (type == L'h' || type == L'k') ) {
        sc_error("Locked cells encountered. Nothing changed");
        return;
    }
#ifdef UNDO
    create_undo_action();
#endif
    int ic = cmd_multiplier + 1;

    switch (type) {

        case L'j':
            fix_marks(sh,  (rf - r + 1) * cmd_multiplier, 0, r, sh->maxrow, c, cf);
#ifdef UNDO
            save_undo_range_shift(cmd_multiplier, 0, r, c, rf + (rf-r+1) * (cmd_multiplier - 1), cf);
#endif
            while (ic--) shift_range(sh, ic, 0, r, c, rf, cf);
            break;

        case L'k':
            fix_marks(sh, -(rf - r + 1) * cmd_multiplier, 0, r, sh->maxrow, c, cf);
            yank_area(sh, r, c, rf + (rf-r+1) * (cmd_multiplier - 1), cf, 'a', cmd_multiplier); // keep ents in yanklist for sk
#ifdef UNDO
            ents_that_depends_on_range(sh, r, c, rf + (rf-r+1) * (cmd_multiplier - 1), cf);
            copy_to_undostruct(sh, r, c, rf + (rf-r+1) * (cmd_multiplier - 1), cf, UNDO_DEL, HANDLE_DEPS, NULL);
            save_undo_range_shift(-cmd_multiplier, 0, r, c, rf + (rf-r+1) * (cmd_multiplier - 1), cf);
#endif
            while (ic--) shift_range(sh, -ic, 0, r, c, rf, cf);
            if (get_conf_int("autocalc") && ! roman->loading) EvalAll();
#ifdef UNDO
            copy_to_undostruct(sh, 0, 0, -1, -1, UNDO_ADD, HANDLE_DEPS, NULL);
#endif
            break;

        case L'h':
            fix_marks(sh, 0, -(cf - c + 1) * cmd_multiplier, r, rf, c, sh->maxcol);
            yank_area(sh, r, c, rf, cf + (cf-c+1) * (cmd_multiplier - 1), 'a', cmd_multiplier); // keep ents in yanklist for sk
#ifdef UNDO
            // here we save in undostruct, all the ents that depends on the deleted one (before change)
            ents_that_depends_on_range(sh, r, c, rf, cf + (cf-c+1) * (cmd_multiplier - 1));
            copy_to_undostruct(sh, r, c, rf, cf + (cf-c+1) * (cmd_multiplier - 1), UNDO_DEL, HANDLE_DEPS, NULL);
            save_undo_range_shift(0, -cmd_multiplier, r, c, rf, cf + (cf-c+1) * (cmd_multiplier - 1));
#endif
            while (ic--) shift_range(sh, 0, -ic, r, c, rf, cf);

            if (get_conf_int("autocalc") && ! roman->loading) EvalAll();
            //update(TRUE); // this is used just to make debugging easier
#ifdef UNDO
            copy_to_undostruct(sh, 0, 0, -1, -1, UNDO_ADD, HANDLE_DEPS, NULL);
#endif
            break;

        case L'l':
            fix_marks(sh, 0,  (cf - c + 1) * cmd_multiplier, r, rf, c, sh->maxcol);
#ifdef UNDO
            save_undo_range_shift(0, cmd_multiplier, r, c, rf, cf + (cf-c+1) * (cmd_multiplier - 1));
#endif
            while (ic--) shift_range(sh, 0, ic, r, c, rf, cf);
            break;
    }
#ifdef UNDO
    end_undo_action();
    extern struct ent_ptr * deps;
    if (deps != NULL) free(deps);
    deps = NULL;
#endif
    roman->modflg++;
    /* just for testing
    sync_refs();
    rebuild_graph();
    sync_refs();
    rebuild_graph(); */
    cmd_multiplier = 0;
    return;
}

/**
 * \brief shift_range()
 * \details Shift a range of cells
 * \param[in] struct sheet * sh
 * \param[in] delta_rows
 * \param[in] delta_cols
 * \param[in] tlrow
 * \param[in] tlcol
 * \param[in] brrow
 * \param[in] brcol
 *
 * \return none
 */

void shift_range(struct sheet * sh, int delta_rows, int delta_cols, int tlrow, int tlcol, int brrow, int brcol) {
    sh->currow = tlrow;
    sh->curcol = tlcol;

    if (delta_rows > 0)      shift_cells_down (sh, brrow - tlrow + 1, brcol - tlcol + 1);
    else if (delta_rows < 0) shift_cells_up   (sh, brrow - tlrow + 1, brcol - tlcol + 1);

    if (delta_cols > 0)      shift_cells_right(sh, brrow - tlrow + 1, brcol - tlcol + 1);
    else if (delta_cols < 0) shift_cells_left (sh, brrow - tlrow + 1, brcol - tlcol + 1);

    return;
}

/**
 * \brief Shift cells down
 *
 * \param[in] deltarows
 * \param[in] deltacols
 *
 * \return none
 */

void shift_cells_down(struct sheet * sh, int deltarows, int deltacols) {
    int r, c;
    struct ent ** pp;
    //if (sh->currow + deltarows > sh->maxrow) sh->maxrow = sh->currow + deltarows;
    //commented for #569
    //if (sh->currow > sh->maxrow) sh->maxrow = sh->currow;
    sh->maxrow += deltarows;
    if ((sh->maxrow >= sh->maxrows) && !growtbl(sh, GROWROW, sh->maxrow, 0)) {
        sh->maxrow = sh->maxrows - 1;
        return;
    }

    for (r = sh->maxrow; r > sh->currow + deltarows - 1; r--) {
        for (c = sh->curcol; c < sh->curcol + deltacols; c++) {
            pp = ATBL(sh, sh->tbl, r, c);
            pp[0] = *ATBL(sh, sh->tbl, r-deltarows, c);
            if ( pp[0] ) pp[0]->row += deltarows;
        }
    }
    // blank new ents
    for (c = sh->curcol; c < sh->curcol + deltacols; c++)
        for (r = sh->currow; r < sh->currow + deltarows; r++) {
            pp = ATBL(sh, sh->tbl, r, c);
            *pp = (struct ent *) 0;
        }
    return;
}

/**
 * \brief Shift cells right
 *
 * \param[in] deltaros
 * \param[in] deltacols
 *
 * \return none
 */

void shift_cells_right(struct sheet * sh, int deltarows, int deltacols) {
    int r, c;
    struct ent ** pp;

    //commented for #569
    //if (sh->curcol + deltacols > sh->maxcol) sh->maxcol = sh->curcol + deltacols;
    sh->maxcol += deltacols;

    if ((sh->maxcol >= sh->maxcols) && !growtbl(sh, GROWCOL, 0, sh->maxcol)) {
        sh->maxcol = sh->maxcols - 1;
        return;
    }

    int lim = sh->maxcol - sh->curcol - deltacols;
    for (r=sh->currow; r < sh->currow + deltarows; r++) {
        pp = ATBL(sh, sh->tbl, r, sh->maxcol);
        for (c = lim; c-- >= 0; pp--)
            if ((pp[0] = pp[-deltacols])) pp[0]->col += deltacols;

        pp = ATBL(sh, sh->tbl, r, sh->curcol);
        for (c = sh->curcol; c < sh->curcol + deltacols; c++, pp++)
            *pp = (struct ent *) 0;
    }
    return;
}

/**
 * \brief Shift cells up
 *
 * \param[in] deltarows
 * \param[in] deltacols
 *
 * \return none
 */

void shift_cells_up(struct sheet * sh, int deltarows, int deltacols) {
    int r, c;
    struct ent ** pp;

    for (r = sh->currow; r <= sh->maxrow; r++) {
        for (c = sh->curcol; c < sh->curcol + deltacols; c++) {

            if (r < sh->currow + deltarows) {
                pp = ATBL(sh, sh->tbl, r, c);

                /* delete vertex in graph
                   unless vertex is referenced by other. Shall comment this? See NOTE1 above */
                vertexT * v = getVertex(graph, sh, *pp, 0);
                if (v != NULL && v->back_edges == NULL ) destroy_vertex(sh, *pp);

                if (*pp) {
                   mark_ent_as_deleted(*pp, TRUE); //important: this mark the ents as deleted
                   //clearent(*pp);
                   //free(*pp);
                   *pp = NULL;
                }
            }
            if (r <= sh->maxrow - deltarows) {
                pp = ATBL(sh, sh->tbl, r, c);
                pp[0] = *ATBL(sh, sh->tbl, r + deltarows, c);
                if ( pp[0] ) pp[0]->row -= deltarows;
            }
            //blank bottom ents
            if (r > sh->maxrow - deltarows) {
                pp = ATBL(sh, sh->tbl, r, c);
                *pp = (struct ent *) 0;
            }
        }
    }
    return;
}

/**
 * \brief Shift cells left
 *
 * \param[in] deltarows
 * \param[in] deltacols
 *
 * \return none
 */

void shift_cells_left(struct sheet * sh, int deltarows, int deltacols) {
    int r, c;
    struct ent ** pp;

    for (c = sh->curcol; c <= sh->maxcol; c++) {
        for (r = sh->currow; r < sh->currow + deltarows; r++) {

            if (c < sh->curcol + deltacols) {
                pp = ATBL(sh, sh->tbl, r, c);

                /* delete vertex in graph
                   unless vertex is referenced by other */
                vertexT * v = getVertex(graph, sh, *pp, 0);
                if (v != NULL && v->back_edges == NULL ) destroy_vertex(sh, *pp);

                if (*pp) {
                   mark_ent_as_deleted(*pp, TRUE); //important: this mark the ents as deleted
                   //clearent(*pp);
                   //free(*pp);
                   *pp = NULL;
                }
            }
            if (c <= sh->maxcol - deltacols) {
                pp = ATBL(sh, sh->tbl, r, c);
                pp[0] = *ATBL(sh, sh->tbl, r, c + deltacols);
                if ( pp[0] ) pp[0]->col -= deltacols;
            }
            //blank bottom ents
            if (c > sh->maxcol - deltacols) {
                pp = ATBL(sh, sh->tbl, r, c);
                *pp = (struct ent *) 0;
            }
        }
    }
    return;
}
