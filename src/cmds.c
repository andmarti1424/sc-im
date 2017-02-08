#include <ncurses.h>
#include <stdlib.h>
#include <ctype.h>   // for isdigit
#include <wchar.h>
#include <wctype.h>
#include "maps.h"
#include "yank.h"
#include "marks.h"
#include "cmds.h"
#include "buffer.h"
#include "screen.h"
#include "conf.h"    // for conf parameters
#include "color.h"   // for set_ucolor
#include "xmalloc.h" // for scxfree
#include "vmtbl.h"   // for growtbl
#include "utils/string.h" // for add_char
#include "y.tab.h"   // for yyparse
#include "dep_graph.h"
#ifdef UNDO
#include "undo.h"
#endif

void syncref(register struct enode *e);
extern unsigned int shall_quit;
char insert_edit_submode;
struct ent * freeents = NULL; // keep deleted ents around before sync_refs
wchar_t interp_line[BUFFERSIZE];
extern graphADT graph;
extern int yyparse(void);


// mark_ent_as_deleted
// This structure is used to keep ent structs around before they
// are actualy deleted (memory freed) to allow the sync_refs routine a chance to fix the
// variable references.
// if delete flag is set, is_deleted flag of an ent is set
void mark_ent_as_deleted(register struct ent * p, int delete) {
    if (p == NULL) return;
    if (delete) p->flags |= is_deleted;

    p->next = freeents;     /* put this ent on the front of freeents */
    freeents = p;

    return;
}

// flush_saved: iterates throw freeents (ents marked as deleted)
// calls clearent for freeing ents contents memory
// and free ent pointer. this function should always be called
// at exit. this is mandatory, just in case we want to UNDO any changes.
void flush_saved() {
    register struct ent * p;
    register struct ent * q;

    p = freeents;
    while (p != NULL) {
        (void) clearent(p);
        q = p->next;
        free(p);
        p = q;
    }
    freeents = NULL;
    return;
}

// sync_refs and syncref are used to REMOVE references to
// deleted struct ents.
// Note that the deleted structure must still
// be hanging around before the call, but not referenced
// by an entry in tbl.
// IMPROVE: Shouldn't traverse the whole table.
void sync_refs() {
    int i, j;
    register struct ent * p;
    // sync_ranges();
    for (i=0; i <= maxrow; i++)
    for (j=0; j <= maxcol; j++)
        if ( (p = *ATBL(tbl, i, j)) && p->expr ) {
               syncref(p->expr);
               //sc_info("%d %d %d", i, j, ++k);
            }
    return;
}

void syncref(register struct enode * e) {
    //if (e == (struct enode *)0) {
    //
    if ( e == NULL ) {
        return;
    } else if ( e->op == ERR_ ) {
        e->e.o.left = NULL;
        e->e.o.right = NULL;
        return;
    } else if ( e->op == REF_ ) {
        e->op = REF_;
        e->e.o.left = NULL;
        e->e.o.right = NULL;
        return;
    } else if (e->op & REDUCE) {
        e->e.r.right.vp = lookat(e->e.r.right.vp->row, e->e.r.right.vp->col);
        e->e.r.left.vp = lookat(e->e.r.left.vp->row, e->e.r.left.vp->col);
    } else {
        switch (e->op) {
        case 'v':
            if (e->e.v.vp->flags & is_deleted) {
                //e->op = ERR_;
                //e->e.o.left = NULL;
                //e->e.o.right = NULL;
                break;
            } else if (e->e.v.vp->flags & may_sync)
                e->e.v.vp = lookat(e->e.v.vp->row, e->e.v.vp->col);
            break;
        case 'k':
            break;
        case '$':
            break;
        default:
            syncref(e->e.o.right);
            syncref(e->e.o.left);
            break;
        }
    }
    return;
}

// Delete a column
void deletecol() {
    int r, c, i;
    struct ent ** pp;

    if (any_locked_cells(0, curcol, maxrow, curcol)) {
        sc_info("Locked cells encountered. Nothing changed");
        return;
    }

    // mark ent of column to erase with is_deleted flag
    for (r = 0; r <= maxrow; r++) {
        pp = ATBL(tbl, r, curcol);
        if ( *pp != NULL ) {
            mark_ent_as_deleted(*pp, TRUE);
            //clearent(*pp);
            //free(*pp);
            *pp = NULL;
        }
    }

    /*
    extern struct ent_ptr * deps;
    int n = 0;
    struct ent * p;
    //ents_that_depends_on_range(0, curcol, maxrow, curcol);
    if (deps != NULL) {
         n = deps->vf;
         for (i = 0; i < n; i++)
             if ((p = *ATBL(tbl, deps[i].vp->row, deps[i].vp->col)) && p->expr)
                 EvalJustOneVertex(p, deps[i].vp->row, deps[i].vp->col, 0);
    }*/
    EvalAll();

    // Copy references from right column cells to left column (which gets removed)
    for (r = 0; r <= maxrow; r++) {
        for (c = curcol; c < maxcol; c++) {
            pp = ATBL(tbl, r, c);

            // nota: pp[1] = ATBL(tbl, r, c+1);
            if ( pp[1] != NULL ) pp[1]->col--;
            pp[0] = pp[1];
        }

        // Free last column memory (Could also initialize 'ent' to zero with `cleanent`).
        pp = ATBL(tbl, r, maxcol);
        *pp = (struct ent *) 0;
    }

    // Fix columns precision and width
    for (i = curcol; i < maxcols - 2; i++) {
        fwidth[i] = fwidth[i+1];
        precision[i] = precision[i+1];
        realfmt[i] = realfmt[i+1];
        col_hidden[i] = col_hidden[i+1];
    }

    for (; i < maxcols - 1; i++) {
        fwidth[i] = DEFWIDTH;
        precision[i] = DEFPREC;
        realfmt[i] = DEFREFMT;
        col_hidden[i] = FALSE;
    }

    maxcol--;
    sync_refs();
    //flush_saved(); // we have to flush_saved only at exit.
    //this is because we have to keep ents in case we want to UNDO?
    modflg++;
    return;
}

// Copy a cell (struct ent).  "special" indicates special treatment when
// merging two cells for the "pm" command, merging formats only for the
// "pf" command, or for adjusting cell references when transposing with
// the "pt" command.  r1, c1, r2, and c2 define the range in which the dr
// and dc values should be used.
void copyent(register struct ent * n, register struct ent * p, int dr, int dc,
             int r1, int c1, int r2, int c2, int special) {
    if (!n || !p) {
        sc_error("internal error");
        return;
    }

    n->flags = may_sync;
    if (p->flags & is_deleted)
        n->flags |= is_deleted;

    if (special != 'f') {
        if (p->flags & is_valid) {
            n->v = p->v;
            n->flags |= p->flags & is_valid;
        }
        if (special != 'v' && p->expr) {
            n->expr = copye(p->expr, dr, dc, r1, c1, r2, c2, special == 't');
            if (p->flags & is_strexpr)
                n->flags |= is_strexpr;
            else
                n->flags &= ~is_strexpr;
        }
        if (p->label) {
            if (n->label) scxfree(n->label);
                n->label = scxmalloc((unsigned) (strlen(p->label) + 1));
            (void) strcpy(n->label, p->label);
            n->flags &= ~is_leftflush;
            n->flags |= ((p->flags & is_label) | (p->flags & is_leftflush));
        }
        n->flags |= p->flags & is_locked;
    }
    if (p->format && special != 'v') {
        if (n->format) scxfree(n->format);
            n->format = scxmalloc((unsigned) (strlen(p->format) + 1));
        (void) strcpy(n->format, p->format);
    } else if (special != 'v' && special != 'f')
        n->format = NULL;

    if (special != 'v') {
        n->pad = p->pad;
        if (n->ucolor) { // remove current cellcolor format in n ent
            free(n->ucolor);
            n->ucolor = NULL;
        }
        if (p->ucolor) { // copy new cellcolor format from p to n ent
            n->ucolor = (struct ucolor *) malloc (sizeof(struct ucolor));
            n->ucolor->fg = p->ucolor->fg;
            n->ucolor->bg = p->ucolor->bg;
            n->ucolor->bold = p->ucolor->bold;
            n->ucolor->dim = p->ucolor->dim;
            n->ucolor->reverse = p->ucolor->reverse;
            n->ucolor->standout = p->ucolor->standout;
            n->ucolor->underline = p->ucolor->underline;
            n->ucolor->blink = p->ucolor->blink;
        }
    }

    // used for undoing / redoing cells that has errors
    n->cellerror = p->cellerror;

    if (special == 'c' && n->expr)
        EvalJustOneVertex(n, n->row, n->col, 0);

    n->flags |= is_changed;
    n->row = p->row;
    n->col = p->col;
    return;
}

