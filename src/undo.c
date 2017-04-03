#ifdef UNDO
/*
----------------------------------------------------------------------------------------
UNDO and REDO features works with an 'undo' struct list.
Which contains:
      p_ant: pointer to 'undo' struct. If NULL, this node is the first change
             for the session.
      struct ent * added: 'ent' elements added by the change
      struct ent * removed: 'ent' elements removed by the change
      struct ent * aux_ents: 'ent' elements that needs to be around, but out of tbl.
                              these are used to update formulas correctly, upon shift/deletion of ents.
      struct undo_range_shift * range_shift: range shifted by change
      row_hidded: integers list (int *) hidden rows on screen
      row_showed: integers list (int *) visible rows on screen
      col_hidded: integers list (int *) hidden columns on screen
      col_showed: integers list (int *) visible columns on screen
             NOTE: the first position of the lists contains (number of elements - 1) in the list
      struct undo_cols_format * cols_format: list of 'undo_col_info' elements used for
             undoing / redoing changes in columns format (fwidth, precision y realfmt)
      p_sig: pointer to 'undo' struct, If NULL, this node is the last change in
             the session.

Follows one level UNDO/REDO scheme. A change (C1) is made, then an UNDO operation, and
another change (C2). From there later changes are removed.
Scheme:

+ C1 -> + -> UNDO -
^                 \
|_                |
  \---------------/
|
|
|
 \-> C2 --> + ...

undo_shift_range struct contains:
    int delta_rows: delta rows for the range shift
    int delta_cols: delta columns for the range shift
    int tlrow:      Upper left row defining the range of the shift
                    (As if a cell range shift is made)
    int tlcol:      Upper left column defining the range of the shift
                    (As if a cell range shift is made)
    int brrow:      Lower right row defining the range of the shift
                    (As if a cell range shift is made)
    int brcol:      Lower right column defining the range of the shift
                    (As if a cell range shift is made)

Implemented actions for UNDO/REDO:
1. Remove content from cell or range
2. Input content into a cell
3. Edit a cell
4. Change alginment of range or cell
5. Paste range or cell
6. Shift range or cell with sh, sj, sk, sl
7. Insert row or column
8. Delete row or column
9. Paste row or column
10. Hide/show rows and columns
11. Sort of a range
12. Change in the format of a range or cell
13. '-' and '+' commands in normal mode
14. Lock and unlock of cells
15. datefmt command
16. Change in format of a column as a result of the 'f' command
17. Change in format of a column as a result of auto_jus
18. Change format of columns as a result of ic dc
19. fill command
20. unformat

NOT implemented:
1. undo of freeze / unfreeze command

----------------------------------------------------------------------------------------
*/

#include <stdlib.h>
#include "undo.h"
#include "macros.h"
#include "curses.h"
#include "conf.h"
#include "sc.h"
#include "cmds.h"
#include "color.h" // for set_ucolor
#include "marks.h"
#include "shift.h"
#include "dep_graph.h"

// undolist
static struct undo * undo_list = NULL;

// current position in the list
static int undo_list_pos = 0;

// Number of elements in the list
static int undo_list_len = 0;

// Temporal variable
static struct undo undo_item;

// Init 'unto_item'
void create_undo_action() {
    undo_item.added       = NULL;
    undo_item.removed     = NULL;
    undo_item.p_ant       = NULL;
    undo_item.p_sig       = NULL;
    undo_item.range_shift = NULL;
    undo_item.cols_format = NULL;
    undo_item.aux_ents    = NULL;

    undo_item.row_hidded  = NULL;
    undo_item.row_showed  = NULL;
    undo_item.col_hidded  = NULL;
    undo_item.col_showed  = NULL;
    return;
}

// Save undo_item copy with 'ent' elements modified, and the undo range shift
// struct into the undolist
void end_undo_action() {
    add_to_undolist(undo_item);

    // in case we need to dismissed this undo_item!
    if ((undo_item.added      == NULL && undo_item.aux_ents == NULL &&
        undo_item.removed     == NULL && undo_item.range_shift == NULL &&
        undo_item.row_hidded  == NULL && undo_item.row_showed  == NULL &&
        undo_item.cols_format == NULL &&
        undo_item.col_hidded  == NULL && undo_item.col_showed  == NULL) || loading) {
        if (undo_list->p_ant != NULL) undo_list = undo_list->p_ant;
        undo_list_pos--;
        clear_from_current_pos();
    }

    return;
}

