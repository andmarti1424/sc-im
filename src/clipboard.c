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
#include "cmds/cmds.h"
#include "file.h"
#include "conf.h"
#include "utils/string.h"

extern struct session * session;


/**
* \brief Paste to sc-im content stored on clipboard
* \return 0 on success; -1 on error
*/
int paste_from_clipboard() {
    struct roman * roman = session->cur_doc;
    char *clipboard_cmd = get_conf_value("default_paste_from_clipboard_cmd");
    if (!clipboard_cmd || !*clipboard_cmd) return -1;

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
    char syscmd[PATHLEN];
    int ret = snprintf(syscmd, PATHLEN, "%s >> %s", clipboard_cmd, template);
    if (ret < 0 || ret >= PATHLEN) {
        sc_error("Error while pasting from clipboard");
        ret = -1;
        goto out;
    }
    ret = 0;
    system(syscmd);

    // traverse the temp file
    FILE * fp = fdopen(fd, "r");
    char line_in[BUFFERSIZE];
    wchar_t line_interp[FBUFLEN] = L"";
    int c, r = roman->cur_sh->currow;
    char * token;
    char delim[2] = { '\t', '\0' } ;

    while ( ! feof(fp) && (fgets(line_in, sizeof(line_in), fp) != NULL) ) {
        // Split string using the delimiter
        token = xstrtok(line_in, delim);
        c = roman->cur_sh->curcol;
        while( token != NULL ) {
            if (r > MAXROWS - GROWAMT - 1 || c > ABSMAXCOLS - 1) break;
            clean_carrier(token);
            char * num = str_replace (token, " ", ""); //trim
            char * st = token;
            if (strlen(num) && isnumeric(num))
                swprintf(line_interp, BUFFERSIZE, L"let %s%d=%s", coltoa(c), r, num);
            else {
                if (strlen(st) > MAX_IB_LEN) {
                    sc_debug("Content from clipboard exceeds maximum width for a label. Cutting it to %d chars", MAX_IB_LEN);
                    st[MAX_IB_LEN-1]='\0';
                }
                char * std = str_replace(st, "\"", "\\\""); // backspace double quotes
                swprintf(line_interp, BUFFERSIZE, L"label %s%d=\"%s\"", coltoa(c), r, std);
                free(std);
            }
            if (strlen(st)) send_to_interp(line_interp);
            c++;
            token = xstrtok(NULL, delim);
            free(num);
            //free(st);
            if (c > roman->cur_sh->maxcol) roman->cur_sh->maxcol = c;
        }
        r++;
        if (r > roman->cur_sh->maxrow) roman->cur_sh->maxrow = r;
        if (r > MAXROWS - GROWAMT - 1 || c > ABSMAXCOLS - 1) break;
    }
    if (get_conf_int("autocalc")) EvalAll();
    sc_info("Content pasted from clipboard");

out:
    // close file descriptor
    close(fd);

    // remove temp file
    unlink(template);
    return ret;
}


/**
 * @brief Copies a range of cells to clipboard
 * \return 0 on success; -1 on error
 */
int copy_to_clipboard(int r0, int c0, int rn, int cn) {
    char *clipboard_cmd = get_conf_value("default_copy_to_clipboard_cmd");
    if (!clipboard_cmd || !*clipboard_cmd) return -1;

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
    char syscmd[PATHLEN];
    int ret = snprintf(syscmd, PATHLEN, "%s %s", clipboard_cmd, template);
    if (ret < 0 || ret >= PATHLEN) {
        sc_error("Error while copying to clipboard");
        ret = -1;
    } else {
        system(syscmd);
        sc_info("Content copied to clipboard");
        ret = 0;
    }

    // close file descriptor
    close(fd);

    // remove temp file
    unlink(template);

    return ret;
}


/**
* @brief save_plain()
* \details copy a range of cell to an open file stream
* Note: The file must already be open.
* \param[in] fout output file
* \param[in] r0
* \param[in] c0
* \param[in] rn
* \param[in] cn
* \return 0 on success
* \return -1 on error
*/
int save_plain(FILE * fout, int r0, int c0, int rn, int cn) {
    if (fout == NULL) return -1;
    struct roman * roman = session->cur_doc;
    int conf_clipboard_delimited_tab = get_conf_int("copy_to_clipboard_delimited_tab");
    int row, col;
    register struct ent ** pp;
    wchar_t out[FBUFLEN] = L"";
    char num [FBUFLEN] = "";
    char text[FBUFLEN] = "";
    char formated_s[FBUFLEN] = "";
    int res = -1;
    int align = 1;
    int emptyfield=-1;

    for (row = r0; row <= rn; row++) {
        // ignore hidden rows
        //if (row_hidden[row]) continue;

        for (pp = ATBL(roman->cur_sh, roman->cur_sh->tbl, row, col = c0); col <= cn; col++, pp++) {
            // ignore hidden cols
            //if (col_hidden[col]) continue;
            emptyfield=-1;

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
                        sprintf(num, "%.*f", roman->cur_sh->precision[col], (*pp)->v);
                    }
                }
                else {
                     emptyfield++;
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
                else {
                     emptyfield++;
               }
                if(emptyfield){
                   fwprintf(fout, L"\t");
                }
                if (! conf_clipboard_delimited_tab) {
                    pad_and_align(text, num, roman->cur_sh->fwidth[col], align, 0, out, roman->cur_sh->row_format[row]);
                    fwprintf(fout, L"%ls", out);
                } else if ( (*pp)->flags & is_valid) {
                    fwprintf(fout, L"%s\t", num);
                } else if ( (*pp)->label) {
                    fwprintf(fout, L"%s\t", text);
                }
            } else if (! conf_clipboard_delimited_tab) {
                fwprintf(fout, L"%*s", roman->cur_sh->fwidth[col], " ");
            } else {
                fwprintf(fout, L"\t");
            }
        }
        if (row != rn) fwprintf(fout, L"\n");
    }
    return 0;
}
