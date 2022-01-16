/*******************************************************************************
 * Copyright (c) 2013-2021, Andrés Martinelli <andmarti@gmail.com>             *
 * All rights reserved.                                                        *
 *                                                                             *
 * This file is a part of sc-im                                                *
 *                                                                             *
 * sc-im is a spreadsheet program that is based on sc. The original authors    *
 * of sc are James Gosling and Mark Weiser, and mods were later added by       *
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
#include <limits.h>
#include "cmds.h"
#include "../main.h"
#include "../maps.h"
#include "../yank.h"
#include "../marks.h"
#include "../buffer.h"
#include "../tui.h"
#include "../conf.h"    // for conf parameters
#include "../xmalloc.h" // for scxfree
#include "../vmtbl.h"   // for growtbl
#include "../utils/string.h" // for add_char
#include "../y.tab.h"   // for yyparse
#include "../graph.h"
#ifdef UNDO
#include "../undo.h"
#endif

extern struct session * session;
extern int shall_quit;
extern graphADT graph;
extern int yyparse(void);

char insert_edit_submode;
wchar_t interp_line[BUFFERSIZE];
struct ent * freeents = NULL;    // keep deleted ents around before sync_refs


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
void mark_ent_as_deleted(struct ent * p, int delete) {
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
 * TODO: any malloc of ents should check here before asking for memory
 *
 * \return none
 */