int etype(register struct enode *e) {
    if (e == (struct enode *)0)
        return NUM;
    switch (e->op) {
        case UPPER: case LOWER: case CAPITAL:
        case O_SCONST: case '#': case DATE: case FMT: case STINDEX:
        case EXT: case SVAL: case SUBSTR:
            return (STR);

        case '?':
        case IF:
            return (etype(e->e.o.right->e.o.left));

        case 'f':
            return (etype(e->e.o.right));

        case O_VAR: {
            register struct ent *p;
            p = e->e.v.vp;
            if (p->expr)
                return (p->flags & is_strexpr ? STR : NUM);
            else if (p->label)
                return (STR);
            else
                return (NUM);
        }

        default:
            return (NUM);
    }
    return -1;
}

// ignorelock is used when sorting so that locked cells can still be sorted
void erase_area(int sr, int sc, int er, int ec, int ignorelock, int mark_as_deleted) {
    int r, c;
    struct ent **pp;

    if (sr > er) {
        r = sr; sr = er; er = r;
    }

    if (sc > ec) {
        c = sc; sc = ec; ec = c;
    }

    if (sr < 0)
        sr = 0;
    if (sc < 0)
        sc = 0;
    checkbounds(&er, &ec);

    // mark the ent as deleted
    // Do a lookat() for the upper left and lower right cells of the range
    // being erased to make sure they are included in the delete buffer so
    // that pulling cells always works correctly even if the cells at one
    // or more edges of the range are all empty.
    (void) lookat(sr, sc);
    (void) lookat(er, ec);
    for (r = sr; r <= er; r++) {
    for (c = sc; c <= ec; c++) {
        pp = ATBL(tbl, r, c);
        if (*pp && (!((*pp)->flags & is_locked) || ignorelock)) {

            /* delete vertex in graph
               unless vertex is referenced by other */
            vertexT * v = getVertex(graph, *pp, 0);
            //if (v != NULL && v->back_edges == NULL )
            if (v != NULL ) destroy_vertex(*pp);

            if (mark_as_deleted) {
                mark_ent_as_deleted(*pp, TRUE);
            } else {
                clearent(*pp);// free memory
                cleanent(*pp);
                mark_ent_as_deleted(*pp, FALSE);
            }
            *pp = NULL;
        }
    }
    }
    return;
}

struct enode * copye(register struct enode *e, int Rdelta, int Cdelta, int r1, int c1, int r2, int c2, int transpose) {
    register struct enode * ret;
    static struct enode * range = NULL;

    if (e == (struct enode *) 0) {
        ret = (struct enode *) 0;

    } else if (e->op & REDUCE) {
        int newrow, newcol;
        //if (freeenodes) {
        //    ret = freeenodes;
        //    freeenodes = ret->e.o.left;
        //} else
        ret = (struct enode *) scxmalloc((unsigned) sizeof (struct enode));
        ret->op = e->op;
        newrow = e->e.r.left.vf & FIX_ROW ||
        e->e.r.left.vp->row < r1 || e->e.r.left.vp->row > r2 ||
        e->e.r.left.vp->col < c1 || e->e.r.left.vp->col > c2 ?
        e->e.r.left.vp->row :
        transpose ? r1 + Rdelta + e->e.r.left.vp->col - c1 :
        e->e.r.left.vp->row + Rdelta;
        newcol = e->e.r.left.vf & FIX_COL ||
        e->e.r.left.vp->row < r1 || e->e.r.left.vp->row > r2 ||
        e->e.r.left.vp->col < c1 || e->e.r.left.vp->col > c2 ?
        e->e.r.left.vp->col :
        transpose ? c1 + Cdelta + e->e.r.left.vp->row - r1 :
        e->e.r.left.vp->col + Cdelta;
        ret->e.r.left.vp = lookat(newrow, newcol);
        ret->e.r.left.vf = e->e.r.left.vf;
        newrow = e->e.r.right.vf & FIX_ROW ||
        e->e.r.right.vp->row < r1 || e->e.r.right.vp->row > r2 ||
        e->e.r.right.vp->col < c1 || e->e.r.right.vp->col > c2 ?
        e->e.r.right.vp->row :
        transpose ? r1 + Rdelta + e->e.r.right.vp->col - c1 :
        e->e.r.right.vp->row + Rdelta;
        newcol = e->e.r.right.vf & FIX_COL ||
        e->e.r.right.vp->row < r1 || e->e.r.right.vp->row > r2 ||
        e->e.r.right.vp->col < c1 || e->e.r.right.vp->col > c2 ?
        e->e.r.right.vp->col :
        transpose ? c1 + Cdelta + e->e.r.right.vp->row - r1 :
        e->e.r.right.vp->col + Cdelta;
        ret->e.r.right.vp = lookat(newrow, newcol);
        ret->e.r.right.vf = e->e.r.right.vf;
    } else {
        struct enode *temprange=0;
        //if (freeenodes) {
        //    ret = freeenodes;
        //    freeenodes = ret->e.o.left;
        //} else
        ret = (struct enode *) scxmalloc((unsigned) sizeof (struct enode));
        ret->op = e->op;
        switch (ret->op) {
            case SUM:
            case PROD:
            case AVG:
            case COUNT:
            case STDDEV:
            case MAX:
            case MIN:
                temprange = range;
                range = e->e.o.left;
                r1 = 0;
                c1 = 0;
                r2 = maxrow;
                c2 = maxcol;
        }
        switch (ret->op) {
            case 'v':
                {
                    int newrow, newcol;
                    if (range && e->e.v.vp->row >= range->e.r.left.vp->row &&
                        e->e.v.vp->row <= range->e.r.right.vp->row &&
                        e->e.v.vp->col >= range->e.r.left.vp->col &&
                        e->e.v.vp->col <= range->e.r.right.vp->col) {
                            newrow = range->e.r.left.vf & FIX_ROW ? e->e.v.vp->row : e->e.v.vp->row + Rdelta;
                            newcol = range->e.r.left.vf & FIX_COL ? e->e.v.vp->col : e->e.v.vp->col + Cdelta;
                    } else {
                        newrow = e->e.v.vf & FIX_ROW ||
                        e->e.v.vp->row < r1 || e->e.v.vp->row > r2 ||
                        e->e.v.vp->col < c1 || e->e.v.vp->col > c2 ?
                        e->e.v.vp->row : transpose ? r1 + Rdelta + e->e.v.vp->col - c1 :
                        e->e.v.vp->row + Rdelta;
                        newcol = e->e.v.vf & FIX_COL ||
                        e->e.v.vp->row < r1 || e->e.v.vp->row > r2 ||
                        e->e.v.vp->col < c1 || e->e.v.vp->col > c2 ?
                        e->e.v.vp->col : transpose ? c1 + Cdelta + e->e.v.vp->row - r1 :
                        e->e.v.vp->col + Cdelta;
                    }
                    ret->e.v.vp = lookat(newrow, newcol);
                    ret->e.v.vf = e->e.v.vf;
                    break;
                }
            case 'k':
                ret->e.k = e->e.k;
                break;
            case 'f':
            case 'F':
                if ((range && ret->op == 'F') || (!range && ret->op == 'f'))
                    Rdelta = Cdelta = 0;
                ret->e.o.left = copye(e->e.o.left, Rdelta, Cdelta, r1, c1, r2, c2, transpose);
                ret->e.o.right = (struct enode *)0;
                break;
            case '$':
            case EXT:
                ret->e.s = scxmalloc((unsigned) strlen(e->e.s)+1);
                (void) strcpy(ret->e.s, e->e.s);
                if (e->op == '$')    /* Drop through if ret->op is EXT */
                    break;
            default:
                if (e->op == ERR_) {
                    //ret->e.o.left = NULL;
                    //ret->e.o.right = NULL;
                    //ret->e.o.right = (struct enode *)0;
                    break; /* fix #108 */
                }
                ret->e.o.left = copye(e->e.o.left, Rdelta, Cdelta, r1, c1, r2, c2, transpose);
                ret->e.o.right = copye(e->e.o.right, Rdelta, Cdelta, r1, c1, r2, c2, transpose);
                break;
        }
        switch (ret->op) {
            case SUM:
            case PROD:
            case AVG:
            case COUNT:
            case STDDEV:
            case MAX:
            case MIN:
                range = temprange;
        }
    }
    return ret;
}

