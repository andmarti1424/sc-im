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
 * \file TODO <filename>
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 2017-07-18
 * \brief TODO Write a tbrief file description.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "xls.h"
#include "../sc.h"
#include "../cmds/cmds.h"
#include "../color.h"
#include "../macros.h"
#include "../utils/string.h"

/*
 * xls.h is part of libxls. make sure its installed and headers are in path.
 * build must be done with '-lxlsreader'
 */
#ifdef XLS
#include <xls.h>
#endif

extern struct session * session;

/**
 * \brief TODO <brief function description>
 *
 * \details This function loads an excel file into tbl. As sc-im still
 * does not handle multiple sheets, if excel file has multiple sheets,
 * only the first one is read.
 *
 * \return -1 on error
 */
int open_xls(char * fname, char * encoding) {
#ifdef XLS
    struct roman * roman = session->cur_doc;
    struct sheet * sh = roman->cur_sh;

    // Set date format reading LOCALE
    char fmt[15] = "%d/%m/%Y";

    #ifdef USELOCALE
    #include <locale.h>
    #include <langinfo.h>
    char * loc = NULL;
    char * f = NULL;
    loc = setlocale(LC_TIME, "");

    if (loc != NULL) {
        f = nl_langinfo(D_FMT);
        strcpy(fmt, f);
    }
    #endif

    // Read XLS file
    xlsWorkBook * pWB;
    xlsWorkSheet * pWS;
    WORD r, c;
    pWB = xls_open(fname, encoding);

    wchar_t line_interp[FBUFLEN] = L"";
    struct ent * n;

    if (pWB == NULL) {
        sc_error("Error loading %s", fname);
        return -1;
    }

    pWS = xls_getWorkSheet(pWB, 0); //only the first sheet
    if (pWS == NULL) return -1;
    xls_parseWorkSheet(pWS);

    for (r = 0; r <= pWS->rows.lastrow; r++) { // rows
        for (c = 0; c <= pWS->rows.lastcol; c++) { // cols
            xlsCell * cell = xls_cell(pWS, r, c);
            if ((! cell) || (cell->isHidden)) continue;

            // TODO enable rowspan ?
            //if (cell->rowspan > 1) continue;

            struct st_xf_data * xf = &pWB->xfs.xf[cell->xf];

            //sc_debug("%d %d fmt:%d id:%x %d %d", r, c, xf->format, cell->id, cell->d, cell->l);

            // these are dates
            if (((xf->format >= 14 && xf->format <= 22) ||
                (xf->format >= 165 && xf->format <= 180) ||
                xf->format == 278 || xf->format == 185 || xf->format == 196 || xf->format
                == 217 || xf->format == 326 )
               && cell->id != 0x06
               //&& cell->id != 0x27e
               && cell->id != 0x0BD
               && cell->id != 0x203 ) {

                swprintf(line_interp, FBUFLEN, L"let %s%d=%.15g", coltoa(c), r, (cell->d - 25569) * 86400);
                send_to_interp(line_interp);
                n = lookat(sh, r, c);
                n->format = 0;
                char * s = scxmalloc((unsigned)(strlen(fmt) + 2));
                sprintf(s, "%c", 'd');
                strcat(s, "%d/%m/%Y");
                n->format = s;
                continue;

            // display the value of the cell (either numeric or string)
            } else if (cell->id == 0x27e || cell->id == 0x0BD || cell->id == 0x203) {
                swprintf(line_interp, FBUFLEN, L"let %s%d=%.15g", coltoa(c), r, cell->d);

            } else if (cell->id == 0x06) { // formula
                if (cell->l == 0) {        // its a number
                    swprintf(line_interp, FBUFLEN, L"let %s%d=%.15g", coltoa(c), r, cell->d);
                } else {
                    if (!strcmp((char *) cell->str, "bool")) {          // its boolean, and test cell->d
                        swprintf(line_interp, FBUFLEN, L"label %s%d=\"%s\"", coltoa(c), r, (int) cell->d ? "true" : "false");
                    } else if (! strcmp((char *) cell->str, "error")) { // formula is in error
                        swprintf(line_interp, FBUFLEN, L"label %s%d=\"%s\"", coltoa(c), r, "error"); //FIXME
                    } else {
                        swprintf(line_interp, FBUFLEN, L"label %s%d=\"%s\"", coltoa(c), r, (char *) cell->str);
                    }
                }

            } else if (cell->str != NULL) {
                int pad_pos;
                if ((pad_pos = str_in_str((char *) cell->str, "\n")) != -1) ((char *) cell->str)[pad_pos] = '\0'; // For spanning
                // clean_carrier((char *) cell->str); // For spanning
                swprintf(line_interp, FBUFLEN, L"label %s%d=\"%s\"", coltoa(c), r, (char *) cell->str);
            } else {
                swprintf(line_interp, FBUFLEN, L"label %s%d=\"%s\"", coltoa(c), r, "");
            }
            send_to_interp(line_interp);
        }
    }
    xls_close_WS(pWS);
    xls_close_WB(pWB);
    auto_fit(sh, 0, sh->maxcols, DEFWIDTH);
    return 0;
#else
    return -1;
#endif
}
