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
 * \file marks.c
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 2017-07-18
 * \brief TODO Write a tbrief file description.
 */

#include <stdlib.h>
#include "marks.h"
#include "macros.h"

#define NUM_MARKS 128
static struct mark * marks;

// 'a' - 'z' = 26
// '0' - '1' = 10
/**
 * \brief TODO Document create_mark_array()
 *
 * \return none
 */

void create_mark_array() {
    marks = (struct mark *) calloc(NUM_MARKS, sizeof(struct mark) );
    return;
}

/**
 * \brief TODO Document free_marks_array()
 *
 * \return none
 */

void free_marks_array() {
    free(marks);
    return;
}

/**
 * \brief TODO Document get_mark()
 *
 * \details 'a' = 97
 * \param[in] c
 *
 * \return none
 */

struct mark * get_mark(char c) {
    return (marks + c);
}

/**
 * \brief TODO Document set_range_mark()
 *
 * \param[in] c
 * \param[in] s
 *
 * \return none
 */

void set_range_mark(char c, struct srange * s) {
    // Delete marked ranges when recording a new one with same char
    del_ranges_by_mark(c);

    (marks + c)->rng = s;
    (marks + c)->row = -1;
    (marks + c)->col = -1;
    return;
}

/**
 * \brief TODO Document set_cell_mark()
 *
 * \return none
 */

void set_cell_mark(char c, int row, int col) {
    // Delete marked ranges when recording a new one with same char
    del_ranges_by_mark(c);

    (marks + c)->rng = NULL;
    (marks + c)->row = row;
    (marks + c)->col = col;
    return;
}

/**
 * \brief TODO Document fix_marks()
 *
 * \param[in] deltar
 * \param[in] deltac
 * \param[in] row_desde
 * \param[in] row_hasta
 * \param[in] coldesde
 * \param[in] col_hasta
 *
 * \return none
 */

void fix_marks(int deltar, int deltac, int row_desde, int row_hasta, int col_desde, int col_hasta) {
    int i;
    for (i = 0; i < NUM_MARKS-1; i++) {
        struct mark * m = marks + i;
        if (m->row >= row_desde && m->row <= row_hasta &&
            m->col >= col_desde && m->col <= col_hasta ) {
                m->row += deltar;
                m->col += deltac;
        }
    }
    return;
}
