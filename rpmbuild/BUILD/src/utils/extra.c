/*******************************************************************************
 * Copyright (c) 2013-2017, Andrés Martinelli <andmarti@gmail.com              *
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
 * \file extra.c
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 2017-07-18
 * \brief TODO Write a brief file description.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <curses.h>
#include "extra.h"
#include "../sc.h"
#include "../macros.h"
//#include "../range.h"

extern int find_range(char * name, int len, struct ent * lmatch, struct ent * rmatch, struct range ** rng);

#define freen(x) nofreeNULL(x)
void nofreeNULL(void *x) {
    if (x != NULL)
        free(x);
    return;
}

/**
 * \brief Returns the ROW/COL cell name
 *
 * \param[in] row
 * \param[in] col
 *
 * \return cell name (e.g. 'D4')
 */

char * v_name(int row, int col) {
    struct ent *v;
    struct range *r;
    static char buf[20];

    v = lookat(row, col);
    if ( ! find_range((char *) 0, 0, v, v, &r) ) {
        return (r->r_name);
    } else {
        (void) sprintf(buf, "%s%d", coltoa(col), row);
        return (buf);
    }
}

/**
 * \brief Parse BUF_IN to get a cell name. Skip first blocks with IGNORE_FIRST_BLOCKS
 *
 * \param[in] ignore_first_blocks
 * \param[in] buf_in
 *
 * \return cell name
 */

char * parse_cell_name(int ignore_first_blocks, struct block * buf_in) {
    struct block * b = buf_in;
    static char cell_name[3]; //length of max col is 3 (ZZZ)
    cell_name[0] = '\0';

    while (ignore_first_blocks--) b = b->pnext;
    while( b != NULL) {
          (void) sprintf(cell_name + strlen(cell_name), "%c", b->value);
          b = b->pnext;
    }
    return cell_name;
}