/* Modified 9/17/90 THA to handle more formats */
void doformat(int c1, int c2, int w, int p, int r) {
    register int i;
    int crows = 0;
    int ccols = c2;

    if (c1 >= maxcols && !growtbl(GROWCOL, 0, c1)) c1 = maxcols-1 ;
    if (c2 >= maxcols && !growtbl(GROWCOL, 0, c2)) c2 = maxcols-1 ;

    if (w == 0) {
        sc_info("Width too small - setting to 1");
        w = 1;
    }

    if (! atoi(get_conf_value("nocurses")) && w > COLS - rescol - 2) {
        sc_info("Width too large - Maximum = %d", COLS - rescol - 2);
        w = COLS - rescol - 2;
    }

    if (p > w) {
        sc_info("Precision too large");
        p = w;
    }

    checkbounds(&crows, &ccols);
    if (ccols < c2) {
        sc_error("Format statement failed to create implied column %d", c2);
        return;
    }

    for (i = c1; i <= c2; i++)
        fwidth[i] = w, precision[i] = p, realfmt[i] = r;

    modflg++;
    return;

}

void formatcol(int c) {
    int arg = 1;
    int i;

    switch (c) {
        case '<':
        case 'h':
        case OKEY_LEFT:
            for (i = curcol; i < curcol + arg; i++) {
                fwidth[i]--;
                if (fwidth[i] < 1)
                    fwidth[i] = 1;
            }
            modflg++;
            break;
        case '>':
        case 'l':
        case OKEY_RIGHT:
            for (i = curcol; i < curcol + arg; i++) {
                fwidth[i]++;
                if (fwidth[i] > COLS - rescol - 2)
                    fwidth[i] = COLS - rescol - 2;
            }
            modflg++;
            break;
        case '-':
        case 'j':
        case OKEY_DOWN:
            for (i = curcol; i < curcol + arg; i++) {
                precision[i]--;
                if (precision[i] < 0)
                    precision[i] = 0;
            }
            modflg++;
            break;
        case '+':
        case 'k':
        case OKEY_UP:
            for (i = curcol; i < curcol + arg; i++)
                precision[i]++;
            modflg++;
            break;
    }
    sc_info("Current format is %d %d %d", fwidth[curcol], precision[curcol], realfmt[curcol]);
    update(TRUE);
    return;
}

// Insert a single row.  It will be inserted before currow
// if after is 0; after if it is 1.
void insert_row(int after) {
    int    r, c;
    struct ent ** tmprow, ** pp, ** qq;
    struct ent * p;
    int lim = maxrow - currow + 1;

    if (currow > maxrow) maxrow = currow;
    maxrow++;
    lim = maxrow - lim + after;
    if (maxrow >= maxrows && ! growtbl(GROWROW, maxrow, 0)) return;

    tmprow = tbl[maxrow];
    for (r = maxrow; r > lim; r--) {
        row_hidden[r] = row_hidden[r-1];
        tbl[r] = tbl[r-1];
        for (c = 0, pp = ATBL(tbl, r, 0); c < maxcols; c++, pp++)
            if (*pp) (*pp)->row = r;
    }
    tbl[r] = tmprow;        // the last row is never used

    // if padding exists in the old currow, we copy it to the new row!
    for (c = 0; c < maxcols; c++) {
        if (r >= 0 && (qq = ATBL(tbl, r+1, c)) && (*qq) && (*qq)->pad) {
            p = lookat(r, c);
            p->pad = (*qq)->pad;
        }
    }

    modflg++;
    return;
}

// Insert a single col. The col will be inserted
// BEFORE CURCOL if after is 0;
// AFTER  CURCOL if it is 1.
void insert_col(int after) {
    int r, c;
    register struct ent ** pp, ** qq;
    struct ent * p;
    int lim = maxcol - curcol - after + 1;

    if (curcol + after > maxcol)
        maxcol = curcol + after;
    maxcol++;

    if ((maxcol >= maxcols) && !growtbl(GROWCOL, 0, maxcol))
        return;

    for (c = maxcol; c >= curcol + after + 1; c--) {
        fwidth[c] = fwidth[c-1];
        precision[c] = precision[c-1];
        realfmt[c] = realfmt[c-1];
        col_hidden[c] = col_hidden[c-1];
    }
    for (c = curcol + after; c - curcol - after < 1; c++) {
        fwidth[c] = DEFWIDTH;
        precision[c] =  DEFPREC;
        realfmt[c] = DEFREFMT;
        col_hidden[c] = FALSE;
    }

    for (r=0; r <= maxrow; r++) {
        pp = ATBL(tbl, r, maxcol);
        for (c = lim; --c >= 0; pp--)
            if ((pp[0] = pp[-1])) pp[0]->col++;

        pp = ATBL(tbl, r, curcol + after);
        for (c = curcol + after; c - curcol - after < 1; c++, pp++)
            *pp = (struct ent *) 0;
    }

    // if padding exists in the old curcol, we copy it to the new col!
    for (r = 0; r < maxrows; r++) {
        if (c >= 0 && (qq = ATBL(tbl, r, c+1)) && (*qq) && (*qq)->pad) {
            p = lookat(r, c);
            p->pad = (*qq)->pad;
        }
    }

    curcol += after;
    modflg++;
    return;
}

// delete a row
void deleterow() {
    register struct ent ** pp;
    int r, c;

    if (any_locked_cells(currow, 0, currow, maxcol)) {
        sc_info("Locked cells encountered. Nothing changed");

    } else {
#ifdef UNDO
        // here we save in undostruct, all the ents that depends on the deleted one (before change)
        extern struct ent_ptr * deps;
        int i, n = 0;
        ents_that_depends_on_range(currow, 0, currow, maxcol);
        if (deps != NULL) {
            n = deps->vf;
            for (i = 0; i < n; i++) {
                copy_to_undostruct(deps[i].vp->row, deps[i].vp->col, deps[i].vp->row, deps[i].vp->col, 'd');
            }
        }
#endif

        //flush_saved();
        erase_area(currow, 0, currow, maxcol, 0, 1);
        if (currow > maxrow) return;

        for (r = currow; r < maxrows - 1; r++) {
            for (c = 0; c < maxcols; c++) {
                if (r <= maxrow - 1) {
                    pp = ATBL(tbl, r, c);
                    pp[0] = *ATBL(tbl, r + 1, c);
                    if ( pp[0] ) pp[0]->row--;
                }
            }
        }

        maxrow--;
        sync_refs();
        //flush_saved(); // we have to flush only at exit. this is in case we want to UNDO
        modflg++;

#ifdef UNDO
        // here we save in undostruct, all the ents that depends on the deleted one (after the change)
        for (i = 0; i < n; i++)
            if (deps[i].vp->row >= currow)
                copy_to_undostruct(deps[i].vp->row+1, deps[i].vp->col, deps[i].vp->row+1, deps[i].vp->col, 'a');
            else
                copy_to_undostruct(deps[i].vp->row, deps[i].vp->col, deps[i].vp->row, deps[i].vp->col, 'a');

        if (deps != NULL) free(deps);
        deps = NULL;
#endif

    }
    return;
}

void ljustify(int sr, int sc, int er, int ec) {
    struct ent *p;
    int i, j;

    if (sr > er) {
        i = sr;
        sr = er;
        er = i;
    }
    if (sc > ec) {
        i = sc;
        sc = ec;
        ec = i;
    }
    for (i = sr; i <= er; i++) {
        for (j = sc; j <= ec; j++) {
            p = *ATBL(tbl, i, j);
            if (p && p->label) {
                p->flags &= ~is_label;
                p->flags |= is_leftflush | is_changed;
                changed++;
                modflg++;
            }
        }
    }
    return;
}

