/*******************************************************************************
 * Copyright (c) 2013-2021, Andrés Martinelli <andmarti@gmail.com>             *
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
 * \file cmds.c
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 05/04/2021
 * \brief TODO Write brief file description
 */

#include <stdlib.h>
#include <ctype.h>   // for isdigit
#include <wchar.h>
#include <wctype.h>
#include "main.h"
#include "maps.h"
#include "yank.h"
#include "marks.h"
#include "cmds.h"
#include "buffer.h"
#include "tui.h"
#include "conf.h"    // for conf parameters
#include "xmalloc.h" // for scxfree
#include "vmtbl.h"   // for growtbl
#include "utils/string.h" // for add_char
#include "y.tab.h"   // for yyparse
#include "dep_graph.h"
#include "freeze.h"
#ifdef UNDO
#include "undo.h"
#endif

void syncref(register struct enode *e);
extern int shall_quit;
char insert_edit_submode;
struct ent * freeents = NULL; // keep deleted ents around before sync_refs
wchar_t interp_line[BUFFERSIZE];
extern graphADT graph;
extern int yyparse(void);

int offscr_sc_rows = 0, offscr_sc_cols = 0; /**< off screen spreadsheet rows and columns */
int center_hidden_cols = 0;
int center_hidden_rows = 0;

/**
 * \brief Maintain ent strucs until they are release for deletion by sync_refs.
 *
 * \details This structure is used to keep ent structs around before
 * they are actually deleted (memory freed) to allow the sync_refs
 * routine a chance to fix the variable references. If delete flag
 * is set, is_deleted flag of an ent is set.
 *
 * \param[in] p
 * \param[in] delete
 *
 * \return none
 */
void mark_ent_as_deleted(register struct ent * p, int delete) {
    if (p == NULL) return;
    if (delete) p->flags |= is_deleted;

    p->next = freeents;     /* put this ent on the front of freeents */
    freeents = p;

    return;
}

/**
 * \brief TODO Write brief description
 *
 * \details Iterates throw freeents (ents marked as deleted). Calls
 * clearent for freeing ents contents memory and free ent pointer. This
 * function should always be called at exit. This is mandatory, just in
 * case we want to UNDO any changes.
 *
 * \return none
 */
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

/**
 * \brief TODO Write brief description
 *
 * \details Used to remove references to deleted struct ents.
 *
 * \details Note that the deleted structure must still be hanging
 * around before the call, but not referenced by an entry in tbl.
 *
 * \return none
 */
// TODO Improve this function such that it does not traverse the whole  table
void sync_refs() {
    int i, j;
    register struct ent * p;
    for (i=0; i <= maxrow; i++)
    for (j=0; j <= maxcol; j++)
        if ( (p = *ATBL(tbl, i, j)) && p->expr ) {
            syncref(p->expr);
        }
    return;
}

/**
 * \brief TODO Write brief description
 *
 * Used to remove a reference to deleted a single struct ent.
 *
 * Note that the deleted structure must still be hanging around before the
 * call, but not referenced by an entry in tbl.
 *
 * Example usage:
 * @code
 *     syncref(<variable>);
 * @endcode
 * returns: none
 */
