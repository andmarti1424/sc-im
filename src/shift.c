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
 * \file shift.c
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 2017-07-18
 * \brief TODO Write a tbrief file description.
 */

#include <stdio.h>
#include <stdlib.h>
#include "shift.h"
#include "sc.h"
#include "vmtbl.h"   // for growtbl
#include "cmds.h"
#include "dep_graph.h"
#include "undo.h"
#include "marks.h"
#include "yank.h"
#include "conf.h"
#include "tui.h"

extern graphADT graph;
extern int cmd_multiplier;

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

void shift(int r, int c, int rf, int cf, wchar_t type) {
    if ( any_locked_cells(r, c, rf, cf) && (type == L'h' || type == L'k') ) {
        sc_error("Locked cells encountered. Nothing changed");
        return;
    }
#ifdef UNDO
    create_undo_action();

    // here we save in undostruct, all the ents that depends on the deleted one (before change)
    extern struct ent_ptr * deps;
    int i;
#endif
    int ic = cmd_multiplier + 1;

    switch (type) {

        case L'j':
            fix_marks(  (rf - r + 1) * cmd_multiplier, 0, r, maxrow, c, cf);
#ifdef UNDO
            save_undo_range_shift(cmd_multiplier, 0, r, c, rf + (rf-r+1) * (cmd_multiplier - 1), cf);
#endif
            while (ic--) shift_range(ic, 0, r, c, rf, cf);
            break;

        case L'k':
            fix_marks( -(rf - r + 1) * cmd_multiplier, 0, r, maxrow, c, cf);
            yank_area(r, c, rf + (rf-r+1) * (cmd_multiplier - 1), cf, 'a', cmd_multiplier); // keep ents in yanklist for sk
#ifdef UNDO
            copy_to_undostruct(r, c, rf + (rf-r+1) * (cmd_multiplier - 1), cf, 'd');
            save_undo_range_shift(-cmd_multiplier, 0, r, c, rf + (rf-r+1) * (cmd_multiplier - 1), cf);
            ents_that_depends_on_range(r, c, rf + (rf-r+1) * (cmd_multiplier - 1), cf);
            for (i = 0; deps != NULL && i < deps->vf; i++)
                copy_to_undostruct(deps[i].vp->row, deps[i].vp->col, deps[i].vp->row, deps[i].vp->col, 'd');
#endif
            while (ic--) shift_range(-ic, 0, r, c, rf, cf);
            if (atoi(get_conf_value("autocalc")) && ! loading) EvalAll();
#ifdef UNDO
            // update(TRUE); this is used just to make debugging easier
            for (i = 0; deps != NULL && i < deps->vf; i++) // TODO here save just ents that are off the shifted range
                copy_to_undostruct(deps[i].vp->row, deps[i].vp->col, deps[i].vp->row, deps[i].vp->col, 'a');
#endif
            break;

        case L'h':
            fix_marks(0, -(cf - c + 1) * cmd_multiplier, r, rf, c, maxcol);
            yank_area(r, c, rf, cf + (cf-c+1) * (cmd_multiplier - 1), 'a', cmd_multiplier); // keep ents in yanklist for sk
#ifdef UNDO
            copy_to_undostruct(r, c, rf, cf + (cf-c+1) * (cmd_multiplier - 1), 'd');
            save_undo_range_shift(0, -cmd_multiplier, r, c, rf, cf + (cf-c+1) * (cmd_multiplier - 1));
            ents_that_depends_on_range(r, c, rf, cf + (cf-c+1) * (cmd_multiplier - 1));
            for (i = 0; deps != NULL && i < deps->vf; i++) {
                copy_to_undostruct(deps[i].vp->row, deps[i].vp->col, deps[i].vp->row, deps[i].vp->col, 'd');
            }
#endif
            while (ic--) shift_range(0, -ic, r, c, rf, cf);

            if (atoi(get_conf_value("autocalc")) && ! loading) EvalAll();
            //update(TRUE); // this is used just to make debugging easier
#ifdef UNDO
            for (i = 0; deps != NULL && i < deps->vf; i++) {
                if (deps[i].vp->col > cf || deps[i].vp->col <= c) {
                    copy_to_undostruct(deps[i].vp->row, deps[i].vp->col, deps[i].vp->row, deps[i].vp->col, 'a');
                }
            }
#endif
            break;

        case L'l':
            fix_marks(0,  (cf - c + 1) * cmd_multiplier, r, rf, c, maxcol);
#ifdef UNDO
            save_undo_range_shift(0, cmd_multiplier, r, c, rf, cf + (cf-c+1) * (cmd_multiplier - 1));
#endif
            while (ic--) shift_range(0, ic, r, c, rf, cf);
            break;
    }
#ifdef UNDO
    end_undo_action();
    if (deps != NULL) free(deps);
    deps = NULL;
#endif
    /* just for testing
    sync_refs();
    rebuild_graph();
    sync_refs();
    rebuild_graph(); */
    cmd_multiplier = 0;
    return;
}