void rjustify(int sr, int sc, int er, int ec) {
    struct ent *p;
    int i, j;

    if (sr > er) {
        i = sr;
        sr = er;
        er = i;
    }
    if (sc > ec) {
        i = sc;
        sc = ec;
        ec = i;
    }
    for (i = sr; i <= er; i++) {
        for (j = sc; j <= ec; j++) {
            p = *ATBL(tbl, i, j);
            if (p && p->label) {
                p->flags &= ~(is_label | is_leftflush);
                p->flags |= is_changed;
                changed++;
                modflg++;
            }
        }
    }
    return;
}

void center(int sr, int sc, int er, int ec) {
    struct ent *p;
    int i, j;

    if (sr > er) {
        i = sr;
        sr = er;
        er = i;
    }
    if (sc > ec) {
        i = sc;
        sc = ec;
        ec = i;
    }
    for (i = sr; i <= er; i++) {
        for (j = sc; j <= ec; j++) {
            p = *ATBL(tbl, i, j);
            if (p && p->label) {
                p->flags &= ~is_leftflush;
                p->flags |= is_label | is_changed;
                changed++;
                modflg++;
            }
        }
    }
    return;
}

void chg_mode(char strcmd){
    switch (strcmd) {
        case '=': 
            curmode = INSERT_MODE;
            break;
        case '<':
            curmode = INSERT_MODE;
            break;
        case '>':
            curmode = INSERT_MODE;
            break;
        case '\\':
            curmode = INSERT_MODE;
            break;
        case 'E':
            curmode = EDIT_MODE;
            break;
        case 'e':
            curmode = EDIT_MODE;
            break;
        case ':':
            curmode = COMMAND_MODE;
            break;
        case '.':
            curmode = NORMAL_MODE;
            break;
        case 'v':
            curmode = VISUAL_MODE;
            break;
    } 
    return;
}


// del selected cells
// can be a single cell or a range
void del_selected_cells() {
    int tlrow = currow;
    int tlcol = curcol;
    int brrow = currow;
    int brcol = curcol;

    // is we delete a range
    if (is_range_selected() != -1) {
        srange * r = get_selected_range();
        tlrow = r->tlrow;
        tlcol = r->tlcol;
        brrow = r->brrow;
        brcol = r->brcol;
    }

    if (any_locked_cells(tlrow, tlcol, brrow, brcol)) {
        sc_error("Locked cells encountered. Nothing changed");
        return;
    }

    if (is_range_selected() != -1)
        yank_area(tlrow, tlcol, brrow, brcol, 'a', 1);
    else
        yank_area(tlrow, tlcol, brrow, brcol, 'e', 1);

    #ifdef UNDO
    create_undo_action();
    copy_to_undostruct(tlrow, tlcol, brrow, brcol, 'd');

    // here we save in undostruct, all the ents that depends on the deleted one (before change)
    extern struct ent_ptr * deps;
    int i, n = 0;
    ents_that_depends_on_range(tlrow, tlcol, brrow, brcol);
    if (deps != NULL) {
        n = deps->vf;
        for (i = 0; i < n; i++)
            copy_to_undostruct(deps[i].vp->row, deps[i].vp->col, deps[i].vp->row, deps[i].vp->col, 'd');
    }
    #endif

    erase_area(tlrow, tlcol, brrow, brcol, 0, 0);
    modflg++;
    sync_refs();
    //flush_saved(); DO NOT UNCOMMENT! flush_saved shall not be called other than at exit.

    EvalAll();

    #ifdef UNDO
    // here we save in undostruct, all the ents that depends on the deleted one (after the change)
    for (i = 0; i < n; i++)
        copy_to_undostruct(deps[i].vp->row, deps[i].vp->col, deps[i].vp->row, deps[i].vp->col, 'a');

    if (deps != NULL) free(deps);
    deps = NULL;
    #endif


    #ifdef UNDO
    copy_to_undostruct(tlrow, tlcol, brrow, brcol, 'a');
    end_undo_action();
    #endif

    return;
}


// Change cell content sending inputline to interpreter
void insert_or_edit_cell() {
    char ope[BUFFERSIZE] = "";
    switch (insert_edit_submode) {
        case '=':
            strcpy(ope, "let");
            break;
        case '<':
            strcpy(ope, "leftstring");
            break;
        case '>':
            strcpy(ope, "rightstring");
            break;
        case '\\':
            strcpy(ope, "label");
            break;
    }
    if (inputline[0] == L'"') {
        del_wchar(inputline, 0);
    } else if (insert_edit_submode != '=' && inputline[0] != L'"') {
        add_wchar(inputline, L'\"', 0);
        add_wchar(inputline, L'\"', wcslen(inputline));
    }

    #ifdef UNDO
    create_undo_action();
    copy_to_undostruct(currow, curcol, currow, curcol, 'd');

    // here we save in undostruct, all the ents that depends on the deleted one (before change)
    extern struct ent_ptr * deps;
    int i, n = 0;
    ents_that_depends_on_range(currow, curcol, currow, curcol);
    if (deps != NULL) {
        n = deps->vf;
        for (i = 0; i < n; i++)
            copy_to_undostruct(deps[i].vp->row, deps[i].vp->col, deps[i].vp->row, deps[i].vp->col, 'd');
    }
    #endif

    if (getVertex(graph, lookat(currow, curcol), 0) != NULL) destroy_vertex(lookat(currow, curcol));

    // ADD PADDING INTELLIGENCE HERE ?
    (void) swprintf(interp_line, BUFFERSIZE, L"%s %s = %ls", ope, v_name(currow, curcol), inputline);

    send_to_interp(interp_line);

    #ifdef UNDO
    copy_to_undostruct(currow, curcol, currow, curcol, 'a');
    // here we save in undostruct, all the ents that depends on the deleted one (after change)
    if (deps != NULL) free(deps);
    ents_that_depends_on_range(currow, curcol, currow, curcol);
    if (deps != NULL) {
        n = deps->vf;
        for (i = 0; i < n; i++)
            copy_to_undostruct(deps[i].vp->row, deps[i].vp->col, deps[i].vp->row, deps[i].vp->col, 'a');
        free(deps);
        deps = NULL;
    }
    end_undo_action();
    #endif

    inputline[0] = L'\0';
    inputline_pos = 0;
    real_inputline_pos = 0;
    chg_mode('.');
    clr_header(input_win, 0);

    char * opt = get_conf_value("newline_action");
    switch (opt[0]) {
        case 'j':
            currow = forw_row(1)->row;
            break;
        case 'l':
            curcol = forw_col(1)->col;
            break;
    }
    update(TRUE);
    return;
}

// Send command to interpreter
// wide_char version
void send_to_interp(wchar_t * oper) {
    if (atoi(get_conf_value("nocurses"))) {
        int pos = -1;
        if ((pos = wstr_in_wstr(oper, L"\n")) != -1)
            oper[pos] = L'\0';
        sc_debug("Interp GOT: %ls", oper);
        //wprintf(L"Interp GOT: %ls", oper);
    }
    wcstombs(line, oper, BUFFERSIZE);

    linelim = 0;
    yyparse();
    if (atoi(get_conf_value("autocalc")) && ! loading) EvalAll();
    return;
}

// Send command to interpreter
void send_to_interpp(char * oper) {
    if (atoi(get_conf_value("nocurses"))) {
        sc_debug("Interp GOT: %s", oper);
    }
    strcpy(line, oper);
    linelim = 0;
    yyparse();
    if (atoi(get_conf_value("autocalc")) && ! loading) EvalAll();
    return;
}
/* return a pointer to a cell's [struct ent *], creating if needed */
struct ent * lookat(int row, int col) {
    register struct ent **pp;