void syncref(register struct enode * e) {
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

/**
 * \brief TODO Write brief description
 *
 * \param[in] col
 * \param[in] mult
 *
 * \return none
 */
void deletecol(int col, int mult) {
    if (any_locked_cells(0, col, maxrow, col + mult)) {
        sc_error("Locked cells encountered. Nothing changed");
        return;
    }
#ifdef UNDO
    create_undo_action();
    // here we save in undostruct, all the ents that depends on the deleted one (before change)
    ents_that_depends_on_range(0, col, maxrow, col - 1 + mult);
    copy_to_undostruct(0, col, maxrow, col - 1 + mult, UNDO_DEL, HANDLE_DEPS, NULL);
    save_undo_range_shift(0, -mult, 0, col, maxrow, col - 1 + mult);

    int i;
    for (i=col; i < col + mult; i++)
        add_undo_col_format(i, 'R', fwidth[i], precision[i], realfmt[i]);
#endif

    fix_marks(0, -mult, 0, maxrow,  col + mult -1, maxcol);
    if (!loading) yank_area(0, col, maxrow, col + mult - 1, 'c', mult);

    // do the job
    int_deletecol(col, mult);

    //EvalAll();

    if (!loading) modflg++;

#ifdef UNDO
    // here we save in undostruct, just the ents that depends on the deleted one (after change)
    copy_to_undostruct(0, 0, -1, -1, UNDO_ADD, HANDLE_DEPS, NULL);

    extern struct ent_ptr * deps;
    if (deps != NULL) free(deps);
    deps = NULL;

    end_undo_action();
#endif
    return;
}

/**
 * \brief TODO Write a brief description
 *
 * \details Delete a column. Parameters col = column to delete
 * multi = cmds multiplier. (commonly 1)
 *
 * \param[in] col
 * \param[in] mult
 *
 * \return none
 */
void int_deletecol(int col, int mult) {
    register struct ent ** pp;
    int r, c, i;

    while (mult--) {
        // mark ent of column to erase with is_deleted flag
        for (r = 0; r <= maxrow; r++) {
            pp = ATBL(tbl, r, col);
            if ( *pp != NULL ) {
                mark_ent_as_deleted(*pp, TRUE);
                //clearent(*pp);
                //free(*pp);
                *pp = NULL;
            }
        }

        rebuild_graph(); // Rebuild of graph is needed

        // Copy references from right column cells to left column (which gets removed)
        for (r = 0; r <= maxrow; r++) {
            for (c = col; c < maxcol; c++) {
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
        for (i = col; i < maxcols - 2; i++) {
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
        EvalAll();
        //flush_saved(); // we have to flush_saved only at exit.
        //this is because we have to keep ents in case we want to UNDO
    }

    return;
}

/**
 * \brief TODO Write a brief description
 *
 * \details Copy cell (struct ent). "special" indicates special treatment
 * when merging two cells for the "pm" command, merging formate only for
 * the "pf" command, or for adjusting cell references when transposing
 * with the "pt" command. r1, c1, r2, and c2 define the range in which the
 * dr and dc values should be used. Special =='u' means special copy from
 * spreadsheet to undo struct. Since its mandatory to make isolated copies
 * of p->expr->e.o.right.e.v.vp and p->expr->e.o.right.e.v.vp
 *
 * \param[in] n
 * \param[in] p
 * \param[in] dr
 * \param[in] dc
 * \param[in] r1
 * \param[in] c1
 * \param[in] r2
 * \param[in] c2
 * \param[in] special
 * \return none
 */
void copyent(register struct ent * n, register struct ent * p, int dr, int dc, int r1, int c1, int r2, int c2, int special) {
    if (!n || !p) {
        sc_error("copyent: internal error");
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
        if (special != 'v' && p->expr && special != 'u') {
            n->expr = copye(p->expr, dr, dc, r1, c1, r2, c2, special == 't');
#ifdef UNDO
        } else if (special == 'u' && p->expr) { // from spreadsheet to undo
            n->expr = copye(p->expr, dr, dc, r1, c1, r2, c2, 2);
#endif
        }
        if (p->expr && p->flags & is_strexpr)
            n->flags |= is_strexpr;
        else if (p->expr)
            n->flags &= ~is_strexpr;

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
            n->ucolor->italic = p->ucolor->italic;
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

/**
 * \brief TODO Write brief description
 * \return NUM; STR; etc.
 */
int etype(register struct enode *e) {
    if (e == (struct enode *)0)
        return NUM;
    switch (e->op) {
        case UPPER: case LOWER: case CAPITAL:
        case O_SCONST: case '#': case DATE: case FMT: case STINDEX:
        case EXT: case LUA: case SVAL: case SUBSTR:
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

/**
 * \brief TODO Write a brief function description
 * \details ignorelock is used when sorting so that locked cells
 * can still be sorted
 * \param[in] sr
 * \param[in] sc
 * \param[in] er
 * \param[in] ec
 * \param[in] ignorelock
 * \param[in] mark_as_deleted
 * \return none
 */
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

    /* mark the ent as deleted
     * Do a lookat() for the upper left and lower right cells of the range
     * being erased to make sure they are included in the delete buffer so
     * that pulling cells always works correctly even if the cells at one
     * or more edges of the range are all empty.
     */
    (void) lookat(sr, sc);
    (void) lookat(er, ec);
    for (r = sr; r <= er; r++) {
        for (c = sc; c <= ec; c++) {
            pp = ATBL(tbl, r, c);
            if (*pp && (!((*pp)->flags & is_locked) || ignorelock)) {

                /* delete vertex in graph
                   only if this vertex is not referenced by other */
                vertexT * v = getVertex(graph, *pp, 0);
                if (v != NULL && v->back_edges == NULL )
                    destroy_vertex(*pp);

                if (mark_as_deleted) {
                    mark_ent_as_deleted(*pp, TRUE);
                } else {
                    clearent(*pp); // free memory
                    cleanent(*pp); // fill ent with empty values
                    mark_ent_as_deleted(*pp, FALSE);
                }
                *pp = NULL;
            }
        }
    }
    return;
}

/**
 * \brief TODO Write a brief function description
 * \details Function to copy an expression. It returns the copy.
 * special = 1 means transpose
 * special = 2 means copy from spreadsheet to undo struct
 * \param[in] e
 * \param[in] Rdelta
 * \param[in] Cdelta
 * \param[in] r1
 * \param[in] c1
 * \param[in] r2
 * \param[in] c2
 * \param[in] special
 * \return none
 */
struct enode * copye(register struct enode *e, int Rdelta, int Cdelta, int r1, int c1, int r2, int c2, int special) {
    register struct enode * ret;
    static struct enode * range = NULL;

    if (e == (struct enode *) 0) {
        ret = (struct enode *) 0;

    } else if (e->op & REDUCE) {
        int newrow, newcol;
        ret = (struct enode *) scxmalloc((unsigned) sizeof (struct enode));
        ret->op = e->op;
        newrow = e->e.r.left.vf & FIX_ROW || e->e.r.left.vp->row < r1 || e->e.r.left.vp->row > r2 || e->e.r.left.vp->col < c1 || e->e.r.left.vp->col > c2 ?  e->e.r.left.vp->row : special == 1 ? r1 + Rdelta + e->e.r.left.vp->col - c1 : e->e.r.left.vp->row + Rdelta;
        newcol = e->e.r.left.vf & FIX_COL || e->e.r.left.vp->row < r1 || e->e.r.left.vp->row > r2 || e->e.r.left.vp->col < c1 || e->e.r.left.vp->col > c2 ?  e->e.r.left.vp->col : special == 1 ? c1 + Cdelta + e->e.r.left.vp->row - r1 : e->e.r.left.vp->col + Cdelta;
        ret->e.r.left.vp =
        lookat(newrow, newcol);
        ret->e.r.left.vf = e->e.r.left.vf;
        newrow = e->e.r.right.vf & FIX_ROW || e->e.r.right.vp->row < r1 || e->e.r.right.vp->row > r2 || e->e.r.right.vp->col < c1 || e->e.r.right.vp->col > c2 ?  e->e.r.right.vp->row : special == 1 ? r1 + Rdelta + e->e.r.right.vp->col - c1 : e->e.r.right.vp->row + Rdelta;
        newcol = e->e.r.right.vf & FIX_COL || e->e.r.right.vp->row < r1 || e->e.r.right.vp->row > r2 || e->e.r.right.vp->col < c1 || e->e.r.right.vp->col > c2 ?  e->e.r.right.vp->col : special == 1 ? c1 + Cdelta + e->e.r.right.vp->row - r1 : e->e.r.right.vp->col + Cdelta;
        ret->e.r.right.vp =
        lookat(newrow, newcol);
        ret->e.r.right.vf = e->e.r.right.vf;
    } else {
        struct enode *temprange=0;
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
                    if (range && e->e.v.vp->row >= range->e.r.left.vp->row && e->e.v.vp->row <= range->e.r.right.vp->row && e->e.v.vp->col >= range->e.r.left.vp->col && e->e.v.vp->col <= range->e.r.right.vp->col) {
                        newrow = range->e.r.left.vf & FIX_ROW ? e->e.v.vp->row : e->e.v.vp->row + Rdelta;
                        newcol = range->e.r.left.vf & FIX_COL ? e->e.v.vp->col : e->e.v.vp->col + Cdelta;
                    } else {
                        newrow = e->e.v.vf & FIX_ROW || e->e.v.vp->row < r1 || e->e.v.vp->row > r2 || e->e.v.vp->col < c1 || e->e.v.vp->col > c2 ?  e->e.v.vp->row : special == 1 ? r1 + Rdelta + e->e.v.vp->col - c1 : e->e.v.vp->row + Rdelta;
                        newcol = e->e.v.vf & FIX_COL || e->e.v.vp->row < r1 || e->e.v.vp->row > r2 || e->e.v.vp->col < c1 || e->e.v.vp->col > c2 ?  e->e.v.vp->col : special == 1 ? c1 + Cdelta + e->e.v.vp->row - r1 : e->e.v.vp->col + Cdelta;
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
                ret->e.o.left = copye(e->e.o.left, Rdelta, Cdelta, r1, c1, r2, c2, special);
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
                ret->e.o.left = copye(e->e.o.left, Rdelta, Cdelta, r1, c1, r2, c2, special);
                ret->e.o.right = copye(e->e.o.right, Rdelta, Cdelta, r1, c1, r2, c2, special);
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

/**
 * \brief dorowformat()
 * \details: apply a row format in lines(size) to a row (r)
 * \param[in] r
 * \param[in] size
 * \return none
 */
void dorowformat(int r, unsigned char size) {
    if (size < 1 || size > UCHAR_MAX || size > LINES - RESROW - 1) { sc_error("Invalid row format"); return; }

#ifdef UNDO
    if (! loading) {
        create_undo_action();
        add_undo_row_format(r, 'R', rowformat[r]);
    }
#endif
    if (r >= maxrows && !growtbl(GROWROW, 0, r)) r = maxrows-1 ;
    checkbounds(&r, &curcol);
    rowformat[r] = size;

#ifdef UNDO
    if (!loading) {
        add_undo_row_format(r, 'A', rowformat[r]);
        end_undo_action();
    }
#endif
    if (! loading) modflg++;
    return;
}

/**
 * \brief TODO Write brief function description
 * \details Note: Modified 9/17/90 THA to handle more formats.
 * \param[in] c1
 * \param[in] c2
 * \param[in] w
 * \param[in] p
 * \param[in] r
 * \return none
 */
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

    if (! get_conf_int("nocurses") && w > COLS - rescol - 2) {
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

/**
 * \brief TODO Document formatcol)
 * \param[in] c
 * \return none
 */
void formatcol(int c) {
    int arg = 1;
    int i;

    switch (c) {
        case '<':
        case 'h':
        case OKEY_LEFT:
            for (i = curcol; i < curcol + arg; i++) {
                if (fwidth[i] <= 2) {
                    sc_error("Cannot resize column any longer");
                    return;
                }
                fwidth[i]--;
                if (fwidth[i] <= 1)
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
        case OKEY_DOWN:
            for (i = curcol; i < curcol + arg; i++) {
                precision[i]--;
                if (precision[i] < 0)
                    precision[i] = 0;
            }
            modflg++;
            break;
        case '+':
        case OKEY_UP:
            for (i = curcol; i < curcol + arg; i++)
                precision[i]++;
            modflg++;
            break;
    }
    sc_info("Current format is %d %d %d", fwidth[curcol], precision[curcol], realfmt[curcol]);
    ui_update(TRUE);
    return;
}

/**
 * \brief TODO Document insert_row()
 * \details Insert a single rox. It will be inserted before currow.
 * if after is 0; after if it is 1.
 * \param[in] after
 * \returnsnone
 */
void insert_row(int after) {
    int r, c;
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
        rowformat[r] = rowformat[r-1];
        tbl[r] = tbl[r-1];
        for (c = 0, pp = ATBL(tbl, r, 0); c < maxcols; c++, pp++)
            if (*pp) (*pp)->row = r;
    }
    tbl[r] = tmprow;        // the last row is never used
    rowformat[r] = 1;

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

/**
 * \brief Insert new column
 * \details Insert a cingle column. The column will be inserted
 * BEFORE CURCOL if after is 0;
 * AFTER CURCOL if it is 1.
 * \param[in] after
 * \return none
 */
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

/**
 * \brief Delete a row
 * \param[in] row
 * \param[in] mult
 * \return none
 */
void deleterow(int row, int mult) {
    if (any_locked_cells(row, 0, row + mult - 1, maxcol)) {
        sc_error("Locked cells encountered. Nothing changed");
        return;
    }
#ifdef UNDO
    create_undo_action();
    // here we save in undostruct, all the ents that depends on the deleted one (before change)
    ents_that_depends_on_range(row, 0, row + mult - 1, maxcol);
    copy_to_undostruct(row, 0, row + mult - 1, maxcol, UNDO_DEL, HANDLE_DEPS, NULL);
    save_undo_range_shift(-mult, 0, row, 0, row - 1 + mult, maxcol);
    int i;
    for (i=row; i < row + mult; i++)
        add_undo_row_format(i, 'R', rowformat[currow]);
#endif

    fix_marks(-mult, 0, row + mult - 1, maxrow, 0, maxcol);
    if (!loading) yank_area(row, 0, row + mult - 1, maxcol, 'r', mult);

    // do the job
    int_deleterow(row, mult);

    //flush_saved(); // we have to flush only at exit. this is in case we want to UNDO

    if (!loading) modflg++;

#ifdef UNDO
    // here we save in undostruct, just the ents that depends on the deleted one (after the change)
    copy_to_undostruct(0, 0, -1, -1, UNDO_ADD, HANDLE_DEPS, NULL);

    extern struct ent_ptr * deps;
    if (deps != NULL) free(deps);
    deps = NULL;

    end_undo_action();
#endif
    return;
}

/**
 * \brief Delete a row
 * \details Delete a row - internal function
 * \param[in] row row to delete
 * \param[in] multi commands multiplier (usually 1)
 * \return none
 */
void int_deleterow(int row, int mult) {
    register struct ent ** pp;
    register struct ent * q;
    int r, c;

    //if (currow > maxrow) return;

    while (mult--) {
        // we need to decrease row of the row that we delete if
        // other cells refers to this one.
        for (c = 0; c < maxcols; c++) {
            if (row <= maxrow) {
                pp = ATBL(tbl, row, c);
                if ((q = *ATBL(tbl, row, c)) != NULL) q->row--;
            }
        }
        sync_refs();

        // and after that the erase_area of the deleted row
        erase_area(row, 0, row, maxcol, 0, 1); //important: this mark the ents as deleted

        // and we decrease ->row of all rows after the deleted one
        for (r = row; r < maxrows - 1; r++) {
            for (c = 0; c < maxcols; c++) {
                if (r <= maxrow) {
                    pp = ATBL(tbl, r, c);
                    pp[0] = *ATBL(tbl, r+1, c);
                    if ( pp[0] ) pp[0]->row--;
                }
            }
            //update row_hidden and rowformat here
            row_hidden[r] = row_hidden[r+1];
            rowformat[r] = rowformat[r+1];
        }

        rebuild_graph(); //TODO CHECK HERE WHY REBUILD IS NEEDED. See NOTE1 in shift.c
        sync_refs();
        //if (get_conf_int("autocalc") && ! loading) EvalAll();
        EvalAll();
        maxrow--;
    }
    return;
}

/**
 * \brief Document ljustify()
 * \param[in] sr
 * \param[in] sc
 * \param[in] er
 * \param[in] ec
 * \return none
 */
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
                modflg++;
            }
        }
    }
    return;
}

/**
 * \brief TODO Document rjustify()
 * \param[in] sr
 * \param[in] sc
 * \param[in] er
 * \param[in] ec
 * \return none
 */
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
                modflg++;
            }
        }
    }
    return;
}

/**
 * \brief TODO Cocument center()
 * \param[in] sr
 * \param[in] sc
 * \param[in] er
 * \param[in] ec
 * \return none
 */
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
                modflg++;
            }
        }
    }
    return;
}

