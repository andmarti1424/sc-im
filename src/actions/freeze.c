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
 * \file freeze.c
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 28/05/2021
 * \brief source code for handling the freeze of rows and columns.
 */

#include <stdlib.h>

#include "freeze.h"
#include "../macros.h"
#include "../tui.h"
#include "../undo.h"

extern struct session * session;

/**
 * \brief handle_freeze. freeze/unfreeze a row/column
 * \param[in] tl_ent: top ent that defines area
 * \param[in] br_ent: bottom ent that defines area
 * \param[in] value: 0 (unfreeze) or 1 (freeze)
 * \param[in] type: 'r' or 'c'
 * \return none
 */
void handle_freeze(struct sheet * sh, struct ent * tl_ent, struct ent * br_ent, char value, char type) {
    int i;

#ifdef UNDO
    create_undo_action();
#endif
    if (type == 'r')
        for (i=tl_ent->row; i<=br_ent->row; i++) {
            sh->row_frozen[i]=value;
#ifdef UNDO
            undo_freeze_unfreeze(i, -1, value == 1 ? 'f' : 'u', 1);
#endif
        }
    else if (type == 'c')
        for (i=tl_ent->col; i<=br_ent->col; i++) {
            sh->col_frozen[i]=value;
#ifdef UNDO
            undo_freeze_unfreeze(-1, i, value == 1 ? 'f' : 'u', 1);
#endif
        }
#ifdef UNDO
    end_undo_action();
#endif
    return;
}