    checkbounds(&row, &col);
    pp = ATBL(tbl, row, col);
    if ( *pp == NULL) {
        //if (freeents == NULL) {
            //*pp = (struct ent *) scxmalloc( (unsigned) sizeof(struct ent) );
            *pp = (struct ent *) malloc( (unsigned) sizeof(struct ent) );
        //} else {
        //    sc_debug("lookat. reuse of deleted ent row:%d col:%d", row, col);
        //    *pp = freeents;
        //    freeents = freeents->next;
        //}
        (*pp)->label = (char *) 0;
        (*pp)->flags = may_sync;
        (*pp)->expr = (struct enode *) 0;
        (*pp)->v = (double) 0.0;
        (*pp)->format = (char *) 0;
        (*pp)->cellerror = CELLOK;
        (*pp)->next = NULL;
        (*pp)->ucolor = NULL;
        (*pp)->pad = 0;
    }
    (*pp)->row = row;
    (*pp)->col = col;
    if (row > maxrow) maxrow = row;
    if (col > maxcol) maxcol = col;
    return (*pp);
}

// cleanent: blank an ent
void cleanent(struct ent * p) {
    if (!p) return;
    p->label = (char *) 0;
    //p->row = 0;
    //p->col = 0;
    p->flags = may_sync;
    p->expr = (struct enode *) 0;
    p->v = (double) 0.0;
    p->format = (char *) 0;
    p->cellerror = CELLOK;
    p->next = NULL;
    p->ucolor = NULL;
    p->pad = 0;
    return;
}

// clearent: free memory of an ent and its contents
void clearent(struct ent * v) {
    if (!v) return;

    label(v, "", -1);
    v->v = (double)0;

    if (v->expr) efree(v->expr);
    v->expr = NULL;

    if (v->format) scxfree(v->format);
    v->format = NULL;

    if (v->ucolor) free(v->ucolor);
    v->ucolor = NULL;

    v->flags = is_changed;
    changed++;
    modflg++;

    return;
}

void scroll_left(int n) {
    while (n--) {
        if (! offscr_sc_cols ) {
            break;
        }
        int a = 1;
        int b = 0;
        offscr_sc_cols--;
        while (a != b && curcol) {
            a = offscr_sc_cols;
            calc_offscr_sc_cols();
            b = offscr_sc_cols;
            if (a != b) {
                curcol --;
                offscr_sc_cols = a;
            }
        }
    }
    return;
}

void scroll_right(int n) {
    while (n--) {
        // This while statement allow the cursor to shift to the right when the
        // las visible column is reached in the screen
        while (curcol < offscr_sc_cols + 1) {
            curcol++;
        }
        offscr_sc_cols++;
    }
    return;
}

void scroll_down(int n) {
    while (n--) {
        if (currow == offscr_sc_rows) {
            forw_row(1);
            unselect_ranges();
        }
        offscr_sc_rows++;
    }
    return;
}

void scroll_up(int n) {
    while (n--) {
        if (offscr_sc_rows)
            offscr_sc_rows--;
        else
            break;
        if (currow == offscr_sc_rows + LINES - RESROW - 1) {
            back_row(1);
            unselect_ranges();
        }
    }
    return;
}

struct ent * left_limit() {
    int c = 0;
    while ( col_hidden[c] && c < curcol ) c++; 
    return lookat(currow, c);
}

struct ent * right_limit() {
    register struct ent *p;
    int c = maxcols - 1;
    while ( (! VALID_CELL(p, currow, c) && c > 0) || col_hidden[c]) c--;
    return lookat(currow, c);
}

struct ent * goto_top() {
    int r = 0;
    while ( row_hidden[r] && r < currow ) r++; 
    return lookat(r, curcol);
}

struct ent * goto_bottom() {
    register struct ent *p;
    int r = maxrows - 1;
    while ( (! VALID_CELL(p, r, curcol) && r > 0) || row_hidden[r]) r--;
    return lookat(r, curcol);
}

// moves curcol back one displayed column
struct ent * back_col(int arg) {
    int c = curcol;
    while (--arg >= 0) {
    if (c)
        c--;
    else {
        sc_info ("At column A");
        break; 
    }
    while( col_hidden[c] && c )
        c--;
    }
    //rowsinrange = 1;
    //colsinrange = fwidth[curcol];
    return lookat(currow, c);
}

/* moves curcol forward one displayed column */
struct ent * forw_col(int arg) {
    int c = curcol;
    while (--arg >= 0) {
        if (c < maxcols - 1)
            c++;
        else
            if (! growtbl(GROWCOL, 0, arg)) {    /* get as much as needed */
                //sc_error("cannot grow");
                return lookat(currow, curcol);
                //break;
            } else
                c++;
        while (col_hidden[c] && (c < maxcols - 1))
            c++;
    }
    //rowsinrange = 1;
    //colsinrange = fwidth[curcol];
    return lookat(currow, c);
}

/* moves currow forward one displayed row */
struct ent * forw_row(int arg) {
    int r = currow;
    while (arg--) {
        if (r < maxrows - 1)
            r++;
        else {
            if (! growtbl(GROWROW, arg, 0)) {
                //sc_error("cannot grow");
                return lookat(currow, curcol);
            } else 
                r++;
        }
        while (row_hidden[r] && (r < maxrows - 1)) {
            r++;
        }
    }
    return lookat(r, curcol);
}

/* moves currow backward one displayed row */
struct ent * back_row(int arg) {
    int r = currow;
    while (arg--) {
        if (r) r--;
        else {
            sc_info("At row zero");
            break;
        }
        while (row_hidden[r] && r)
            r--;
    }
    return lookat(r, curcol);
}

struct ent * go_end() {
    int r = currow, c = curcol;
    int raux = r, caux = c;
    register struct ent *p;
    do {
        if (c < maxcols - 1)
            c++;
        else {
            if (r < maxrows - 1) {
                r++;
                c = 0;
            } else break;
        }
        if (VALID_CELL(p, r, c) && ! col_hidden[c] && ! row_hidden[r]) { raux = r; caux = c; }
    } while ( r < maxrows || c < maxcols );
    if ( ! VALID_CELL(p, r, c) && ! col_hidden[c] && ! row_hidden[r] )
        return lookat(raux, caux);
    return NULL;
}

struct ent * go_home() {
    return lookat(0, 0);
}

// if ticks a cell, returns struct ent *
// if ticks a range, return struct ent * to top left cell
struct ent * tick(char c) {
    //tick cell
    int r = get_mark(c)->row;
    if (r != -1)
        return lookat(r, get_mark(c)->col);

    // tick range
    if (curmode != VISUAL_MODE) {
        get_mark(c)->rng->selected = 1; 
        return lookat(get_mark(c)->rng->tlrow, get_mark(c)->rng->tlcol);
    }
    return NULL;
}

struct ent * go_forward() {
    int r = currow, c = curcol;
    int r_ori = r, c_ori = c;
    register struct ent * p;
    do {
        if (c < maxcols - 1) {
            c++;
        } else {
            if (r < maxrows - 1) {
                r++;
                c = 0;
            } else break;
        }
        if (VALID_CELL(p, r, c) && ! col_hidden[c] && ! row_hidden[r] )
            return lookat(r, c);
    } while (r < maxrows || c < maxcols);

    return lookat(r_ori, c_ori);
}

struct ent * go_backward() {
    int r = currow, c = curcol;
    int r_ori = r, c_ori = c;
    register struct ent * p;
    do {
        if (c) 
            c--;
        else {
            if (r) {
                r--;
                c = maxcols - 1;
            } else break;
        }
        if ( VALID_CELL(p, r, c) && ! col_hidden[c] && ! row_hidden[r] )
            return lookat(r, c);
    } while ( currow || curcol );

    return lookat(r_ori, c_ori);
}

struct ent * vert_top() {
    return currow < LINES - RESROW - 1 ? lookat(0, curcol) : lookat(offscr_sc_rows, curcol);
}

struct ent * vert_middle() {
    int bottom = offscr_sc_rows + LINES - RESROW - 2;
    if (bottom > maxrow) bottom = maxrow;
    return lookat( ((currow < LINES - RESROW - 1 ? 0 : offscr_sc_rows) + bottom) / 2, curcol);
}

struct ent * vert_bottom() {
    int c = offscr_sc_rows + LINES - RESROW - 2;
    if (c > maxrow) c = maxrow;
    return lookat(c, curcol);
}

struct ent * go_bol() {
    return lookat(currow, offscr_sc_cols);
}

