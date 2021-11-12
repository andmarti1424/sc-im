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
 * \file cmds_insert.c
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 2017-07-18
 * \brief TODO Write a tbrief file description.
 */

#include <string.h>
#include <wchar.h>
#include <stdlib.h>

#include "cmds_insert.h"
#include "cmds.h"
#include "cmds_visual.h"
#include "../tui.h"
#include "../buffer.h"
#include "../sc.h"            // for rescol
#include "../utils/string.h"
#include "../marks.h"
#include "../conf.h"

#ifdef INS_HISTORY_FILE
char ori_insert_edit_submode;
#endif

extern void ins_in_line(wint_t d);
extern struct session * session;

/**
 * \brief TODO Document do_insertmode()
 *
 * \param[in] sb
 *
 * returns: none
 */

void do_insertmode(struct block * sb) {
    struct roman * roman = session->cur_doc;
    struct sheet * sh = roman->cur_sh;

    if (sb->value == ctl('v') ) {  // VISUAL SUBMODE
        visual_submode = insert_edit_submode;
        chg_mode('v');
        start_visualmode(sh->currow, sh->curcol, sh->currow, sh->curcol);
        return;

    } else if (sb->value == OKEY_LEFT) {   // LEFT
        if (inputline_pos) {
            real_inputline_pos--;
            int l = wcwidth(inputline[real_inputline_pos]);
            inputline_pos -= l;
        }
        ui_show_header();
        return;

    } else if (sb->value == OKEY_RIGHT) {  // RIGHT
        int max = wcswidth(inputline, wcslen(inputline));
        if (inputline_pos < max) {
            int l = wcwidth(inputline[real_inputline_pos++]);
            inputline_pos += l;
        }
        ui_show_header();
        return;

#ifdef INS_HISTORY_FILE
    } else if (sb->value == OKEY_UP || sb->value == ctl('p') ||         // UP
               sb->value == OKEY_DOWN || sb->value == ctl('n')) {       // DOWN

        int delta = 0, i, cmp;
        if (sb->value == OKEY_UP || sb->value == ctl('p')) {            // up
            for (i=insert_history->pos; -i+1 < insert_history->len; i--, delta--)
                if (wcslen(get_line_from_history(insert_history, 0))) {
                    if (! (cmp = wcsncmp(inputline, &get_line_from_history(insert_history, i-1)[1], wcslen(get_line_from_history(insert_history, 0))))) {
                        delta--;
                        break;
                    } else if (insert_history->len == 2-i && cmp) {
                        delta=0;
                        break;
                    }
                } else if (!wcslen(get_line_from_history(insert_history, 0))) {
                    delta--;
                    break;
                }
        } else if (sb->value == OKEY_DOWN || sb->value == ctl('n')) {   // down
            for (i=insert_history->pos; i != 0; i++, delta++)
                if (wcslen(get_line_from_history(insert_history, 0))) {
                    if (! (cmp = wcsncmp(inputline, &get_line_from_history(insert_history, i+1)[1], wcslen(get_line_from_history(insert_history, 0))))) {
                        delta++;
                        break;
                    } else if (insert_history->pos == 0 && cmp) {
                        delta=0;
                        break;
                    }
                } else if (!wcslen(get_line_from_history(insert_history, 0))) {
                    delta++;
                    break;
                }
        }
        insert_history->pos += delta;

        wchar_t word [COLS];
        wcscpy(word, get_line_from_history(insert_history, insert_history->pos));
        if (insert_history->pos == 0)
            insert_edit_submode = ori_insert_edit_submode;
        else {
            insert_edit_submode = word[0];
            del_wchar(word, 0);
        }
        wcscpy(inputline, word);
        inputline_pos = wcswidth(inputline, real_inputline_pos);

        chg_mode(insert_edit_submode);
        ui_show_header();
        return;
#endif

    } else if (sb->value == OKEY_BS || sb->value == OKEY_BS2) {  // BS
        if ( ! wcslen(inputline) || ! real_inputline_pos ) return;

        int l = wcwidth(inputline[real_inputline_pos - 1]);
        real_inputline_pos--;
        del_wchar(inputline, real_inputline_pos);
        inputline_pos -= l;
        ui_show_header();
#ifdef INS_HISTORY_FILE
        if (insert_history->pos == 0)
            del_wchar(get_line_from_history(insert_history, insert_history->pos), real_inputline_pos); // Clean history
#endif

    } else if (sb->value == OKEY_DEL) {    // DEL
        int max = wcswidth(inputline, wcslen(inputline));
        if (inputline_pos > max) return;
        del_wchar(inputline, real_inputline_pos);
#ifdef INS_HISTORY_FILE
        if (insert_history->pos == 0)
            del_wchar(get_line_from_history(insert_history, insert_history->pos), real_inputline_pos); // Clean history
#endif
        ui_show_header();

    } else if (sb->value == OKEY_TAB) {    // TAB
        if (inputline_pos && wcslen(inputline) >= inputline_pos) {
            real_inputline_pos--;
            int l = wcwidth(inputline[real_inputline_pos]);
            inputline_pos -= l;
        }
        chg_mode(insert_edit_submode == '=' ? 'e' : 'E');
        ui_show_header();

    } else if (find_val(sb, OKEY_ENTER)) { // ENTER
        char ope[BUFFERSIZE] = "";
        wchar_t content[BUFFERSIZE] = L"";
        wcscpy(content, inputline);

        switch (insert_edit_submode) {
            case '=':
                strcpy(ope, "let");
                break;
            case '<':
                strcpy(ope, "leftstring");
                break;
            case '>':
                strcpy(ope, "rightstring");
                break;
            case '\\':
                strcpy(ope, "label");
                break;
        }

        if (content[0] == L'"') {
            del_wchar(content, 0);
        } else if (insert_edit_submode != '=' && content[0] != L'"') {
            add_wchar(content, L'\"', 0);
            add_wchar(content, L'\"', wcslen(content));
        }

        enter_cell_content(sh, sh->currow, sh->curcol, ope, content);

#ifdef INS_HISTORY_FILE
        /*
         * if exists in history an item with same text to the command typed
         * (counting from the second position) it is moved to the beginning of list.
         * (first element in list means last command executed)
         */
        del_item_from_history(insert_history, 0);
        wchar_t copy[BUFFERSIZE];

        swprintf(copy, BUFFERSIZE, L"%c%ls", insert_edit_submode, inputline);
        int moved = move_item_from_history_by_str(insert_history, copy, -1);
        if (! moved) add(insert_history, copy);
        insert_history->pos = 0;
#endif

        inputline[0] = L'\0';
        inputline_pos = 0;
        real_inputline_pos = 0;
        chg_mode('.');
        //ui_clr_header(1);

        char * opt = get_conf_value("newline_action");
        switch (opt[0]) {
            case 'j':
                sh->currow = forw_row(sh, 1)->row;
                break;
            case 'l':
                sh->curcol = forw_col(sh, 1)->col;
                break;
        }
        ui_update(TRUE);
        return;


    } else if (sb->value == OKEY_HOME) {   // HOME
        real_inputline_pos = 0;
        inputline_pos = wcswidth(inputline, real_inputline_pos);
        ui_show_header();
        return;

    } else if (sb->value == OKEY_END) {    // END
        real_inputline_pos = wcslen(inputline);
        inputline_pos = wcswidth(inputline, real_inputline_pos);
        ui_show_header();
        return;

    // Write new char !!
    //} else if ( wcslen(inputline) < (COLS - 16) && sc_isprint(sb->value)) {
    } else if ( sc_isprint(sb->value)) {
        //DEBUG sc_info("2: %d %lc", sb->value, sb->value);
        ins_in_line(sb->value);
        ui_show_header();
#ifdef INS_HISTORY_FILE
        if (insert_history->pos == 0) {          // Only if editing the new command
            wchar_t * sl = get_line_from_history(insert_history, 0);
            add_wchar(sl, sb->value, real_inputline_pos-1); // Insert into history
        }
#endif

    } else if (sb->value == ctl('r') && get_bufsize(sb) == 2 &&        // C-r      // FIXME ???
        (sb->pnext->value - (L'a' - 1) < 1 || sb->pnext->value > 26)) {
        wchar_t cline [BUFFERSIZE];
        int i, r = get_mark(sb->pnext->value)->row;
        if (r != -1) {
            swprintf(cline, BUFFERSIZE, L"%s%d", coltoa(get_mark(sb->pnext->value)->col), r);
        } else {
            swprintf(cline, BUFFERSIZE, L"%s%d:", coltoa(get_mark(sb->pnext->value)->rng->tlcol), get_mark(sb->pnext->value)->rng->tlrow);
            swprintf(cline + wcslen(cline), BUFFERSIZE, L"%s%d", coltoa(get_mark(sb->pnext->value)->rng->brcol), get_mark(sb->pnext->value)->rng->brrow);
        }
        for(i = 0; i < wcslen(cline); i++) ins_in_line(cline[i]);

#ifdef INS_HISTORY_FILE
        if (insert_history->pos == 0) {          // Only if editing the new command
            wchar_t * sl = get_line_from_history(insert_history, 0);
            wcscat(sl, cline);                        // Insert into history
        }
#endif
        ui_show_header();
        return;
    }
    return;
}