void flush_saved() {
    struct ent * p;
    struct ent * q;

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
 * \brief sync_refs()
 * \details Used to remove references to deleted struct ents.
 * \details Note that the deleted structure must still be hanging
 * around before the call, but not referenced by an entry in tbl.
 * \return none
 */
// TODO Improve this function such that it does not traverse the whole table. Use the graph.
void sync_refs(struct sheet * sh) {
    int i, j;
    struct ent * p;
    for (i=0; i <= sh->maxrow; i++)
        for (j=0; j <= sh->maxcol; j++)
            if ( (p = *ATBL(sh, sh->tbl, i, j)) && p->expr ) {
                syncref(sh, p->expr);
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
void syncref(struct sheet * sh, struct enode * e) {
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
        //e->e.r.right.vp = lookat(e->e.r.right.sheet, e->e.r.right.vp->row, e->e.r.right.vp->col);
        e->e.r.right.vp = lookat(sh, e->e.r.right.vp->row, e->e.r.right.vp->col);
        // e->e.r.left.vp = lookat(e->e.r.left.sheet, e->e.r.left.vp->row, e->e.r.left.vp->col);
        e->e.r.left.vp = lookat(sh, e->e.r.left.vp->row, e->e.r.left.vp->col);
    } else {
        switch (e->op) {
        case 'v':
            if (e->e.v.vp->flags & is_deleted) {
                //e->op = ERR_;
                //e->e.o.left = NULL;
                //e->e.o.right = NULL;
                break;
            } else if (e->e.v.vp->flags & may_sync)
                e->e.v.vp = lookat(e->e.v.sheet == NULL ? sh : e->e.v.sheet, e->e.v.vp->row, e->e.v.vp->col);
            break;
        case 'k':
            break;
        case '$':
            break;
        default:
            syncref(sh, e->e.o.right);
            syncref(sh, e->e.o.left);
            break;
        }
    }
    return;
}


/**
 * \brief deletecol()
 *
 * \param[in] struct sheet * sh
 * \param[in] col
 * \param[in] mult
 *
 * \return none
 */
void deletecol(struct sheet * sh, int col, int mult) {

    struct roman * roman = session->cur_doc;

    //if (col - 1 + mult >= sh->maxcols) {
    if (col - 1 + mult > sh->maxcol) {
        sc_error("current column + multiplier exceeds max column. Nothing changed");
        return;
    } else if (any_locked_cells(sh, 0, col, sh->maxrow, col - 1 + mult)) {
        sc_error("Locked cells encountered. Nothing changed");
        return;
    }
#ifdef UNDO
    create_undo_action();
    // here we save in undostruct, all the ents that depends on the deleted one (before change)
    ents_that_depends_on_range(sh, 0, col, sh->maxrow, col - 1 + mult);
    copy_to_undostruct(sh, 0, col, sh->maxrow, col - 1 + mult, UNDO_DEL, HANDLE_DEPS, NULL);
    save_undo_range_shift(0, -mult, 0, col, sh->maxrow, col - 1 + mult);

    int i;
    for (i=col; i < col + mult; i++) {
        add_undo_col_format(i, 'R', sh->fwidth[i], sh->precision[i], sh->realfmt[i]);
        if (sh->col_hidden[i]) undo_hide_show(-1, i, 's', 1);
        else undo_hide_show(-1, i, 'h', 1);
        // TODO undo col_frozen
    }
#endif

    fix_marks(sh, 0, -mult, 0, sh->maxrow,  col + mult -1, sh->maxcol);
    if (! roman->loading) yank_area(sh, 0, col, sh->maxrow, col + mult - 1, 'c', mult);

    // do the job
    int_deletecol(sh, col, mult);

    // if (get_conf_int("autocalc")) EvalAll();

    if (! roman->loading) roman->modflg++;

#ifdef UNDO
    // here we save in undostruct, just the ents that depends on the deleted one (after change)
    copy_to_undostruct(sh, 0, 0, -1, -1, UNDO_ADD, HANDLE_DEPS, NULL);

    extern struct ent_ptr * deps;
    if (deps != NULL) free(deps);
    deps = NULL;

    end_undo_action();
#endif
    return;
}


/**
 * \brief int_deletecol()
 *
 * \details Delete a column. Parameters col = column to delete
 * multi = cmds multiplier. (commonly 1)
 *
 * \param[in] struct sheet * sh
 * \param[in] col
 * \param[in] mult
 *
 * \return none
 */
void int_deletecol(struct sheet * sh, int col, int mult) {
    struct ent ** pp;
    int r, c, i;

    while (mult--) {
        /* mark ent of column to erase with is_deleted flag
        for (r = 0; r <= sh->maxrow; r++) {
            pp = ATBL(sh, sh->tbl, r, col);
            if ( *pp != NULL ) {
                mark_ent_as_deleted(*pp, TRUE);
                //clearent(*pp);
                //free(*pp);
                *pp = NULL;
            }
        }*/
        erase_area(sh, 0, col, sh->maxrow, col, 0, 1); //important: this mark the ents as deleted

        rebuild_graph(); // Rebuild of graph is needed.
        //But shouldnt! TODO

        // Copy references from right column cells to left column (which gets removed)
        for (r = 0; r <= sh->maxrow; r++) {
            for (c = col; c < sh->maxcol; c++) {
                pp = ATBL(sh, sh->tbl, r, c);

                // nota: pp[1] = ATBL(sh, sh->tbl, r, c+1);
                pp[0] = pp[1];
                if ( pp[0] != NULL ) pp[0]->col--;
            }

            // Free last column memory (Could also initialize 'ent' to zero with `cleanent`).
            pp = ATBL(sh, sh->tbl, r, sh->maxcol);
            *pp = (struct ent *) 0;
        }

        // Fix columns precision and width
        for (i = col; i < sh->maxcols - 1; i++) {
            sh->fwidth[i] = sh->fwidth[i+1];
            sh->precision[i] = sh->precision[i+1];
            sh->realfmt[i] = sh->realfmt[i+1];
            sh->col_hidden[i] = sh->col_hidden[i+1];
            sh->col_frozen[i] = sh->col_frozen[i+1];
        }

        for (; i < sh->maxcols - 1; i++) {
            sh->fwidth[i] = DEFWIDTH;
            sh->precision[i] = DEFPREC;
            sh->realfmt[i] = DEFREFMT;
            sh->col_hidden[i] = FALSE;
            sh->col_frozen[i] = FALSE;
        }

        if (sh->maxcol) sh->maxcol--;
        sync_refs(sh);
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
 *
 * struct ent * n should be already alloc'ed
 */
void copyent(struct ent * n, struct sheet * sh_p, struct ent * p, int dr, int dc, int r1, int c1, int r2, int c2, int special) {
    if (!p) {
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
            n->expr = copye(p->expr, sh_p, dr, dc, r1, c1, r2, c2, special == 't');
#ifdef UNDO
        } else if (special == 'u' && p->expr) { // from spreadsheet to undo
            n->expr = copye(p->expr, sh_p, dr, dc, r1, c1, r2, c2, 2);
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
    if (p->label && special == 'f' && n->label) {
        n->flags &= ~is_leftflush;
        n->flags |= ((p->flags & is_label) | (p->flags & is_leftflush));
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

    if (special == 'c' && n->expr) {
        // sc_debug("reeval in copyent");
        EvalJustOneVertex(sh_p, n, 0);
    }

    n->flags |= is_changed;
    n->row = p->row;
    n->col = p->col;
    return;
}


/**
 * \brief etype(): return type of an enode
 * \return NUM; STR; etc.
 */
int etype(struct enode *e) {
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
            struct ent *p;
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
 * \param[in] struct sheet * sh
 * \param[in] sr
 * \param[in] sc
 * \param[in] er
 * \param[in] ec
 * \param[in] ignorelock
 * \param[in] mark_as_deleted
 * \return none
 */
void erase_area(struct sheet * sh, int sr, int sc, int er, int ec, int ignorelock, int mark_as_deleted) {
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
    checkbounds(sh, &er, &ec);

    /* mark the ent as deleted
     * Do a lookat() for the upper left and lower right cells of the range
     * being erased to make sure they are included in the delete buffer so
     * that pulling cells always works correctly even if the cells at one
     * or more edges of the range are all empty.
     */
    (void) lookat(sh, sr, sc);
    (void) lookat(sh, er, ec);
    for (r = sr; r <= er; r++) {
        for (c = sc; c <= ec; c++) {
            pp = ATBL(sh, sh->tbl, r, c);
            if (*pp && (!((*pp)->flags & is_locked) || ignorelock)) {

                /* delete vertex in graph
                   only if this vertex is not referenced by other */
                vertexT * v = getVertex(graph, sh, *pp, 0);
                if (v != NULL && v->back_edges == NULL )
                    destroy_vertex(sh, *pp);

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
 * \brief copye()
 * \details Function used to get a copy of an expression.
 * \param[in] e: expression to be copied
 * \param[in] Rdelta
 * \param[in] Cdelta
 * \param[in] r1
 * \param[in] c1
 * \param[in] r2
 * \param[in] c2
 * \param[in] special
 * special = 1 means transpose
 * special = 2 means copy from spreadsheet to undo struct
 * \return struct enode *
 */
struct enode * copye(struct enode *e, struct sheet * sh, int Rdelta, int Cdelta, int r1, int c1, int r2, int c2, int special) {
    struct enode * ret;
    static struct enode * range = NULL;

    if (e == NULL) {
        ret = (struct enode *) 0;

    } else if (e->op & REDUCE) {
        int newrow, newcol;
        ret = (struct enode *) scxmalloc((unsigned) sizeof (struct enode));
        ret->op = e->op;
        //ret->e.s = NULL;
        //ret->e.o.s = NULL;
        ret->e.r.left.expr = e->e.r.left.expr ? copye(e->e.r.left.expr, e->e.r.left.sheet, Rdelta, Cdelta, r1, c1, r2, c2, special) : NULL; // important to initialize
        ret->e.r.right.expr = e->e.r.right.expr ? copye(e->e.r.right.expr, e->e.r.right.sheet, Rdelta, Cdelta, r1, c1, r2, c2, special) : NULL; // important to initialize
        newrow = e->e.r.left.vf & FIX_ROW || e->e.r.left.vp->row < r1 || e->e.r.left.vp->row > r2 || e->e.r.left.vp->col < c1 || e->e.r.left.vp->col > c2 ?  e->e.r.left.vp->row : special == 1 ? r1 + Rdelta + e->e.r.left.vp->col - c1 : e->e.r.left.vp->row + Rdelta;
        newcol = e->e.r.left.vf & FIX_COL || e->e.r.left.vp->row < r1 || e->e.r.left.vp->row > r2 || e->e.r.left.vp->col < c1 || e->e.r.left.vp->col > c2 ?  e->e.r.left.vp->col : special == 1 ? c1 + Cdelta + e->e.r.left.vp->row - r1 : e->e.r.left.vp->col + Cdelta;
        ret->e.r.left.vp = lookat(sh, newrow, newcol);
        ret->e.r.left.vf = e->e.r.left.vf;
        ret->e.r.left.sheet = e->e.r.left.sheet;
        newrow = e->e.r.right.vf & FIX_ROW || e->e.r.right.vp->row < r1 || e->e.r.right.vp->row > r2 || e->e.r.right.vp->col < c1 || e->e.r.right.vp->col > c2 ?  e->e.r.right.vp->row : special == 1 ? r1 + Rdelta + e->e.r.right.vp->col - c1 : e->e.r.right.vp->row + Rdelta;
        newcol = e->e.r.right.vf & FIX_COL || e->e.r.right.vp->row < r1 || e->e.r.right.vp->row > r2 || e->e.r.right.vp->col < c1 || e->e.r.right.vp->col > c2 ?  e->e.r.right.vp->col : special == 1 ? c1 + Cdelta + e->e.r.right.vp->row - r1 : e->e.r.right.vp->col + Cdelta;
        ret->e.r.right.vp = lookat(sh, newrow, newcol);
        ret->e.r.right.vf = e->e.r.right.vf;
        ret->e.r.right.sheet = e->e.r.right.sheet;
    } else {
        struct enode *temprange=0;
        ret = (struct enode *) scxmalloc((unsigned) sizeof (struct enode));
        ret->op = e->op;
        ret->e.s = NULL;
        ret->e.o.s = NULL;
        //ret->e.r.left.expr  = copye(e->e.r.left.expr,  e->e.r.left.sheet, Rdelta, Cdelta, r1, c1, r2, c2, special);
        //ret->e.r.right.expr = copye(e->e.r.right.expr, e->e.r.right.sheet, Rdelta, Cdelta, r1, c1, r2, c2, special);
        //ret->e.r.left.expr  = NULL;
        //ret->e.r.right.expr = NULL;
        ret->e.r.left.vp = e->e.r.left.vp;
        ret->e.r.right.vp = e->e.r.right.vp;
        ret->e.r.left.sheet = e->e.r.left.sheet;
        ret->e.r.right.sheet = e->e.r.right.sheet;
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
                r2 = sh->maxrow;
                c2 = sh->maxcol;
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
                    ret->e.v.vp = lookat(e->e.v.sheet != NULL ? e->e.v.sheet : sh, newrow, newcol);
                    ret->e.v.vf = e->e.v.vf;
                    ret->e.v.sheet = e->e.v.sheet;
                    break;
                }
            case 'k':
                ret->e.k = e->e.k;
                break;
            case 'f':
            case 'F':
                if ((range && ret->op == 'F') || (!range && ret->op == 'f'))
                    Rdelta = Cdelta = 0;
                ret->e.o.left = copye(e->e.o.left, sh, Rdelta, Cdelta, r1, c1, r2, c2, special);
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
                ret->e.o.left = copye(e->e.o.left, e->e.v.sheet != NULL ? e->e.v.sheet : sh, Rdelta, Cdelta, r1, c1, r2, c2, special);
                ret->e.o.right = copye(e->e.o.right, e->e.v.sheet != NULL ? e->e.v.sheet : sh, Rdelta, Cdelta, r1, c1, r2, c2, special);
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
void dorowformat(struct sheet * sh, int r, unsigned char size) {
    struct roman * roman = session->cur_doc;
    if (size < 1 || size > UCHAR_MAX || size > SC_DISPLAY_ROWS) { sc_error("Invalid row format"); return; }

    if (r >= sh->maxrows && !growtbl(sh, GROWROW, 0, r)) r = sh->maxrows-1 ;
    checkbounds(sh, &r, &(sh->curcol));
    sh->row_format[r] = size;
    if (! roman->loading) roman->modflg++;
    return;
}


/**
 * \brief doformat()
 * \details Note: Modified 9/17/90 THA to handle more formats.
 * \param[in] struct sheet * sh
 * \param[in] c1
 * \param[in] c2
 * \param[in] w
 * \param[in] p
 * \param[in] r
 * \return none
 */
void doformat(struct sheet * sh, int c1, int c2, int w, int p, int r) {
    struct roman * roman = session->cur_doc;
    int i;
    int crows = 0;
    int ccols = c2;

    if (c1 >= sh->maxcols && !growtbl(sh, GROWCOL, 0, c1)) c1 = sh->maxcols-1 ;
    if (c2 >= sh->maxcols && !growtbl(sh, GROWCOL, 0, c2)) c2 = sh->maxcols-1 ;

    if (w == 0) {
        sc_info("Width too small - setting to 1");
        w = 1;
    }

    if (! get_conf_int("nocurses") && w > SC_DISPLAY_COLS - 2) {
        sc_info("Width too large - Maximum = %d", SC_DISPLAY_COLS - 2);
        w = SC_DISPLAY_COLS - 2;
    }

    if (p > w) {
        sc_info("Precision too large");
        p = w;
    }

    checkbounds(sh, &crows, &ccols);
    if (ccols < c2) {
        sc_error("Format statement failed to create implied column %d", c2);
        return;
    }

    for (i = c1; i <= c2; i++)
        sh->fwidth[i] = w, sh->precision[i] = p, sh->realfmt[i] = r;

    roman->modflg++;
    return;

}


/**
 * \brief formatcol()
 * \param[in] struct sheet * sh
 * \param[in] c
 * \return none
 */
void formatcol(struct sheet * sh, int c) {
    struct roman * roman = session->cur_doc;
    int arg = 1;
    int i;

    switch (c) {
        case '<':
        case 'h':
        case OKEY_LEFT:
            for (i = sh->curcol; i < sh->curcol + arg; i++) {
                if (sh->fwidth[i] <= 2) {
                    sc_error("Cannot resize column any longer");
                    return;
                }
                sh->fwidth[i]--;
                if (sh->fwidth[i] <= 1)
                    sh->fwidth[i] = 1;
            }
            roman->modflg++;
            break;
        case '>':
        case 'l':
        case OKEY_RIGHT:
            for (i = sh->curcol; i < sh->curcol + arg; i++) {
                sh->fwidth[i]++;
                if (sh->fwidth[i] > SC_DISPLAY_COLS - 2)
                    sh->fwidth[i] = SC_DISPLAY_COLS - 2;
            }
            roman->modflg++;
            break;
        case '-':
            for (i = sh->curcol; i < sh->curcol + arg; i++) {
                sh->precision[i]--;
                if (sh->precision[i] < 0)
                    sh->precision[i] = 0;
            }
            roman->modflg++;
            break;
        case '+':
            for (i = sh->curcol; i < sh->curcol + arg; i++)
                sh->precision[i]++;
            roman->modflg++;
            break;
    }
    sc_info("Current format is %d %d %d", sh->fwidth[sh->curcol], sh->precision[sh->curcol], sh->realfmt[sh->curcol]);
    ui_update(TRUE);
    return;
}


/**
 * \brief insert_row()
 * \details Insert a single row. It will be inserted before currow
 * if after is 0. After if it is 1.
 * \param[in] struct sheet * sh
 * \param[in] after
 * \returnsnone
 */
void insert_row(struct sheet * sh, int after) {
    int r, c;
    struct ent ** tmprow, ** pp, ** qq;
    struct ent * p;
    int lim = sh->maxrow - sh->currow + 1;

    if (sh->currow > sh->maxrow) sh->maxrow = sh->currow;
    sh->maxrow++;
    lim = sh->maxrow - lim + after;
    if (sh->maxrow >= sh->maxrows && ! growtbl(sh, GROWROW, sh->maxrow, 0)) {
        sc_error("cannot grow sheet larger");
        return;
    }

    tmprow = sh->tbl[sh->maxrow];
    for (r = sh->maxrow; r > lim; r--) {
        sh->row_hidden[r] = sh->row_hidden[r-1];
        sh->row_frozen[r] = sh->row_frozen[r-1];
        sh->row_format[r] = sh->row_format[r-1];
        sh->tbl[r] = sh->tbl[r-1];
        for (c = 0, pp = ATBL(sh, sh->tbl, r, 0); c < sh->maxcols; c++, pp++)
            if (*pp) (*pp)->row = r;
    }
    sh->tbl[r] = tmprow;        // the last row is never used
    sh->row_format[r] = 1;

    // if padding exists in the old currow, we copy it to the new row!
    for (c = 0; c < sh->maxcols; c++) {
        if (r >= 0 && (qq = ATBL(sh, sh->tbl, r+1, c)) && (*qq) && (*qq)->pad) {
            p = lookat(sh, r, c);
            p->pad = (*qq)->pad;
        }
    }

    // TODO pass roman as parameter
    session->cur_doc->modflg++;
    return;
}


/**
 * \brief insert_col()
 * \details Insert a single column. The column will be inserted
 * BEFORE CURCOL if after is 0;
 * AFTER CURCOL if it is 1.
 * \param[in] struct sheet * sh
 * \param[in] after
 * \return none
 */
void insert_col(struct sheet * sh, int after) {
    struct roman * roman = session->cur_doc;
    int r, c;
    struct ent ** pp, ** qq;
    struct ent * p;
    int lim = sh->maxcol - sh->curcol - after + 1;

    if (sh->curcol + after > sh->maxcol)
        sh->maxcol = sh->curcol + after;
    sh->maxcol++;

    if ((sh->maxcol >= sh->maxcols) && !growtbl(sh, GROWCOL, 0, sh->maxcol)) {
        sc_error("cannot grow sheet wider");
        return;
    }

    for (c = sh->maxcol; c >= sh->curcol + after + 1; c--) {
        sh->fwidth[c] = sh->fwidth[c-1];
        sh->precision[c] = sh->precision[c-1];
        sh->realfmt[c] = sh->realfmt[c-1];
        sh->col_hidden[c] = sh->col_hidden[c-1];
        sh->col_frozen[c] = sh->col_frozen[c-1];
    }
    for (c = sh->curcol + after; c - sh->curcol - after < 1; c++) {
        sh->fwidth[c] = DEFWIDTH;
        sh->precision[c] =  DEFPREC;
        sh->realfmt[c] = DEFREFMT;
        sh->col_hidden[c] = FALSE;
        sh->col_frozen[c] = FALSE;
    }

    for (r=0; r <= sh->maxrow; r++) {
        pp = ATBL(sh, sh->tbl, r, sh->maxcol);
        for (c = lim; --c >= 0; pp--)
            if ((pp[0] = pp[-1])) pp[0]->col++;

        pp = ATBL(sh, sh->tbl, r, sh->curcol + after);
        for (c = sh->curcol + after; c - sh->curcol - after < 1; c++, pp++)
            *pp = (struct ent *) 0;
    }

    // if padding exists in the old curcol, we copy it to the new col!
    for (r = 0; r < sh->maxrows; r++) {
        if (c >= 0 && (qq = ATBL(sh, sh->tbl, r, c+1)) && (*qq) && (*qq)->pad) {
            p = lookat(sh, r, c);
            p->pad = (*qq)->pad;
        }
    }

    sh->curcol += after;
    roman->modflg++;
    return;
}


/**
 * \brief deleterow()
 * \details Delete a row
 * \param[in] struct sheet * sh
 * \param[in] row
 * \param[in] mult
 * \return none
 */
void deleterow(struct sheet * sh, int row, int mult) {
    struct roman * roman = session->cur_doc;
    //if (row + mult - 1 >= sh->maxrows) {
    if (row + mult - 1 > sh->maxrow) {
        sc_error("current row + multiplier exceeds max row. Nothing changed");
        return;
    } else if (any_locked_cells(sh, row, 0, row + mult - 1, sh->maxcol)) {
        sc_error("Locked cells encountered. Nothing changed");
        return;
    }
#ifdef UNDO
    create_undo_action();
    // here we save in undostruct, all the ents that depends on the deleted one (before change)
    ents_that_depends_on_range(sh, row, 0, row + mult - 1, sh->maxcol);
    copy_to_undostruct(sh, row, 0, row + mult - 1, sh->maxcol, UNDO_DEL, HANDLE_DEPS, NULL);
    save_undo_range_shift(-mult, 0, row, 0, row - 1 + mult, sh->maxcol);
    int i;
    for (i=row; i < row + mult; i++) {
        add_undo_row_format(i, 'R', sh->row_format[sh->currow]);
        if (sh->row_hidden[i]) undo_hide_show(i, -1, 's', 1);
        //else undo_hide_show(i, -1, 'h', 1);
    }
#endif

    fix_marks(sh, -mult, 0, row + mult - 1, sh->maxrow, 0, sh->maxcol);
    if (! roman->loading) yank_area(sh, row, 0, row + mult - 1, sh->maxcol, 'r', mult);

    // do the job
    int_deleterow(sh, row, mult);

    //flush_saved(); // we have to flush only at exit. this is in case we want to UNDO

    if (! roman->loading) roman->modflg++;

#ifdef UNDO
    // here we save in undostruct, just the ents that depends on the deleted one (after the change)
    copy_to_undostruct(sh, 0, 0, -1, -1, UNDO_ADD, HANDLE_DEPS, NULL);

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
 * \param[in] struct sheet * sh
 * \param[in] int row - row to delete
 * \param[in] int multi - command multiplier (usually 1)
 * \return none
 */
void int_deleterow(struct sheet * sh, int row, int mult) {
    struct ent ** pp;
    struct ent * q;
    int r, c;

    //if (sh->currow > sh->maxrow) return;

    while (mult--) {
        // we need to decrease row of the row that we delete if
        // other cells refers to this one.
        for (c = 0; c < sh->maxcols; c++) {
            if (row <= sh->maxrow) {
                pp = ATBL(sh, sh->tbl, row, c);
                if ((q = *ATBL(sh, sh->tbl, row, c)) != NULL && q->row > 0) q->row--;
            }
        }
        sync_refs(sh);

        // and after that the erase_area of the deleted row
        erase_area(sh, row, 0, row, sh->maxcol, 0, 1); //important: this mark the ents as deleted

        // and we decrease ->row of all rows after the deleted one
        for (r = row; r < sh->maxrows - 1; r++) {
            for (c = 0; c < sh->maxcols; c++) {
                if (r <= sh->maxrow) {
                    pp = ATBL(sh, sh->tbl, r, c);
                    pp[0] = *ATBL(sh, sh->tbl, r+1, c);
                    if ( pp[0] ) pp[0]->row--;
                }
            }
            //update row_hidden and row_format here
            sh->row_hidden[r] = sh->row_hidden[r+1];
            sh->row_frozen[r] = sh->row_frozen[r+1];
            sh->row_format[r] = sh->row_format[r+1];
        }

        rebuild_graph(); //TODO CHECK HERE WHY REBUILD IS NEEDED. See NOTE1 in shift.c
        sync_refs(sh);
        EvalAll();
        if (sh->maxrow) sh->maxrow--;
    }
    return;
}


/**
 * \brief ljustify()
 * \param[in] struct sheet * sh
 * \param[in] sr
 * \param[in] sc
 * \param[in] er
 * \param[in] ec
 * \return none
 */
void ljustify(struct sheet * sh, int sr, int sc, int er, int ec) {
    struct roman * roman = session->cur_doc;
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
            p = *ATBL(sh, sh->tbl, i, j);
            if (p && p->label) {
                p->flags &= ~is_label;
                p->flags |= is_leftflush | is_changed;
                roman->modflg++;
            }
        }
    }
    return;
}


/**
 * \brief rjustify()
 * \param[in] struct sheet * sh
 * \param[in] sr
 * \param[in] sc
 * \param[in] er
 * \param[in] ec
 * \return none
 */
void rjustify(struct sheet * sh, int sr, int sc, int er, int ec) {
    struct roman * roman = session->cur_doc;
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
            p = *ATBL(sh, sh->tbl, i, j);
            if (p && p->label) {
                p->flags &= ~(is_label | is_leftflush);
                p->flags |= is_changed;
                roman->modflg++;
            }
        }
    }
    return;
}


/**
 * \brief center()
 * \param[in] struct sheet * sh
 * \param[in] sr
 * \param[in] sc
 * \param[in] er
 * \param[in] ec
 * \return none
 */
void center(struct sheet * sh, int sr, int sc, int er, int ec) {
    struct roman * roman = session->cur_doc;
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
            p = *ATBL(sh, sh->tbl, i, j);
            if (p && p->label) {
                p->flags &= ~is_leftflush;
                p->flags |= is_label | is_changed;
                roman->modflg++;
            }
        }
    }
    return;
}


/**
 * @brief chg_mode()
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
 * \brief del_selected_cells()
 * \details Delete selected cell or range of cells.
 * \param[in] struct sheet * sh
 * \return none
 */
void del_selected_cells(struct sheet * sh) {
    struct roman * roman = session->cur_doc;
    int tlrow = sh->currow;
    int tlcol = sh->curcol;
    int brrow = sh->currow;
    int brcol = sh->curcol;
    extern struct ent_ptr * deps;

    // is we delete a range
    if (is_range_selected() != -1) {
        srange * r = get_selected_range();
        tlrow = r->tlrow;
        tlcol = r->tlcol;
        brrow = r->brrow;
        brcol = r->brcol;
    }

    if (any_locked_cells(sh, tlrow, tlcol, brrow, brcol)) {
        sc_error("Locked cells encountered. Nothing changed");
        return;
    }

    if (is_range_selected() != -1)
        yank_area(sh, tlrow, tlcol, brrow, brcol, 'a', 1);
    else
        yank_area(sh, tlrow, tlcol, brrow, brcol, 'e', 1);

#ifdef UNDO
    create_undo_action();
    // here we save in undostruct, all the ents that depends on the deleted one (before change)
    ents_that_depends_on_range(sh, tlrow, tlcol, brrow, brcol);
    copy_to_undostruct(sh, tlrow, tlcol, brrow, brcol, UNDO_DEL, HANDLE_DEPS, NULL);
    if (deps != NULL) { free(deps); deps = NULL; }
#endif

    erase_area(sh, tlrow, tlcol, brrow, brcol, 0, 0); //important: this erases the ents, but does NOT mark them as deleted
    roman->modflg++;
    sync_refs(sh);
    //flush_saved(); DO NOT UNCOMMENT! flush_saved shall not be called other than at exit.

    /* TODO
     * EvalRange() cleans deps so it can eval.
     * we should keep it so me can copy to undostruct later on
     * Here, we might want to make that free of deps inside EvalRange optional as a parameter
     */
    //EvalRange(sh, tlrow, tlcol, brrow, brcol);
    EvalAll();

#ifdef UNDO
    // here we save in undostruct, all the ents that depends on the deleted one (after the change)
    ents_that_depends_on_range(sh, tlrow, tlcol, brrow, brcol);
    copy_to_undostruct(sh, tlrow, tlcol, brrow, brcol, UNDO_ADD, HANDLE_DEPS, NULL);
    if (deps != NULL) { free(deps); deps = NULL; }
    end_undo_action();
#endif

    return;
}


/**
 * \brief enter_cell_content()
 * \details Enter cell content on a cell.
 * Covers commands LET, LABEL, LEFTSTRING, and RIGHTSTRING
 * \param[in] struct sheet * sh
 * \param[in] r
 * \param[in] c
 * \param[in] submode
 * \param[in] content
 * \return none
 */
void enter_cell_content(struct sheet * sh, int r, int c, char * submode,  wchar_t * content) {
    (void) swprintf(interp_line, BUFFERSIZE, L"%s %s = %ls", submode, v_name(r, c), content);
    send_to_interp(interp_line);
    if (get_conf_int("autocalc") && ! session->cur_doc->loading) EvalRange(sh, r, c, r, c);
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
    // commented on 28/04/2021. EvalAll should be used only on certain ocasions.
    // Use EvalRange instead when possible, and certainly not here everytime sending to interp.
    //if (get_conf_int("autocalc") && ! roman->loading) EvalAll();
    return;
}


/**
 * \brief lookat()
 * \details Return a pointer to a cell's [struct ent *], creating if needed
 * \param[in] struct sheet * sh
 * \param[in] row
 * \param[in] col
 * \return none
 */
struct ent * lookat(struct sheet * sh, int row, int col) {
    struct ent **pp;

    checkbounds(sh, &row, &col);
    pp = ATBL(sh, sh->tbl, row, col);
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
    if (row > sh->maxrow) sh->maxrow = row;
    if (col > sh->maxcol) sh->maxcol = col;
    return (*pp);
}


/**
 * \brief cleanent()
 * \details Blank an ent
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
 * \brief clearent()
 * \details Free memory of an ent and its contents
 * \param[in] v
 * \return none
 */
void clearent(struct ent * v) {
    struct roman * roman = session->cur_doc;
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

    roman->modflg++;
    return;
}


/**
 * \brief back_col()
 * \details Moves curcol back one displayed column
 * \param[in] struct sheet * sh
 * \param[in] arg
 * \return lookat
 */
struct ent * back_col(struct sheet * sh, int arg) {
    int c = sh->curcol;

    while (--arg >= 0) {
        if (c) {
            sh->curcol = --c;
        } else {
            sc_info ("At column A");
            break;
        }
        while (sh->col_hidden[c] && c) {
            sh->curcol = --c;
        }
    }
    if (sh->curcol < sh->offscr_sc_cols)
        sh->offscr_sc_cols = sh->curcol;

    return lookat(sh, sh->currow, c);
}


/**
 * \brief forw_col()
 * \details Moves curcol forward one displayed column
 * \param[in] struct sheet * sh
 * \param[in] arg
 * \return lookat
 */
struct ent * forw_col(struct sheet * sh, int arg) {
    int c = sh->curcol;

    while (--arg >= 0) {
        if (c < sh->maxcols - 1) {
            c++;
        } else {
            if (! growtbl(sh, GROWCOL, 0, arg)) {    /* get as much as needed */
                sc_error("cannot grow sheet wider");
                return lookat(sh, sh->currow, sh->curcol);
            } else {
                c++;
            }
        }
        while (sh->col_hidden[c] && c < sh->maxcols - 1) {
            c++;
        }

    }
    return lookat(sh, sh->currow, c);
}


/**
 * \brief forw_row()
 * \details Move currow forward one displayed row
 * \param[in] struct sheet * sh
 * \param[in] arg
 * \return lookat
 */
struct ent * forw_row(struct sheet * sh, int arg) {
    int r = sh->currow;

    while (arg--) {
        if (r < sh->maxrows - 1)
            r++;
        else {
            if (! growtbl(sh, GROWROW, arg, 0)) {
                sc_error("cannot grow sheet larger");
                return lookat(sh, sh->currow, sh->curcol);
            } else
                r++;
        }
        while (sh->row_hidden[r] && r < sh->maxrows - 1) r++;
    }
    return lookat(sh, r, sh->curcol);
}


/**
 * \brief back_row()
 * \details Moves currow backward on displayed row
 * \param[in] struct sheet * sh
 * \return lookat
 */
struct ent * back_row(struct sheet * sh, int arg) {
    int r = sh->currow;

    while (--arg >= 0) {
        if (r) {
            --r;
        } else {
            sc_info ("At row 0");
            break;
        }
        while (sh->row_hidden[r] && r) --r;
    }
    return lookat(sh, r, sh->curcol);
}


/**
 * \brief scroll_down()
 * \param[in] struct sheet * sh
 * \param[in] n
 * \return none
 */
void scroll_down(struct sheet * sh, int n) {
    while (n--) {
        int last, currow_orig = sh->currow;
        /* find last mobile row */
        calc_mobile_rows(sh, &last);
        /* move to next non-hidden non-frozen row */
        do {
            lookat(sh, ++last, sh->curcol);
            if (last >= sh->maxrows)
                return;
        } while (sh->row_hidden[last] || sh->row_frozen[last]);
        /* this will adjust offscr_sc_rows */
        sh->currow = last;
        calc_mobile_rows(sh, NULL);
        /* restore currow */
        sh->currow = currow_orig;
        if (sh->currow < sh->offscr_sc_rows)
            sh->currow = sh->offscr_sc_rows;
        unselect_ranges();
    }
}


/**
 * \brief scroll_up()
 * \param[in] struct sheet * sh
 * \param[in] n
 * \return none
 */
void scroll_up(struct sheet * sh, int n) {
    while (n--) {
        int first, last, currow_orig = sh->currow;
        /* move to previous non-hidden non-frozen row */
        first = sh->offscr_sc_rows;
        do {
            if (--first < 0)
                return;
        } while (sh->row_hidden[first] || sh->row_frozen[first]);
        sh->offscr_sc_rows = first;
        /* find corresponding last mobile row */
        sh->currow = first;
        calc_mobile_rows(sh, &last);
        /* restore/adjust currow */
        sh->currow = currow_orig;
        if (sh->currow > last)
            sh->currow = last;
        unselect_ranges();
    }
}


/**
 * \brief go_home()
 * \param[in] struct sheet * sh
 * \return lookat
 */
struct ent * go_home(struct sheet * sh) {
    return lookat(sh, 0, 0);
}


/**
 * \brief vert_top() - for command H in normal mode
 * \param[in] struct sheet * sh
 * \return lookat
 */
struct ent * vert_top(struct sheet * sh) {
    int r = sh->offscr_sc_rows;
    while (sh->row_hidden[r] || sh->row_frozen[r]) r++;
    return lookat(sh, r, sh->curcol);
}


/**
 * \brief vert_bottom() - for command L in normal mode
 * \param[in] struct sheet * sh
 * \return lookat
 */
struct ent * vert_bottom(struct sheet * sh) {
    int last;
    calc_mobile_rows(sh, &last);
    return lookat(sh, last, sh->curcol);
}

/**
 * \brief vert_middle() - for command M in normal mode
 * \param[in] struct sheet * sh
 * \return lookat
 */
struct ent * vert_middle(struct sheet * sh) {
    int i;
    int midscreen_pos = (SC_DISPLAY_ROWS - 1)/2;
    int curr_pos = 0;
    int mobile_rows = calc_mobile_rows(sh, NULL);

    for (i = 0; i < sh->maxrows; i++) {
        if (sh->row_hidden[i])
            continue;
        if (! sh->row_frozen[i]) {
            if (i < sh->offscr_sc_rows)
                continue;
            if (--mobile_rows < 0)
                continue;
        }

        /* compare middle of current row against middle of screen */
        if (curr_pos + (sh->row_format[i] - 1)/2 >= midscreen_pos)
            return lookat(sh, i, sh->curcol);

        curr_pos += sh->row_format[i];
    }
    return NULL;
}


/**
 * \brief go_end(): go to last valid cell of grid
 * \param[in] struct sheet * sh
 * \return lookat; NULL otherwise
 */
struct ent * go_end(struct sheet * sh) {
    int r = 0, c = 0;
    int raux = r, caux = c;
    struct ent *p;
    do {
        if (c < sh->maxcols - 1)
            c++;
        else {
            if (r < sh->maxrows - 1) {
                r++;
                c = 0;
            } else break;
        }
        if (VALID_CELL(sh, p, r, c) && ! sh->col_hidden[c] && ! sh->row_hidden[r]) { raux = r; caux = c; }
    } while ( r < sh->maxrows || c < sh->maxcols );
    if ( ! VALID_CELL(sh, p, r, c) && ! sh->col_hidden[c] && ! sh->row_hidden[r] )
        return lookat(sh, raux, caux);
    return NULL;
}


/**
 * \brief tick(): return an ent_ptr to an ent (or range) previously marked with the 'm' command.
 * \details
 * if ticks a cell, malloc's and returns an ent_ptr with struct ent * in ->vp.
 * if ticks a range, malloc's and returns struct ent * to top left cell in ->vp.
 * \return ent_ptr (should be free later on).
 */
struct ent_ptr * tick(char ch) {
    struct roman * roman = session->cur_doc;
    struct sheet * sh = roman->cur_sh;
    int r, c;
    struct mark * m = get_mark(ch);
    if (m->sheet != NULL) sh = m->sheet;
    struct ent_ptr * ep_result = calloc(1, sizeof(struct ent_ptr));

    //tick cell
    r = m->row;
    if (r != -1) {
        checkbounds(sh, &r, &(sh->curcol));
        ep_result->sheet = sh;
        ep_result->vp = lookat(sh, r, m->col);
        return ep_result;
    }

    // tick range
    if (curmode != VISUAL_MODE) {
        r = m->rng->tlrow;
        c = m->rng->tlcol;
        m->rng->selected = 1;
        checkbounds(sh, &r, &c);
        ep_result->sheet = sh;
        ep_result->vp = lookat(sh, r, c);
        return ep_result;
    }
    free(ep_result);
    return NULL;
}


/**
 * \brief scroll_right()
 * \param[in] struct sheet * sh
 * \param[in] n
 * \return none
 */
void scroll_right(struct sheet * sh, int n) {
    while (n--) {
        int last, curcol_orig = sh->curcol;
        /* find last mobile column */
        calc_mobile_cols(sh, &last);
        /* move to next non-hidden non-frozen column */
        do {
            lookat(sh, sh->currow, ++last);
            if (last >= sh->maxcols)
                return;
        } while (sh->col_hidden[last] || sh->col_frozen[last]);
        /* this will adjust offscr_sc_cols */
        sh->curcol = last;
        calc_mobile_cols(sh, NULL);
        /* restore curcol */
        sh->curcol = curcol_orig;
        if (sh->curcol < sh->offscr_sc_cols)
            sh->curcol = sh->offscr_sc_cols;
        unselect_ranges();
    }
}

/**
 * \brief scroll_left()
 * \param[in] struct sheet * sh
 * \param[in] n
 * \return none
 */
void scroll_left(struct sheet * sh, int n) {
    while (n--) {
        int first, last, curcol_orig = sh->curcol;
        /* move to previous non-hidden non-frozen column */
        first = sh->offscr_sc_cols;
        do {
            if (--first < 0)
                return;
        } while (sh->col_hidden[first] || sh->col_frozen[first]);
        sh->offscr_sc_cols = first;
        /* find corresponding last mobile column */
        sh->curcol = first;
        calc_mobile_cols(sh, &last);
        /* restore/adjust curcol */
        sh->curcol = curcol_orig;
        if (sh->curcol > last)
            sh->curcol = last;
        unselect_ranges();
    }
}


/**
 * \brief left_limit()
 * \param[in] struct sheet * sh
 * \return lookat
 */
struct ent * left_limit(struct sheet * sh) {
    int c = 0;
    while (sh->col_hidden[c] && c < sh->curcol ) c++;
    return lookat(sh, sh->currow, c);
}


/**
 * \brief right_limit()
 * \details get the last valid cell to the right
 * \param[in] struct sheet * sh
 * \param[in] row where to check
 * \return lookat
 */
struct ent * right_limit(struct sheet * sh, int row) {
    struct ent *p;
    int c = sh->maxcols - 1;
    while ( (! VALID_CELL(sh, p, row, c) && c > 0) || sh->col_hidden[c]) c--;
    return lookat(sh, row, c);
}


/**
 * \brief goto_top()
 * \param[in] struct sheet * sh
 * \return lookat
 */
struct ent * goto_top(struct sheet * sh) {
    int r = 0;
    while (sh->row_hidden[r] && r < sh->currow ) r++;
    return lookat(sh, r, sh->curcol);
}


/**
 * \brief goto_bottom()
 * \param[in] struct sheet * sh
 * \return lookat
 */
struct ent * goto_bottom(struct sheet * sh) {
    struct ent *p;
    int r = sh->maxrows - 1;
    while ( (! VALID_CELL(sh, p, r, sh->curcol) && r > 0) || sh->row_hidden[r]) r--;
    return lookat(sh, r, sh->curcol);
}


/**
 * \brief goto_last_col()
 * \details traverse the table and see which is the max column that has content
 * this is because maxcol changes when moving cursor.
 * this function is used when exporting files
 * \param[in] struct sheet * sh
 * \return lookat
 */
struct ent * goto_last_col(struct sheet * sh) {
    int r, mr = sh->maxrows;
    int c, mc = 0;
    struct ent *p;
    int rf = 0;

    for (r = 0; r < mr; r++) {
        for (c = 0; c < sh->maxcols; c++) {
            if (c >= mc && VALID_CELL(sh, p, r, c)) { mc = c; rf = r; }
        }
    }
    return lookat(sh, rf, mc);
}


/**
 * \brief go_forward()
 * \param[in] struct sheet * sh
 * \return lookat
 */
struct ent * go_forward(struct sheet * sh) {
    int r = sh->currow, c = sh->curcol;
    int r_ori = r, c_ori = c;
    struct ent * p;
    do {
        if (c < sh->maxcols - 1) {
            c++;
        } else {
            if (r < sh->maxrows - 1) {
                r++;
                c = 0;
            } else break;
        }
        if (VALID_CELL(sh, p, r, c) && ! sh->col_hidden[c] && ! sh->row_hidden[r] )
            return lookat(sh, r, c);
    } while (r < sh->maxrows || c < sh->maxcols);

    return lookat(sh, r_ori, c_ori);
}


/**
 * \brief go_bol()
 * \param[in] struct sheet * sh
 * \return lookat
 */
struct ent * go_bol(struct sheet * sh) {
    return lookat(sh, sh->currow, sh->offscr_sc_cols);
}


/**
 * \brief go_eol()
 * \param[in] struct sheet * sh
 * \return none
 */
struct ent * go_eol(struct sheet * sh) {
    int last_col;
    calc_mobile_cols(sh, &last_col);
    return lookat(sh, sh->currow, last_col);
}


/**
 * \brief horiz_middle()
 * \param[in] struct sheet * sh
 * \return lookat; NULL otherwise
 */
struct ent * horiz_middle(struct sheet * sh) {
    int i;
    int midscreen_pos = (SC_DISPLAY_COLS - 1)/2;
    int curr_pos = 0;
    int mobile_cols = calc_mobile_cols(sh, NULL);

    for (i = 0; i < sh->maxcols; i++) {
        if (sh->col_hidden[i])
            continue;
        if (! sh->col_frozen[i]) {
            if (i < sh->offscr_sc_cols)
                continue;
            if (--mobile_cols < 0)
                continue;
        }

        /* compare middle of current col against middle of screen */
        if (curr_pos + (sh->fwidth[i] - 1)/2 >= midscreen_pos)
            return lookat(sh, sh->currow, i);

        curr_pos += sh->fwidth[i];
    }
    return NULL;
}


/**
 * \brief go_backward()
 * \param[in] struct sheet * sh
 * \return lookat
 */
struct ent * go_backward(struct sheet * sh) {
    int r = sh->currow, c = sh->curcol;
    int r_ori = r, c_ori = c;
    struct ent * p;
    do {
        if (c)
            c--;
        else {
            if (r) {
                r--;
                c = sh->maxcols - 1;
            } else break;
        }
        if ( VALID_CELL(sh, p, r, c) && ! sh->col_hidden[c] && ! sh->row_hidden[r] )
            return lookat(sh, r, c);
    } while ( sh->currow || sh->curcol );

    return lookat(sh, r_ori, c_ori);
}


/**
 * \brief auto_fit()
 * \param[in] struct sheet * sh
 * \param[in] ci
 * \param[in] cf
 * \param[in] min
 * \return none
 */
void auto_fit(struct sheet * sh, int ci, int cf, int min) {
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

    checkbounds(sh, &(sh->maxrow), &cf);

    for (c = ci; c <= cf; c++) {
#ifdef UNDO
        add_undo_col_format(c, 'R', sh->fwidth[c], sh->precision[c], sh->realfmt[c]);
#endif
        sh->fwidth[c] = min;
        for (r = 0; r <= sh->maxrow; r++) {
            if ((p = *ATBL(sh, sh->tbl, r, c)) != NULL) {
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
                    sprintf(field, "%.*f", sh->precision[c], p->v);
                    sum += strlen(field);
                }
                if (sum > sh->fwidth[c] && sum < SC_DISPLAY_COLS)
                    sh->fwidth[c] = sum;
            }
        }
#ifdef UNDO
        add_undo_col_format(c, 'A', sh->fwidth[c], sh->precision[c], sh->realfmt[c]);
#endif
    }
#ifdef UNDO
    end_undo_action();
#endif

    return;
}


/**
 * \brief valueize_area()
 * \details
 * Delete a cell expression and turn into constant.
 * Deletes the expression associated with a cell and
 * turns it into a constant containing whatever was on the screen.
 *
 * \param[in] struct sheet * sh
 * \param[in] sr
 * \param[in] sc
 * \param[in] er
 * \param[in] ec
 *
 * \return none
 */
void valueize_area(struct sheet * sh, int sr, int sc, int er, int ec) {
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
    checkbounds(sh, &er, &ec);

    #ifdef UNDO
    create_undo_action();
    copy_to_undostruct(sh, sr, sc, er, ec, UNDO_DEL, IGNORE_DEPS, NULL);
    #endif
    for (r = sr; r <= er; r++) {
        for (c = sc; c <= ec; c++) {
            p = *ATBL(sh, sh->tbl, r, c);
            if (p && p->flags & is_locked) {
                sc_error(" Cell %s%d is locked", coltoa(c), r);
                continue;
            }
            if (p && p->expr) {
                efree(p->expr);
                p->expr = (struct enode *)0;
                p->flags &= ~is_strexpr;

                // TODO move this to depgraph ?
                vertexT * v_cur = getVertex(graph, sh, p, 0);
                if (v_cur != NULL) { // just in case

                    // for each edge in edges, we look for the reference to the vertex we are deleting, and we erase it!
                    edgeT * e = v_cur->edges;
                    while (e != NULL) { // && v_cur->back_edges == NULL) {
                        delete_reference(v_cur, e->connectsTo, 1);

                        // delete vertex only if it end up having no edges, no expression, no value, no label....
                        if (e->connectsTo->edges == NULL && e->connectsTo->back_edges == NULL && !e->connectsTo->ent->expr && !(e->connectsTo->ent->flags & is_valid) && ! e->connectsTo->ent->label)
                            destroy_vertex(sh, e->connectsTo->ent);
                        //     WARNING: an orphan vertex now represents an ent that has an enode thats
                        //     need to be evaluated, but do not depend in another cell.
                        e = e->next;
                    }

                    destroy_list_edges(v_cur->edges);
                    v_cur->edges = NULL;

                    /* delete vertex in graph
                       only if this vertex is not referenced by other */
                    if (v_cur->back_edges == NULL ) destroy_vertex(sh, p);
                    }
            }
        }
    }
    #ifdef UNDO
    copy_to_undostruct(sh, sr, sc, er, ec, UNDO_ADD, IGNORE_DEPS, NULL);
    end_undo_action();
    #endif
    sc_info("Removed formulas from range");
    return;
}


/**
 * \brief select_inner_range()
 * \param[in] struct sheet * sh
 * \param[in] vir_tlrow
 * \param[in] vir_tlcol
 * \param[in] vir_brrow
 * \param[in] vir_brcol
 * \return none
 */
void select_inner_range(struct sheet * sh, int * vir_tlrow, int * vir_tlcol, int * vir_brrow, int * vir_brcol) {
    struct ent * p;
    int rr, cc, r, c, mf = 1;

    while (mf != 0) {
        mf = 0;
        for (rr = *vir_tlrow; rr <= *vir_brrow; rr++) {
            for (cc = *vir_tlcol; cc <= *vir_brcol; cc++)
                for (r=-1; r<=1; r++)
                    for (c=-1; c<=1; c++) {
                        if (r == 0 && c == 0) continue;
                        else if (rr + r < 0 || cc + c < 0 || rr + r > sh->maxrow || cc + c > sh->maxcol) continue;
                        p = *ATBL(sh, sh->tbl, rr + r, cc + c);
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
 * \brief locked_cell()
 * \details Check if cell is locked
 * \param[in] struct sheet * sh
 * \param[in] int r
 * \param[in] int c
 * \return 1 if cell if locked; 0 otherwise
 */
int locked_cell(struct sheet * sh, int r, int c) {
    struct ent *p = *ATBL(sh, sh->tbl, r, c);
    if (p && (p->flags & is_locked)) {
        sc_error("Cell %s%d is locked", coltoa(c), r) ;
        return 1;
    }
    return 0;
}


/**
 * \brief any_locked_cells()
 * \details Check if area contains locked cells
 * \param[in] struct sheet * sh
 * \param[in] r1
 * \param[in] c1
 * \param[in] r2
 * \param[in] c2
 * \return 1 if area contains a locked cell; 0 otherwise
 */
int any_locked_cells(struct sheet * sh, int r1, int c1, int r2, int c2) {
    int r, c;
    struct ent * p ;

    for (r = r1; r <= r2; r++)
        for (c = c1; c <= c2; c++) {
            p = *ATBL(sh, sh->tbl, r, c);
            if (p && (p->flags & is_locked))
                return 1;
        }
    return 0;
}


/**
 * \brief fsum()
 * \details sum special command
 * \param[in] struct sheet * sh
 * \return none
 */
int fsum(struct sheet * sh) {
    int r = sh->currow, c = sh->curcol;
    struct ent * p;

    if (r > 0 && (*ATBL(sh, sh->tbl, r-1, c) != NULL) && (*ATBL(sh, sh->tbl, r-1, c))->flags & is_valid) {
        for (r = sh->currow-1; r >= 0; r--) {
            p = *ATBL(sh, sh->tbl, r, c);
            if (p == NULL) break;
            if (! (p->flags & is_valid)) break;
        }
        if (sh->currow != r) {
            swprintf(interp_line, BUFFERSIZE, L"let %s%d = @SUM(", coltoa(sh->curcol), sh->currow);
            swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%s%d:", coltoa(sh->curcol), r+1);
            swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%s%d)", coltoa(sh->curcol), sh->currow-1);
        }
    } else if (c > 0 && (*ATBL(sh, sh->tbl, r, c-1) != NULL) && (*ATBL(sh, sh->tbl, r, c-1))->flags & is_valid) {
        for (c = sh->curcol-1; c >= 0; c--) {
            p = *ATBL(sh, sh->tbl, r, c);
            if (p == NULL) break;
            if (! (p->flags & is_valid)) break;
        }
        if (sh->curcol != c) {
            swprintf(interp_line, BUFFERSIZE, L"let %s%d = @SUM(", coltoa(sh->curcol), sh->currow);
            swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%s%d:", coltoa(c+1), r);
            swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%s%d)", coltoa(sh->curcol-1), r);
        }
    }

    if ((sh->currow != r || sh->curcol != c) && wcslen(interp_line))
        send_to_interp(interp_line);
    EvalRange(sh, sh->currow, sh->curcol, sh->currow, sh->curcol);
    return 0;
}


/**
 * \brief fcopy()
 * \param[in] struct sheet * sh
 * \param[in] action
 * \return -1 on error; 0 otherwise
 */
int fcopy(struct sheet * sh, char * action) {
    int r, ri, rf, c, ci, cf;
    struct ent * pdest;
    struct ent * pact;
    int p = is_range_selected();
    struct srange * sr = NULL;
    if (p != -1) sr = get_range_by_pos(p);

    if (p == -1) { // no range selected
        // fail if the cursor is on the first column
        if (sh->curcol == 0) {
            sc_error("Can't fcopy with no arguments while on column 'A'");
            return -1;
        }

        ri = sh->currow;
        ci = sh->curcol;
        cf = sh->curcol;
        for (r=ri+1; r<sh->maxrow && (*ATBL(sh, sh->tbl, r, cf-1)) != NULL && (*ATBL(sh, sh->tbl, r, cf-1))->flags & is_valid ; r++) {}
        rf = --r;
    } else { // range is selected
        ri = sr->tlrow;
        ci = sr->tlcol;
        cf = sr->brcol;
        rf = sr->brrow;
    }

    // check if all cells that will be copied to somewhere have a formula in them
    if (! strcmp(action, "") || ! strcmp(action, "cells")) {
        if (!  *ATBL(sh, sh->tbl, ri, ci)       ) goto formula_not_found;
        if (! (*ATBL(sh, sh->tbl, ri, ci))->expr) goto formula_not_found;
    } else if (! strcmp(action, "c") || ! strcmp(action, "columns")) {
        for (c=ci; c<=cf; c++) {
            if (!  *ATBL(sh, sh->tbl, ri, c)       ) goto formula_not_found;
            if (! (*ATBL(sh, sh->tbl, ri, c))->expr) goto formula_not_found;
        }
    } else if (! strcmp(action, "r") || ! strcmp(action, "rows")) {
        for (r=ri; r<=rf; r++) {
            if (!  *ATBL(sh, sh->tbl, r, ci)       ) goto formula_not_found;
            if (! (*ATBL(sh, sh->tbl, r, ci))->expr) goto formula_not_found;
        }
    } else {
        sc_error("Invalid parameter");
    }

    goto all_formulas_found;

    formula_not_found:
    sc_error("At least 1 formula not found. Nothing changed");
    return -1;

    all_formulas_found:

    if (any_locked_cells(sh, ri, ci, rf, cf)) {
        swprintf(interp_line, BUFFERSIZE, L"");
        sc_error("Locked cells encountered. Nothing changed");
        return -1;
    }
#ifdef UNDO
    create_undo_action();
    copy_to_undostruct(sh, ri, ci, rf, cf, UNDO_DEL, IGNORE_DEPS, NULL);
#endif

    if (! strcmp(action, "")) {
        // copy first column down (old behavior), for backwards compatibility
        pact = *ATBL(sh, sh->tbl, ri, ci);
        for (r=ri+1; r<=rf; r++) {
            pdest = lookat(sh, r, ci);
            copyent(pdest, sh, pact, r - ri, 0, 0, 0, sh->maxrows, sh->maxcols, 'c');
        }
    } else if (! strcmp(action, "c") || ! strcmp(action, "columns")) {
        // copy all selected columns down
        for (c=ci; c<=cf; c++) {
            pact = *ATBL(sh, sh->tbl, ri, c);
            for (r=ri+1; r<=rf; r++) {
                pdest = lookat(sh, r, c);
                copyent(pdest, sh, pact, r - ri, 0, 0, 0, sh->maxrows, sh->maxcols, 'c');
            }
        }
    } else if (! strcmp(action, "r") || ! strcmp(action, "rows")) {
        // copy all selected rows right
        for (r=ri; r<=rf; r++) {
            pact = *ATBL(sh, sh->tbl, r, ci);
            for (c=ci+1; c<=cf; c++) {
                pdest = lookat(sh, r, c);
                copyent(pdest, sh, pact, 0, c - ci, 0, 0, sh->maxrows, sh->maxcols, 'c');
            }
        }
    } else if (! strcmp(action, "cells")) {
        // copy selected cell down and right
        pact = *ATBL(sh, sh->tbl, ri, ci);
        for (r=ri; r<=rf; r++) {
            for(c=(r==ri?ci+1:ci); c<=cf; c++) {
                pdest = lookat(sh, r, c);
                copyent(pdest, sh, pact, r - ri, c - ci, 0, 0, sh->maxrows, sh->maxcols, 'c');
            }
        }
    }
    EvalRange(sh, ri, ci, rf, cf);

#ifdef UNDO
    copy_to_undostruct(sh, ri, ci, rf, cf, UNDO_ADD, IGNORE_DEPS, NULL);
    end_undo_action();
#endif

    return 0;
}


/**
 * \brief pad()
 * \details Add padding to cells
 * \param[in] struct sheet * sh
 * \param[in] int n
 * \param[in] int r1
 * \param[in] int c1
 * \param[in] int r2
 * \param[in] int c2
 * \details Add padding to cells. This set padding of a range.
 * \return -1 if locked cell is encountered; 1 if padding exceeded
 * column width; 0 otherwise
 */
int pad(struct sheet * sh, int n, int r1, int c1, int r2, int c2) {
    int r, c;
    struct ent * p ;
    int pad_exceed_width = 0;

    if (any_locked_cells(sh, r1, c1, r2, c2)) {
        sc_info("Locked cells encountered. Nothing changed");
        return -1;
     }

#ifdef UNDO
    create_undo_action();
    copy_to_undostruct(sh, r1, c1, r2, c2, UNDO_DEL, IGNORE_DEPS, NULL);
#endif

    for (r = r1; r <= r2; r++) {
        for (c = c1; c <= c2; c++) {
            if (n > sh->fwidth[c]) {
                pad_exceed_width = 1;
                continue;
            }
            if ((p = *ATBL(sh, sh->tbl, r, c)) != NULL) p->pad = n;
            session->cur_doc->modflg++;
        }
    }

#ifdef UNDO
    copy_to_undostruct(sh, r1, c1, r2, c2, UNDO_ADD, IGNORE_DEPS, NULL);
    end_undo_action();
#endif

    if (pad_exceed_width) {
        sc_error(" Could not add padding in some cells. Padding exceeded column width");
        return 1;
    }
    return 0;
}


/**
 * \brief fix_row_hidden()
 * \param[in] struct sheet * sh
 * \param[in] int deltar
 * \param[in] int ri
 * \param[in] int rf
 * \details fix hidden rows after undoing ir dr etc..
 * \return none
 */
void fix_row_hidden(struct sheet * sh, int deltar, int ri, int rf) {
    int r;
    int d = deltar;

    // decrease / for dr
    if (deltar > 0)
        while (d-- > 0)
            for (r = ri; r < rf; r++)
                sh->row_hidden[r] = sh->row_hidden[r+1];

    // increase / for ir
    if (deltar < 0)
        while (d++ < 0)
            for (r = rf; r > ri; r--)
                sh->row_hidden[r] = sh->row_hidden[r-1];
    return;
}


/**
 * \brief fix_col_hidden()
 * \details fix hidden cols after undoing ic dc etc..
 * \param[in] struct sheet * sh
 * \param[in] int deltac
 * \param[in] int ci
 * \param[in] int cf
 * \return none
 */
void fix_col_hidden(struct sheet * sh, int deltac, int ci, int cf) {
    int c;
    int d = deltac;

    // decrease / for dc
    if (deltac > 0)
        while (d-- > 0)
            for (c = ci; c < cf; c++)
                sh->col_hidden[c] = sh->col_hidden[c+1];

    // increase / for ic
    if (deltac < 0)
        while (d++ < 0)
            for (c = cf; c > ci; c--)
                sh->col_hidden[c] = sh->col_hidden[c-1];
    return;
}


/**
 * \brief fix_row_frozen()
 * \details fix frozen rows after undoing ir dr etc..
 * \param[in] struct sheet * sh
 * \param[in] int deltar
 * \param[in] int ri
 * \param[in] int rf
 * \return none
 */
void fix_row_frozen(struct sheet * sh, int deltar, int ri, int rf) {
    int r;
    int d = deltar;

    // decrease / for dr
    if (deltar > 0)
        while (d-- > 0)
            for (r = ri; r < rf; r++)
                sh->row_frozen[r] = sh->row_frozen[r+1];

    // increase / for ir
    if (deltar < 0)
        while (d++ < 0)
            for (r = rf; r > ri; r--)
                sh->row_frozen[r] = sh->row_frozen[r-1];
    return;
}


/**
 * \brief fix_col_frozen()
 * \details fix frozen cols after undoing ic dc etc..
 * \param[in] struct sheet * sh
 * \param[in] int deltac
 * \param[in] int ci
 * \param[in] int cf
 * \return none
 */
void fix_col_frozen(struct sheet * sh, int deltac, int ci, int cf) {
    int c;
    int d = deltac;

    // decrease / for dc
    if (deltac > 0)
        while (d-- > 0)
            for (c = ci; c < cf; c++)
                sh->col_frozen[c] = sh->col_frozen[c+1];

    // increase / for ic
    if (deltac < 0)
        while (d++ < 0)
            for (c = cf; c > ci; c--)
                sh->col_frozen[c] = sh->col_frozen[c-1];
    return;
}


/**
* \brief convert_string_to_number
* \details converts string content of range of cells to numeric value
* \return 0 on success; -1 on error
*/
int convert_string_to_number(int r0, int c0, int rn, int cn) {
    struct roman * roman = session->cur_doc;
    int row, col;
    register struct ent ** pp;
    wchar_t out[FBUFLEN] = L"";
    for (row = r0; row <= rn; row++) {
        // ignore hidden rows
        //if (row_hidden[row]) continue;

        for (pp = ATBL(roman->cur_sh, roman->cur_sh->tbl, row, col = c0); col <= cn; col++, pp++) {
            // ignore hidden cols
            //if (col_hidden[col]) continue;

            if (*pp) {
                // If a string exists
                if ((*pp)->label) {
                    char * num = str_replace((*pp)->label," ","");
                    (*pp)->label[0] = '\0';
                    swprintf(out, BUFFERSIZE, L"let %s%d=%s", coltoa(col), row, num);
                    send_to_interp(out);
                    free(num);
                }
            }
        }
    }
    return 0;
}


/**
 * \brief Compute number of mobile (unfrozen) rows to fit on screen
 *
 * \details This function finds the number of mobile rows that can fit on
 * the screen. This number excludes frozen rows as they never leave the
 * screen hence their total height is subtracted from the available screen
 * area. This number also excludes hidden rows. Displayed mobile rows are
 * considered starting from offscr_sc_rows, however currow must be within
 * the displayed rows. If currow is found to be outside the displayed set
 * of rows then offscr_sc_rows is adjusted accordingly.
 *
 * \param[in] struct sheet * sh
 * \param[in] last_p If not NULL then the last mobile row to fit is stored there
 *
 * \return Number of mobile rows displayable on the screen
 */
int calc_mobile_rows(struct sheet * sh, int *last_p) {
    int i, row_space, mobile_rows, last;

    /*
     * Compute the number of frozen rows and the space they need.
     * Eventually this should be added/subtracted when individual rows
     * are frozen/unfrozen/enlarged/reduced/deleted and not recomputed
     * every time here... or at least have a global flag indicating that
     * nothing has changed and that this loop can be skipped.
     */
    sh->nb_frozen_rows = 0;
    sh->nb_frozen_screenrows = 0;
    for (i = 0; i < sh->maxrows; i++) {
        if (sh->row_hidden[i])
            continue;
        if (sh->row_frozen[i]) {
            sh->nb_frozen_rows++;
            sh->nb_frozen_screenrows += sh->row_format[i];
        }
    }

    /* Adjust display start if currow is above it */
    if (sh->currow < sh->offscr_sc_rows)
        sh->offscr_sc_rows = sh->currow;

    /* Determine the space available for mobile rows. */
    row_space = SC_DISPLAY_ROWS - sh->nb_frozen_screenrows;

    /*
     * Find how many visible mobile rows can fit in there
     * and remember which one is the last to fit.
     */
    mobile_rows = 0;
    last = sh->offscr_sc_rows;
    for (i = sh->offscr_sc_rows; i < sh->maxrows; i++) {
        count_rows_downward:
        if (sh->row_hidden[i])
            continue;
        if (sh->row_frozen[i])
            continue;
        if (sh->row_format[i] > row_space)
            break;
        row_space -= sh->row_format[i];
        mobile_rows++;
        last = i;
    }

    /*
     * If currow is beyond the last row here then we must  start over,
     * moving backward this time, to properly position start of display.
     */
    if (last < sh->currow) {
        row_space = SC_DISPLAY_ROWS - sh->nb_frozen_screenrows;
        mobile_rows = 0;
        last = sh->currow;
        for (i = sh->currow; i >= 0; i--) {
            if (sh->row_hidden[i])
                continue;
            if (sh->row_frozen[i])
                continue;
            if (sh->row_format[i] > row_space)
                break;
            row_space -= sh->row_format[i];
            mobile_rows++;
            last = i;
        }
        sh->offscr_sc_rows = last;
        last = sh->currow;

        /*
         * Yet if the rows to follow have a smaller height than those we just
         * counted then maybe they could fill the remaining space at the
         * bottom edge of the screen.
         */
        i = last + 1;
        if (row_space > 0 && i < sh->maxrows)
            goto count_rows_downward;
    }

    if (last_p)
        *last_p = last;
    return mobile_rows;
}

/**
 * \brief Compute number of mobile (unfrozen) columns to fit on screen
 *
 * \details This function finds the number of mobile columns that can fit on
 * the screen. This number excludes frozen columns as they never leave the
 * screen, hence their total height is subtracted from the available screen
 * area. This number also excludes hidden columns. Displayed mobile columnss
 * are considered starting from offscr_sc_cols, however curcol must be within
 * the displayed columns. If curcol is found to be outside the displayed set
 * of columns then offscr_sc_cols is adjusted accordingly.
 *
 * \param[in] struct sheet * sh
 * \param[in] last_p If not NULL then the last mobile column to fit is stored there
 *
 * \return Number of mobile columns displayable on the screen
 */
int calc_mobile_cols(struct sheet * sh, int *last_p) {
    int i, col_space, mobile_cols, last;

    /*
     * Compute the number of frozen columns and the space they need.
     * Eventually this should be added/subtracted when individual
     * columns are frozen/unfrozen/enlarged/reduced/deleted and not
     * recomputed every time here... or at least have a flag indicating
     * that nothing has changed and that this loop may be skipped.
     */
    sh->nb_frozen_cols = 0;
    sh->nb_frozen_screencols = 0;
    for (i = 0; i < sh->maxcols; i++) {
        if (sh->col_hidden[i])
            continue;
        if (sh->col_frozen[i]) {
            sh->nb_frozen_cols++;
            sh->nb_frozen_screencols += sh->fwidth[i];
        }
    }

    /* Adjust display start if curcol is left of it */
    if (sh->curcol < sh->offscr_sc_cols)
        sh->offscr_sc_cols = sh->curcol;

    /* Determine the space available for mobile columns. */
    col_space = SC_DISPLAY_COLS - sh->nb_frozen_screencols;

    /*
     * Find how many visible mobile columns can fit in there
     * and remember which one is the last to fit.
     */
    mobile_cols = 0;
    last = sh->offscr_sc_cols;
    for (i = sh->offscr_sc_cols; i < sh->maxcols; i++) {
        count_cols_rightward:
        if (sh->col_hidden[i])
            continue;
        if (sh->col_frozen[i])
            continue;
        if (sh->fwidth[i] > col_space)
            break;
        col_space -= sh->fwidth[i];
        mobile_cols++;
        last = i;
    }

    /*
     * If curcol is beyond the last column here then we start over,
     * moving backward this time, to properly position start of display.
     */
    if (last < sh->curcol) {
        col_space = SC_DISPLAY_COLS - sh->nb_frozen_screencols;
        mobile_cols = 0;
        last = sh->curcol;
        for (i = sh->curcol; i >= 0; i--) {
            if (sh->col_hidden[i])
                continue;
            if (sh->col_frozen[i])
                continue;
            if (sh->fwidth[i] > col_space)
                break;
            col_space -= sh->fwidth[i];
            mobile_cols++;
            last = i;
        }
        sh->offscr_sc_cols = last;
        last = sh->curcol;

        /*
         * Yet if the columns to follow have a smaller width than those we
         * just counted then maybe they could fill the remaining space at the
         * right edge of the screen.
         */
        i = last + 1;
        if (col_space > 0 && i < sh->maxcols)
            goto count_cols_rightward;
    }

    if (last_p)
        *last_p = last;
    return mobile_cols;
}


/**
 * \brief pad_and_align
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
 * \param[in] rowfmt: rowheight
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
    char * str_in;

    // Here we handle \" and replace them with "
    str_in = str_replace(str_value, "\\\"", "\"");

    const char * mbsptr;
    mbsptr = str_in;

    // create wcs string based on multibyte string..
    memset( &state, '\0', sizeof state );
    result = mbsrtowcs(wcs_value, &mbsptr, BUFFERSIZE, &state);
    if ( result != (size_t)-1 )
        str_len = wcswidth(wcs_value, wcslen(wcs_value));

    if (str_len == 2 && str_in[0] == '\\') {
        wmemset(str_out + wcslen(str_out), str_in[1], col_width);
        free(str_in);
        return;
    } else if (str_len == 3 && str_in[0] == '\\' && str_in[1] == '\\') {
        wmemset(str_out + wcslen(str_out), str_in[2], col_width);
        free(str_in);
        return;
    }

    // If padding exceedes column width, returns n number of '-' needed to fill column width
    if (padding >= col_width ) {
        wmemset(str_out + wcslen(str_out), L' ', col_width);
        free(str_in);
        return;
    }

    // If content exceedes column width, outputs n number of '*' needed to fill column width
    if (str_len + num_len + padding > col_width * rowfmt && ! get_conf_int("truncate") &&
        ! get_conf_int("overlap") && ! get_conf_int("autowrap")) {
        if (padding) wmemset(str_out + wcslen(str_out), L' ', padding);
        wmemset(str_out + wcslen(str_out), L'*', col_width - padding);
        free(str_in);
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
        swprintf(str_out + wcslen(str_out), BUFFERSIZE, L"%s", str_in);

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

    // on each \n chars, replace with n number of L' ' to complete column width
    int posnl, leftnl = 0;
    while ((posnl = wstr_in_wstr(str_out, L"\\n")) != -1) {
        del_range_wchars(str_out, posnl, posnl+1);
        if (posnl <= col_width) leftnl = col_width - posnl;
        else leftnl = col_width - posnl % col_width;
        while (leftnl-- > 0) add_wchar(str_out, L' ', posnl);
    }

    free(str_in);
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
        else if (buf->value == L'?')        result = MOVEMENT_CMD; // search backwards
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
                 buf->pnext->value == L'f' ||
                 buf->pnext->value == L'M' ||
                 buf->pnext->value == L'g' ||
                 buf->pnext->value == L'G' ||
                 buf->pnext->value == L'0' ||
                 buf->pnext->value == L'l' ||
                 buf->pnext->value == L't' ||
                 buf->pnext->value == L'T' ||
                 buf->pnext->value == L'$'))
                 result = MOVEMENT_CMD;

        else if (buf->value == L'g' && bs > 3 && buf->pnext->value == L'o' && timeout >= COMPLETECMDTIMEOUT)
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

        else if (buf->value == L'a' && bs == 2 &&    // autofit
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
            //  FIXME add better validation
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
