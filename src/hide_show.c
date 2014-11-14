#include "sc.h"
extern int cmd_multiplier;

/* mark a row as hidden */
void hide_row(int from_row, int arg) {
    register int r2;

    r2 = from_row + arg - 1;
    if (from_row < 0 || from_row > r2) {
        error("Cannot hide row: Invalid range.");
        return;
    }
    if (r2 >= maxrows - 1) {
        if (! growtbl(GROWROW, arg + 1, 0)) {
            error("You can't hide the last row");
            return;
        }
    }
    modflg++;

    // zap here tantas veces como dígito de efecto multiplicador.
    while ( from_row <= r2)
        row_hidden[ from_row++ ] = 1;

}

/* mark a column as hidden */
void hide_col(int from_col, int arg) {
    int c2 = from_col + arg - 1;
    if (from_col < 0 || from_col > c2) {
        error ("Cannot hide col: Invalid range.");
        return;
    }
    if (c2 >= maxcols - 1) {
        if ((arg >= ABSMAXCOLS - 1) || ! growtbl(GROWCOL, 0, arg + 1)) {
            error("You can't hide the last col");
            return;
        }
    }
    modflg++;
    while (from_col <= c2)
        col_hidden[from_col ++] = TRUE;
}

/* mark a row as not-hidden */
void show_row(int from_row, int arg) {
    int r2 = from_row + arg - 1;
    if (from_row < 0 || from_row > r2) {
        error ("Cannot show row: Invalid range.");
        return;
    }
    if (r2 > maxrows - 1) {
        r2 = maxrows - 1;
    }
    modflg++;
    while (from_row <= r2)
        row_hidden[from_row++] = 0;
}

/* mark a column as not-hidden */
void show_col(int from_col, int arg) {
    int c2 = from_col + arg - 1;
    if (from_col < 0 || from_col > c2) {
        error ("Cannot show col: Invalid range.");
        return;
    }
    if (c2 > maxcols - 1) {
        c2 = maxcols - 1;
    }
    modflg++;
    while (from_col <= c2)
        col_hidden[ from_col++ ] = FALSE;
}
