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
 * \file sort.c
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 2017-07-18
 * \brief TODO Write a brief file description.
 */

/* Adaptation of Chuck Martin's code - <nrocinu@myrealbox.com> */

#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

#include "../macros.h"
#include "../yank.h"
#include "../cmds/cmds.h"
#include "../conf.h"
#include "../color.h"
#include "../xmalloc.h" // for scxfree

int compare(const void * row1, const void * row2);

struct sortcrit {
    int direction, type, column;
} * sort;

int howmany;
extern struct session * session;

/**
 * \brief TODO Write a brief function description>
 *
 * \param[in] left
 * \param[in] right
 * \param[in] criteria
 *
 * \return none
 */

void sortrange(struct sheet * sh, struct ent * left, struct ent * right, char * criteria) {
    struct roman * roman = session->cur_doc;
    int minr, minc, maxr, maxc, r, c;
    int * rows, col = 0;
    int cp = 0;

    minr = left->row < right->row ? left->row : right->row;
    minc = left->col < right->col ? left->col : right->col;
    maxr = left->row > right->row ? left->row : right->row;
    maxc = left->col > right->col ? left->col : right->col;

    sort = (struct sortcrit *) scxmalloc((2 * sizeof(struct sortcrit)));

    // Save 'ent' elements in the range to the 'rows' structure
    rows = (int *) scxmalloc((maxr - minr + 1) * sizeof(int));
    for (r = minr, c = 0; r <= maxr; r++, c++)
        rows[c] = r;

    if (! criteria) {
        sort[0].direction = 1;
        sort[0].type = 1;
        sort[0].column = minc;
        sort[1].direction = 1;
        sort[1].type = 0;
        sort[1].column = minc;
        howmany = 2;
    } else {
        howmany = 0;
        while (criteria[cp]) {
            if (howmany > 1)
                sort = (struct sortcrit *) scxrealloc((char *) sort,  (howmany + 1) * (sizeof(struct sortcrit)));

            switch (criteria[cp++]) {
                case '+':
                    sort[howmany].direction = 1;
                    break;
                case '-':
                    sort[howmany].direction = -1;
                    break;
                default:
                    sc_error("Invalid sort criteria");
                    return;
            }
            switch (criteria[cp++]) {
                case '#':
                    sort[howmany].type = 0;
                    break;
                case '$':
                    sort[howmany].type = 1;
                    break;
                default:
                    sc_error("Invalid sort criteria");
                    return;
            }
            if (criteria[cp]) {
                col = toupper(criteria[cp++]) - 'A';
            } else {
                sc_error("Invalid sort criteria");
                return;
            }
            if (criteria[cp] && criteria[cp] != '+' && criteria[cp] != '-' && criteria[cp] != ';')
                col = (col + 1) * 26 + toupper(criteria[cp++]) - 'A';
            sort[howmany].column = col;
            if (col < minc || col > maxc) {
                sc_error("Invalid sort criteria");
                return;
            }
            cp++;
            howmany++;
            if (cp > strlen(criteria))
                break;
        }
    }

    // Sort 'rows' structure
    qsort(rows, maxr - minr + 1, sizeof(int), compare);

    //currow = minr;
    //curcol = minc;

    yank_area(sh, minr, minc, maxr, maxc, 's', 1); // save yanklist in the original range


    // Fix the 'ent' elements in the sorted range
    struct ent_ptr * p_aux, * yl = get_yanklist();

    for (c = 0, p_aux = yl; p_aux; p_aux = p_aux->next) {
        if (rows[c] != p_aux->vp->row) {
            for (c = 0; c <= maxr - minr && rows[c] != p_aux->vp->row; c++) ;
            if (c > maxr - minr) {
                sc_error("sort error");
                return;
            }
        }
        p_aux->vp->row = minr + c;
    }

    sh->currow = minr;
    sh->curcol = minc;

    paste_yanked_ents(sh, 0, 's'); // paste ents over currow and curcol
    roman->modflg++;

    scxfree((char *) sort);
    scxfree((char *) rows);

    if (criteria) scxfree(criteria);
}

/**
 * \brief TODO Write a brief function description>
 *
 * \param[in] row1
 * \param[in] row2
 *
 * \return result
 */

int compare(const void * row1, const void * row2) {
    struct roman * roman = session->cur_doc;
    struct sheet * sh = roman->cur_sh;
    struct ent * p1;
    struct ent * p2;
    double diff;
    int result = 0;
    int i;

    for (i = 0; !result && i < howmany; i++) {
        p1 = *ATBL(sh, sh->tbl, *((int *) row1), sort[i].column);
        p2 = *ATBL(sh, sh->tbl, *((int *) row2), sort[i].column);

        if (sort[i].type) {
            if (p1 && p1->label) {
                if (p2 && p2->label) {
                    result = strcmp(p1->label, p2->label);
                } else {
                    result = -1;
                }
            } else if (p2 && p2->label) {
                result = 1;
            }
        } else if (p1 && p2 && p1->flags & is_valid && p2->flags & is_valid) {
                diff = (p1->v - p2->v);
                result = (diff > 0 ? 1 : diff < 0 ? -1 : 0);
        } else if (p1 && p1->flags & is_valid) {
                result = -1;
        } else if (p2 && p2->flags & is_valid) {
            result = 1;
        }
        result *= sort[i].direction;
    }

    if (! result) result = (*((int *) row1) - *((int *) row2));

    return (result);
}
