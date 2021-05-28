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
 * \file vmtbl.c
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 2017-07-18
 * \brief TODO Write a tbrief file description.
 */

#include <stdio.h>
#include <stdlib.h> // for atoi
#include <unistd.h>

#include "vmtbl.h"
#include "sc.h"
#include "macros.h"
#include "conf.h"
#include "trigger.h"
#include "tui.h"

#define ATBL_P(tbl, row, col)    (*(tbl + row) + (col))

extern struct session * session;

/*
 * check to see if *rowp && *colp are currently allocated, if not expand the
 * current size if we can.
 */
#ifndef PSC
/**
 * \brief TODO Document checkbounds()
 *
 * \param[in] rowp
 * \param[in] colp
 *
 * \return none
 */

void checkbounds(struct sheet * sh, int * rowp, int * colp) {
    if (*rowp < 0)
        *rowp = 0;
    else if (*rowp >= sh->maxrows) {
        if (*colp >= sh->maxcols) {
            if (!growtbl(sh, GROWBOTH, *rowp, *colp)) {
                *rowp = sh->maxrows - 1;
                *colp = sh->maxcols - 1;
            }
            return;
        } else {
            if (!growtbl(sh, GROWROW, *rowp, 0))
                *rowp = sh->maxrows - 1;
            return;
        }
    }
    if (*colp < 0)
        *colp = 0;
    else if (*colp >= sh->maxcols) {
        if (!growtbl(sh, GROWCOL, 0, *colp))
            *colp = sh->maxcols - 1;
    }
}
#endif /* ! PSC */

/* scxrealloc will just scxmalloc if oldptr is == NULL */
#define GROWALLOC(newptr, oldptr, nelem, type, msg) \
    newptr = (type *) scxrealloc((char *) oldptr, \
        (unsigned)(nelem * sizeof(type))); \
    if (newptr == (type *)NULL) { \
    sc_error(msg); \
    return (FALSE); \
    } \
    oldptr = newptr /* wait incase we can't alloc */

#ifndef PSC
static char nolonger[] = "The table can't be any longer";
#endif /* !PSC */

static char nowider[] = "The table can't be any wider";

/**
 * \brief TODO Document growtbl()
 *
 * \details Grow the main && auxillary tables (reset maxrows/mascols
 * as needed). toprow &&/|| topcol tells us a better guess of how big
 * to become. We return TRUE if we could grow, FALSE if not.
 * \param[in] rowcol
 * \param[in] toprow
 * \param[in] topcol
 *
 * \return TRUE if we could grow
 * \return FALSE if we cannot grow
 */

