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
 * \file subtotal.c
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 2017-07-18
 * \brief TODO Write a tbrief file description.
 */

#include <wchar.h>
#include "../sc.h"
#include "../macros.h"
#include "../cmds/cmds.h"
#include "shift.h"
#include "../tui.h"

/*
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

#include "yank.h"
#include "conf.h"
#include "color.h"
#include "xmalloc.h" // for scxfree
*/

extern struct session * session;

/**
 * \brief TODO  Document subtotal()
 *
 * \details Example command: subtotal A @sum C. If you want to replace a presxistant
 * subtotals, you should use: rsubtotal A @sum C.
 *
 * \param[in] r used in defining the range of the data to be rearranged with subtotals
 * \param[in] c used in defining the range of the data to be rearranged with subtotals
 * \param[in] rf used in defining the range of the data to be rearranged with subtotals
 * \param[in] cf used in defining the range of the data to be rearranged with subtotals
 * \param[in] group_col
 * \param[in] operation the operation to be done over the group can be one of the
 * following: @sum, @prod, @avg, @count, @stddev, @max, @min
 * \param[in] ope_col the operation column
 * \param[in] replace_subtotal
 *
 * \return none
 */

int subtotal(int r, int c, int rf, int cf, int group_col, char * operation, int ope_col, int replace_subtotals) {
    struct roman * roman = session->cur_doc;
    struct sheet * sh = roman->cur_sh;
    // check ope_col and group_col are valid
    if (ope_col < c || ope_col > cf || group_col < c || group_col > cf) return -1;

    // check if they are headers in first row
    struct ent * p, * q;
    int headers_in_first_row = 0;
    if ((p = *ATBL(sh, sh->tbl, r, ope_col)) && p->label &&
        (q = *ATBL(sh, sh->tbl, r+1, ope_col)) && ! q->label) headers_in_first_row=1;

    // group operation shall be done over text content !
    wchar_t cline [BUFFERSIZE];
    p = *ATBL(sh, sh->tbl, r + headers_in_first_row, group_col);
    swprintf(cline, BUFFERSIZE, L"+$%s", coltoa(group_col));

    // sort the range
    extern wchar_t interp_line[BUFFERSIZE];
    swprintf(interp_line, BUFFERSIZE, L"sort %s%d:", coltoa(c), r + headers_in_first_row);
    swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%s%d \"%ls\"", coltoa(cf), rf, cline);
    send_to_interp(interp_line);

    // traverse the range and replace subtotals
    //
    // TODO replace subtotals only if replace_subtotals is set
    int i, j, is_subtotal_row;
    extern int cmd_multiplier;
    //if (replace_subtotals) {
    for (i=r+headers_in_first_row; i <= rf; i++) {
        is_subtotal_row=0;
        for (j=c; j<cf; j++) {
            p = *ATBL(sh, sh->tbl, i, j);
            if (p && p->label && p->label[0] == '+' && p->label[1] == '@') { is_subtotal_row=1; break; }
        }
        if (is_subtotal_row) {
           cmd_multiplier = 1;
           shift(sh, i, c, i, cf, L'k');
           i--;
           rf--;
        }
    }
    //}

    // traverse the range and add subtotals
    int new_rows = 0;
    wchar_t cmd[BUFFERSIZE];
    int row_start_range = r + headers_in_first_row;
    for (i=r+headers_in_first_row+1; i <= rf + new_rows + 1; i++) {
        p = *ATBL(sh, sh->tbl, i-1, group_col);
        q = *ATBL(sh, sh->tbl, i, group_col);

        // TODO ignore preexistance subtotals by default

        if ( (p && q && p->label && q->label && strcmp(q->label, p->label) != 0)
           || i == rf + new_rows + 1) {
           cmd_multiplier = 1;
           shift(sh, i, c, i, cf, L'j');

           swprintf(cmd, BUFFERSIZE, L"rightstring %s%d = \"+%s(%s)\"", coltoa(group_col), i, operation, p->label);
           send_to_interp(cmd);

           swprintf(cmd, BUFFERSIZE, L"let %s%d = %s(%s%d:%s%d)", coltoa(ope_col), i, operation,
           coltoa(ope_col), row_start_range, coltoa(ope_col), i-1);
           send_to_interp(cmd);
           valueize_area(sh, i, ope_col, i, ope_col);

           new_rows++;
           i++;
           row_start_range = i;
        }
    }

    return 0;
}