/**
 * @brief TODO Document chg_mode
 * \param[in] strcmd
 * \return none
 */
void chg_mode(char strcmd){
    lastmode = curmode;
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

/**
 * \brief Delete selected cells
 * \details Delete selected cell or range of cells.
 * \return none
 */
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
    // here we save in undostruct, all the ents that depends on the deleted one (before change)
    ents_that_depends_on_range(tlrow, tlcol, brrow, brcol);
    copy_to_undostruct(tlrow, tlcol, brrow, brcol, UNDO_DEL, HANDLE_DEPS, NULL);
#endif

    erase_area(tlrow, tlcol, brrow, brcol, 0, 0); //important: this erases the ents, but does NOT mark them as deleted
    modflg++;
    sync_refs();
    //flush_saved(); DO NOT UNCOMMENT! flush_saved shall not be called other than at exit.

    EvalAll();

#ifdef UNDO
    // here we save in undostruct, all the ents that depends on the deleted one (after the change)
    copy_to_undostruct(tlrow, tlcol, brrow, brcol, UNDO_ADD, HANDLE_DEPS, NULL);
    extern struct ent_ptr * deps;
    if (deps != NULL) free(deps);
    deps = NULL;
    end_undo_action();
#endif

    return;
}

/**
 * \brief Enter cell content on a cell
 * \details Enter cell content on a cell.
 * Covers commands LET, LABEL, LEFTSTRING, and RIGHTSTRING
 * \param[in] r
 * \param[in] c
 * \param[in] submode
 * \param[in] content
 * \return none
 */
void enter_cell_content(int r, int c, char * submode,  wchar_t * content) {
    // TODO - ADD PADDING INTELLIGENCE HERE ??
    (void) swprintf(interp_line, BUFFERSIZE, L"%s %s = %ls", submode, v_name(r, c), content);
    send_to_interp(interp_line);
}

/**
 * @brief Send command to interpreter
 * \details Send command to interpreter
 * wide_char version
 * \param[in] oper
 * \return none
 */
void send_to_interp(wchar_t * oper) {
    if (get_conf_int("nocurses")) {
        int pos = -1;
        if ((pos = wstr_in_wstr(oper, L"\n")) != -1)
            oper[pos] = L'\0';
        sc_debug("Interp GOT: %ls", oper);
    }
    wcstombs(line, oper, BUFFERSIZE);

    linelim = 0;
    yyparse();
    linelim = -1;
    line[0]='\0';
    if (get_conf_int("autocalc") && ! loading) EvalAll();
    return;
}

/**
 * \brief Return a pointer to a cell's [struct ent *]
 * Return a pointer to a cell's [struct ent *], creating if needed
 * \param[in] row
 * \param[in] col
 * \return none
 */
struct ent * lookat(int row, int col) {
    register struct ent **pp;