struct ent * go_eol() {
    return lookat(currow, offscr_sc_cols + calc_offscr_sc_cols() - 1);
}

struct ent * horiz_middle() {
    int i;
    int ancho = rescol;
    int visibles = calc_offscr_sc_cols();
    for (i = offscr_sc_cols; i < offscr_sc_cols + visibles; i++) {
        ancho += fwidth[i];
        if (ancho >= (COLS-rescol)/2) {
            return lookat(currow, i);
        }
    }
    return NULL;
}

void auto_justify(int ci, int cf, int min) {
    // column width is not set below the min value
    int r, c, sum = 0;
    char field[1024] = "";
    struct ent * p;
    wchar_t widestring[BUFFERSIZE] = { L'\0' };
    mbstate_t state;
    size_t result;
    const char * mbsptr;

#ifdef UNDO
    create_undo_action();
#endif

    checkbounds(&maxrow, &cf);

    for (c = ci; c <= cf; c++) {
#ifdef UNDO
        add_undo_col_format(c, 'R', fwidth[c], precision[c], realfmt[c]);
#endif
        fwidth[c] = min;
        for (r = 0; r <= maxrow; r++) {
            if ((p = *ATBL(tbl, r, c)) != NULL) {
                sum = 0;
                if (p->pad) sum += p->pad;
                if (p->label) {
                    memset( &state, '\0', sizeof state );
                    mbsptr = p->label;
                    result = mbsrtowcs(widestring, &mbsptr, BUFFERSIZE, &state);
                    if ( result != (size_t)-1 )
                        sum += wcswidth(widestring, wcslen(widestring));
                }
                if (p->flags & is_valid) {
                    sprintf(field, "%.*f", precision[c], p->v);
                    sum += strlen(field);
                }
                if (sum > fwidth[c] && sum < COLS-rescol)
                    fwidth[c] = sum;
            }
        }
#ifdef UNDO
        add_undo_col_format(c, 'A', fwidth[c], precision[c], realfmt[c]);
#endif
    }
#ifdef UNDO
    end_undo_action();
#endif

    return;
}

/*
 * deletes the expression associated w/ a cell and turns it into a constant
 * containing whatever was on the screen
 */
void valueize_area(int sr, int sc, int er, int ec) {
    int r, c;
    struct ent *p;

    if (sr > er) {
        r = sr; sr = er; er= r;
    }

    if (sc > ec) {
        c = sc; sc = ec; ec= c;
    }

    if (sr < 0)
        sr = 0; 
    if (sc < 0)
        sc = 0;
    checkbounds(&er, &ec);

    #ifdef UNDO
    create_undo_action();
    #endif
    for (r = sr; r <= er; r++) {
        for (c = sc; c <= ec; c++) {
            p = *ATBL(tbl, r, c);
            if (p && p->flags & is_locked) {
                sc_error(" Cell %s%d is locked", coltoa(c), r);
                continue;
            }
    #ifdef UNDO
            copy_to_undostruct(r, c, r, c, 'd');
    #endif
            if (p && p->expr) {
                efree(p->expr);
                p->expr = (struct enode *)0;
                p->flags &= ~is_strexpr;
            }
    #ifdef UNDO
            copy_to_undostruct(r, c, r, c, 'a');
    #endif
        }
    }
    #ifdef UNDO
    end_undo_action();
    #endif
    return;
}

void select_inner_range(int * vir_tlrow, int * vir_tlcol, int * vir_brrow, int * vir_brcol) {
    struct ent * p;
    int rr, cc, r, c, mf = 1;

    while (mf != 0) {
        mf = 0;
        for (rr = *vir_tlrow; rr <= *vir_brrow; rr++) {
            for (cc = *vir_tlcol; cc <= *vir_brcol; cc++)
                for (r=-1; r<=1; r++)
                    for (c=-1; c<=1; c++) {
                        if (r == 0 && c == 0) continue;
                        else if (rr + r < 0 || cc + c < 0 || rr + r > maxrow || cc + c > maxcol) continue;
                        p = *ATBL(tbl, rr + r, cc + c);
                        if ( p != NULL && (p->label || p->flags & is_valid) ) {
                            if (*vir_brcol < cc + c) {
                                *vir_brcol = cc + c;
                                mf=1;
                            }
                            if (*vir_brrow < rr + r) {
                                *vir_brrow = rr + r;
                                mf=1;
                            }
                            if (*vir_tlcol > cc + c) {
                                *vir_tlcol = cc + c;
                                mf=1;
                            }
                            if (*vir_tlrow > rr + r) {
                                *vir_tlrow = rr + r;
                                mf = 1;
                            }
                        }
                    }
            if (mf) break;
        } // rr
    }
    return;
}

/* Returns 1 if cell is locked, 0 otherwise */
int locked_cell(int r, int c) {
    struct ent *p = *ATBL(tbl, r, c);
    if (p && (p->flags & is_locked)) {
        sc_error("Cell %s%d is locked", coltoa(c), r) ;
        return 1;
    }
    return 0;
}

// Check if area contains locked cells
int any_locked_cells(int r1, int c1, int r2, int c2) {
    int r, c;
    struct ent * p ;

    for (r = r1; r <= r2; r++)
    for (c = c1; c <= c2; c++) {
        p = *ATBL(tbl, r, c);
        if (p && (p->flags & is_locked))
            return 1;
    }
    return 0;
}

// sum special command
int fsum() {
    int r = currow, c = curcol;
    struct ent * p;

#ifdef UNDO
    create_undo_action();
    copy_to_undostruct(currow, curcol, currow, curcol, 'd');
#endif
    if (r > 0 && (*ATBL(tbl, r-1, c) != NULL) && (*ATBL(tbl, r-1, c))->flags & is_valid) {
        for (r = currow-1; r >= 0; r--) {
            p = *ATBL(tbl, r, c);
            if (p == NULL) break;
            if (! (p->flags & is_valid)) break;
        }
        if (currow == r) {
#ifdef UNDO
            dismiss_undo_item();
            end_undo_action();
#endif
        } else {
            swprintf(interp_line, BUFFERSIZE, L"let %s%d = @SUM(", coltoa(curcol), currow);
            swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%s%d:", coltoa(curcol), r+1);
            swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%s%d)", coltoa(curcol), currow-1);
        }
    } else if (c > 0 && (*ATBL(tbl, r, c-1) != NULL) && (*ATBL(tbl, r, c-1))->flags & is_valid) {
        for (c = curcol-1; c >= 0; c--) {
            p = *ATBL(tbl, r, c);
            if (p == NULL) break;
            if (! (p->flags & is_valid)) break;
        }
        if (curcol == c) {
#ifdef UNDO
            dismiss_undo_item();
            end_undo_action();
#endif
        } else {
            swprintf(interp_line, BUFFERSIZE, L"let %s%d = @SUM(", coltoa(curcol), currow);
            swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%s%d:", coltoa(c+1), r);
            swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%s%d)", coltoa(curcol-1), r);
        }
    }

    if ((currow != r || curcol != c) && wcslen(interp_line)) {
        send_to_interp(interp_line);
#ifdef UNDO
        copy_to_undostruct(currow, curcol, currow, curcol, 'a');
        end_undo_action();
#endif
    }
    return 0;
}

// fcopy special command
int fcopy() {
    int r, ri, rf, ci, cf;
    struct ent * pdest;
    struct ent * pact;
    int p = is_range_selected();
    struct srange * sr = NULL;
    if (p != -1) sr = get_range_by_pos(p);

    if ( ( p == -1 && (*ATBL(tbl, currow, curcol) == NULL || ! (*ATBL(tbl, currow, curcol))->expr))
            ||   ( p != -1 && (*ATBL(tbl, sr->tlrow, sr->tlcol) == NULL || ! (*ATBL(tbl, sr->tlrow, sr->tlcol))->expr) )
       ) {
        sc_error("Formula not found. Nothing changed");
        return -1;
    }

    if (p == -1) { // no range selected
        ri = currow;
        ci = curcol;
        cf = curcol;
        for (r=ri+1; r<maxrow && (*ATBL(tbl, r, cf-1)) != NULL && (*ATBL(tbl, r, cf-1))->flags & is_valid ; r++) {}
        rf = --r;
    } else { // range is selected
        ri = sr->tlrow;
        ci = sr->tlcol;
        cf = sr->tlcol;
        rf = sr->brrow;
    }

    if (any_locked_cells(ri, ci, rf, cf)) {
        swprintf(interp_line, BUFFERSIZE, L"");
        sc_error("Locked cells encountered. Nothing changed");
        return -1;
    }
#ifdef UNDO
    create_undo_action();
    copy_to_undostruct(ri, ci, rf, cf, 'd');
#endif

    //do
    pact = *ATBL(tbl, ri, ci);
    for (r=ri+1; r<=rf; r++) {
        pdest = lookat(r, ci);
        copyent(pdest, pact, r - ri, 0, 0, 0, maxrows, maxcols, 'c');
    }

#ifdef UNDO
    copy_to_undostruct(ri, ci, rf, cf, 'a');
    end_undo_action();
#endif

    return 0;
}

