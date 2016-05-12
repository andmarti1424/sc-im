#include <stdio.h>
#include <stdlib.h>
#include "shift.h"
#include "sc.h"
#include "vmtbl.h"   // for growtbl
#include "cmds.h"

// Shift a range of 'ENTS'
// TODO rewrite shift_range (without using copyent), so it doesn't depend on
// shift_cells, shift_cells_up, shift_cells_down, shift_cells_left,
// shift_cells_right. And delete those
/*
void shift_range(int direction, int arg, int tlrow, int tlcol, int brrow, int brcol) {

    currow = tlrow;
    curcol = tlcol;

    info("%d %d %d %d", direction, arg, brrow - tlrow + 1, brcol - tlcol + 1);

    if (direction == 'j')      while(arg--) shift_cells_down (brrow - tlrow + 1, brcol - tlcol + 1);
    else if (direction == 'k') while(arg--) shift_cells_up   (brrow - tlrow + 1, brcol - tlcol + 1);
    else if (direction == 'l') while(arg--) shift_cells_right(brrow - tlrow + 1, brcol - tlcol + 1);
    else if (direction == 'h') while(arg--) shift_cells_left (brrow - tlrow + 1, brcol - tlcol + 1);

    return;
}
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

// Shift cells down
void shift_cells_down(int deltarows, int deltacols) {
    int r, c;
    struct ent **pp;
    register struct ent * p;
    register struct ent *n;
    int lim = maxrow - currow + 1;

    // Move cell content
    if (currow > maxrow) maxrow = currow;
    maxrow += deltarows;

    lim = maxrow - lim;
    if ((maxrow >= maxrows) && !growtbl(GROWROW, maxrow, 0))
        return;

    for (r = maxrow; r > lim; r--) {
        for (c = curcol; c < curcol + deltacols; c++) {
            p = *ATBL(tbl, r - deltarows, c);
            if (p) {
                n = lookat(r, c);
                copyent(n, p, 0, 0, 0, 0, r - deltarows, c, 0);
                n->row += deltarows;
                p = (struct ent *)0;

                pp = ATBL(tbl, r - deltarows, c);
                clearent(*pp);
            }
        }
    }
    return;
}

// Shift cells left
void shift_cells_left(int deltarows, int deltacols) {
    int r, j, c;
    register struct ent *p;
    register struct ent *n;
    struct ent **pp;
    for (j = 0; j < deltacols; j++)
        for (r = currow; r < currow + deltarows; r++)
            for (c = curcol; c < maxcol; c++) {

                // Free 'ent' memory
                pp = ATBL(tbl, r, c);
                clearent(*pp);

                p = *ATBL(tbl, r, c + 1);
                if (p && ( (p->flags & is_valid) || (p->expr && (p->flags & is_strexpr)) || p->label )  ) {
                    n = lookat(r, c);
                    (void) copyent(n, p, 0, 0, 0, 0, r, c, 0); // copy p a n
                    n->col--;
                    pp = ATBL(tbl, r, c + 1);
                } else { // When shifting the cells from the last column
                    pp = ATBL(tbl, r, c);
                }
                clearent(*pp);
            }
    return;
}

// Shift cells right
void shift_cells_right(int deltarows, int deltacols) {
    int r, j, c;
    register struct ent *p;
    register struct ent *n;
    struct ent **pp;

    for (j = 0; j < deltacols; j++)
    for (r = currow; r < currow + deltarows; r++)
        for (c = maxcol; c >= curcol; c--) {
            if ((p = *ATBL(tbl, r, c))) {
                n = lookat(r, c + 1);
                (void) copyent(n, p, 0, 0, 0, 0, r, c, 0);
                n->col++;
                pp = ATBL(tbl, r, c);
                clearent(*pp);
            }
        }
    return;
}

// Shift cells up
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