    checkbounds(&row, &col);
    pp = ATBL(tbl, row, col);
    if ( *pp == NULL) {
         *pp = (struct ent *) malloc( (unsigned) sizeof(struct ent) );
        (*pp)->label = (char *) 0;
        (*pp)->flags = may_sync;
        (*pp)->expr = (struct enode *) 0;
        (*pp)->trigger = (struct trigger *) 0;
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

/**
 * \brief Blank an ent
 * \param[in] p
 * \return none
 */
void cleanent(struct ent * p) {
    if (!p) return;
    p->label = (char *) 0;
    // do not uncomment. if so mod erase_area.
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

/**
 * \brief Free memory of an ent and its contents
 * \param[in] v
 * \return none
 */
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

    modflg++;

    return;
}

/**
 * \brief Moves curcol back one displayed column
 * \param[in] arg
 * \return lookat
 */
struct ent * back_col(int arg) {
    extern int center_hidden_cols;
    int freeze = freeze_ranges && (freeze_ranges->type == 'c' ||  freeze_ranges->type == 'a') ? 1 : 0;
    int c = curcol;

    while (--arg >= 0) {
        if (c) {
            // need to update curcol here so center_hidden_cols
            // get update correctly after calc_offscr_sc_cols
            curcol = --c;
            calc_offscr_sc_cols();
        } else {
            sc_info ("At column A");
            break;
        }
        while ((col_hidden[c] || (freeze && c > freeze_ranges->br->col
        && c < freeze_ranges->br->col + center_hidden_cols)) && c) {
            // need to update curcol here so center_hidden_cols
            // get update correctly after calc_offscr_sc_cols
            curcol = --c;
            calc_offscr_sc_cols();
        }
    }

    return lookat(currow, c);
}

/**
 * \brief Moves curcol forward one displayed column
 * \param[in] arg
 * \return lookat
 */
struct ent * forw_col(int arg) {
    int c = curcol;
    extern int center_hidden_cols;
    int freeze = freeze_ranges && (freeze_ranges->type == 'c' ||  freeze_ranges->type == 'a') ? 1 : 0;

    while (--arg >= 0) {
        if (c < maxcols - 1)
            c++;
        else
            if (! growtbl(GROWCOL, 0, arg)) {    /* get as much as needed */
                sc_error("cannot grow");
                return lookat(currow, curcol);
            } else
                c++;
        while ((col_hidden[c] || (freeze && c > freeze_ranges->br->col && c <= freeze_ranges->br->col + center_hidden_cols)) && (c < maxcols - 1))
            c++;

    }
    return lookat(currow, c);
}

/**
 * \brief Move currow forward one displayed row
 * \param[in] arg
 * \return lookat
 */
struct ent * forw_row(int arg) {
    int r = currow;
    extern int center_hidden_rows;
    int freeze = freeze_ranges && (freeze_ranges->type == 'r' ||  freeze_ranges->type == 'a') ? 1 : 0;

    while (arg--) {
        if (r < maxrows - 1)
            r++;
        else {
            if (! growtbl(GROWROW, arg, 0)) {
                sc_error("cannot grow");
                return lookat(currow, curcol);
            } else
                r++;
        }
        while ((row_hidden[r] || (freeze && r > freeze_ranges->br->row && r <= freeze_ranges->br->row + center_hidden_rows)) && (r < maxrows - 1))
            r++;
    }
    return lookat(r, curcol);
}

/**
 * \brief Moves currow backward on displayed row
 * \return lookat
 */
struct ent * back_row(int arg) {
    int bkprow = currow;
    int r = currow;
    extern int center_hidden_rows;
    //int freeze = freeze_ranges && (freeze_ranges->type == 'r' ||  freeze_ranges->type == 'a') ? 1 : 0;

    while (arg--) {
        if (r) {
            // need to update currow here so center_hidden_rows
            // get update correctly after calc_offscr_sc_rows
            //currow = --r;
            //calc_offscr_sc_rows();
            r--;
        } else {
            sc_info("At row zero");
            break;
        }
        //while ((row_hidden[r] || (freeze && r > freeze_ranges->br->row && r < freeze_ranges->br->row + center_hidden_rows)) && r)
        while ((row_hidden[r] && r)) {
            // need to update currow here so center_hidden_rows
            // get update correctly after calc_offscr_sc_rows
            //currow = --r;
            //calc_offscr_sc_rows();
            r--;
        }
    }
    currow = bkprow;
    return lookat(r, curcol);
}

/**
 * \brief Document scroll_down()
 * \param[in] n
 * \return none
 */
void scroll_down(int n) {
    extern int center_hidden_rows;
    int freezer = freeze_ranges && (freeze_ranges->type == 'r' ||  freeze_ranges->type == 'a') ? 1 : 0;
    int tlrow = freezer ? freeze_ranges->tl->row : 0;
    int brrow = freezer ? freeze_ranges->br->row : 0;
    while (currow < maxrows && n--) {
    //while (n--) {
        if ( (!freezer && currow == offscr_sc_rows) ||
             ( freezer && currow == offscr_sc_rows + center_hidden_rows + brrow - tlrow + 1) // && currow != tlrow)
        ) {
            currow = forw_row(1)->row;
            unselect_ranges();
        }
        if (freezer && offscr_sc_rows == tlrow) {
            center_hidden_rows++;
        } else {
            offscr_sc_rows++;
        }
    }
    return;
}

/**
 * @brief Document scroll_up()
 * \param[in] n
 * \return none
 */
void scroll_up(int n) {
    extern int center_hidden_rows;
    int freezer = freeze_ranges && (freeze_ranges->type == 'r' ||  freeze_ranges->type == 'a') ? 1 : 0;
    int r, i;
    int brrow = freezer ? freeze_ranges->br->row : 0;
    int tlrow = freezer ? freeze_ranges->tl->row : 0;

    while (n--) {
        // check what is the last row visible (r)
        i = 0, r = offscr_sc_rows-1;
        while (i < LINES - RESROW - 1 && r < maxrows - 1) {
            r++;
            if (row_hidden[r]) continue;
            else if (r < offscr_sc_rows && ! (freezer && r >= tlrow && r <= brrow)) continue;
            else if (freezer && r > brrow && r <= brrow + center_hidden_rows) continue;
            else if (freezer && r < tlrow && r >= tlrow - center_hidden_rows) continue;
            i++;
        }

        if (freezer && center_hidden_rows && r != brrow) {
            center_hidden_rows--;
        } else if (offscr_sc_rows && (!freezer || r > brrow) ) {
            offscr_sc_rows--;
        } else if (offscr_sc_rows && freezer && r == brrow) {
            offscr_sc_rows--;
            center_hidden_rows++;
        } else {
            sc_info("cannot scroll no longer");
            break;
        }
        if (currow == r) {
            currow = back_row(1)->row;
            unselect_ranges();
        }
    }
    return;
}

/**
 * \brief TODO Document go_home()
 * \return lookat
 */
struct ent * go_home() {
    return lookat(0, 0);
}

/**
 * \brief vert_top() - for command H in normal mode
 * \return lookat
 */
struct ent * vert_top() {
    extern int center_hidden_rows;
    int freezer = freeze_ranges && (freeze_ranges->type == 'r' ||  freeze_ranges->type == 'a') ? 1 : 0;
    int brrow = freezer ? freeze_ranges->br->row : 0;

    int r = offscr_sc_rows;
    while ( row_hidden[r] && r < currow ) r++;
    if (freezer && currow > brrow + center_hidden_rows + 1) while (row_hidden[r] || r <= brrow + center_hidden_rows) r++;
    return lookat(r, curcol);
}

/**
 * \brief vert_bottom() - for command L in normal mode
 * \return lookat
 */
struct ent * vert_bottom() {
    extern int center_hidden_rows;
    int freezer = freeze_ranges && (freeze_ranges->type == 'r' ||  freeze_ranges->type == 'a') ? 1 : 0;
    int brrow = freezer ? freeze_ranges->br->row : 0;
    int tlrow = freezer ? freeze_ranges->tl->row : 0;

    int i = 0, r = offscr_sc_rows-1;
    while (i < LINES - RESROW - 1) {
        r++;
        if (row_hidden[r]) continue;
        else if (r < offscr_sc_rows && ! (freezer && r >= tlrow && r <= brrow)) continue;
        else if (freezer && r > brrow && r <= brrow + center_hidden_rows) continue;
        else if (freezer && r < tlrow && r >= tlrow - center_hidden_rows) continue;
        i+= rowformat[r];
    }
    if (r > maxrows) r = maxrows;
    return lookat(r, curcol);
}

/**
 * \brief vert_middle() - for command M in normal mode
 * \return lookat
 */
struct ent * vert_middle() {
    extern int center_hidden_rows;
    int freezer = freeze_ranges && (freeze_ranges->type == 'r' ||  freeze_ranges->type == 'a') ? 1 : 0;
    int brrow = freezer ? freeze_ranges->br->row : 0;
    int tlrow = freezer ? freeze_ranges->tl->row : 0;

    int top = offscr_sc_rows;
    while ( (row_hidden[top] && top < currow) || (freezer && top >= tlrow && top <= brrow) ||
        (freezer && top > brrow && top <= brrow + center_hidden_rows) ||
        (freezer && top < tlrow && top >= tlrow - center_hidden_rows)) top++;
    int mid = (vert_bottom()->row + top) / 2;
    while ( row_hidden[mid] && mid < currow ) mid++; // just in case mid is hidden
    return lookat(mid, curcol);
}

/**
 * \brief TODO Document go_end()
 * \return lookat; NULL otherwise
 */
struct ent * go_end() {
    int r = 0, c = 0;
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

/**
 * \brief TODO Document tick()
 * \details if ticks a cell, returns struct ent *
 * if ticks a range, return struct ent * to top left cell
 * \return lookat; NULL otherwise
 */
struct ent * tick(char ch) {
    int r, c;
    struct mark * m = get_mark(ch);
    //tick cell
    r = m->row;

    if (r != -1) {
        checkbounds(&r, &curcol);
        return lookat(r, m->col);
    }

    // tick range
    if (curmode != VISUAL_MODE) {
        r = m->rng->tlrow;
        c = m->rng->tlcol;
        m->rng->selected = 1;
        checkbounds(&r, &c);
        return lookat(r, c);
    }
    return NULL;
}

/**
 * \brief TODO  Document scroll_right()
 * \param[in] n
 * \return none
 */
void scroll_right(int n) {
    extern int center_hidden_cols;
    int freezec = freeze_ranges && (freeze_ranges->type == 'c' ||  freeze_ranges->type == 'a') ? 1 : 0;
    int brcol = freezec ? freeze_ranges->br->col : 0;
    int tlcol = freezec ? freeze_ranges->tl->col : 0;

    while (curcol < maxcols && n--) {
        if ((freezec && curcol == offscr_sc_cols + center_hidden_cols + brcol - tlcol + 1) ||
           (!freezec && curcol == offscr_sc_cols)) {
            curcol = forw_col(1)->col;
            unselect_ranges();
        }
        if (freezec && offscr_sc_cols == freeze_ranges->tl->col) {
            center_hidden_cols++;
        } else {
            offscr_sc_cols++;
        }
        while (curcol < offscr_sc_cols) curcol++;
    }
    return;
}

/**
 * @brief TODO Document scroll_left()
 * \param[in] n
 * \return none
 */
void scroll_left(int n) {
    extern int center_hidden_cols;
    int freezec = freeze_ranges && (freeze_ranges->type == 'c' ||  freeze_ranges->type == 'a') ? 1 : 0;

    while (n--) {
        int a = 1;
        int b = 0, c = 0, d = 0;
        if (freezec && center_hidden_cols) {
            center_hidden_cols--;
        } else if (offscr_sc_cols) {
            offscr_sc_cols--;
        } else {
            sc_info("cannot scroll no longer");
            break;
        }
        while (a != b && curcol) {
            a = offscr_sc_cols;
            c = center_hidden_cols;
            calc_offscr_sc_cols();
            b = offscr_sc_cols;
            d = center_hidden_cols;
            if (a != b || c != d) {
                curcol = back_col(1)->col;
                offscr_sc_cols = a;
                center_hidden_cols = c;
            }
        }
    }
    return;
}

/**
 * \brief TODO Document left_limit()
 *
 * \return lookat
 */
struct ent * left_limit() {
    int c = 0;
    while ( col_hidden[c] && c < curcol ) c++;
    return lookat(currow, c);
}

/**
 * \brief TODO Document right_limit()
 *
 * \return lookat
 */
struct ent * right_limit() {
    register struct ent *p;
    int c = maxcols - 1;
    while ( (! VALID_CELL(p, currow, c) && c > 0) || col_hidden[c]) c--;
    return lookat(currow, c);
}

/**
 * \brief TODO Document goto_top()
 *
 * \return lookat
 */
struct ent * goto_top() {
    int r = 0;
    center_hidden_rows=0;
    center_hidden_cols=0;
    while ( row_hidden[r] && r < currow ) r++;
    return lookat(r, curcol);
}

/**
 * \brief TODO Document goto_bottom()
 *
 * \return lookat
 */
// FIXME to handle freeze rows/cols
struct ent * goto_bottom() {
    register struct ent *p;
    int r = maxrows - 1;
    while ( (! VALID_CELL(p, r, curcol) && r > 0) || row_hidden[r]) r--;
    return lookat(r, curcol);
}

/**
 * \brief TODO Document goto_last_col()
 * traverse the table and see which is the max column that has content
 * this is because maxcol changes when moving cursor.
 * this function is used when exporting files
 * \return lookat
 */
struct ent * goto_last_col() {
    int r, mr = maxrows;
    int c, mc = 0;
    register struct ent *p;
    int rf = 0;

    for (r=0; r<mr; r++) {
        for (c=0; c<maxcols; c++) {
            if (c >= mc && VALID_CELL(p, r, c)) { mc = c; rf = r; }
        }
    }
    return lookat(rf, mc);
}

/**
 * @brief TODO Document go_forward()
 *
 * \return lookat
 */
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

/**
 * \brief TODO Document go_bol()
 *
 * \return lookat
 */
struct ent * go_bol() {
    return lookat(currow, offscr_sc_cols);
}

/**
 * \brief TODO Document go_eol()
 * \return none
 */
struct ent * go_eol() {
    return lookat(currow, offscr_sc_cols + calc_offscr_sc_cols() - 1);
}

/**
 * \brief TODO Document horiz_middle()
 * \return lookat; NULL otherwise
 */
struct ent * horiz_middle() {
    int i;
    int ancho = rescol;
    int visibles = calc_offscr_sc_cols();
    int freeze = freeze_ranges && (freeze_ranges->type == 'c' ||  freeze_ranges->type == 'a') ? 1 : 0;
    int tlcol = freeze ? freeze_ranges->tl->col : 0;
    int brcol = freeze ? freeze_ranges->br->col : 0;
    extern int center_hidden_cols;

    for (i = offscr_sc_cols; i < offscr_sc_cols + visibles; i++) {
        //if not shown, continue
        if (col_hidden[i]) continue;
        else if (freeze && i > brcol && i <= brcol + center_hidden_cols) continue;
        else if (freeze && i < tlcol && i >= tlcol - center_hidden_cols) continue;

        ancho += fwidth[i];
        if (ancho >= (COLS-rescol)/2) {
            return lookat(currow, i);
        }
    }
    return NULL;
}

/**
 * \brief TODO Document go_backward()
 *
 * \return lookat
 */
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

/**
 * \brief TODO Document auto_justify()
 *
 * \param[in] ci
 * \param[in] cf
 * \param[in] min
 *
 * \return none
 */
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

/**
 * \brief Delete a cell expression and turn into constant
 *
 * \details Deletes the expression associated with a cell and
 * turns it into a constant containing whatever was on the screen.
 *
 * \param[in] sr
 * \param[in] sc
 * \param[in] er
 * \param[in] ec
 *
 * \return none
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
    copy_to_undostruct(sr, sc, er, ec, UNDO_DEL, IGNORE_DEPS, NULL);
    #endif
    for (r = sr; r <= er; r++) {
        for (c = sc; c <= ec; c++) {
            p = *ATBL(tbl, r, c);
            if (p && p->flags & is_locked) {
                sc_error(" Cell %s%d is locked", coltoa(c), r);
                continue;
            }
            if (p && p->expr) {
                efree(p->expr);
                p->expr = (struct enode *)0;
                p->flags &= ~is_strexpr;

                // TODO move this to depgraph ?
                vertexT * v_cur = getVertex(graph, p, 0);
                if (v_cur != NULL) { // just in case

                    // for each edge in edges, we look for the reference to the vertex we are deleting, and we erase it!
                    edgeT * e = v_cur->edges;
                    while (e != NULL) { // && v_cur->back_edges == NULL) {
                        delete_reference(v_cur, e->connectsTo, 1);

                        // delete vertex only if it end up having no edges, no expression, no value, no label....
                        if (e->connectsTo->edges == NULL && e->connectsTo->back_edges == NULL && !e->connectsTo->ent->expr && !(e->connectsTo->ent->flags & is_valid) && ! e->connectsTo->ent->label)
                            destroy_vertex(e->connectsTo->ent);
                        //     WARNING: an orphan vertex now represents an ent that has an enode thats
                        //     need to be evaluated, but do not depend in another cell.
                        e = e->next;
                    }

                    destroy_list_edges(v_cur->edges);
                    v_cur->edges = NULL;

                    /* delete vertex in graph
                       only if this vertex is not referenced by other */
                    if (v_cur->back_edges == NULL ) destroy_vertex(p);
                    }
            }
        }
    }
    #ifdef UNDO
    copy_to_undostruct(sr, sc, er, ec, UNDO_ADD, IGNORE_DEPS, NULL);
    end_undo_action();
    #endif
    return;
}