int growtbl(struct sheet * sh, int rowcol, int toprow, int topcol) {
    int * fwidth2;
    int * precision2;
    int * realfmt2;
    int newcols;
#ifndef PSC
    struct ent *** tbl2;
    struct ent ** nullit;
    int cnt;
    unsigned char * col_hidden2;
    unsigned char * row_hidden2;
    unsigned char * col_frozen2;
    unsigned char * row_frozen2;
    unsigned char * row_format2;
    int newrows;
    int i;

    newrows = sh->maxrows;
#endif /* ! PSC */
    newcols = sh->maxcols;
    if (rowcol == GROWNEW) {
#ifndef PSC
        sh->maxrows = toprow = 0;
    /* when we first start up, fill the screen w/ cells */
        {
        int startval;
        startval = SC_DISPLAY_ROWS;
        newrows = startval > MINROWS ? startval : MINROWS;
        startval = SC_DISPLAY_COLS / DEFWIDTH;
        newcols = startval > MINCOLS ? startval : MINCOLS;
        }
#else
        newcols = MINCOLS;
#endif /* !PSC */
        sh->maxcols = topcol = 0;
    }
#ifndef PSC
    /* set how much to grow */
    if ((rowcol == GROWROW) || (rowcol == GROWBOTH)) {
        if ((sh->maxcols == MAXROWS) || (toprow >= MAXROWS)) {
            sc_error(nolonger);
            return (FALSE);
        }

        if (toprow > sh->maxrows)
            newrows = GROWAMT + toprow;
        else
            newrows += GROWAMT;

        /* If we're close but less than MAXROWS, clip to max value */
        if ( newrows > MAXROWS )
            newrows = MAXROWS;
    }
#endif /* !PSC */
    if ((rowcol == GROWCOL) || (rowcol == GROWBOTH)) {
        if ((sh->maxcols == ABSMAXCOLS) || (topcol >= ABSMAXCOLS)) {
            sc_error(nowider);
            return (FALSE);
        }

        if (topcol > sh->maxcols)
            newcols = GROWAMT + topcol;
        else
            newcols += GROWAMT;

        if (newcols > ABSMAXCOLS)
            newcols = ABSMAXCOLS;
    }

#ifndef PSC
    if ((rowcol == GROWROW) || (rowcol == GROWBOTH) || (rowcol == GROWNEW)) {
        struct ent *** lnullit;
        int lcnt;

        GROWALLOC(row_hidden2, sh->row_hidden, newrows, unsigned char, nolonger);
        memset(sh->row_hidden + sh->maxrows, 0, (newrows - sh->maxrows) * sizeof(unsigned char));

        GROWALLOC(row_format2, sh->row_format, newrows, unsigned char, nolonger);
        memset(sh->row_format + sh->maxrows, 1, (newrows - sh->maxrows) * sizeof(unsigned char));

        GROWALLOC(row_frozen2, sh->row_frozen, newrows, unsigned char, nolonger);
        memset(sh->row_frozen + sh->maxrows, 0, (newrows - sh->maxrows) * sizeof(unsigned char));

        /*
         * alloc tbl row pointers, per net.lang.c, calloc does not
         * necessarily fill in NULL pointers
         */
        GROWALLOC(tbl2, sh->tbl, newrows, struct ent **, nolonger);
        for (lnullit = sh->tbl + sh->maxrows, lcnt = 0; lcnt < newrows - sh->maxrows; lcnt++, lnullit++)
            *lnullit = (struct ent **)NULL;
  /*    memset(tbl + maxrows, (char *) NULL, (newrows - maxrows) * (sizeof(struct ent **)));*/
    }
#endif /* !PSC */

    if ((rowcol == GROWCOL) || (rowcol == GROWBOTH) || (rowcol == GROWNEW)) {
        GROWALLOC(fwidth2, sh->fwidth, newcols, int, nowider);
        GROWALLOC(precision2, sh->precision, newcols, int, nowider);
        GROWALLOC(realfmt2, sh->realfmt, newcols, int, nowider);
#ifdef PSC
        memset(sh->fwidth + sh->maxcols, 0, (newcols - sh->maxcols) * sizeof(int));
        memset(sh->precision + sh->maxcols, 0, (newcols - sh->maxcols) * sizeof(int));
        memset(sh->realfmt + sh->maxcols, 0, (newcols - sh->maxcols) * sizeof(int));
    }
#else
        GROWALLOC(col_hidden2, sh->col_hidden, newcols, unsigned char, nowider);
        memset(sh->col_hidden + sh->maxcols, 0, (newcols - sh->maxcols) * sizeof(unsigned char));
        for (i = sh->maxcols; i < newcols; i++) {
            sh->fwidth[i] = DEFWIDTH;
            sh->precision[i] = DEFPREC;
            sh->realfmt[i] = DEFREFMT;
        }

        GROWALLOC(col_frozen2, sh->col_frozen, newcols, unsigned char, nolonger);
        memset(sh->col_frozen + sh->maxcols, 0, (newcols - sh->maxcols) * sizeof(unsigned char));

        /* [re]alloc the space for each row */
        for (i = 0; i < sh->maxrows; i++) {
            if ((sh->tbl[i] = (struct ent **) scxrealloc((char *)sh->tbl[i], (unsigned) (newcols * sizeof(struct ent **)))) == (struct ent **)0) {
                sc_error(nowider);
                return(FALSE);
            }
            for (nullit = ATBL_P(sh->tbl, i, sh->maxcols), cnt = 0; cnt < newcols - sh->maxcols; cnt++, nullit++)
                *nullit = (struct ent *)NULL;
        /*        memset((char *) ATBL(tbl,i, sh->maxcols), 0, (newcols - sh->maxcols) * sizeof(struct ent **)); */
        }
    }
    else
        i = sh->maxrows;

    /* fill in the bottom of the table */
    for (; i < newrows; i++) {
        if ((sh->tbl[i] = (struct ent **) scxmalloc((unsigned)(newcols * sizeof(struct ent **)))) == (struct ent **) 0) {
            sc_error(nowider);
            return(FALSE);
        }
        for (nullit = sh->tbl[i], cnt = 0; cnt < newcols; cnt++, nullit++)
            *nullit = (struct ent *)NULL;
        /* memset((char *) sh->tbl[i], 0, newcols * sizeof(struct ent **));*/
    }

    sh->maxrows = newrows;
#endif /* PSC */

    sh->rescol = 2;
    if (sh->maxrows > 10)      sh->rescol = 3;
    if (sh->maxrows > 100)     sh->rescol = 4;
    if (sh->maxrows > 1000)    sh->rescol = 5;
    if (sh->maxrows > 10000)   sh->rescol = 6;
    if (sh->maxrows > 100000)  sh->rescol = 7;
    if (sh->maxrows > 1000000) sh->rescol = 8;

    sh->maxcols = newcols;
    return (TRUE);
}

/**
 * \brief ATBL(): function to get ent from grid
 *
 * \param[in] tlb
 * \param[in] row
 * \param[in] col
 *
 * \return none
 */

struct ent ** ATBL(struct sheet * sh, struct ent ***tbl, int row, int col) {
    struct ent **ent=(*(sh->tbl+row)+(col));
    struct ent *v= *ent;

    if ((v) && (v->trigger) && ((v->trigger->flag & TRG_READ) == TRG_READ)) {
        //sc_debug("row:%d %d", v->row, v->col);
        do_trigger(v,TRG_READ);
    }
    return ent;
}
