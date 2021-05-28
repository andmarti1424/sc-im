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
 * \file hide_show.c
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 2017-07-18
 * \brief TODO Write a tbrief file description.
 */

#include <stdlib.h>

#include "../sc.h"
#include "../macros.h"
#include "../tui.h"
#include "hide_show.h"
#include "../conf.h"
#include "../vmtbl.h"   // for growtbl

#ifdef UNDO
#include "../undo.h"
extern struct undo undo_item;
#endif

extern struct session * session;


/**
 * \brief Mark a row as hidden
 *
 * \param[in] from_row
 * \param[in] arg
 *
 * \return none
 */
void hide_row(int from_row, int arg) {
    struct roman * roman = session->cur_doc;
    struct sheet * sh = roman->cur_sh;
    register int r2;

    r2 = from_row + arg - 1;
    if (from_row < 0 || from_row > r2) {
        sc_error("Cannot hide row: Invalid range.");
        return;
    }
    if (r2 >= sh->maxrows - 1) {
        // error: tried to hide a row higher than maxrow.
        lookat(sh, from_row + arg + 1, sh->curcol); //FIXME this HACK
        if (! growtbl(sh, GROWROW, arg + 1, 0)) {
            sc_error("You can't hide the last row");
            return;
        }
    }

    if (! roman->loading) {
        roman->modflg++;
        #ifdef UNDO
        create_undo_action();
        undo_hide_show(from_row, -1, 'h', arg);
        end_undo_action();
        #endif
    }
    while ( from_row <= r2)
        sh->row_hidden[ from_row++ ] = TRUE;
    return;
}


/**
 * \brief Mark a column as hidden
 *
 * \param[in] from_col
 * \param[in] arg
 *
 * \return none
 */
void hide_col(int from_col, int arg) {
    struct roman * roman = session->cur_doc;
    struct sheet * sh = roman->cur_sh;
    int c2 = from_col + arg - 1;
    if (from_col < 0 || from_col > c2) {
        sc_error ("Cannot hide col: Invalid range.");
        return;
    }
    if (c2 >= sh->maxcols - 1) {
        // sc_error: tried to hide a column higher than maxcol.
        lookat(sh, sh->currow, from_col + arg + 1); //FIXME this HACK
        if ((arg >= ABSMAXCOLS - 1) || ! growtbl(sh, GROWCOL, 0, arg + 1)) {
            sc_error("You can't hide the last col");
            return;
        }
    }

    if (! roman->loading) {
        roman->modflg++;
        #ifdef UNDO
        create_undo_action();
        create_undo_action();
        create_undo_action();
        undo_hide_show(-1, from_col, 'h', arg);
        end_undo_action();
        #endif
    }
    while (from_col <= c2)
        sh->col_hidden[ from_col++ ] = TRUE;
    return;
}


/**
 * \brief Mark a row as not-hidden
 *
 * \param[in] from_row
 * \param[in] arg
 *
 * \return none
 */
void show_row(int from_row, int arg) {
    struct roman * roman = session->cur_doc;
    struct sheet * sh = roman->cur_sh;
    int r2 = from_row + arg - 1;
    if (from_row < 0 || from_row > r2) {
        sc_error ("Cannot show row: Invalid range.");
        return;
    }
    if (r2 > sh->maxrows - 1) {
        r2 = sh->maxrows - 1;
    }

    roman->modflg++;
    #ifdef UNDO
    create_undo_action();
    #endif
    while (from_row <= r2) {
        #ifdef UNDO
        if (sh->row_hidden[from_row] ) undo_hide_show(from_row, -1, 's', 1);
        #endif
        sh->row_hidden[ from_row++ ] = FALSE;
    }
    #ifdef UNDO
    end_undo_action();
    #endif
    return;
}


/**
 * \brief Mark a column as not-hidden
 *
 * \param[in] from_col
 * \param[in] arg
 *
 * \return none
 */
void show_col(int from_col, int arg) {
    struct roman * roman = session->cur_doc;
    struct sheet * sh = roman->cur_sh;
    int c2 = from_col + arg - 1;
    if (from_col < 0 || from_col > c2) {
        sc_error ("Cannot show col: Invalid range.");
        return;
    }
    if (c2 > sh->maxcols - 1) {
        c2 =  sh->maxcols - 1;
    }

    roman->modflg++;
    #ifdef UNDO
    create_undo_action();
    #endif
    while (from_col <= c2) {
        #ifdef UNDO
        if (sh->col_hidden[from_col] ) undo_hide_show(-1, from_col, 's', 1);
        #endif
         sh->col_hidden[ from_col++ ] = FALSE;
    }
    #ifdef UNDO
    end_undo_action();
    #endif
    return;
}


/**
 * \brief TODO Document show_hiddenrows
 *
 * \return none
 */
void show_hiddenrows() {
    struct roman * roman = session->cur_doc;
    struct sheet * sh = roman->cur_sh;
    int r, c = 0;
    for (r = 0; r < sh->maxrow; r++) {
        if (sh->row_hidden[r]) c++;
    }
    char valores[12 * c + 20];
    valores[0]='\0';
    strcpy(valores, "Hidden rows:\n"); // 20
    for (r = 0; r < sh->maxrow; r++) {
       if (sh->row_hidden[r]) sprintf(valores + strlen(valores), "- %d\n", r); // 12
    }
    ui_show_text(valores);

    return;
}


/**
 * \brief TODO Document show_hiddencols
 *
 * \return none
 */
void show_hiddencols() {
    struct roman * roman = session->cur_doc;
    struct sheet * sh = roman->cur_sh;
    int c, count = 0;
    for (c = 0; c < sh->maxcol; c++) {
        if (sh->col_hidden[c]) count++;
    }
    char valores[8 * c + 20];
    valores[0]='\0';
    strcpy(valores, "Hidden cols:\n"); // 20
    for (c = 0; c < sh->maxcol; c++) {
       if (sh->col_hidden[c]) sprintf(valores + strlen(valores), "- %s\n", coltoa(c)); // 8
    }
    ui_show_text(valores);

    return;
}