// add padd to cells!
// this sets n to padding of a range
int pad(int n, int r1, int c1, int r2, int c2) {
    int r, c;
    struct ent * p ;
    int pad_exceed_width = 0;

    if (any_locked_cells(r1, c1, r2, c2)) {
        sc_info("Locked cells encountered. Nothing changed");
        return -1;
     }

#ifdef UNDO
    create_undo_action();
#endif

    for (r = r1; r <= r2; r++) {
        for (c = c1; c <= c2; c++) {
            if (n > fwidth[c]) {
                pad_exceed_width = 1;
                continue;
            }
            //p = lookat(r, c);
            if ((p = *ATBL(tbl, r, c)) != NULL) {
#ifdef UNDO
                copy_to_undostruct(r, c, r, c, 'd');
#endif
                p->pad = n;
#ifdef UNDO
                copy_to_undostruct(r, c, r, c, 'a');
#endif
            }
            modflg++;
        }
    }

#ifdef UNDO
    end_undo_action();
#endif

    if (pad_exceed_width) {
        sc_error(" Could not add padding in some cells. Padding exceeded column width");
        return 1;
    }
    return 0;
}

// Check if the buffer content is a valid command
// res = 0 or NO_CMD : buf has no command
// res = 1 or EDITION_CMD : buf has a command
// res = 2 or MOVEMENT_CMD : buf has a movement command or a command that do not
// change cell content, and should not be considered by the '.' command
int is_single_command (struct block * buf, long timeout) {
    if (buf->value == L'\0') return NO_CMD;
    int res = NO_CMD;
    int bs = get_bufsize(buf);

    if (curmode == COMMAND_MODE && bs == 1 && ( buf->value != ctl('r') ||
        buf->value == ctl('v')) ) {
        res = MOVEMENT_CMD;

    } else if ( curmode == COMMAND_MODE && bs == 2 && buf->value == ctl('r') &&
        (buf->pnext->value - (L'a' - 1) < 1 || buf->pnext->value > 26)) {
        res = MOVEMENT_CMD;

    } else if (curmode == INSERT_MODE && bs == 1 && ( buf->value != ctl('r') ||
        buf->value == ctl('v')) ) {
        res = MOVEMENT_CMD;

    } else if (curmode == INSERT_MODE && bs == 2 && buf->value == ctl('r') &&
        (buf->pnext->value - (L'a' - 1) < 1 || buf->pnext->value > 26)) {
        res = MOVEMENT_CMD;

    } else if (curmode == EDIT_MODE && bs == 1) {
        res = MOVEMENT_CMD;

    } else if (curmode == NORMAL_MODE && bs == 1) {
        // commands for changing mode
        if (buf->value == L':')             res = MOVEMENT_CMD;
        else if (buf->value == L'\\')       res = MOVEMENT_CMD;
        else if (buf->value == L'<')        res = MOVEMENT_CMD;
        else if (buf->value == L'>')        res = MOVEMENT_CMD;
        else if (buf->value == L'=')        res = MOVEMENT_CMD;
        else if (buf->value == L'e')        res = MOVEMENT_CMD;
        else if (buf->value == L'E')        res = MOVEMENT_CMD;
        else if (buf->value == L'v')        res = MOVEMENT_CMD;

        else if (buf->value == L'Q')        res = MOVEMENT_CMD;  //TEST
        else if (buf->value == L'A')        res = MOVEMENT_CMD;  //TEST
        else if (buf->value == L'W')        res = MOVEMENT_CMD;  //TEST

        // movement commands
        else if (buf->value == L'j')        res = MOVEMENT_CMD;
        else if (buf->value == L'k')        res = MOVEMENT_CMD;
        else if (buf->value == L'h')        res = MOVEMENT_CMD;
        else if (buf->value == L'l')        res = MOVEMENT_CMD;
        else if (buf->value == L'0')        res = MOVEMENT_CMD;
        else if (buf->value == L'$')        res = MOVEMENT_CMD;
        else if (buf->value == OKEY_HOME)  res = MOVEMENT_CMD;
        else if (buf->value == OKEY_END)   res = MOVEMENT_CMD;
        else if (buf->value == L'#')        res = MOVEMENT_CMD;
        else if (buf->value == L'^')        res = MOVEMENT_CMD;
        else if (buf->value == OKEY_LEFT)  res = MOVEMENT_CMD;
        else if (buf->value == OKEY_RIGHT) res = MOVEMENT_CMD;
        else if (buf->value == OKEY_DOWN)  res = MOVEMENT_CMD;
        else if (buf->value == OKEY_UP)    res = MOVEMENT_CMD;
        else if (buf->value == OKEY_PGUP)  res = MOVEMENT_CMD;
        else if (buf->value == OKEY_PGDOWN)  res = MOVEMENT_CMD;
        else if (buf->value == ctl('f'))   res = MOVEMENT_CMD;
        else if (buf->value == ctl('j'))   res = EDITION_CMD;
        else if (buf->value == ctl('d'))   res = EDITION_CMD;
        else if (buf->value == ctl('b'))   res = MOVEMENT_CMD;
        else if (buf->value == ctl('a'))   res = MOVEMENT_CMD;
        else if (buf->value == L'G')        res = MOVEMENT_CMD;
        else if (buf->value == L'H')        res = MOVEMENT_CMD;
        else if (buf->value == L'M')        res = MOVEMENT_CMD;
        else if (buf->value == L'L')        res = MOVEMENT_CMD;
        else if (buf->value == ctl('y'))   res = MOVEMENT_CMD;
        else if (buf->value == ctl('e'))   res = MOVEMENT_CMD;
        else if (buf->value == ctl('l'))   res = MOVEMENT_CMD;
        else if (buf->value == L'w')        res = MOVEMENT_CMD;
        else if (buf->value == L'b')        res = MOVEMENT_CMD;
        else if (buf->value == L'/')        res = MOVEMENT_CMD; // search
        else if (buf->value == L'n')        res = MOVEMENT_CMD; // repeat last goto cmd
        else if (buf->value == L'N')        res = MOVEMENT_CMD; // repeat last goto cmd - backwards

        // edition commands
        else if (buf->value == L'x')        res = EDITION_CMD;  // cuts a cell
        else if (buf->value == L'u')        res = EDITION_CMD;  // undo
        else if (buf->value == ctl('r'))    res = EDITION_CMD;  // redo
        else if (buf->value == L'@')        res = EDITION_CMD;  // EvalAll
        else if (buf->value == L'{')        res = EDITION_CMD;
        else if (buf->value == L'}')        res = EDITION_CMD;
        else if (buf->value == L'|')        res = EDITION_CMD;
        else if (buf->value == L'p')        res = EDITION_CMD;  // paste yanked cells below or left
        else if (buf->value == L't')        res = EDITION_CMD;  // paste yanked cells above or right
        else if (buf->value == L'-')        res = EDITION_CMD;
        else if (buf->value == L'+')        res = EDITION_CMD;

        else if (isdigit(buf->value) && atoi(get_conf_value("numeric")) )
                                           res = MOVEMENT_CMD; // repeat last command

        else if (buf->value == L'.')        res = MOVEMENT_CMD; // repeat last command
        else if (buf->value == L'y' && is_range_selected() != -1) 
                                            res = EDITION_CMD;  // yank range

    } else if (curmode == NORMAL_MODE) {

        if (buf->value == L'g' && bs == 2 && (
                 buf->pnext->value == L'M' || 
                 buf->pnext->value == L'g' ||
                 buf->pnext->value == L'G' ||
                 buf->pnext->value == L'0' ||
                 buf->pnext->value == L'l' ||
                 buf->pnext->value == L'$'))
                 res = MOVEMENT_CMD;

        else if (buf->value == L'g' && bs > 2 && timeout >= COMPLETECMDTIMEOUT)
                 res = MOVEMENT_CMD; // goto cell
                 // TODO add validation: buf->pnext->value debe ser letra

        else if (buf->value == L'P' && bs == 2 && (
            buf->pnext->value == L'v' || 
            buf->pnext->value == L'f' || 
            buf->pnext->value == L'c' ) )   res = EDITION_CMD;  // paste yanked cells below or left

        else if (buf->value == L'T' && bs == 2 && (
            buf->pnext->value == L'v' || 
            buf->pnext->value == L'f' || 
            buf->pnext->value == L'c' ) )   res = EDITION_CMD;  // paste yanked cells above or right

        else if (buf->value == L'a' && bs == 2 &&    // autojus
                 buf->pnext->value == L'a') res = EDITION_CMD;

        else if (buf->value == L'Z' && bs == 2 && timeout >= COMPLETECMDTIMEOUT &&  // Zap (or hide) col or row
               ( buf->pnext->value == L'c' ||
                 buf->pnext->value == L'r')) res = EDITION_CMD;

        else if (buf->value == L'S' && bs == 2 && timeout >= COMPLETECMDTIMEOUT &&  // Zap (or hide) col or row
               ( buf->pnext->value == L'c' ||
                 buf->pnext->value == L'r')) res = EDITION_CMD;

        else if (buf->value == L'y' && bs == 2 &&    // yank cell
               ( buf->pnext->value == L'y' ||
                 buf->pnext->value == L'r' ||
                 buf->pnext->value == L'c') ) res = EDITION_CMD;

        else if (buf->value == L'm' && bs == 2 &&    // mark
               ((buf->pnext->value - (L'a' - 1)) < 1 ||
                 buf->pnext->value > 26)) res = MOVEMENT_CMD;

        else if (buf->value == L'c' && bs == 2 &&    // mark
               ((buf->pnext->value - (L'a' - 1)) < 1 || buf->pnext->value > 26)) res = EDITION_CMD;

        else if (buf->value == L'z' && bs == 2 &&    // scrolling
               ( buf->pnext->value == L'h' ||
                 buf->pnext->value == L'l' ||
                 buf->pnext->value == L'z' ||
                 buf->pnext->value == L'm' ||
                 buf->pnext->value == L'.' ||
                 buf->pnext->value == L't' ||
                 buf->pnext->value == L'b' ||
                 buf->pnext->value == L'H' ||
                 buf->pnext->value == L'L')
               ) res = MOVEMENT_CMD;

        else if (buf->value == L'V' && bs == 3 &&    // Vir
                 buf->pnext->value == L'i' &&
                 buf->pnext->pnext->value == L'r')
                 res = MOVEMENT_CMD;

        else if (buf->value == L'd' && bs == 2 &&    // cuts a cell
                 buf->pnext->value == L'd') res = EDITION_CMD;

        else if (buf->value == L'\'' && bs == 2 &&   // tick 
               ((buf->pnext->value - (L'a' - 1)) < 1 ||
                 buf->pnext->value > 26)) res = MOVEMENT_CMD;

        else if (buf->value == L's' && bs == 2 &&    // shift cell down or up
               ( buf->pnext->value == L'j' ||
                 buf->pnext->value == L'k' ||
                 buf->pnext->value == L'h' ||
                 buf->pnext->value == L'l')) res = EDITION_CMD;

        else if (buf->value == L'i' && bs == 2 &&    // Insert row or column
               ( buf->pnext->value == L'r' ||
                 buf->pnext->value == L'c' )) res = EDITION_CMD;

        else if (buf->value == L'o' && bs == 2 &&    // Open row or column
               ( buf->pnext->value == L'r' ||
                 buf->pnext->value == L'c' )) res = EDITION_CMD;

        else if (buf->value == L'd' && bs == 2 &&    // Delete row or column
               ( buf->pnext->value == L'r' ||
                 buf->pnext->value == L'c' )) res = EDITION_CMD;

        else if (buf->value == L'r' && bs == 2 &&    // range lock / unlock / valueize
               ( buf->pnext->value == L'l' ||
                 buf->pnext->value == L'u' ||
                 buf->pnext->value == L'v' )) res = EDITION_CMD;

        else if (buf->value == L'R' && bs == 3 &&    // Create range with two marks
            //  FIXME add validation
               ((buf->pnext->value - (L'a' - 1)) < 1 ||
                 buf->pnext->value > 26) &&
               ((buf->pnext->pnext->value - (L'a' - 1)) < 1 ||
                 buf->pnext->pnext->value > 26)) res = EDITION_CMD;

        else if (buf->value == L'f' && bs == 2 &&    // Format col
               ( buf->pnext->value == L'>' ||
                 buf->pnext->value == L'<' ||
                 buf->pnext->value == L'h' ||
                 buf->pnext->value == OKEY_LEFT ||
                 buf->pnext->value == L'l' ||
                 buf->pnext->value == OKEY_RIGHT ||
                 buf->pnext->value == L'j' ||
                 buf->pnext->value == OKEY_DOWN ||
                 buf->pnext->value == L'k' ||
                 buf->pnext->value == OKEY_UP ||
                 buf->pnext->value == L'-' ||
                 buf->pnext->value == L'+' )) res = EDITION_CMD;

    } else if (curmode == VISUAL_MODE && bs == 1) {
             if (buf->value == L'j' ||
                 buf->value == OKEY_DOWN ||
                 buf->value == L'k' ||
                 buf->value == OKEY_UP    ||
                 buf->value == L'h' ||
                 buf->value == OKEY_LEFT ||
                 buf->value == L'l' ||
                 buf->value == OKEY_RIGHT ||
                 buf->value == L'$' ||
                 buf->value == L'0' ||
                 buf->value == L'#' ||
                 buf->value == L'^' ||
                 buf->value == L'y' ||
                 buf->value == L'x' ||
                 buf->value == L'w' ||
                 buf->value == L'b' ||
                 buf->value == L'H' ||
                 buf->value == L'M' ||
                 buf->value == L'L' ||
                 buf->value == L'G' ||
                 buf->value == ctl('f') ||
                 buf->value == ctl('j') ||
                 buf->value == ctl('d') ||
                 buf->value == ctl('b') ||
                 buf->value == ctl('a') ||
                 buf->value == ctl('o') ||
                 buf->value == ctl('k') ||
                 buf->value == L':'
             )
                 res = MOVEMENT_CMD;
        else if (buf->value == L'{' ||
                 buf->value == L'}' ||
                 buf->value == L'|')
                 res = EDITION_CMD;

    } else if (curmode == VISUAL_MODE && bs == 2) {
            if ((buf->value == L'\'') ||
                (buf->value == L'd' &&
                 buf->pnext->value == L'd')  ||
                (buf->value == L's' && (
                 buf->pnext->value == L'h' ||
                 buf->pnext->value == L'j' ||
                 buf->pnext->value == L'k' ||
                 buf->pnext->value == L'l' ))
            ) {
                 res = MOVEMENT_CMD;
            } else if ((buf->value == L'Z' && (
                 buf->pnext->value == L'r' ||
                 buf->pnext->value == L'c' )) ||
                (buf->value == L'S' && (
                 buf->pnext->value == L'r' ||
                 buf->pnext->value == L'c' )) ) {
                 res = EDITION_CMD;
            } else if (buf->value == L'r' && (
                 buf->pnext->value == L'l' ||
                 buf->pnext->value == L'u' ||
                 buf->pnext->value == L'v' )) {
                 res = EDITION_CMD;
            } else if (buf->value == L'm' && // mark
               ((buf->pnext->value - (L'a' - 1)) < 1 ||
                 buf->pnext->value > 26)) {
                 res = MOVEMENT_CMD;
            }
    }
    return res;
}
