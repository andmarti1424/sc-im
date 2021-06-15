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
 * \file marks.c
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 28/05/2021
 * \brief source code for handling marks
 * NOTE: 'a' - 'z' = 26
 *       '0' - '1' = 10
 *             'a' = 97.
 */

#include <stdlib.h>
#include "marks.h"
#include "macros.h"

#define NUM_MARKS 128

static struct mark * marks;

/**
 * \brief create_mark_array()
 * \details create structure to save the different marks.
 * \return none
 */
void create_mark_array() {
    marks = (struct mark *) calloc(NUM_MARKS, sizeof(struct mark) );
    return;
}


/**
 * \brief free_marks_array()
 * \return none
 */
void free_marks_array() {
    free(marks);
    marks = NULL;
    return;
}


/**
 * \brief get_mark()
 * \details get a mark based on the corresponding char.
 * \param[in] char c
 * \return struct mark *
 */
struct mark * get_mark(char c) {
    return (marks + c);
}


/**
 * \brief save a range over a mark
 * \param[in] char c
 * \param[in] struct sheet * sh
 * \param[in] struct srange * s
 * \return none
 */
void set_range_mark(char c, struct sheet * sh, struct srange * s) {
    // Delete marked ranges when recording a new one with same char
    del_ranges_by_mark(c);

    (marks + c)->sheet = sh;
    (marks + c)->rng = s;
    (marks + c)->row = -1;
    (marks + c)->col = -1;
    return;
}


/**
 * \brief set_cell_mark()
 * \param[in] char c
 * \param[in] struct sheet * sh
 * \param[in] int row
 * \param[in] int col
 * \return none
 */
void set_cell_mark(char c, struct sheet * sh, int row, int col) {
    // Delete marked ranges when recording a new one with same char
    del_ranges_by_mark(c);

    (marks + c)->sheet = sh;
    (marks + c)->rng = NULL;
    (marks + c)->row = row;
    (marks + c)->col = col;
    return;
}


/**
 * \brief fix_marks()
 * \details modify marks after some operations that modify the internal row or command.
 * such as delete a row or column.
 * \param[in] sheet
 * \param[in] deltar
 * \param[in] deltac
 * \param[in] row_desde
 * \param[in] row_hasta
 * \param[in] coldesde
 * \param[in] col_hasta
 * \return none
 */
void fix_marks(struct sheet * sh, int deltar, int deltac, int row_desde, int row_hasta, int col_desde, int col_hasta) {
    int i;
    for (i = 0; i < NUM_MARKS-1; i++) {
        struct mark * m = marks + i;
        if (m->row >= row_desde && m->row <= row_hasta &&
            m->col >= col_desde && m->col <= col_hasta &&
            m->sheet == sh) {
                m->row += deltar;
                m->col += deltac;
                if (m->row < 0) m->row = 0;
                if (m->col < 0) m->col = 0;
        }
    }
    return;
}

/**
 * \brief clean_marks_by_sheet()
 * \details clean the marks links to a sheet
 * \param[in] struct sheet * sh
 * \return none
 */
void clean_marks_by_sheet(struct sheet * sh) {
    int i;
    if (marks == NULL) return;
    for (i = 0; i < NUM_MARKS-1; i++) {
        struct mark * m = marks + i;
        if (m->sheet == sh) {
            m->sheet = NULL;
            m->row = 0;
            m->col = 0;
        }
    }
    return;
}