// Add a undo node to the undolist,
// allocate memory for undo struct,
// fill variable with undo_item value and append it to the list
void add_to_undolist(struct undo u) {
    // If not at the end of the list, remove from the end
    if ( undo_list != NULL && undo_list_pos != len_undo_list() )
        clear_from_current_pos();

    struct undo * ul = (struct undo *) malloc (sizeof(struct undo));
    ul->p_sig = NULL;

    // Add 'ent' elements
    ul->added = u.added;
    ul->removed = u.removed;
    ul->aux_ents = u.aux_ents;
    ul->range_shift = u.range_shift;
    ul->cols_format = u.cols_format;
    ul->row_hidded = u.row_hidded;
    ul->col_hidded = u.col_hidded;
    ul->row_showed = u.row_showed;
    ul->col_showed = u.col_showed;

    if (undo_list == NULL) {
        ul->p_ant = NULL;
        undo_list = ul;
    } else {
        ul->p_ant = undo_list;

        // go to end of list
        while (undo_list->p_sig != NULL) undo_list = undo_list->p_sig;

        undo_list->p_sig = ul;
        undo_list = undo_list->p_sig;
    }
    undo_list_pos++;
    undo_list_len++;
    return;
}

// dismiss current undo_item
// this functions free memory of and struct undo.
// its used in function free_undo_node for that purpose.
//
// but as well, this function shall be called instead of end_undo_action
// in case we want to cancel a previous create_undo_action
// if called for this purpose argument shall be NULL
void dismiss_undo_item(struct undo * ul) {
    struct ent * en;
    struct ent * de;

    if (ul == NULL) ul = &undo_item;

    en = ul->added;     // free added
    while (en != NULL) {
        de = en->next;
        clearent(en);
        free(en);
        en = de;
    }
    en = ul->removed;   // free removed
    while (en != NULL) {
        de = en->next;
        clearent(en);
        free(en);
        en = de;
    }
    en = ul->aux_ents; // free aux_ents
    while (en != NULL) {
        de = en->next;
        clearent(en);
        free(en);
        en = de;
    }
    if (ul->range_shift != NULL) free(ul->range_shift); // Free undo_range_shift memory
    if (ul->cols_format != NULL) {                      // Free cols_format memory
        free(ul->cols_format->cols);
        free(ul->cols_format);
    }
    if (ul->row_hidded  != NULL) free(ul->row_hidded); // Free hidden row memory
    if (ul->col_hidded  != NULL) free(ul->col_hidded); // Free hidden col memory
    if (ul->row_showed  != NULL) free(ul->row_showed); // Free showed row memory
    if (ul->col_showed  != NULL) free(ul->col_showed); // Free showed col memory

    return;
}

// Cascade free UNDO node memory
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

// Remove nodes below the current position from the undolist
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

// Remove undolist content
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

int len_undo_list() {
    return undo_list_len;
}

// Take a range of 'ent' elements and create new ones (as many as elements
// inside the specified range).
// Then copy the content of the original ones to the new ones and save them into
// the 'added' or 'removed' list of undo_item, according to the char type.
void copy_to_undostruct (int row_desde, int col_desde, int row_hasta, int col_hasta, char type) {
    int c, r;
    struct ent * p;
    int repeated;

    for (r = row_desde; r <= row_hasta; r++)
        for (c = col_desde; c <= col_hasta; c++) {
            p = *ATBL(tbl, r, c);
            if (p == NULL) continue;

            // here check that ent to add is not already in the list
            // if so, avoid to add a duplicate ent
            struct ent * lista = type == 'a' ? undo_item.added : undo_item.removed;
            repeated = 0;
            while (lista != NULL) {
                if (lista->row == r && lista->col == c) {
                    repeated = 1;
                    break;
                }
                lista = lista->next;
            }
            if (repeated) continue;

            // not repeated - we malloc an ent and add it to list
            struct ent * e = (struct ent *) malloc( (unsigned) sizeof(struct ent) );
            cleanent(e);
            //copyent(e, lookat(r, c), 0, 0, 0, 0, 0, 0, 0);
            copyent(e, lookat(r, c), 0, 0, 0, 0, 0, 0, 'u');

            // Append 'ent' element at the beginning
            if (type == 'a') {
                e->next = undo_item.added;
                undo_item.added = e;
            } else {
                e->next = undo_item.removed;
                undo_item.removed = e;
            }

        }
    return;
}

