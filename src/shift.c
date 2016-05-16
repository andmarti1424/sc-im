#include <stdio.h>
#include <stdlib.h>
#include "shift.h"
#include "sc.h"
#include "vmtbl.h"   // for growtbl
#include "cmds.h"

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
// REVISED
void shift_cells_down(int deltarows, int deltacols) {
    int r, c;
    struct ent ** pp;
    int lim = maxrow - currow + deltarows;
    if (currow > maxrow) maxrow = currow;
    maxrow += deltarows;
    lim = maxrow - lim + deltarows - 1;
    if ((maxrow >= maxrows) && !growtbl(GROWROW, maxrow, 0))
        return;

    for (r = maxrow; r > currow; r--) {
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


// shift cells right
// REVISED
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
// TODO rewrite without using copyent
void shift_cells_up(int deltarows, int deltacols) {
    register struct ent ** pp;
    register struct ent * n;
    register struct ent * p;
    int r, c, i;
    //if (currow + deltarows - 1 > maxrow) return;
    // Delete cell content
    for (i = 0; i < deltarows; i++)
        for (r = currow; r < maxrow; r++)
            for (c = curcol; c < curcol + deltacols; c++) {

                // Free 'ent' memory
                pp = ATBL(tbl, r, c);
                clearent(*pp);

                p = *ATBL(tbl, r+1, c);
                if (p && ( (p->flags & is_valid) || (p->expr && (p->flags & is_strexpr)) || p->label )  ) {
                    // Copy below cell
                    n = lookat(r, c);
                    (void) copyent(n, p, 0, 0, 0, 0, r, c, 0);
                    n->row--;
                    pp = ATBL(tbl, r+1, c);
                } else {
                    pp = ATBL(tbl, r, c);
                }
                clearent(*pp);
            }
    return;
}

// shift cells left
// TODO rewrite without using copyent
void shift_cells_left(int deltarows, int deltacols) {
    int r, j, c;
    register struct ent * p;
    register struct ent * n;
    struct ent ** pp;
    for (j = 0; j < deltacols; j++)
        for (r = currow; r < currow + deltarows; r++)
            for (c = curcol; c < maxcol; c++) {

                // Free 'ent' memory
                pp = ATBL(tbl, r, c);
                clearent(*pp);

                p = *ATBL(tbl, r, c + 1);
                if (p && ( (p->flags & is_valid) || (p->expr && (p->flags & is_strexpr)) || p->label )  ) {
                    n = lookat(r, c);
                    (void) copyent(n, p, 0, -1, r, 0, maxrow, maxcol, 0); // copy p a n
                    n->col--;
                    pp = ATBL(tbl, r, c + 1);
                } else { // When shifting the cells from the last column
                    pp = ATBL(tbl, r, c);
                }
                clearent(*pp);
            }
    return;
}
