#include <stdio.h>
#include <stdlib.h>
#include "shift.h"
#include "sc.h"
#include "vmtbl.h"   // for growtbl

// ESTA FUNCION DESPLAZA UN RANGO DE ENTS, un deltar y deltac.
// TODO: reescribir shift_range (sin hacer uso de copyent), manera tal que no dependa
// de shift_cells, shift_cells_up, shift_cells_down, shift_cells_left, shift_cells_right.
// borrar todas estas ultimas.
void shift_range(int delta_rows, int delta_cols, int tlrow, int tlcol, int brrow, int brcol) {

    //#include <ncurses.h>
    //extern WINDOW * input_win;
    //mvwprintw(input_win, 1, 0, "%d  %d %d %d ", tlrow, tlcol, brrow, brcol);
    //wclrtoeol(input_win); wrefresh(input_win);
    
    currow = tlrow;
    curcol = tlcol;

    if (delta_rows > 0)      shift_cells_down(brrow - tlrow + 1, brcol - tlcol + 1);
    else if (delta_rows < 0) shift_cells_up(brrow - tlrow + 1, brcol - tlcol + 1);

    if (delta_cols > 0)      shift_cells_right(brrow - tlrow + 1, brcol - tlcol + 1);
    else if (delta_cols < 0) shift_cells_left(brrow - tlrow + 1, brcol - tlcol + 1);

    return;
}

/* Shift cells up / down
 * type 8: Shift cells up
 * type 2: Shift cells down
 * type 4: Shift cells left
 * type 6: Shift cells right
 * deltarows: number of rows to shift
 * deltacols: number of cols to shift
 */
void shift_cells(int type, int deltarows, int deltacols) {
    if (type == 8)
        shift_cells_up(deltarows, deltacols);
    else if (type == 2)
        shift_cells_down(deltarows, deltacols);
    else if (type == 4)
        shift_cells_left(deltarows, deltacols);
    else if (type == 6)
        shift_cells_right(deltarows, deltacols);

    modflg++;
    return;
}

// Shift cells down 
void shift_cells_down(int deltarows, int deltacols) {
    int r, c, i;
    struct ent **pp;
    int lim = maxrow - currow + 1;

    // Move cell content
    if (currow > maxrow) maxrow = currow;
    maxrow += deltarows; 

    lim = maxrow - lim;
    if ((maxrow >= maxrows) && !growtbl(GROWROW, maxrow, 0))
        return;

    for (r = maxrow; r > lim; r--) {
        for (c = curcol; c < curcol + deltacols; c++) {
            register struct ent *p = *ATBL(tbl, r - deltarows, c);
            if (p) {
                register struct ent *n;
                n = lookat(r, c);
                (void) copyent(n, p, 1, 0, 0, 0, r - deltarows, c, 0);
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
    struct ent *p;
    struct ent **pp;
    for (j = 0; j < deltacols; j++)
        for (r = currow; r < currow + deltarows; r++)
            for (c = curcol; c < maxcol; c++) {

                // libero memoria de ent donde estoy parado
                pp = ATBL(tbl, r, c);
                clearent(*pp);

                p = *ATBL(tbl, r, c + 1);
                if (p && ( (p->flags & is_valid) || (p->expr && (p->flags & is_strexpr)) || p->label )  ) {
                    register struct ent *n;
                    n = lookat(r, c);
                    (void) copyent(n, p, 1, 0, 0, 0, r, c, 0); // copio p a n
                    n->col--;
                    pp = ATBL(tbl, r, c + 1);
                } else { // Si desplazo celda de ultima columna
                    pp = ATBL(tbl, r, c);
                }
                clearent(*pp);
            }
    return;
}

// Shift cells right
void shift_cells_right(int deltarows, int deltacols) {
    int r, j, c;
    struct ent *p;
    struct ent **pp;
    for (j = 0; j < deltacols; j++)
    for (r = currow; r < currow + deltarows; r++)
        for (c = maxcol; c >= curcol; c--) {
            register struct ent *p = *ATBL(tbl, r, c);
            if (p) {
                register struct ent *n;
                n = lookat(r, c + 1);
                (void) copyent(n, p, 1, 0, 0, 0, r, c, 0);
                n->col++;
                pp = ATBL(tbl, r, c);
                clearent(*pp);
            }
        }
    return;
}

// Shift cells up 
void shift_cells_up(int deltarows, int deltacols) {
    register struct ent **pp;
    int r, c, i;

    //if (currow + deltarows - 1 > maxrow) return;

    // Delete cell content
    for (i = 0; i < deltarows; i++)
        for (r = currow; r < maxrow; r++) 
            for (c = curcol; c < curcol + deltacols; c++) {

                // libero memoria de ent donde estoy parado
                pp = ATBL(tbl, r, c);
                clearent(*pp);

                register struct ent *p = *ATBL(tbl, r+1, c);
                if (p && ( (p->flags & is_valid) || (p->expr && (p->flags & is_strexpr)) || p->label )  ) {
                    // Copio celda inferior hacia arriba
                    register struct ent *n;
                    n = lookat(r, c);
                    (void) copyent(n, p, 1, 0, 0, 0, r, c, 0);
                    n->row--;
                    pp = ATBL(tbl, r+1, c);
                } else {
                    pp = ATBL(tbl, r, c);
                }
                clearent(*pp);
            }
    return;
}
