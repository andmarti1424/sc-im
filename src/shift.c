#include <stdio.h>
#include <stdlib.h>
#include "shift.h"
#include "sc.h"
#include "vmtbl.h"   // for growtbl
#include "cmds.h"
#include "dep_graph.h"

extern graphADT graph;

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

    for (r = maxrow; r > currow; r--) {
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

    int lim = maxcol - curcol + deltacols;
    for (r=currow; r < currow + deltarows; r++) {
        pp = ATBL(tbl, r, maxcol);
        for (c = lim; --c >= deltacols; pp--)
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

                // delete vertex in graph
                if (*pp && getVertex(graph, *pp, 0) != NULL) destroy_vertex(*pp);

                if (*pp) {
                   mark_ent_as_deleted(*pp);
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

                // delete vertex in graph
                if (*pp && getVertex(graph, *pp, 0) != NULL) destroy_vertex(*pp);

                if (*pp) {
                   mark_ent_as_deleted(*pp);
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