/**
 * \brief TODO Document select_inner_range()
 * \param[in] vir_tlrow
 * \param[in] vir_tlcol
 * \param[in] vir_brrow
 * \param[in] vir_brcol
 * \return none
 */
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

/**
 * \brief Check if cell is locked
 *
 * \return 1 if cell if locked; 0 otherwise
 */
int locked_cell(int r, int c) {
    struct ent *p = *ATBL(tbl, r, c);
    if (p && (p->flags & is_locked)) {
        sc_error("Cell %s%d is locked", coltoa(c), r) ;
        return 1;
    }
    return 0;
}

/**
 * \brief Check if area contains locked cells
 *
 * \param[in] r1
 * \param[in] c1
 * \param[in] r2
 * \param[in] c2
 *
 * \return 1 if area contains a locked cell; 0 otherwise
 */
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

/**
 * \brief sum special command
 * \return none
 */
int fsum() {
    int r = currow, c = curcol;
    struct ent * p;

    if (r > 0 && (*ATBL(tbl, r-1, c) != NULL) && (*ATBL(tbl, r-1, c))->flags & is_valid) {
        for (r = currow-1; r >= 0; r--) {
            p = *ATBL(tbl, r, c);
            if (p == NULL) break;
            if (! (p->flags & is_valid)) break;
        }
        if (currow != r) {
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
        if (curcol != c) {
            swprintf(interp_line, BUFFERSIZE, L"let %s%d = @SUM(", coltoa(curcol), currow);
            swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%s%d:", coltoa(c+1), r);
            swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%s%d)", coltoa(curcol-1), r);
        }
    }

    if ((currow != r || curcol != c) && wcslen(interp_line))
        send_to_interp(interp_line);
    return 0;
}

/**
 * \brief fcopy special command
 * \param[in] action
 * \return -1 on error; 0 otherwise
 */
