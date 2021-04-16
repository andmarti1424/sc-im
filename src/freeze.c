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
 * \file freeze.c
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 2017-07-18
 * \brief TODO Write a tbrief file description.
 */

#include <stdlib.h>

#include "freeze.h"
#include "macros.h"
#include "tui.h"

struct frange * freeze_ranges = NULL;

/**
 * \brief TODO Document add_frange()
 * \details type = 'r' -> freeze a row
 * \details type = 'c' -> freeze a col
 * \details type = 'a' -> freeze an area
 * \param[in] tl_ent
 * \param[in] br_ent
 * \param[in] type
 * \return none
 */

void add_frange(struct ent * tl_ent, struct ent * br_ent, char type) {
    struct frange * f = (struct frange *) malloc(sizeof(struct frange));
    f->tl = tl_ent;
    f->br = br_ent;
    f->type = type;
    f->next = freeze_ranges;
    if (freeze_ranges != NULL) free(freeze_ranges);
    freeze_ranges = f;

    //sc_debug("freeze range: %d %d %d %d - type:%c", freeze_ranges->tl->row, freeze_ranges->tl->col, freeze_ranges->br->row, freeze_ranges->br->col, type);
    return;
}

/**
 * \brief handle_freeze. freeze/unfreeze a row/column
 * \param[in] tl_ent: top ent that defines area
 * \param[in] br_ent: bottom ent that defines area
 * \param[in] value: 0 (unfreeze) or 1 (freeze)
 * \param[in] type: 'r' or 'c'
 * \return none
 */
void handle_freeze(struct ent * tl_ent, struct ent * br_ent, char value, char type) {
    int i;
    if (type == 'r')
        for (i=tl_ent->row; i<=br_ent->row; i++) row_frozen[i]=value;
    else if (type == 'c')
        for (i=tl_ent->col; i<=br_ent->col; i++) col_frozen[i]=value;

    return;
}

/**
 * \brief TODO Document remove_frange()
 * \return none
 */
void remove_frange() {
    extern int center_hidden_rows;
    extern int center_hidden_cols;
    free(freeze_ranges);
    freeze_ranges = NULL;
    center_hidden_rows = 0;
    center_hidden_cols = 0;
    ui_update(TRUE);
    return;
}
