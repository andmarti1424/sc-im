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

void checkbounds(int *rowp, int *colp) {
    if (*rowp < 0)
        *rowp = 0;
    else if (*rowp >= maxrows) {
        if (*colp >= maxcols) {
            if (!growtbl(GROWBOTH, *rowp, *colp)) {
                *rowp = maxrows - 1;
                *colp = maxcols - 1;
            }
            return;
        } else {
            if (!growtbl(GROWROW, *rowp, 0))
                *rowp = maxrows - 1;
            return;
        }
    }
    if (*colp < 0)
        *colp = 0;
    else if (*colp >= maxcols) {
        if (!growtbl(GROWCOL, 0, *colp))
            *colp = maxcols - 1;
    }
}
#endif /* ! PSC */

/* scxrealloc will just scxmalloc if oldptr is == NULL */
#define GROWALLOC(newptr, oldptr, nelem, type, msg) \
    newptr = (type *)scxrealloc((char *)oldptr, \
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

int growtbl(int rowcol, int toprow, int topcol) {
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

    newrows = maxrows;
#endif /* ! PSC */
    newcols = maxcols;
    if (rowcol == GROWNEW) {
#ifndef PSC
        maxrows = toprow = 0;
    /* when we first start up, fill the screen w/ cells */
        {
        int startval;
        startval = LINES - RESROW;
        newrows = startval > MINROWS ? startval : MINROWS;
        startval = ((COLS) - rescol) / DEFWIDTH;
        newcols = startval > MINCOLS ? startval : MINCOLS;
        }
#else
        newcols = MINCOLS;
#endif /* !PSC */
        maxcols = topcol = 0;
    }
#ifndef PSC
    /* set how much to grow */
    if ((rowcol == GROWROW) || (rowcol == GROWBOTH)) {
        if ((maxcols == MAXROWS) || (toprow >= MAXROWS)) {
            sc_error(nolonger);
            return (FALSE);
        }

        if (toprow > maxrows)
            newrows = GROWAMT + toprow;
        else
            newrows += GROWAMT;

        /* If we're close but less than MAXROWS, clip to max value */
        if ( newrows > MAXROWS )
            newrows = MAXROWS;
    }
#endif /* !PSC */
    if ((rowcol == GROWCOL) || (rowcol == GROWBOTH)) {
        if ((maxcols == ABSMAXCOLS) || (topcol >= ABSMAXCOLS)) {
            sc_error(nowider);
            return (FALSE);
        }

        if (topcol > maxcols)
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

        GROWALLOC(row_hidden2, row_hidden, newrows, unsigned char, nolonger);
        memset(row_hidden + maxrows, 0, (newrows - maxrows) * sizeof(unsigned char));

        GROWALLOC(row_format2, row_format, newrows, unsigned char, nolonger);
        memset(row_format + maxrows, 1, (newrows - maxrows) * sizeof(unsigned char));

        GROWALLOC(row_frozen2, row_frozen, newrows, unsigned char, nolonger);
        memset(row_frozen + maxrows, 0, (newrows - maxrows) * sizeof(unsigned char));

        /*
         * alloc tbl row pointers, per net.lang.c, calloc does not
         * necessarily fill in NULL pointers
         */
        GROWALLOC(tbl2, tbl, newrows, struct ent **, nolonger);
        for (lnullit = tbl + maxrows, lcnt = 0; lcnt < newrows - maxrows; lcnt++, lnullit++)
            *lnullit = (struct ent **)NULL;
  /*    memset(tbl + maxrows, (char *) NULL, (newrows - maxrows) * (sizeof(struct ent **)));*/
    }
#endif /* !PSC */

    if ((rowcol == GROWCOL) || (rowcol == GROWBOTH) || (rowcol == GROWNEW)) {
        GROWALLOC(fwidth2, fwidth, newcols, int, nowider);
        GROWALLOC(precision2, precision, newcols, int, nowider);
        GROWALLOC(realfmt2, realfmt, newcols, int, nowider);
#ifdef PSC
        memset(fwidth + maxcols, 0, (newcols - maxcols) * sizeof(int));
        memset(precision + maxcols, 0, (newcols - maxcols) * sizeof(int));
        memset(realfmt + maxcols, 0, (newcols - maxcols) * sizeof(int));
    }
#else
        GROWALLOC(col_hidden2, col_hidden, newcols, unsigned char, nowider);
        memset(col_hidden + maxcols, 0, (newcols - maxcols) * sizeof(unsigned char));
        for (i = maxcols; i < newcols; i++) {
            fwidth[i] = DEFWIDTH;
            precision[i] = DEFPREC;
            realfmt[i] = DEFREFMT;
        }

        GROWALLOC(col_frozen2, col_frozen, newcols, unsigned char, nolonger);
        memset(col_frozen + maxcols, 0, (newcols - maxcols) * sizeof(unsigned char));

        /* [re]alloc the space for each row */
        for (i = 0; i < maxrows; i++) {
            if ((tbl[i] = (struct ent **) scxrealloc((char *)tbl[i], (unsigned) (newcols * sizeof(struct ent **)))) == (struct ent **)0) {
                sc_error(nowider);
                return(FALSE);
            }
            for (nullit = ATBL_P(tbl, i, maxcols), cnt = 0; cnt < newcols - maxcols; cnt++, nullit++)
                *nullit = (struct ent *)NULL;
        /*        memset((char *) ATBL(tbl,i, maxcols), 0, (newcols - maxcols) * sizeof(struct ent **)); */
        }
    }
    else
        i = maxrows;

    /* fill in the bottom of the table */
    for (; i < newrows; i++) {
        if ((tbl[i] = (struct ent **) scxmalloc((unsigned)(newcols * sizeof(struct ent **)))) == (struct ent **) 0) {
            sc_error(nowider);
            return(FALSE);
        }
        for (nullit = tbl[i], cnt = 0; cnt < newcols; cnt++, nullit++)
            *nullit = (struct ent *)NULL;
        /* memset((char *) tbl[i], 0, newcols * sizeof(struct ent **));*/
    }

    maxrows = newrows;

    if (maxrows > 1000)    rescol = 5;
    if (maxrows > 10000)   rescol = 6;
    if (maxrows > 100000)  rescol = 7;
    if (maxrows > 1000000) rescol = 8;
#endif /* PSC */

    maxcols = newcols;
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

struct ent ** ATBL(struct ent ***tbl, int row, int col) {
    struct ent **ent=(*(tbl+row)+(col));
    struct ent *v= *ent;

    if ((v) && (v->trigger) && ((v->trigger->flag & TRG_READ) == TRG_READ))
          do_trigger(v,TRG_READ);
    return ent;
}