int fcopy(char * action) {
    int r, ri, rf, c, ci, cf;
    struct ent * pdest;
    struct ent * pact;
    int p = is_range_selected();
    struct srange * sr = NULL;
    if (p != -1) sr = get_range_by_pos(p);

    if (p == -1) { // no range selected
        // fail if the cursor is on the first column
        if (curcol == 0) {
            sc_error("Can't fcopy with no arguments while on column 'A'");
            return -1;
        }

        ri = currow;
        ci = curcol;
        cf = curcol;
        for (r=ri+1; r<maxrow && (*ATBL(tbl, r, cf-1)) != NULL && (*ATBL(tbl, r, cf-1))->flags & is_valid ; r++) {}
        rf = --r;
    } else { // range is selected
        ri = sr->tlrow;
        ci = sr->tlcol;
        cf = sr->brcol;
        rf = sr->brrow;
    }

    // check if all cells that will be copied to somewhere have a formula in them
    if (! strcmp(action, "") || ! strcmp(action, "cells")) {
        if (!  *ATBL(tbl, ri, ci)       ) goto formula_not_found;
        if (! (*ATBL(tbl, ri, ci))->expr) goto formula_not_found;
    } else if (! strcmp(action, "c") || ! strcmp(action, "columns")) {
        for (c=ci; c<=cf; c++) {
            if (!  *ATBL(tbl, ri, c)       ) goto formula_not_found;
            if (! (*ATBL(tbl, ri, c))->expr) goto formula_not_found;
        }
    } else if (! strcmp(action, "r") || ! strcmp(action, "rows")) {
        for (r=ri; r<=rf; r++) {
            if (!  *ATBL(tbl, r, ci)       ) goto formula_not_found;
            if (! (*ATBL(tbl, r, ci))->expr) goto formula_not_found;
        }
    } else {
        sc_error("Invalid parameter");
    }

    goto all_formulas_found;

    formula_not_found:
    sc_error("At least 1 formula not found. Nothing changed");
    return -1;

    all_formulas_found:

    if (any_locked_cells(ri, ci, rf, cf)) {
        swprintf(interp_line, BUFFERSIZE, L"");
        sc_error("Locked cells encountered. Nothing changed");
        return -1;
    }
#ifdef UNDO
    create_undo_action();
    copy_to_undostruct(ri, ci, rf, cf, UNDO_DEL, IGNORE_DEPS, NULL);
#endif

    if (! strcmp(action, "")) {
        // copy first column down (old behavior), for backwards compatibility
        pact = *ATBL(tbl, ri, ci);
        for (r=ri+1; r<=rf; r++) {
            pdest = lookat(r, ci);
            copyent(pdest, pact, r - ri, 0, 0, 0, maxrows, maxcols, 'c');
        }
    } else if (! strcmp(action, "c") || ! strcmp(action, "columns")) {
        // copy all selected columns down
        for (c=ci; c<=cf; c++) {
            pact = *ATBL(tbl, ri, c);
            for (r=ri+1; r<=rf; r++) {
                pdest = lookat(r, c);
                copyent(pdest, pact, r - ri, 0, 0, 0, maxrows, maxcols, 'c');
            }
        }
    } else if (! strcmp(action, "r") || ! strcmp(action, "rows")) {
        // copy all selected rows right
        for (r=ri; r<=rf; r++) {
            pact = *ATBL(tbl, r, ci);
            for (c=ci+1; c<=cf; c++) {
                pdest = lookat(r, c);
                copyent(pdest, pact, 0, c - ci, 0, 0, maxrows, maxcols, 'c');
            }
        }
    } else if (! strcmp(action, "cells")) {
        // copy selected cell down and right
        pact = *ATBL(tbl, ri, ci);
        for (r=ri; r<=rf; r++) {
            for(c=(r==ri?ci+1:ci); c<=cf; c++) {
                pdest = lookat(r, c);
                copyent(pdest, pact, r - ri, c - ci, 0, 0, maxrows, maxcols, 'c');
            }
        }
    }

#ifdef UNDO
    copy_to_undostruct(ri, ci, rf, cf, UNDO_ADD, IGNORE_DEPS, NULL);
    end_undo_action();
#endif

    return 0;
}

/**
 * \brief Add padding to cells
 *
 * \details Add padding to cells. This set padding of a range.
 *
 * \return -1 if locked cell is encountered; 1 if padding exceeded
 * column width; 0 otherwise
 */
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
    copy_to_undostruct(r1, c1, r2, c2, UNDO_DEL, IGNORE_DEPS, NULL);
#endif

    for (r = r1; r <= r2; r++) {
        for (c = c1; c <= c2; c++) {
            if (n > fwidth[c]) {
                pad_exceed_width = 1;
                continue;
            }
            if ((p = *ATBL(tbl, r, c)) != NULL) p->pad = n;
            modflg++;
        }
    }

#ifdef UNDO
    copy_to_undostruct(r1, c1, r2, c2, UNDO_ADD, IGNORE_DEPS, NULL);
    end_undo_action();
#endif

    if (pad_exceed_width) {
        sc_error(" Could not add padding in some cells. Padding exceeded column width");
        return 1;
    }
    return 0;
}

/**
 * \brief Calculate number of hidden columns to the left
 *
 * \details Calculate number of hidden columns to the left.
 * q are the number of columns that are before offscr_sc_cols that are shown
 * because they are frozen.
 *
 * \return number of hidden columns to the left
 */
int calc_offscr_sc_cols() {
    int q = 0, i, cols = 0, col = 0;
    int freeze = freeze_ranges && (freeze_ranges->type == 'c' ||  freeze_ranges->type == 'a') ? 1 : 0;
    int tlcol = freeze ? freeze_ranges->tl->col : 0;
    int brcol = freeze ? freeze_ranges->br->col : 0;

    // pick up col counts
    while (freeze && curcol > brcol && curcol <= brcol + center_hidden_cols) center_hidden_cols--;
    if (offscr_sc_cols - 1 <= curcol) {
        for (i = 0, q = 0, cols = 0, col = rescol; i < maxcols && col + fwidth[i] <= COLS; i++) {
            if (i < offscr_sc_cols && ! (freeze && i >= tlcol && i <= brcol)) continue;
            else if (freeze && i > brcol && i <= brcol + center_hidden_cols) continue;
            else if (freeze && i < tlcol && i >= tlcol - center_hidden_cols) continue;
            if (i < offscr_sc_cols && freeze && i >= tlcol && i <= brcol && ! col_hidden[i] ) q++;
            cols++;

            if (! col_hidden[i]) col += fwidth[i];
        }
    }

    // get  off screen cols
    while ( offscr_sc_cols + center_hidden_cols + cols - 1      < curcol || curcol < offscr_sc_cols
            || (freeze && curcol < tlcol && curcol >= tlcol - center_hidden_cols)) {

        // izq
        if (offscr_sc_cols - 1 == curcol) {
            if (freeze && offscr_sc_cols + cols + center_hidden_cols >= brcol && brcol - cols - offscr_sc_cols + 2 > 0)
                center_hidden_cols = brcol - cols - offscr_sc_cols + 2;
            offscr_sc_cols--;

        // derecha
        } else if (offscr_sc_cols + center_hidden_cols + cols == curcol &&
            (! freeze || (curcol > brcol && offscr_sc_cols < tlcol) || (curcol >= cols && offscr_sc_cols < tlcol))) {
            offscr_sc_cols++;

        // derecha con freeze cols a la izq.
        } else if (offscr_sc_cols + center_hidden_cols + cols == curcol) {
            center_hidden_cols++;

        // derecha con freeze a la derecha
        } else if (freeze && curcol < tlcol && curcol >= tlcol - center_hidden_cols ) {
            center_hidden_cols--;
            offscr_sc_cols++;

        } else {
            // Try to put the cursor in the center of the screen
            col = (COLS - rescol - fwidth[curcol]) / 2 + rescol;
            if (freeze && curcol > brcol) {
                offscr_sc_cols = tlcol;
                center_hidden_cols = curcol - brcol; //FIXME shall we count frozen cols to center??
            } else {
                offscr_sc_cols = curcol;
                center_hidden_cols = 0;
            }
            for (i=curcol-1; i >= 0 && col - fwidth[i] - 1 > rescol; i--) {
                if (freeze && curcol > brcol) center_hidden_cols--;
                else offscr_sc_cols--;
                if ( ! col_hidden[i]) col -= fwidth[i];
            }
        }
        // Now pick up the counts again
        for (i = 0, cols = 0, col = rescol, q = 0; i < maxcols && col + fwidth[i] <= COLS; i++) {
            if (i < offscr_sc_cols && ! (freeze && i >= tlcol && i <= brcol)) continue;
            else if (freeze && i > brcol && i <= brcol + center_hidden_cols) continue;
            else if (freeze && i < tlcol && i >= tlcol - center_hidden_cols) continue;
            if (i < offscr_sc_cols && freeze && i >= tlcol && i <= brcol && ! col_hidden[i]) q++;
            cols++;

            if (! col_hidden[i]) col += fwidth[i];
        }
    }
    while (freeze && curcol > brcol && curcol <= brcol + center_hidden_cols) center_hidden_cols--;
    return cols + center_hidden_cols - q;
}

/**
 * \brief Calculate the number of hidden rows above
 * \return the number of hidden rows above
 */
