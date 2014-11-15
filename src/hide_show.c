#include "sc.h"
#include "macros.h"
#include "undo.h"
extern struct undo undo_item;

/* mark a row as hidden */
void hide_row(int from_row, int arg) {
    register int r2;

    r2 = from_row + arg - 1;
    if (from_row < 0 || from_row > r2) {
        error("Cannot hide row: Invalid range.");
        return;
    }
    if (r2 >= maxrows - 1) {
        // error: tried to hide a row higher than maxrow.
        lookat(from_row + arg + 1, curcol); //FIXME this HACK
        if (! growtbl(GROWROW, arg + 1, 0)) {
            error("You can't hide the last row");
            return;
        }
    }

    modflg++;
    create_undo_action();
    undo_hide_show(from_row, -1, 'h', arg);
    while ( from_row <= r2)
        row_hidden[ from_row++ ] = TRUE;
    end_undo_action();
    return;
}

/* mark a column as hidden */
void hide_col(int from_col, int arg) {
    int c2 = from_col + arg - 1;
    if (from_col < 0 || from_col > c2) {
        error ("Cannot hide col: Invalid range.");
        return;
    }
    if (c2 >= maxcols - 1) {
        // error: tried to hide a column higher than maxcol.
        lookat(currow, from_col + arg + 1); //FIXME this HACK
        if ((arg >= ABSMAXCOLS - 1) || ! growtbl(GROWCOL, 0, arg + 1)) {
            error("You can't hide the last col");
            return;
        }
    }

    modflg++;
    create_undo_action();
    undo_hide_show(-1, from_col, 'h', arg);
    while (from_col <= c2)
        col_hidden[ from_col++ ] = TRUE;
    end_undo_action();
    return;
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
    create_undo_action();
    undo_hide_show(from_row, -1, 's', arg);
    while (from_row <= r2)
        row_hidden[ from_row++ ] = FALSE;
    end_undo_action();
    return;
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
    create_undo_action();
    undo_hide_show(-1, from_col, 's', arg);
    while (from_col <= c2)
        col_hidden[ from_col++ ] = FALSE;
    end_undo_action();
    return;
}
