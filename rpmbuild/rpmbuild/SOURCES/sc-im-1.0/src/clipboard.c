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
 * \file clipboard.c 
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 2017-07-18
 * \brief Clipboard functions
 *
 */

#include <wchar.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "sc.h"
#include "macros.h"
#include "tui.h"
#include "clipboard.h"
#include "cmds.h"
#include "file.h"
#include "conf.h"
#include "utils/string.h"

/**
* \brief Pastes from clipboard
*
* \return 0 on success; -1 on error
*/

int paste_from_clipboard() {
    if (! strlen(get_conf_value("default_paste_from_clipboard_cmd"))) return -1;

    // create tmp file
    char template[] = "/tmp/sc-im-clipboardXXXXXX";
    int fd = mkstemp(template);
    if (fd == -1) {
        sc_error("Error while pasting from clipboard");
        return -1;
    }

    // get temp file pointer based on temp file descriptor
    //FILE * fpori = fdopen(fd, "w");

    // copy content from clipboard to temp file
    char syscmd[PATHLEN + strlen(get_conf_value("default_paste_from_clipboard_cmd")) + 1];
    sprintf(syscmd, "%s", get_conf_value("default_paste_from_clipboard_cmd"));
    sprintf(syscmd + strlen(syscmd), " >> %s", template);
    system(syscmd);

    // traverse the temp file
    FILE * fp = fdopen(fd, "r");
    char line_in[BUFFERSIZE];
    wchar_t line_interp[FBUFLEN] = L"";
    int c, r = currow;
    char * token;
    char delim[2] = { '\t', '\0' } ;

    while ( ! feof(fp) && (fgets(line_in, sizeof(line_in), fp) != NULL) ) {
        // Split string using the delimiter
        token = xstrtok(line_in, delim);
        c = curcol;
        while( token != NULL ) {
            if (r > MAXROWS - GROWAMT - 1 || c > ABSMAXCOLS - 1) break;
            clean_carrier(token);
            //char * st = str_replace (token, " ", ""); //trim
            char * st = token;
            if (strlen(st) && isnumeric(st))
                swprintf(line_interp, BUFFERSIZE, L"let %s%d=%s", coltoa(c), r, st);
            else
                swprintf(line_interp, BUFFERSIZE, L"label %s%d=\"%s\"", coltoa(c), r, st);
            if (strlen(st)) send_to_interp(line_interp);
            c++;
            token = xstrtok(NULL, delim);
            //free(st);
            if (c > maxcol) maxcol = c;
        }
        r++;
        if (r > maxrow) maxrow = r;
        if (r > MAXROWS - GROWAMT - 1 || c > ABSMAXCOLS - 1) break;
    }
    sc_info("Content pasted from clipboard");

    // close file descriptor
    close(fd);

    // remove temp file
    unlink(template);
    return 0;
}

/**
* @brief Copies to clipboard
*
* \return 0 on success; -1 on error
*/

int copy_to_clipboard(int r0, int c0, int rn, int cn) {
    if (! strlen(get_conf_value("default_copy_to_clipboard_cmd"))) return -1;

    // create tmp file
    char template[] = "/tmp/sc-im-clipboardXXXXXX";
    int fd = mkstemp(template);
    if (fd == -1) {
        sc_error("Error while copying to clipboard");
        return -1;
    }

    // get temp file pointer based on temp file descriptor
    FILE * fp = fdopen(fd, "w");

    // save plain text to file
    save_plain(fp, r0, c0, rn, cn);
    fclose(fp);

    // copy to clipboard
    char syscmd[PATHLEN + strlen(get_conf_value("default_copy_to_clipboard_cmd")) + 1];
    sprintf(syscmd, "%s", get_conf_value("default_copy_to_clipboard_cmd"));
    sprintf(syscmd + strlen(syscmd), " %s", template);
    system(syscmd);

    sc_info("Content copied to clipboard");

    // close file descriptor
    close(fd);

    // remove temp file
    unlink(template);

    return 0;
}

/**
* @brief TODO Write a brief function description
*
* \details Note: The file must already be open.
*
* \param[in] fout output file
* \param[in] r0
* \param[in] c0
* \param[in] rn
* \param[in] cn
*
* \return 0 on success
*/

// TODO Does this check if the file is already open?
// TODO What are the returns? Does 0 mean success?
int save_plain(FILE * fout, int r0, int c0, int rn, int cn) {
    int row, col;
    register struct ent ** pp;
    wchar_t out[FBUFLEN] = L"";
    char num [FBUFLEN] = "";
    char text[FBUFLEN] = "";
    char formated_s[FBUFLEN] = "";
    int res = -1;
    int align = 1;

    for (row = r0; row <= rn; row++) {
        // ignore hidden rows
        //if (row_hidden[row]) continue;

        for (pp = ATBL(tbl, row, col = c0); col <= cn; col++, pp++) {
            // ignore hidden cols
            //if (col_hidden[col]) continue;

            if (*pp) {
                num [0] = '\0';
                text[0] = '\0';
                out [0] = L'\0';
                formated_s[0] = '\0';
                res = -1;
                align = 1;

                // If a numeric value exists
                if ( (*pp)->flags & is_valid) {
                    res = ui_get_formated_value(pp, col, formated_s);
                    // res = 0, indicates that in num we store a date
                    // res = 1, indicates a format is applied in num
                    if (res == 0 || res == 1) {
                        strcpy(num, formated_s);
                    } else if (res == -1) {
                        sprintf(num, "%.*f", precision[col], (*pp)->v);
                    }
                }

                // If a string exists
                if ((*pp)->label) {
                    strcpy(text, (*pp)->label);
                    align = 1;                                // right alignment
                    if ((*pp)->flags & is_label) {            // center alignment
                        align = 0;
                    } else if ((*pp)->flags & is_leftflush) { // left alignment
                        align = -1;
                    } else if (res == 0) {                    // res must ¿NOT? be zero for label to be printed
                        text[0] = '\0';
                    }
                }
                if (! atoi(get_conf_value("copy_to_clipboard_delimited_tab"))) {
                    pad_and_align(text, num, fwidth[col], align, 0, out);
                    fwprintf(fout, L"%ls", out);
                } else if ( (*pp)->flags & is_valid) {
                    fwprintf(fout, L"%s\t", num);
                } else if ( (*pp)->label) {
                    fwprintf(fout, L"%s\t", text);
                }
            } else if (! atoi(get_conf_value("copy_to_clipboard_delimited_tab"))) {
                fwprintf(fout, L"%*s", fwidth[col], " ");
            } else {
                fwprintf(fout, L"\t");
            }
        }
        if (row != rn) fwprintf(fout, L"\n");
    }
    return 0;
}