int calc_offscr_sc_rows() {
    int q, i, rows = 0, row = 0;
    int freeze = freeze_ranges && (freeze_ranges->type == 'r' ||  freeze_ranges->type == 'a') ? 1 : 0;
    int tlrow = freeze ? freeze_ranges->tl->row : 0;
    int brrow = freeze ? freeze_ranges->br->row : 0;

    // pick up row counts
    while (freeze && currow > brrow && currow <= brrow + center_hidden_rows) center_hidden_rows--;
    if (offscr_sc_rows - 1 <= currow) {
        for (i = 0, q = 0, rows = 0, row=RESROW; i < maxrows && row + rowformat[i] <= LINES; i++) {
            if (i < offscr_sc_rows && ! (freeze && i >= tlrow && i <= brrow)) continue;
            else if (freeze && i > brrow && i <= brrow + center_hidden_rows) continue;
            else if (freeze && i < tlrow && i >= tlrow - center_hidden_rows) continue;
            if (i < offscr_sc_rows && freeze && i >= tlrow && i <= brrow && ! row_hidden[i] ) q++;
            rows++;
            if (i == maxrows - 1) return rows + center_hidden_rows - q;
            if (! row_hidden[i]) row += rowformat[i];
        }
    }

    // get off screen rows
    while ( offscr_sc_rows + center_hidden_rows + rows - RESROW < currow || currow < offscr_sc_rows
            || (freeze && currow < tlrow && currow >= tlrow - center_hidden_rows)) {

        // move up
        if (offscr_sc_rows - 1 == currow) {
            if (freeze && offscr_sc_rows + rows + center_hidden_rows >= brrow && brrow - rows - offscr_sc_rows + 3 > 0)
                center_hidden_rows = brrow - rows - offscr_sc_rows + 3;
            offscr_sc_rows--;

        // move down
        } else if (offscr_sc_rows + center_hidden_rows + rows - 1 == currow &&
            (! freeze || (currow > brrow && offscr_sc_rows < tlrow) || (currow >= rows - 1 && offscr_sc_rows < tlrow))) {
            offscr_sc_rows++;

        // move down with freezen rows in top
        } else if (offscr_sc_rows + center_hidden_rows + rows - 1 == currow) {
            center_hidden_rows++;

        // move down with freezen rows in bottom
        } else if (freeze && currow < tlrow && currow >= tlrow - center_hidden_rows ) {
            center_hidden_rows--;
            offscr_sc_rows++;

        } else {
            // Try to put the cursor in the center of the screen
            row = (LINES - RESROW) / 2 + RESROW;
            if (freeze && currow > brrow) {
                offscr_sc_rows = tlrow;
                center_hidden_rows = currow - 2; //FIXME
            } else {
                offscr_sc_rows = currow;
                center_hidden_rows = 0;
            }
            for (i=currow-1; i >= 0 && row - rowformat[i] - 1 > RESROW && i < maxrows; i--) {
                if (freeze && currow > brrow) center_hidden_rows--;
                else offscr_sc_rows--;
                if (! row_hidden[i]) row -= rowformat[i];
            }
        }
        // Now pick up the counts again
        for (i = 0, rows = 0, row = RESROW, q = 0; i < maxrows && row + rowformat[i] <= LINES; i++) {
            if (i < offscr_sc_rows && ! (freeze && i >= tlrow && i <= brrow)) continue;
            else if (freeze && i > brrow && i <= brrow + center_hidden_rows) continue;
            else if (freeze && i < tlrow && i >= tlrow - center_hidden_rows) continue;
            if (i < offscr_sc_rows && freeze && i >= tlrow && i <= brrow && ! row_hidden[i]) q++;
            rows++;
            if (i == maxrows - 1) return rows + center_hidden_rows - q;

            if (! row_hidden[i]) row += rowformat[i];
        }
    }

    while (freeze && currow > brrow && currow <= brrow + center_hidden_rows) center_hidden_rows--;
    return rows + center_hidden_rows - q;
}

/**
 * \brief TODO Write a longer funciton description.
 *
 * \details This function aligns text of a cell (align = 0 center,
 * align = 1 right, align = -1 left). Also adds padding between cells.
 *
 * \param[in] srt_value
 * \param[in] numeric_value
 * \param[in] col_width
 * \param[in] align
 * \param[in] padding
 * \param[in] str_out
 *
 * \return resulting string to be printed to the screen
 */
void pad_and_align (char * str_value, char * numeric_value, int col_width, int align, int padding, wchar_t * str_out, int rowfmt) {
    int str_len  = 0;
    int num_len  = strlen(numeric_value);
    str_out[0] = L'\0';

    wchar_t wcs_value[BUFFERSIZE] = { L'\0' };
    mbstate_t state;
    size_t result;
    const char * mbsptr;
    mbsptr = str_value;

    // Here we handle \" and replace them with "
    int pos;
    while ((pos = str_in_str(str_value, "\\\"")) != -1)
        del_char(str_value, pos);

    // create wcs string based on multibyte string..
    memset( &state, '\0', sizeof state );
    result = mbsrtowcs(wcs_value, &mbsptr, BUFFERSIZE, &state);
    if ( result != (size_t)-1 )
        str_len = wcswidth(wcs_value, wcslen(wcs_value));

    // If padding exceedes column width, returns n number of '-' needed to fill column width
    if (padding >= col_width ) {
        wmemset(str_out + wcslen(str_out), L'#', col_width);
        return;
    }

    // If content exceedes column width, outputs n number of '*' needed to fill column width
    if (str_len + num_len + padding > col_width * rowfmt && !get_conf_int("truncate") && !get_conf_int("overlap") ) {
        if (padding) wmemset(str_out + wcslen(str_out), L'#', padding);
        wmemset(str_out + wcslen(str_out), L'*', col_width - padding);
        return;
    }

    // padding
    if (padding) swprintf(str_out, BUFFERSIZE, L"%*ls", padding, L"");

    // left spaces
    int left_spaces = 0;
    if (align == 0) {                                           // center align
        left_spaces = (col_width - padding - str_len) / 2;
        if (num_len > left_spaces) left_spaces = col_width - padding - str_len - num_len;
    } else if (align == 1 && str_len && ! num_len) {       // right align
        left_spaces = col_width - padding - str_len;
    }
    while (left_spaces-- > 0) add_wchar(str_out, L' ', wcslen(str_out));

    // add text
    if (align != 1 || ! num_len)
        swprintf(str_out + wcslen(str_out), BUFFERSIZE, L"%s", str_value);

    // spaces after string value
    int spaces = col_width - padding - str_len - num_len;
    if (align == 1){
      if(num_len) spaces += str_len;
      else spaces = 0;
    }
    if (align == 0) spaces -= (col_width - padding - str_len) / 2;
    while (spaces-- > 0) add_wchar(str_out, L' ', wcslen(str_out));

    // add number
    int fill_with_number = col_width - str_len - padding;
    if (num_len && num_len >= fill_with_number) {
        swprintf(str_out + wcslen(str_out), BUFFERSIZE, L"%.*s", fill_with_number, & numeric_value[num_len - fill_with_number]);
    } else if (num_len) {
        swprintf(str_out + wcslen(str_out), BUFFERSIZE, L"%s", numeric_value);
    }

    // Similar condition to max width '*' condition above, but just trims instead
    if (str_len + num_len + padding > col_width * rowfmt && get_conf_int("truncate")) {
        str_out[col_width] = '\0';
    }

    return;
}

/**
 * \brief Check if the buffer content is a valid command
 *
 * \details Check if the buffer content is a valid command
 * result = 0 or NO_CMD : buf has no command
 * result = 1 or EDITION_CMD : buf has a command
 * result = 2 or MOVEMENT_CMD : buf has a movement command or a command that do not
 * change cell content, and should not be considered by the '.' command
 *
 * \return result
 */