// this is used to keep aux ents in the undo struct
// used for undoing dr dc sk and sh commands..
// it stores a copy of the ent received as parameter and returns the point to that copy
struct ent * add_undo_aux_ent(struct ent * e) {
    struct ent * e_new = (struct ent *) malloc( (unsigned) sizeof(struct ent) );
    cleanent(e_new);
    copyent(e_new, e, 0, 0, 0, 0, 0, 0, 'u'); // copy ent with special='u' (undo)
    e_new->next = undo_item.aux_ents;         // add e_new to beginning of list
    undo_item.aux_ents = e_new;
    return e_new;
}

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

// Takes a range, a rows and columns delta and save them in the undo struct
// Used to shift ranges when UNDO or REDO without duplicating 'ent' elements
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

// This function is used for undoing and redoing
// changes caused by commands that hide/show rows/columns of screen
// such as Zr Zc Sc Sr commands.
// it stores in four different lists (int * list) the row or columns numbers
// that are showed or hidden because of a change.
// As these lists are dynamically built, in the first position of every list,
// we always store the number of elements that the list has.
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

// Do UNDO operation
// Shift a range of an undo shift range to the original position, if any, append
// 'ent' elements from 'removed' and remove those from 'added'
void do_undo() {
    if (undo_list == NULL || undo_list_pos == 0) {
        sc_error("No UNDO's left");
        return;
    }
    //sc_info("%d %d", undo_list_pos, len_undo_list());

    int ori_currow = currow;
    int ori_curcol = curcol;
    int mf = modflg; // save modflag status

    struct undo * ul = undo_list;


    struct ent * i = ul->added;
    while (i != NULL) {
        struct ent * pp = *ATBL(tbl, i->row, i->col);
        clearent(pp);
        cleanent(pp);
        i = i->next;
    }

    // Make undo shift, if any
    if (ul->range_shift != NULL) {
        // fix marks
        if (ul->range_shift->delta_rows > 0)      // sj
            fix_marks(-(ul->range_shift->brrow - ul->range_shift->tlrow + 1), 0, ul->range_shift->tlrow, maxrow, ul->range_shift->tlcol, ul->range_shift->brcol);
        else if (ul->range_shift->delta_rows < 0) // sk
            fix_marks( (ul->range_shift->brrow - ul->range_shift->tlrow + 1), 0, ul->range_shift->tlrow, maxrow, ul->range_shift->tlcol, ul->range_shift->brcol);
        if (ul->range_shift->delta_cols > 0)      // sl
            fix_marks(0, -(ul->range_shift->brcol - ul->range_shift->tlcol + 1), ul->range_shift->tlrow, ul->range_shift->brrow, ul->range_shift->tlcol, maxcol);
        else if (ul->range_shift->delta_cols < 0) // sh
            fix_marks(0,  (ul->range_shift->brcol - ul->range_shift->tlcol + 1), ul->range_shift->tlrow, ul->range_shift->brrow, ul->range_shift->tlcol, maxcol);

        shift_range(- ul->range_shift->delta_rows, - ul->range_shift->delta_cols,
            ul->range_shift->tlrow, ul->range_shift->tlcol, ul->range_shift->brrow, ul->range_shift->brcol);

        // shift col_formats here
        if (ul->range_shift->tlcol >= 0 && ul->range_shift->tlrow == 0 && ul->range_shift->brrow == maxrow) { // && ul->range_shift->delta_cols > 0) {
            int i;
            if (ul->range_shift->delta_cols > 0)
            for (i = ul->range_shift->brcol + ul->range_shift->delta_cols; i <= maxcol; i++) {
                fwidth[i - ul->range_shift->delta_cols] = fwidth[i];
                precision[i - ul->range_shift->delta_cols] = precision[i];
                realfmt[i - ul->range_shift->delta_cols] = realfmt[i];
            }
            else
            for (i = maxcol; i >= ul->range_shift->tlcol - ul->range_shift->delta_cols; i--) {
                 fwidth[i] = fwidth[i + ul->range_shift->delta_cols];
                 precision[i] = precision[i + ul->range_shift->delta_cols];
                 realfmt[i] = realfmt[i + ul->range_shift->delta_cols];
            }
        }
    }

    //update(TRUE); //FIXME remove this line. its just to help debugging

    // Change cursor position
    //if (ul->removed != NULL) {
    //    currow = ul->removed->row;
    //    curcol = ul->removed->col;
    //}

    // Append 'ent' elements from the removed ones
    struct ent * j = ul->removed;
    while (j != NULL) {
        struct ent * h;
        if ((h = *ATBL(tbl, j->row, j->col))) clearent(h);
        struct ent * e_now = lookat(j->row, j->col);
        (void) copyent(e_now, j, 0, 0, 0, 0, 0, 0, 0);
        j = j->next;
    }

    // Show hidden cols and rows
    // Hide visible cols and rows
    if (ul->col_hidded != NULL) {
        int * pd = ul->col_hidded;
        int left = *(pd++);
        while (left--) {
            col_hidden[*(pd++)] = FALSE;
        }
    }
    else if (ul->col_showed  != NULL) {
        int * pd = ul->col_showed;
        int left = *(pd++);
        while (left--) {
            col_hidden[*(pd++)] = TRUE;
        }
    }
    else if (ul->row_hidded  != NULL) {
        int * pd = ul->row_hidded;
        int left = *(pd++);
        while (left--) {
            row_hidden[*(pd++)] = FALSE;
        }
    }
    else if (ul->row_showed  != NULL) {
        int * pd = ul->row_showed;
        int left = *(pd++);
        while (left--) {
            row_hidden[*(pd++)] = TRUE;
        }
    }

    // Restore previous col format
    if (ul->cols_format != NULL) {
        struct undo_cols_format * uf = ul->cols_format;
        int size = uf->length;
        int i;

        for (i=0; i < size; i++) {
           if (uf->cols[i].type == 'R') {
               fwidth[uf->cols[i].col]    = uf->cols[i].fwidth;
               precision[uf->cols[i].col] = uf->cols[i].precision;
               realfmt[uf->cols[i].col]   = uf->cols[i].realfmt;
           }
        }
    }

    // for every ent in added and removed, we reeval expression to update graph
    struct ent * ie = ul->added;
    while (ie != NULL) {
        struct ent * p;
        if ((p = *ATBL(tbl, ie->row, ie->col)) && p->expr)
            EvalJustOneVertex(p, ie->row, ie->col, 1);
        ie = ie->next;
    }
    ie = ul->removed;
    while (ie != NULL) {
        struct ent * p;
        if ((p = *ATBL(tbl, ie->row, ie->col)) && p->expr)
            EvalJustOneVertex(p, ie->row, ie->col, 1);
        ie = ie->next;
    }

    // Restores cursor position
    currow = ori_currow;
    curcol = ori_curcol;

    // decrease modflg
    modflg= mf - 1;

    if (undo_list->p_ant != NULL) undo_list = undo_list->p_ant;
    undo_list_pos--;
    sc_info("Change: %d of %d", undo_list_pos, len_undo_list());
    return;
}

