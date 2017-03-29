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

extern graphADT graph;
extern int cmd_multiplier;

/* shift function - handles undo
   should also be called from GRAM.Y
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
            rebuild_graph(); // FIXME check why have to rebuild graph here. See NOTE1 below.
            if (atoi(get_conf_value("autocalc")) && ! loading) EvalAll();
#ifdef UNDO
            copy_to_undostruct(r, c, rf + (rf-r+1) * (cmd_multiplier - 1), cf, 'a');
            for (i = 0; deps != NULL && i < deps->vf; i++)
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
            for (i = 0; deps != NULL && i < deps->vf; i++)
                copy_to_undostruct(deps[i].vp->row, deps[i].vp->col, deps[i].vp->row, deps[i].vp->col, 'd');
#endif
            while (ic--) shift_range(0, -ic, r, c, rf, cf);
            rebuild_graph(); // FIXME check why have to rebuild graph here.
            if (atoi(get_conf_value("autocalc")) && ! loading) EvalAll();
#ifdef UNDO
            copy_to_undostruct(r, c, rf, cf + (cf-c+1) * (cmd_multiplier - 1), 'a');
            for (i = 0; deps != NULL && i < deps->vf; i++)
                copy_to_undostruct(deps[i].vp->row, deps[i].vp->col, deps[i].vp->row, deps[i].vp->col, 'a');
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
    cmd_multiplier = 0;
    return;
}

// shift a range of 'ENTS'
void shift_range(int delta_rows, int delta_cols, int tlrow, int tlcol, int brrow, int brcol) {
    currow = tlrow;
    curcol = tlcol;

    if (delta_rows > 0)      shift_cells_down (brrow - tlrow + 1, brcol - tlcol + 1);
    else if (delta_rows < 0) shift_cells_up   (brrow - tlrow + 1, brcol - tlcol + 1);

    if (delta_cols > 0)      shift_cells_right(brrow - tlrow + 1, brcol - tlcol + 1);
    else if (delta_cols < 0) shift_cells_left (brrow - tlrow + 1, brcol - tlcol + 1);

    return;
}


// shift cells down
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
            //sc_debug("delta down");
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


// shift cells right
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


// shift cells up
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
                   mark_ent_as_deleted(*pp, TRUE);
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


// shift cells left
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
                   mark_ent_as_deleted(*pp, TRUE);
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