int is_single_command (struct block * buf, long timeout) {
    if (buf->value == L'\0') return NO_CMD;
    int result = NO_CMD;
    int bs = get_bufsize(buf);

    if (curmode == COMMAND_MODE && bs == 1 && ( buf->value != ctl('r') ||
        buf->value == ctl('v')) ) {
        result = MOVEMENT_CMD;

    } else if ( curmode == COMMAND_MODE && bs == 2 && buf->value == ctl('r') &&
        (buf->pnext->value - (L'a' - 1) < 1 || buf->pnext->value > 26)) {
        result = MOVEMENT_CMD;

    } else if (curmode == INSERT_MODE && bs == 1 && ( buf->value != ctl('r') ||
        buf->value == ctl('v')) ) {
        result = MOVEMENT_CMD;

    } else if (curmode == INSERT_MODE && bs == 2 && buf->value == ctl('r') &&
        (buf->pnext->value - (L'a' - 1) < 1 || buf->pnext->value > 26)) {
        result = MOVEMENT_CMD;

    } else if (curmode == EDIT_MODE && bs == 1) {
        result = MOVEMENT_CMD;

    } else if (curmode == NORMAL_MODE && bs == 1) {
        // commands for changing mode
        if (buf->value == L':')             result = MOVEMENT_CMD;
        else if (buf->value == L'\\')       result = MOVEMENT_CMD;
        else if (buf->value == L'<')        result = MOVEMENT_CMD;
        else if (buf->value == L'>')        result = MOVEMENT_CMD;
        else if (buf->value == L'=')        result = MOVEMENT_CMD;
        else if (buf->value == L'e')        result = MOVEMENT_CMD;
        else if (buf->value == L'E')        result = MOVEMENT_CMD;
        else if (buf->value == L'v')        result = MOVEMENT_CMD;

        else if (buf->value == L'Q')        result = MOVEMENT_CMD;  /* FOR TEST PURPOSES */
        else if (buf->value == L'A')        result = MOVEMENT_CMD;  /* FOR TEST PURPOSES */
        else if (buf->value == L'W')        result = MOVEMENT_CMD;  /* FOR TEST PURPOSES */

        // movement commands
        else if (buf->value == L'j')        result = MOVEMENT_CMD;
        else if (buf->value == L'k')        result = MOVEMENT_CMD;
        else if (buf->value == L'h')        result = MOVEMENT_CMD;
        else if (buf->value == L'l')        result = MOVEMENT_CMD;
        else if (buf->value == L'0')        result = MOVEMENT_CMD;
        else if (buf->value == L'$')        result = MOVEMENT_CMD;
        else if (buf->value == OKEY_HOME)   result = MOVEMENT_CMD;
        else if (buf->value == OKEY_END)    result = MOVEMENT_CMD;
        else if (buf->value == L'#')        result = MOVEMENT_CMD;
        else if (buf->value == L'^')        result = MOVEMENT_CMD;
        else if (buf->value == OKEY_LEFT)   result = MOVEMENT_CMD;
        else if (buf->value == OKEY_RIGHT)  result = MOVEMENT_CMD;
        else if (buf->value == OKEY_DOWN)   result = MOVEMENT_CMD;
        else if (buf->value == OKEY_UP)     result = MOVEMENT_CMD;
        else if (buf->value == OKEY_PGUP)   result = MOVEMENT_CMD;
        else if (buf->value == OKEY_PGDOWN) result = MOVEMENT_CMD;
        else if (buf->value == ctl('f'))    result = MOVEMENT_CMD;
        else if (buf->value == ctl('j'))    result = EDITION_CMD;
        else if (buf->value == ctl('d'))    result = EDITION_CMD;
        else if (buf->value == ctl('b'))    result = MOVEMENT_CMD;
        else if (buf->value == ctl('a'))    result = MOVEMENT_CMD;
        else if (buf->value == L'G')        result = MOVEMENT_CMD;
        else if (buf->value == L'H')        result = MOVEMENT_CMD;
        else if (buf->value == L'M')        result = MOVEMENT_CMD;
        else if (buf->value == L'L')        result = MOVEMENT_CMD;
        else if (buf->value == ctl('y'))    result = MOVEMENT_CMD;
        else if (buf->value == ctl('e'))    result = MOVEMENT_CMD;
        else if (buf->value == ctl('l'))    result = MOVEMENT_CMD;
        else if (buf->value == L'w')        result = MOVEMENT_CMD;
        else if (buf->value == L'b')        result = MOVEMENT_CMD;
        else if (buf->value == L'/')        result = MOVEMENT_CMD; // search
        else if (buf->value == L'n')        result = MOVEMENT_CMD; // repeat last goto cmd
        else if (buf->value == L'N')        result = MOVEMENT_CMD; // repeat last goto cmd - backwards

        // edition commands
        else if (buf->value == L'x')        result = EDITION_CMD;  // cuts a cell
        else if (buf->value == L'u')        result = MOVEMENT_CMD; // undo
        else if (buf->value == ctl('r'))    result = MOVEMENT_CMD; // redo
        else if (buf->value == L'@')        result = EDITION_CMD;  // EvalAll
        else if (buf->value == L'{')        result = EDITION_CMD;
        else if (buf->value == L'}')        result = EDITION_CMD;
        else if (buf->value == L'|')        result = EDITION_CMD;
        else if (buf->value == L'p')        result = EDITION_CMD;  // paste yanked cells below or left
        else if (buf->value == L't')        result = EDITION_CMD;  // paste yanked cells above or right
        else if (buf->value == L'-')        result = EDITION_CMD;
        else if (buf->value == L'+')        result = EDITION_CMD;

        else if (isdigit(buf->value) && get_conf_int("numeric") )
                                            result = MOVEMENT_CMD; // repeat last command

        else if (buf->value == L'.')        result = MOVEMENT_CMD; // repeat last command
        else if (buf->value == L'y' && is_range_selected() != -1)
                                            result = EDITION_CMD;  // yank range

    } else if (curmode == NORMAL_MODE) {

        if (buf->value == L'g' && bs == 2 && (
                 buf->pnext->value == L'M' ||
                 buf->pnext->value == L'g' ||
                 buf->pnext->value == L'G' ||
                 buf->pnext->value == L'0' ||
                 buf->pnext->value == L'l' ||
                 buf->pnext->value == L'$'))
                 result = MOVEMENT_CMD;

        else if (buf->value == L'g' && bs > 3 && buf->pnext->value == L't' && timeout >= COMPLETECMDTIMEOUT)
                 result = MOVEMENT_CMD; // goto cell
                 // TODO add validation: buf->pnext->pnext->value must be a letter

        else if (buf->value == L'P' && bs == 2 && (
            buf->pnext->value == L'v' ||
            buf->pnext->value == L'f' ||
            buf->pnext->value == L'c' ) )   result = EDITION_CMD;  // paste yanked cells below or left

        else if (buf->value == L'T' && bs == 2 && (
            buf->pnext->value == L'v' ||
            buf->pnext->value == L'f' ||
            buf->pnext->value == L'c' ) )   result = EDITION_CMD;  // paste yanked cells above or right

        else if (buf->value == L'a' && bs == 2 &&    // autojus
                 buf->pnext->value == L'a') result = EDITION_CMD;

        else if (buf->value == L'Z' && bs == 2 && timeout >= COMPLETECMDTIMEOUT &&  // Zap (or hide) col or row
               ( buf->pnext->value == L'c' ||
                 buf->pnext->value == L'r')) result = EDITION_CMD;

        else if (buf->value == L'S' && bs == 2 && timeout >= COMPLETECMDTIMEOUT &&  // Zap (or hide) col or row
               ( buf->pnext->value == L'c' ||
                 buf->pnext->value == L'r')) result = EDITION_CMD;

        else if (buf->value == L'y' && bs == 2 &&    // yank cell
               ( buf->pnext->value == L'y' ||
                 buf->pnext->value == L'r' ||
                 buf->pnext->value == L'c') ) result = EDITION_CMD;

        else if (buf->value == L'm' && bs == 2 &&    // mark
               ((buf->pnext->value - (L'a' - 1)) < 1 ||
                 buf->pnext->value > 26)) result = MOVEMENT_CMD;

        else if (buf->value == L'c' && bs == 2 &&    // mark
               ((buf->pnext->value - (L'a' - 1)) < 1 || buf->pnext->value > 26)) result = EDITION_CMD;

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
               ) result = MOVEMENT_CMD;

        else if (buf->value == L'V' && bs == 3 &&    // Vir
                 buf->pnext->value == L'i' &&
                 buf->pnext->pnext->value == L'r')
                 result = MOVEMENT_CMD;

        else if (buf->value == L'd' && bs == 2 &&    // cuts a cell
                 buf->pnext->value == L'd') result = EDITION_CMD;

        else if (buf->value == L'\'' && bs == 2 &&   // tick
               ((buf->pnext->value - (L'a' - 1)) < 1 ||
                 buf->pnext->value > 26)) result = MOVEMENT_CMD;

        else if (buf->value == L's' && bs == 2 &&    // shift cell down or up
               ( buf->pnext->value == L'j' ||
                 buf->pnext->value == L'k' ||
                 buf->pnext->value == L'h' ||
                 buf->pnext->value == L'l')) result = EDITION_CMD;

        else if (buf->value == L'i' && bs == 2 &&    // Insert row or column
               ( buf->pnext->value == L'r' ||
                 buf->pnext->value == L'c' )) result = EDITION_CMD;

        else if (buf->value == L'o' && bs == 2 &&    // Open row or column
               ( buf->pnext->value == L'r' ||
                 buf->pnext->value == L'c' )) result = EDITION_CMD;

        else if (buf->value == L'd' && bs == 2 &&    // Delete row or column
               ( buf->pnext->value == L'r' ||
                 buf->pnext->value == L'c' )) result = EDITION_CMD;

        else if (buf->value == L'r' && bs == 2 &&    // range lock / unlock / valueize
               ( buf->pnext->value == L'l' ||
                 buf->pnext->value == L'u' ||
                 buf->pnext->value == L'v' )) result = EDITION_CMD;

        else if (buf->value == L'R' && bs == 3 &&    // Create range with two marks
            //  FIXME add validation
               ((buf->pnext->value - (L'a' - 1)) < 1 ||
                 buf->pnext->value > 26) &&
               ((buf->pnext->pnext->value - (L'a' - 1)) < 1 ||
                 buf->pnext->pnext->value > 26)) result = EDITION_CMD;

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
                 buf->pnext->value == L'+' ||
                 buf->pnext->value == L'r' ||         // Freeze row / col / area
                 buf->pnext->value == L'c' ||
                 buf->pnext->value == L'a'
                 )
               ) result = EDITION_CMD;

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
                 buf->value == L'p' ||
                 buf->value == L'P' ||
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
                 result = MOVEMENT_CMD;
        else if (buf->value == L'{' ||
                 buf->value == L'}' ||
                 buf->value == L'f' ||
                 buf->value == L'|')
                 result = EDITION_CMD;

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
                 result = MOVEMENT_CMD;
            } else if ((buf->value == L'Z' && (
                 buf->pnext->value == L'r' ||
                 buf->pnext->value == L'c' )) ||
                (buf->value == L'S' && (
                 buf->pnext->value == L'r' ||
                 buf->pnext->value == L'c' )) ) {
                 result = EDITION_CMD;
            } else if (buf->value == L'r' && (
                 buf->pnext->value == L'l' ||
                 buf->pnext->value == L'u' ||
                 buf->pnext->value == L'v' )) {
                 result = EDITION_CMD;
            } else if (buf->value == L'm' && // mark
               ((buf->pnext->value - (L'a' - 1)) < 1 ||
                 buf->pnext->value > 26)) {
                 result = MOVEMENT_CMD;
            }
    }
    return result;
}