// Do REDO
// Shift a range of an undo shift range to the original position, if any, append
// 'ent' elements from 'added' and remove those from 'removed'
void do_redo() {
    //if ( undo_list == NULL || undo_list_pos == len_undo_list()  ) {
    //FIXME check why undo_list_pos can sometimes be > len_undo_list(). it shouldnt!!
    if ( undo_list == NULL || undo_list_pos >= len_undo_list()  ) {
        sc_error("No REDO's left");
        return;
    }
    //sc_info("%d %d", undo_list_pos, len_undo_list());

    int ori_currow = currow;
    int ori_curcol = curcol;
    int mf = modflg; // save modflag status

    if (undo_list->p_ant == NULL && undo_list_pos == 0);
    else if (undo_list->p_sig != NULL) undo_list = undo_list->p_sig;

    struct undo * ul = undo_list;

    // Remove 'ent' elements
    struct ent * i = ul->removed;
    while (i != NULL) {
        struct ent * pp = *ATBL(tbl, i->row, i->col);
        clearent(pp);
        cleanent(pp);
        i = i->next;
    }

    // Make undo shift, if any
    if (ul->range_shift != NULL) {
        // fix marks
        if (ul->range_shift->delta_rows > 0)      // sj
            fix_marks( (ul->range_shift->brrow - ul->range_shift->tlrow + 1), 0, ul->range_shift->tlrow, maxrow, ul->range_shift->tlcol, ul->range_shift->brcol);
        else if (ul->range_shift->delta_rows < 0) // sk
            fix_marks(-(ul->range_shift->brrow - ul->range_shift->tlrow + 1), 0, ul->range_shift->tlrow, maxrow, ul->range_shift->tlcol, ul->range_shift->brcol);
        if (ul->range_shift->delta_cols > 0)      // sl
            fix_marks(0,  (ul->range_shift->brcol - ul->range_shift->tlcol + 1), ul->range_shift->tlrow, ul->range_shift->brrow, ul->range_shift->tlcol, maxcol);
        else if (ul->range_shift->delta_cols < 0) // sh
            fix_marks(0, -(ul->range_shift->brcol - ul->range_shift->tlcol + 1), ul->range_shift->tlrow, ul->range_shift->brrow, ul->range_shift->tlcol, maxcol);

        shift_range(ul->range_shift->delta_rows, ul->range_shift->delta_cols,
            ul->range_shift->tlrow, ul->range_shift->tlcol, ul->range_shift->brrow, ul->range_shift->brcol);

        // shift col_formats here
        if (ul->range_shift->tlcol >= 0 && ul->range_shift->tlrow == 0 && ul->range_shift->brrow == maxrow) {
            int i;
            if (ul->range_shift->delta_cols > 0)
            for (i = maxcol; i >= ul->range_shift->tlcol + ul->range_shift->delta_cols; i--) {
                fwidth[i] = fwidth[i - ul->range_shift->delta_cols];
                precision[i] = precision[i - ul->range_shift->delta_cols];
                realfmt[i] = realfmt[i - ul->range_shift->delta_cols];
            }
            else
            for (i = ul->range_shift->tlcol; i - ul->range_shift->delta_cols <= maxcol; i++) {
                fwidth[i] = fwidth[i - ul->range_shift->delta_cols];
                precision[i] = precision[i - ul->range_shift->delta_cols];
                realfmt[i] = realfmt[i - ul->range_shift->delta_cols];
            }
        }
    }

    //update(TRUE);

    // Change cursor position
    //if (ul->p_sig != NULL && ul->p_sig->removed != NULL) {
    //    currow = ul->p_sig->removed->row;
    //    curcol = ul->p_sig->removed->col;
    //}

    // Append 'ent' elements
    struct ent * j = ul->added;
    while (j != NULL) {
        struct ent * h;
        if ((h = *ATBL(tbl, j->row, j->col))) clearent(h);
        struct ent * e_now = lookat(j->row, j->col);
        (void) copyent(e_now, j, 0, 0, 0, 0, 0, 0, 0);
        j = j->next;
    }

    // Hide previously hidden cols and rows
    // Show previously visible cols and rows
    if (ul->col_hidded != NULL) {
        int * pd = ul->col_hidded;
        int left = *(pd++);
        while (left--) {
            col_hidden[*(pd++)] = TRUE;
        }
    }
    else if (ul->col_showed  != NULL) {
        int * pd = ul->col_showed;
        int left = *(pd++);
        while (left--) {
            col_hidden[*(pd++)] = FALSE;
        }
    }
    else if (ul->row_hidded  != NULL) {
        int * pd = ul->row_hidded;
        int left = *(pd++);
        while (left--) {
            row_hidden[*(pd++)] = TRUE;
        }
    }
    else if (ul->row_showed  != NULL) {
        int * pd = ul->row_showed;
        int left = *(pd++);
        while (left--) {
            row_hidden[*(pd++)] = FALSE;
        }
    }

    // Restore new col format
    if (ul->cols_format != NULL) {
        struct undo_cols_format * uf = ul->cols_format;
        int size = uf->length;
        int i;

        for (i=0; i < size; i++) {
            if (uf->cols[i].type == 'A') {
                fwidth[uf->cols[i].col]    = uf->cols[i].fwidth;
                precision[uf->cols[i].col] = uf->cols[i].precision;
                realfmt[uf->cols[i].col]   = uf->cols[i].realfmt;
            }
        }
    }

    // for every ent in added and removed, we reeval expression to update graph
    struct ent * ie = ul->added;
    while (ie != NULL) {
        struct ent * p;
        if ((p = *ATBL(tbl, ie->row, ie->col)) && p->expr)
            EvalJustOneVertex(p, ie->row, ie->col, 1);
        ie = ie->next;
    }
    ie = ul->removed;
    while (ie != NULL) {
        struct ent * p;
        if ((p = *ATBL(tbl, ie->row, ie->col)) && p->expr)
            EvalJustOneVertex(p, ie->row, ie->col, 1);
        ie = ie->next;
    }

    // Restores cursor position
    currow = ori_currow;
    curcol = ori_curcol;

    // increase modflg
    modflg = mf + 1;

    sc_info("Change: %d of %d", undo_list_pos + 1, len_undo_list());
    undo_list_pos++;

    return;
}
#endif
