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
 * \file pipe.c
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 2017-07-18
 * \brief TODO Write a tbrief file description.
 */

/*
 * Adaptation of Chuck Martin's code - <nrocinu@myrealbox.com>
 */

#include <unistd.h>
#include "sc.h"
#include "conf.h"
#include "main.h"
#include "interp.h"
#include "macros.h"
#include "tui.h"

extern struct roman * roman;
// FIXME - pass fd is not neccesary?
/**
 * \brief TODO Document getnum()
 *
 * \param[in] r0
 * \param[in] c0
 * \param[in] rn
 * \param[in] cn
 * \param[in[ df
 *
 * \return none
 */

void getnum(int r0, int c0, int rn, int cn, FILE * fd) {
    struct sheet * sh = roman->cur_sh;
    struct ent ** pp;
    struct ent * p;
    int r, c;

    for (r = r0; r <= rn; r++) {
        for (c = c0, pp = ATBL(sh, sh->tbl, r, c); c <= cn; pp++, c++) {
            *line = '\0';
            p = *pp;
            if (p) {
                if (p->cellerror) {
                    sprintf(line, "%s", (*pp)->cellerror == CELLERROR ?  "ERROR" : "INVALID");
                } else if (p->flags & is_valid) {
                    sprintf(line, "%.15g", p->v);
                }
            }
            sc_value("%s", line);
            if (brokenpipe) {
                linelim = -1;
                return;
            }

        }
    }
    linelim = -1;
}

/**
 * \brief TODO Document getformat()
 *
 * \param[in] col
 * \param[in] df
 *
 * \return none
 */

void getformat(int col, FILE * fd) {
    struct sheet * sh = roman->cur_sh;
    sprintf(line, "%d %d %d\n", sh->fwidth[col], sh->precision[col], sh->realfmt[col]);
    //write(fd, line, strlen(line));
    sc_value("%s", line);
    linelim = -1;
}

/**
 * \brief TODO Document getfmt()
 *
 * \param[in] r0
 * \param[in] c0
 * \param[in] rn
 * \param[in] cn
 * \param[in] fd
 *
 * \return none
 */

void getfmt(int r0, int c0, int rn, int cn, FILE * fd) {
    struct sheet * sh = roman->cur_sh;
    struct ent    **pp;
    int        r, c;

    for (r = r0; r <= rn; r++) {
        for (c = c0, pp = ATBL(sh, sh->tbl, r, c); c <= cn; pp++, c++) {
            *line = '\0';
            if (*pp && (*pp)->format) sprintf(line, "%s", (*pp)->format);
            sc_value("%s", line);
            if (brokenpipe) {
                linelim = -1;
                return;
            }
        }
    }
    linelim = -1;
}

/**
 * \brief TODO Document getstring()
 *
 * \param[in] r0
 * \param[in] c0
 * \param[in] rn
 * \param[in] cn
 * \param[in] fd
 *
 * \return none
 */

void getstring(int r0, int c0, int rn, int cn, FILE * fd) {
    struct sheet * sh = roman->cur_sh;
    struct ent    **pp;
    int        r, c;

    for (r = r0; r <= rn; r++) {
        for (c = c0, pp = ATBL(sh, sh->tbl, r, c); c <= cn; pp++, c++) {
            *line = '\0';
            if (*pp && (*pp)->label)
                sprintf(line, "%s", (*pp)->label);
            sc_value("%s", line);
            if (brokenpipe) {
                linelim = -1;
                return;
            }
        }
    }
    linelim = -1;
}

/**
 * \brief TODO Document getexp()
 *
 * \param[in] r0
 * \param[in] c0
 * \param[in] rn
 * \param[in] cn
 * \param[in] fd
 *
 * \return none
 */

void getexp(int r0, int c0, int rn, int cn, FILE * fd) {
    struct sheet * sh = roman->cur_sh;
    struct ent    **pp;
    struct ent    *p;
    int        r, c;

    for (r = r0; r <= rn; r++) {
        for (c = c0, pp = ATBL(sh, sh->tbl, r, c); c <= cn; pp++, c++) {
            *line = '\0';
            p = *pp;
            if (p && p->expr) {
                linelim = 0;
                decompile(p->expr, 0);    /* set line to expr */
                line[linelim] = '\0';
                if (*line == '?')
                    *line = '\0';
            }
            sc_value("%s", line);
            if (brokenpipe) {
                linelim = -1;
                return;
            }
        }
    }
    linelim = -1;
}