/**
 * \brief Shift a range to 'ENTS'
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

void shift_range(int delta_rows, int delta_cols, int tlrow, int tlcol, int brrow, int brcol) {
    currow = tlrow;
    curcol = tlcol;

    if (delta_rows > 0)      shift_cells_down (brrow - tlrow + 1, brcol - tlcol + 1);
    else if (delta_rows < 0) shift_cells_up   (brrow - tlrow + 1, brcol - tlcol + 1);

    if (delta_cols > 0)      shift_cells_right(brrow - tlrow + 1, brcol - tlcol + 1);
    else if (delta_cols < 0) shift_cells_left (brrow - tlrow + 1, brcol - tlcol + 1);

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

void shift_cells_down(int deltarows, int deltacols) {
    int r, c;
    struct ent ** pp;
    if (currow > maxrow) maxrow = currow;
    maxrow += deltarows;
    if ((maxrow >= maxrows) && !growtbl(GROWROW, maxrow, 0))
        return;

    for (r = maxrow; r > currow + deltarows - 1; r--) {
        for (c = curcol; c < curcol + deltacols; c++) {
            pp = ATBL(tbl, r, c);
            pp[0] = *ATBL(tbl, r-deltarows, c);
            if ( pp[0] ) pp[0]->row += deltarows;
        }
    }
    // blank new ents
    for (c = curcol; c < curcol + deltacols; c++)
    for (r = currow; r < currow + deltarows; r++) {
        pp = ATBL(tbl, r, c);
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

void shift_cells_right(int deltarows, int deltacols) {
    int r, c;
    struct ent ** pp;

    if (curcol + deltacols > maxcol)
        maxcol = curcol + deltacols;
    maxcol += deltacols;

    if ((maxcol >= maxcols) && !growtbl(GROWCOL, 0, maxcol))
        return;

    int lim = maxcol - curcol - deltacols;
    for (r=currow; r < currow + deltarows; r++) {
        pp = ATBL(tbl, r, maxcol);
        for (c = lim; c-- >= 0; pp--)
            if ((pp[0] = pp[-deltacols])) pp[0]->col += deltacols;

        pp = ATBL(tbl, r, curcol);
        for (c = curcol; c < curcol + deltacols; c++, pp++)
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

void shift_cells_up(int deltarows, int deltacols) {
    int r, c;
    struct ent ** pp;

    for (r = currow; r <= maxrow; r++) {
        for (c = curcol; c < curcol + deltacols; c++) {

            if (r < currow + deltarows) {
                pp = ATBL(tbl, r, c);

                /* delete vertex in graph
                   unless vertex is referenced by other. Shall comment this? See NOTE1 above */
                vertexT * v = getVertex(graph, *pp, 0);
                if (v != NULL && v->back_edges == NULL ) destroy_vertex(*pp);

                if (*pp) {
                   mark_ent_as_deleted(*pp, TRUE); //important: this mark the ents as deleted
                   //clearent(*pp);
                   //free(*pp);
                   *pp = NULL;
                }
            }
            if (r <= maxrow - deltarows) {
                pp = ATBL(tbl, r, c);
                pp[0] = *ATBL(tbl, r + deltarows, c);
                if ( pp[0] ) pp[0]->row -= deltarows;
            }
            //blank bottom ents
            if (r > maxrow - deltarows) {
                pp = ATBL(tbl, r, c);
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

void shift_cells_left(int deltarows, int deltacols) {
    int r, c;
    struct ent ** pp;

    for (c = curcol; c <= maxcol; c++) {
        for (r = currow; r < currow + deltarows; r++) {

            if (c < curcol + deltacols) {
                pp = ATBL(tbl, r, c);

                /* delete vertex in graph
                   unless vertex is referenced by other */
                vertexT * v = getVertex(graph, *pp, 0);
                if (v != NULL && v->back_edges == NULL ) destroy_vertex(*pp);

                if (*pp) {
                   mark_ent_as_deleted(*pp, TRUE); //important: this mark the ents as deleted
                   //clearent(*pp);
                   //free(*pp);
                   *pp = NULL;
                }
            }
            if (c <= maxcol - deltacols) {
                pp = ATBL(tbl, r, c);
                pp[0] = *ATBL(tbl, r, c + deltacols);
                if ( pp[0] ) pp[0]->col -= deltacols;
            }
            //blank bottom ents
            if (c > maxcol - deltacols) {
                pp = ATBL(tbl, r, c);
                *pp = (struct ent *) 0;
            }
        }
    }
    return;
}
