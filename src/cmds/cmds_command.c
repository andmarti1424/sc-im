/*******************************************************************************
 * Copyright (c) 2013-2025, Andrés G. Martinelli <andmarti@gmail.com>          *
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
 *    This product includes software developed by Andrés G. Martinelli         *
 *    <andmarti@gmail.com>.                                                    *
 * 4. Neither the name of the Andrés G. Martinelli nor the                     *
 *   names of other contributors may be used to endorse or promote products    *
 *   derived from this software without specific prior written permission.     *
 *                                                                             *
 * THIS SOFTWARE IS PROVIDED BY ANDRÉS G. MARTINELLI ''AS IS'' AND ANY         *
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED   *
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE      *
 * DISCLAIMED. IN NO EVENT SHALL ANDRÉS G. MARTINELLI BE LIABLE FOR ANY        *
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES  *
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;*
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND *
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT  *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE       *
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.           *
 *******************************************************************************/

/**
 * \file cmds_command.c
 * \author Andrés G. Martinelli <andmarti@gmail.com>
 * \date 2017-07-18
 * \brief TODO Write a tbrief file description.
 */

#include <string.h>
#include <wchar.h>
#include <stdlib.h>
#include <ctype.h>         // for isprint()
#include <signal.h>

#ifndef NO_WORDEXP
#include <wordexp.h>
#endif

#include "cmds_command.h"
#include "cmds_visual.h"
#include "cmds_edit.h"
#include "cmds.h"
#include "../sc.h"            // for rescol
#include "../conf.h"
#include "../utils/string.h"
#include "../utils/dictionary.h"
#include "../tui.h"
#include "../file.h"
#include "../main.h"
#include "../interp.h"
#include "../actions/hide_show.h"
#include "../exec.h"
#include "../help.h"
#include "../marks.h"
#include "../actions/filter.h"
#include "../maps.h"
#include "../formats/xls.h"
#include "../formats/xlsx.h"
#include "../actions/plot.h"

#ifdef UNDO
#include "../undo.h"
#endif

extern char * rev;
extern struct dictionary * user_conf_d;
extern struct session * session;

wchar_t inputline[BUFFERSIZE];
extern wchar_t interp_line[BUFFERSIZE];
int inputline_pos; /**< Position in window. Some chars has 2 chars width */
// see https://en.wikipedia.org/wiki/Halfwidth_and_fullwidth_forms
int real_inputline_pos; /**<  Real position in inputline */

static wchar_t * valid_commands[] = {
L"!",
L"addfilter",
L"autofit",
L"ccopy",
L"cellcolor",
L"cmap",
L"cnoremap",
L"color",
L"cpaste",
L"cunmap",
L"datefmt",
L"define_color",
L"delfilter",
L"delfilters",
L"delsheet",
L"e csv",
L"e! csv",
L"e mkd",
L"e! mkd",
L"e tab",
L"e! tab",
L"e tex",
L"e! tex",
L"e txt",
L"e! txt",
L"e xlsx",
L"e! xlsx",
L"emap",
L"enoremap",
L"eunmap",
L"fcopy",
L"file",
L"fill",
L"filteroff",
L"filteron",
L"format",
L"formatcol",
L"formatrow",
L"freezecol",
L"freezerow",
L"fsum",
L"h",
L"help",
L"hiddencols",
L"hiddenrows",
L"hidecol",
L"hiderow",
L"imap",
L"inoremap",
L"int",
L"iunmap",
L"load!",
L"load",
L"lock",
L"newsheet",
L"nextsheet",
L"nmap",
L"nnoremap",
L"nunmap",
L"pad",
L"plot",
L"plotedit",
L"prevsheet",
L"q!",
L"q",
L"quit!",
L"quit",
L"redefine_color",
L"refresh",
L"renamesheet",
L"set",
L"showcol",
L"showcols",
L"showfilters",
L"showmaps",
L"showrow",
L"showrows",
L"sort",
L"strtonum",
L"subtotal",
L"trigger",
L"unformat",
L"unfreezecol",
L"unfreezerow",
L"unlock",
L"untrigger",
L"valueize",
L"version",
L"vmap",
L"vnoremap",
L"vunmap",
L"w",
L"wq",
L"x",
(wchar_t *) 0
};

/**
 * \brief TODO Document do_commandmode()
 *
 * \param[in] sb
 *
 * \return none
 */

void do_commandmode(struct block * sb) {
    struct roman * roman = session->cur_doc;
    struct sheet * sh = roman->cur_sh;

    // If a visual selected range exists
    int p = is_range_selected();
    struct srange * sr = NULL;
    if (p != -1) sr = get_range_by_pos(p);


    /*
     * Normal KEY handlers for this MODE
     */
    if (sb->value == OKEY_BS || sb->value == OKEY_BS2) {  // BS
        if ( ! wcslen(inputline) || ! real_inputline_pos ) return;
        int l = wcwidth(inputline[real_inputline_pos - 1]);
        real_inputline_pos--;
        del_wchar(inputline, real_inputline_pos);
        inputline_pos -= l;
        ui_show_header();

#ifdef HISTORY_FILE
        if (commandline_history->pos == 0)
            del_wchar(get_line_from_history(commandline_history, commandline_history->pos), real_inputline_pos); // Clean history
#endif
        ui_show_header();
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

    } else if (sb->value == OKEY_DEL) {    // DEL
        if (inputline_pos > wcswidth(inputline, wcslen(inputline))) return;
        del_wchar(inputline, real_inputline_pos);

#ifdef HISTORY_FILE
        if (commandline_history->pos == 0)
            del_wchar(get_line_from_history(commandline_history, commandline_history->pos), real_inputline_pos); // Clean history
#endif
        ui_show_header();
        return;

#ifdef HISTORY_FILE
    } else if (sb->value == OKEY_UP || sb->value == ctl('p') ||         // UP
               sb->value == OKEY_DOWN || sb->value == ctl('n')) {       // DOWN

        int delta = 0, k = 0, i, cmp;
        if (sb->value == OKEY_UP || sb->value == ctl('p')) {            // up
            for (i=commandline_history->pos; -i+1 < commandline_history->len; i--, k--)
                if (wcslen(get_line_from_history(commandline_history, 0))) {
                    if (! (cmp = wcsncmp(inputline, get_line_from_history(commandline_history, i-1), wcslen(get_line_from_history(commandline_history, 0))))) {
                        k--;
                        break;
                    } else if (commandline_history->len == 2-i && cmp) {
                        k=0;
                        break;
                    }
                } else if (!wcslen(get_line_from_history(commandline_history, 0))) {
                    k--;
                    break;
                }
        } else if (sb->value == OKEY_DOWN || sb->value == ctl('n')) {   // down
            for (i=commandline_history->pos; i != 0; i++, k++)
                if (wcslen(get_line_from_history(commandline_history, 0))) {
                    if (! (cmp = wcsncmp(inputline, get_line_from_history(commandline_history, i+1), wcslen(get_line_from_history(commandline_history, 0))))) {
                        k++;
                        break;
                    } else if (commandline_history->pos == 0 && cmp) {
                        k=0;
                        break;
                    }
                } else if (!wcslen(get_line_from_history(commandline_history, 0))) {
                    k++;
                    break;
                }
        }
        delta += k;
        commandline_history->pos += delta;
        wcscpy(inputline, get_line_from_history(commandline_history, commandline_history->pos));
        inputline_pos = wcswidth(inputline, real_inputline_pos);
        ui_show_header();
        return;
#endif

    } else if (sb->value == ctl('v') ) {  // VISUAL SUBMODE
        visual_submode = ':';
        chg_mode('v');
        start_visualmode(sh->currow, sh->curcol, sh->currow, sh->curcol);
        return;

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

#ifdef HISTORY_FILE
        if (commandline_history->pos == 0) {          // Only if editing the new command
            wchar_t * sl = get_line_from_history(commandline_history, 0);
            wcscat(sl, cline);                        // Insert into history
        }
#endif
        ui_show_header();
        return;

    } else if (sb->value == ctl('f')) {               // C-f
        wchar_t cline [BUFFERSIZE];
        int i;
        struct ent * p1 = *ATBL(sh, sh->tbl, sh->currow, sh->curcol);
        if (! p1 || ! p1->format) {
            sc_error("cell has no format");
            return;
        }
        swprintf(cline, BUFFERSIZE, L"%s", p1->format);
        for (i = 0; i < wcslen(cline); i++) ins_in_line(cline[i]);

#ifdef HISTORY_FILE
        if (commandline_history->pos == 0) {          // Only if editing the new command
            wchar_t * sl = get_line_from_history(commandline_history, 0);
            wcscat(sl, cline);                        // Insert into history
        }
#endif
        ui_show_header();
        return;

    } else if ( sb->value == ctl('w') || sb->value == ctl('b') ||
                sb->value == OKEY_HOME || sb->value == OKEY_END) {
        switch (sb->value) {
        case ctl('w'):
            real_inputline_pos = for_word(1, 0, 1) + 1;   // E
            break;
        case ctl('b'):
            real_inputline_pos = back_word(1);            // B
            break;
        case OKEY_HOME:
            real_inputline_pos = 0;                       // 0
            break;
        case OKEY_END:
            real_inputline_pos = wcslen(inputline);       // $
            break;
        }
        inputline_pos = wcswidth(inputline, real_inputline_pos);
        ui_show_header();
        return;

    } else if (sb->value == '\t') {                  // TAB completion
        int i, clen = (sizeof(valid_commands) / sizeof(char *)) - 1;

        if (! get_comp()) copy_to_curcmd(inputline); // keep original cmd

        for (i = 0; i < clen; i++) {
            if ( ! wcscmp(inputline, valid_commands[i]) ) {
                wcscpy(inputline, get_curcmd());
                continue;
            }
            if ( ! wcsncmp(inputline, valid_commands[i], wcslen(inputline))
               ) {
                wcscpy(inputline, valid_commands[i]);
                real_inputline_pos = wcslen(inputline);
                inputline_pos = wcswidth(inputline, real_inputline_pos);
                set_comp(1);
                break;
            }
        }

        // Restore inputline content
        if (i == clen) {
            wcscpy(inputline, get_curcmd());
            real_inputline_pos = wcslen(inputline);
            inputline_pos = wcswidth(inputline, real_inputline_pos);
            set_comp(0);
        }

        ui_show_header();
        return;

    } else if (sc_isprint(sb->value)) {               //  Write new char
        ins_in_line(sb->value);
        ui_show_header();

#ifdef HISTORY_FILE
        if (commandline_history->pos == 0) {          // Only if editing the new command
            wchar_t * sl = get_line_from_history(commandline_history, 0);
            add_wchar(sl, sb->value, real_inputline_pos-1); // Insert into history
        }
#endif
        return;


    /*
     * CONFIRM A COMMAND PRESSING ENTER
     */
    } else if (find_val(sb, OKEY_ENTER)) {

        if ( ! wcscmp(inputline, L"refresh")) {
            sig_winchg(SIGWINCH);

        } else if ( ! wcscmp(inputline, L"help") || ! wcscmp(inputline, L"h") ) {
            help();

        } else if ( ! wcscmp(inputline, L"q!") || ! wcscmp(inputline, L"quit!") ) {
            shall_quit = 2;

        } else if ( ! wcscmp(inputline, L"q") || ! wcscmp(inputline, L"quit") ) {
            shall_quit = 1;

        } else if ( ! wcsncmp(inputline, L"autofit", 7) ) {
            wchar_t cline [BUFFERSIZE];
            wcscpy(cline, inputline);
            int c = sh->curcol, cf = sh->curcol;
            if (p != -1) {
                c = sr->tlcol;
                cf = sr->brcol;
            }
            if ( p != -1 || ! wcscmp(inputline, L"autofit")) {
                swprintf(cline, BUFFERSIZE, L"autofit %s:", coltoa(c));
                swprintf(cline + wcslen(cline), BUFFERSIZE, L"%s", coltoa(cf));
            }
            send_to_interp(cline);

        } else if ( ! wcsncmp(inputline, L"define_color", 12) ) {
            send_to_interp(inputline);

        } else if ( ! wcsncmp(inputline, L"redefine_color", 14) ) {
            send_to_interp(inputline);

        } else if ( ! wcsncmp(inputline, L"load", 4) ) {
            char name [BUFFERSIZE];
            int name_ok = 0;
            int force_rewrite = 0;
            #ifndef NO_WORDEXP
            size_t len;
            wordexp_t p;
            #endif

            wcstombs(name, inputline, BUFFERSIZE);
            if ( ! wcsncmp(inputline, L"load! ", 6) ) {
                force_rewrite = 1;
                del_range_chars(name, 4, 4);
            }

            del_range_chars(name, 0, 4);
            if ( ! strlen(name) ) {
                sc_error("Path to file to load is missing !");
            } else if (roman->modflg && ! force_rewrite ) {
                sc_error("Changes were made since last save. Use '!' to force the load");
            } else {
                #ifdef NO_WORDEXP
                name_ok = 1;
                #else
                wordexp(name, &p, 0);
                if ( p.we_wordc < 1 ) {
                    sc_error("Failed expanding filepath");

                } else if ( (len = strlen(p.we_wordv[0])) >= sizeof(name) ) {
                    sc_error("File path too long");
                    wordfree(&p);
                } else {
                    memcpy(name, p.we_wordv[0], len+1);
                    name_ok = 1;
                    wordfree(&p);
                }
                #endif
            }

            if ( name_ok ) {
                if ( ! file_exists(name)) {
                    sc_error("File %s does not exists!", name);
                } else {
                    delete_structures();
                    create_structures();
                    // create main session
                    session = (struct session *) calloc(1, sizeof(struct session));
                    load_file(name);

                    if (! get_conf_int("nocurses")) {
                      ui_show_header();
                    }
                }
            }
        } else if ( ! wcsncmp(inputline, L"hiderow ", 8) ||
                    ! wcsncmp(inputline, L"showrow ", 8) ||
                    ! wcsncmp(inputline, L"showcol ", 8) ||
                    ! wcsncmp(inputline, L"hidecol ", 8)
                  ) {
            send_to_interp(inputline);

        } else if ( ! wcsncmp(inputline, L"showrows", 8) ) {
            if (p != -1) { // only continue if there is a selected range
                int r, arg;
                sr = get_range_by_pos(p);
                r = sr->tlrow;
                arg = sr->brrow - sr->tlrow + 1;
                show_row(r, arg);
            }

        } else if ( ! wcsncmp(inputline, L"showcols", 8) ) {
            if (p != -1) { // only continue if there is a selected range
                int r, arg;
                sr = get_range_by_pos(p);
                r = sr->tlcol;
                arg = sr->brcol - sr->tlcol + 1;
                show_col(r, arg);
            }

        // range lock / unlock
        } else if ( ! wcsncmp(inputline, L"lock", 4) || ! wcsncmp(inputline, L"unlock", 6) ||
                    ! wcsncmp(inputline, L"valueize", 8) ) {
            int r = sh->currow, c = sh->curcol, rf = sh->currow, cf = sh->curcol;
            if (p != -1) {
                c = sr->tlcol;
                r = sr->tlrow;
                rf = sr->brrow;
                cf = sr->brcol;
            }
            if ( ! wcsncmp(inputline, L"lock", 4) ) lock_cells(sh, lookat(sh, r, c), lookat(sh, rf, cf));
            else if ( ! wcsncmp(inputline, L"unlock", 6) ) unlock_cells(sh, lookat(sh, r, c), lookat(sh, rf, cf));
            else if ( ! wcsncmp(inputline, L"valueize", 8) ) valueize_area(sh, r, c, rf, cf);

        } else if ( ! wcsncmp(inputline, L"datefmt", 7)) {
            wcscpy(interp_line, inputline);

            int r = sh->currow, c = sh->curcol, rf = sh->currow, cf = sh->curcol;
            if (p != -1) { // in case there is a range selected
                c = sr->tlcol;
                r = sr->tlrow;
                rf = sr->brrow;
                cf = sr->brcol;
            }
            wchar_t cline [BUFFERSIZE];
            wcscpy(cline, interp_line);
            int found = wstr_in_wstr(interp_line, L"\"");
            if (found != -1) {
                del_range_wchars(cline, 0, found-1);
                swprintf(interp_line, BUFFERSIZE, L"datefmt %s%d:", coltoa(c), r);
                swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%s%d %ls", coltoa(cf), rf, cline);
                send_to_interp(interp_line);
            }

        } else if ( ! wcsncmp(inputline, L"sort ", 5) ) {
            wcscpy(interp_line, inputline);
            if (p != -1) { // in case there is a range selected
                wchar_t cline [BUFFERSIZE];
                wcscpy(cline, interp_line);
                int found = wstr_in_wstr(interp_line, L"\"");
                if (found != -1) {
                    del_range_wchars(cline, 0, found-1);
                    swprintf(interp_line, BUFFERSIZE, L"sort %s%d:", coltoa(sr->tlcol), sr->tlrow);
                    swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%s%d %ls", coltoa(sr->brcol), sr->brrow, cline);
                }
            }
            sc_info("Sorting..");
            send_to_interp(interp_line);
            sc_info("Done.");

        } else if ( ! wcsncmp(inputline, L"subtotal ", 9) ) {
            int r = sh->currow, c = sh->curcol, rf = sh->currow, cf = sh->curcol, pos, cancel = 0;
            if (p != -1) {
                c = sr->tlcol;
                r = sr->tlrow;
                rf = sr->brrow;
                cf = sr->brcol;
            }
            if (any_locked_cells(sh, r, c, rf, cf)) {
                sc_error("Locked cells encountered. Nothing changed");
            } else {
                wchar_t line [BUFFERSIZE];
                wcscpy(line, inputline);
                del_range_wchars(line, 0, 8);
                if (
                    (pos = wstr_in_wstr(line, L"@sum")) != -1 ||
                    (pos = wstr_in_wstr(line, L"@avg")) != -1 ||
                    (pos = wstr_in_wstr(line, L"@max")) != -1 ||
                    (pos = wstr_in_wstr(line, L"@min")) != -1 ) {
                    add_wchar(line, L'\"', pos);
                    add_wchar(line, L'\"', pos+5);
                } else if (
                    (pos = wstr_in_wstr(line, L"@prod")) != -1) {
                    add_wchar(line, L'\"', pos);
                    add_wchar(line, L'\"', pos+6);
                } else if (
                    (pos = wstr_in_wstr(line, L"@count"))  != -1) {
                    add_wchar(line, L'\"', pos);
                    add_wchar(line, L'\"', pos+7);
                } else if (
                    (pos = wstr_in_wstr(line, L"@stddev")) != -1) {
                    add_wchar(line, L'\"', pos);
                    add_wchar(line, L'\"', pos+8);
                } else {
                    sc_error("Please specify a function to apply the subtotals");
                    cancel = 1;
                }
                if (!cancel) {
                    swprintf(interp_line, BUFFERSIZE, L"subtotal %s%d:", coltoa(c), r);
                    swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%s%d ", coltoa(cf), rf);
                    swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%ls", line);
                    send_to_interp(interp_line);
                    unselect_ranges();
                }
            }

        } else if ( ! wcsncmp(inputline, L"freezecol", 9) ) {
            if (p != -1) {
                swprintf(interp_line, BUFFERSIZE, L"freeze %s:", coltoa(sr->tlcol));
                swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%s", coltoa(sr->brcol));
            } else if (! wcscmp(inputline, L"freezecol")) {
                swprintf(interp_line, BUFFERSIZE, L"freeze %s", coltoa(sh->curcol));
            } else
                swprintf(interp_line, BUFFERSIZE, L"freeze %ls", &inputline[9]);
            send_to_interp(interp_line);

        } else if ( ! wcsncmp(inputline, L"freezerow", 9) ) {
            if (p != -1) {
                swprintf(interp_line, BUFFERSIZE, L"freeze %d:%d", sr->tlrow, sr->brrow);
            } else if (! wcscmp(inputline, L"freezerow")) {
                swprintf(interp_line, BUFFERSIZE, L"freeze %d", sh->currow);
            } else
                swprintf(interp_line, BUFFERSIZE, L"freeze %ls", &inputline[9]);
            send_to_interp(interp_line);

        } else if ( ! wcsncmp(inputline, L"unfreezecol", 11) ) {
            if (p != -1) {
                swprintf(interp_line, BUFFERSIZE, L"unfreeze %s:", coltoa(sr->tlcol));
                swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%s", coltoa(sr->brcol));
            } else if (! wcscmp(inputline, L"unfreezecol")) {
                swprintf(interp_line, BUFFERSIZE, L"unfreeze %s", coltoa(sh->curcol));
            } else
                swprintf(interp_line, BUFFERSIZE, L"unfreeze %ls", &inputline[11]);
            send_to_interp(interp_line);

        } else if ( ! wcsncmp(inputline, L"unfreezerow", 11) ) {
            if (p != -1) {
                swprintf(interp_line, BUFFERSIZE, L"unfreeze %d:%d", sr->tlrow, sr->brrow);
            } else if (! wcscmp(inputline, L"unfreezerow")) {
                swprintf(interp_line, BUFFERSIZE, L"unfreeze %d", sh->currow);
            } else
                swprintf(interp_line, BUFFERSIZE, L"unfreeze %ls", &inputline[11]);
            send_to_interp(interp_line);

        } else if ( ! wcsncmp(inputline, L"addfilter", 9) ) {
            wchar_t cline [BUFFERSIZE];
            char line [BUFFERSIZE];
            wcscpy(cline, inputline);
            int found;
            if ((found = wstr_in_wstr(cline, L"\"")) != -1) {
                del_range_wchars(cline, wcslen(cline), wcslen(cline));
                del_range_wchars(cline, 0, found);
                wcstombs(line, cline, BUFFERSIZE);
                add_filter(line);
                line[strlen(line)-1]='\0'; // remove last "
                sc_info("Added filter: %s", line);
            }

        } else if ( ! wcsncmp(inputline, L"delfilter ", 10) ) {
            wchar_t cline [BUFFERSIZE];
            char line [BUFFERSIZE];
            wcscpy(cline, inputline);
            del_range_wchars(cline, 0, 9);
            wcstombs(line, cline, BUFFERSIZE);
            int id = atoi(line);
            if (del_filter(id) == 0) sc_info("Removed filter: %d", id);

        } else if ( ! wcsncmp(inputline, L"delfilters", 10) ) {
            if (free_filters() == 0) sc_info("Removed filters");

        } else if ( ! wcsncmp(inputline, L"filteron", 8) ) {
            wcscpy(interp_line, inputline);
            if ( ! wcscmp(inputline, L"filteron") && p == -1) { // If there is no selected range and no range in inputline passed
                sc_error("Please specify a range or select one");
            } else if (p != -1) {
                wchar_t cline [BUFFERSIZE];
                wcscpy(cline, interp_line);
                swprintf(interp_line, BUFFERSIZE, L"filteron %s%d:", coltoa(sr->tlcol), sr->tlrow);
                swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%s%d", coltoa(sr->brcol), sr->brrow);
                send_to_interp(interp_line);
            } else { // no range selected. range passed in inputline
                send_to_interp(interp_line);
            }

        } else if ( ! wcsncmp(inputline, L"filteroff", 9) ) {
            disable_filters();

        } else if ( ! wcsncmp(inputline, L"hiddenrows", 10)) {
            show_hiddenrows();

        } else if ( ! wcsncmp(inputline, L"hiddencols", 10)) {
            show_hiddencols();

        } else if ( ! wcsncmp(inputline, L"showfilters", 11)) {
            show_filters();

        } else if ( ! wcsncmp(inputline, L"int ", 4) ) { // send cmd to interpreter
            wcscpy(interp_line, inputline);
            del_range_wchars(interp_line, 0, 3);
            send_to_interp(interp_line);

        } else if ( ! wcsncmp(inputline, L"fill ", 5) ) {
            interp_line[0]=L'\0';
            int r = sh->currow, c = sh->curcol, rf = sh->currow, cf = sh->curcol;
            if (p != -1) {
                c = sr->tlcol;
                r = sr->tlrow;
                rf = sr->brrow;
                cf = sr->brcol;
                swprintf(interp_line, BUFFERSIZE, L"fill %s%d:", coltoa(c), r);
                swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%s%d ", coltoa(cf), rf);
            }

            if (any_locked_cells(sh, r, c, rf, cf)) {
                swprintf(interp_line, BUFFERSIZE, L"");
                sc_error("Locked cells encountered. Nothing changed");
            } else {
                if (p != -1)
                    swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%ls", &inputline[5]);
                else
                    swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%ls", inputline);
                send_to_interp(interp_line);
            }

        } else if ( ! wcsncmp(inputline, L"formatrow ", 10) ) {
            int r = sh->currow, rf = sh->currow, i;
            if (p != -1) {
                r = sr->tlrow;
                rf = sr->brrow;
            }
#ifdef UNDO
            int changed = 0, fmt_ori;
            create_undo_action();
#endif
            for (i=r; i<=rf;i++) {
#ifdef UNDO
                fmt_ori = sh->row_format[i];
                add_undo_row_format(i, 'R', sh->row_format[i]);
#endif
                swprintf(interp_line, BUFFERSIZE, L"format %d %ls", i, &inputline[10]);
                send_to_interp(interp_line);
#ifdef UNDO
                if (fmt_ori != sh->row_format[i]) changed = 1;
                add_undo_row_format(i, 'A', sh->row_format[i]);
#endif
            }
#ifdef UNDO
            if (! changed) dismiss_undo_item(NULL);
            else end_undo_action();
#endif

        } else if ( ! wcsncmp(inputline, L"formatcol ", 10) ) {
            int c = sh->curcol, cf = sh->curcol, i;
            if (p != -1) {
                c = sr->tlcol;
                cf = sr->brcol;
            }
#ifdef UNDO
            int changed = 0;
            int fwidth_ori;
            int precision_ori;
            int realfmt_ori;
            create_undo_action();
#endif
            for (i=c; i<=cf;i++) {
#ifdef UNDO
                fwidth_ori = sh->fwidth[i];
                precision_ori = sh->precision[i];
                realfmt_ori = sh->realfmt[i];
                add_undo_col_format(i, 'R', sh->fwidth[i], sh->precision[i], sh->realfmt[i]);
#endif
                swprintf(interp_line, BUFFERSIZE, L"format %s", coltoa(i));
                swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L" %ls", &inputline[10]);
                send_to_interp(interp_line);
#ifdef UNDO
                if (sh->fwidth[i] != fwidth_ori || sh->precision[i] != precision_ori || sh->realfmt[i] != realfmt_ori) changed = 1;
                add_undo_col_format(i, 'A', sh->fwidth[i], sh->precision[i], sh->realfmt[i]);
#endif
            }
#ifdef UNDO
            if (! changed) dismiss_undo_item(NULL);
            else end_undo_action();
#endif

        } else if ( ! wcsncmp(inputline, L"format ", 7) ) {
            int r = sh->currow, c = sh->curcol, rf = sh->currow, cf = sh->curcol;
            if (p != -1) {
                c = sr->tlcol;
                r = sr->tlrow;
                rf = sr->brrow;
                cf = sr->brcol;
                swprintf(interp_line, BUFFERSIZE, L"fmt %s%d:", coltoa(c), r);
                swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%s%d", coltoa(cf), rf);
            } else
                swprintf(interp_line, BUFFERSIZE, L"fmt %s%d", coltoa(c), r);

            if (any_locked_cells(sh, r, c, rf, cf)) {
                sc_error("Locked cells encountered. Nothing changed");
            } else {
                int l = wcslen(interp_line);
                swprintf(interp_line + l, BUFFERSIZE, L"%ls", inputline);
                del_range_wchars(interp_line, l, l + 5);
                #ifdef UNDO
                create_undo_action();
                copy_to_undostruct(sh, r, c, rf, cf, UNDO_DEL, IGNORE_DEPS, NULL);
                #endif
                send_to_interp(interp_line);
                #ifdef UNDO
                copy_to_undostruct(sh, r, c, rf, cf, UNDO_ADD, IGNORE_DEPS, NULL);
                end_undo_action();
                #endif
            }

        } else if ( ! wcsncmp(inputline, L"ccopy", 5) ) {
            int r = sh->currow, c = sh->curcol, rf = sh->currow, cf = sh->curcol;
            if (p != -1) {
                c = sr->tlcol;
                r = sr->tlrow;
                rf = sr->brrow;
                cf = sr->brcol;
            }
            swprintf(interp_line, BUFFERSIZE, L"ccopy %s%d:", coltoa(c), r);
            swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%s%d", coltoa(cf), rf);
            send_to_interp(interp_line);

        } else if ( ! wcsncmp(inputline, L"strtonum", 8) ) {
            int r = sh->currow, c = sh->curcol, rf = sh->currow, cf = sh->curcol;
            if (p != -1) {
                c = sr->tlcol;
                r = sr->tlrow;
                rf = sr->brrow;
                cf = sr->brcol;
            }
            swprintf(interp_line, BUFFERSIZE, L"strtonum %s%d:", coltoa(c), r);
            swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%s%d", coltoa(cf), rf);
            send_to_interp(interp_line);

        } else if ( ! wcsncmp(inputline, L"cpaste", 6) ) {
            swprintf(interp_line, BUFFERSIZE, L"cpaste");
            send_to_interp(interp_line);

        } else if ( ! wcsncmp(inputline, L"cellcolor ", 10) ) {
            #ifdef USECOLORS
            interp_line[0]=L'\0';
            wchar_t line [BUFFERSIZE];
            wcscpy(line, inputline);
            del_range_wchars(line, 0, 9);
            swprintf(interp_line, BUFFERSIZE, L"cellcolor ");
            if (p != -1) {
                swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L" %s%d:", coltoa(sr->tlcol), sr->tlrow);
                swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%s%d ", coltoa(sr->brcol), sr->brrow);
            }
            swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%ls", line);
            send_to_interp(interp_line);
            #else
            sc_error("Color support not compiled in");
            chg_mode('.');
            inputline[0] = L'\0';
            #endif

        } else if ( ! wcsncmp(inputline, L"unformat", 8) ) {
            #ifdef USECOLORS
            interp_line[0]=L'\0';
            wchar_t line [BUFFERSIZE];
            wcscpy(line, inputline);
            del_range_wchars(line, 0, 7);
            swprintf(interp_line, BUFFERSIZE, L"unformat");
            if (p != -1) {
                swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L" %s%d:", coltoa(sr->tlcol), sr->tlrow);
                swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%s%d ", coltoa(sr->brcol), sr->brrow);
            }
            swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%ls", line);
            send_to_interp(interp_line);
            #else
            sc_error("Color support not compiled in");
            chg_mode('.');
            inputline[0] = L'\0';
            #endif

        } else if ( ! wcsncmp(inputline, L"color ", 6) ) {
            #ifdef USECOLORS
            char line [BUFFERSIZE];
            wcstombs(line, inputline, BUFFERSIZE);
            del_range_chars(line, 0, 5);
            chg_color(line);
            #else
            sc_error("Color support not compiled in");
            chg_mode('.');
            inputline[0] = '\0';
            #endif

        }  else if ( ! wcsncmp(inputline, L"trigger ", 8) ) {
            interp_line[0]=L'\0';
            wchar_t line [BUFFERSIZE];
            wcscpy(line, inputline);
            del_range_wchars(line, 0, 7);
            swprintf(interp_line, BUFFERSIZE, L"trigger ");
            if (p != -1) {
                swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L" %s%d:", coltoa(sr->tlcol), sr->tlrow);
                swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%s%d ", coltoa(sr->brcol), sr->brrow);
            }
            swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%ls", line);
            send_to_interp(interp_line);

        }  else if ( ! wcsncmp(inputline, L"untrigger ", 10) ) {
            wcscpy(interp_line, inputline);
            send_to_interp(interp_line);

        // Change a config value
        }  else if ( ! wcsncmp(inputline, L"set ", 4) ) {
            change_config_parameter(inputline);

        } else if ( ! wcsncmp(inputline, L"pad ", 4) ) {
            int c = sh->curcol, cf = sh->curcol;
            if (p != -1) { // in case there is a range selected
                c = sr->tlcol;
                cf = sr->brcol;
            }
            wcscpy(interp_line, inputline); // pad 5
            swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L" %s:", coltoa(c)); // pad 5 A:
            swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%s", coltoa(cf));  // B
            send_to_interp(interp_line);

        } else if ( ! wcsncmp(inputline, L"plot ", 5) ) {
            int r = sh->currow, c = sh->curcol, rf = sh->currow, cf = sh->curcol;
            if (p != -1) {
                c = sr->tlcol;
                r = sr->tlrow;
                rf = sr->brrow;
                cf = sr->brcol;
            }
            wchar_t aux[wcslen(inputline)+1];
            wcscpy(aux, inputline);
            del_range_wchars(aux, 0, 4);
            swprintf(interp_line, BUFFERSIZE, L"plot \"%ls\" %s%d:", aux, coltoa(c), r);
            swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%s%d", coltoa(cf), rf);
            send_to_interp(interp_line);

        } else if ( ! wcsncmp(inputline, L"plotedit ", 9) ) {
            wchar_t aux[wcslen(inputline)+1];
            wcscpy(aux, inputline);
            del_range_wchars(aux, 0, 8);
            plotedit(aux);

        } else if ( ! wcscmp(inputline, L"set") ) {
            char valores[get_dict_buffer_size(user_conf_d) + 1];
            get_conf_values(valores);
            ui_show_text(valores);

        } else if ( ! wcscmp(inputline, L"version") ) {
            ui_show_text(rev);

        } else if ( ! wcscmp(inputline, L"showmaps") ) {
            extern int len_maps;
            char valores[MAXMAPITEM * len_maps];
            get_mappings(valores);
            ui_show_text(valores);

        } else if ( ! wcsncmp(inputline, L"newsheet", 8) ||
                    ! wcsncmp(inputline, L"delsheet", 8) ||
                    ! wcsncmp(inputline, L"nextsheet", 9) ||
                    ! wcsncmp(inputline, L"renamesheet", 11) ||
                    ! wcsncmp(inputline, L"prevsheet", 9)) {
            send_to_interp(inputline);

        } else if ( ! wcsncmp(inputline, L"nmap", 4) ||
                    ! wcsncmp(inputline, L"imap", 4) ||
                    ! wcsncmp(inputline, L"emap", 4) ||
                    ! wcsncmp(inputline, L"vmap", 4) ||
                    ! wcsncmp(inputline, L"cmap", 4) ||
                    ! wcsncmp(inputline, L"inoremap", 8) ||
                    ! wcsncmp(inputline, L"enoremap", 8) ||
                    ! wcsncmp(inputline, L"nnoremap", 8) ||
                    ! wcsncmp(inputline, L"vnoremap", 8) ||
                    ! wcsncmp(inputline, L"cnoremap", 8) ||
                    ! wcsncmp(inputline, L"iunmap", 6) ||
                    ! wcsncmp(inputline, L"eunmap", 6) ||
                    ! wcsncmp(inputline, L"vunmap", 6) ||
                    ! wcsncmp(inputline, L"cunmap", 6) ||
                    ! wcsncmp(inputline, L"nunmap", 6) ) {
            send_to_interp(inputline);

        } else if ( ! wcsncmp(inputline, L"!", 1) ) {
            char line [BUFFERSIZE];
            wcstombs(line, inputline, BUFFERSIZE);
            int found = str_in_str(line, " ");
            if (found == -1) found++;
            del_range_chars(line, 0, found);
            exec_cmd(line);

        } else if ( ! wcscmp(inputline, L"wq") ) {
          wcscpy(inputline, L"x");
          if (savefile() == 0) shall_quit = 1;

        } else if ( inputline[0] == L'w' ) {
          savefile();

        } else if ( ! wcsncmp(inputline, L"file ", 5) ) {
            char * curfile = session->cur_doc->name;
            char name [BUFFERSIZE];
            int name_ok = 0;
            #ifndef NO_WORDEXP
            size_t len;
            wordexp_t p;
            #endif

            wcstombs(name, inputline, BUFFERSIZE);
            del_range_chars(name, 0, 4);

            #ifdef NO_WORDEXP
            name_ok = 1;
            #else
            wordexp(name, &p, 0);
            if ( p.we_wordc < 1 ) {
                sc_error("Failed to expand filename");
            } else if ( (len = strlen(p.we_wordv[0])) >= sizeof(name) ) {
                sc_error("File path too long");
                wordfree(&p);
            } else {
                memcpy(name, p.we_wordv[0], len+1);
                name_ok = 1;
                wordfree(&p);
            }
            #endif

            if (name_ok) {
                #ifdef AUTOBACKUP
                // check if backup of current loaded file exists.
                // if it exists, remove it.
                if (curfile != NULL && strlen(curfile) && backup_exists(curfile)) remove_backup(curfile);
                #endif
                if (roman->name == NULL) roman->name = malloc(sizeof(char)*PATHLEN);
                strncpy(roman->name, name, PATHLEN - 1);
                sc_info("File name set to \"%s\"", roman->name);
            }

        } else if ( ! wcscmp(inputline, L"file") ) {
            char * curfile = session->cur_doc->name;
            if (curfile == NULL || ! curfile) {
                sc_info("Current file has no name");
            } else {
                sc_info("Current file: \"%s\"", curfile);
            }

        } else if ( inputline[0] == L'x' ) {
            if (savefile() == 0) shall_quit = 1;

        } else if ( ! wcscmp(inputline, L"fcopy") ) {
            fcopy(sh, "");

        } else if ( ! wcsncmp(inputline, L"fcopy ", 6)) {
            wchar_t line [BUFFERSIZE];
            wcscpy(line, inputline);
            del_range_wchars(line, 0, 5);
            char action[BUFFERSIZE];
            wcstombs(action, line, BUFFERSIZE);
            fcopy(sh, action);

        } else if ( ! wcscmp(inputline, L"fsum") ) {
            fsum(sh);

        } else if (
            ! wcsncmp(inputline, L"e csv"  , 5) ||
            ! wcsncmp(inputline, L"e! csv" , 6) ||
            ! wcsncmp(inputline, L"e tex"  , 5) ||
            ! wcsncmp(inputline, L"e! tex" , 6) ||
            ! wcsncmp(inputline, L"e tab"  , 5) ||
            ! wcsncmp(inputline, L"e! tab" , 6) ||
            ! wcsncmp(inputline, L"e mkd"  , 4) ||
            ! wcsncmp(inputline, L"e! mkd" , 5) ||
            ! wcsncmp(inputline, L"e txt" , 5) ||
            ! wcsncmp(inputline, L"e! txt" , 6) ) {
                do_export( p == -1 ? 0 : sr->tlrow, p == -1 ? 0 : sr->tlcol,
                p == -1 ? sh->maxrow : sr->brrow, p == -1 ? sh->maxcol : sr->brcol);

        } else if (
            ! wcsncmp(inputline, L"e xlsx"  , 6) ||
            ! wcsncmp(inputline, L"e! xlsx" , 7)) {
                #if defined(AUTOBACKUP) && defined(XLSX_EXPORT)
                char * curfile = session->cur_doc->name;
                #endif

                #ifndef XLSX_EXPORT
                sc_error("XLSX export support not compiled in");
                #else
                char linea[BUFFERSIZE];
                char filename[PATHLEN] = "";
                int force_rewrite = 0;
                if (inputline[1] == L'!') force_rewrite = 1;
                wcstombs(linea, inputline, BUFFERSIZE); // Use new variable to keep command history untouched
                del_range_chars(linea, 0, 1 + force_rewrite); // Remove 'e' or 'e!' from inputline

                // Get path and file name to write.
                // Use parameter if any.
                if (strlen(linea) > 5) {   // ex. 'xlsx '
                    del_range_chars(linea, 0, 4); // remove 'xlsx'
                    strcpy(filename, linea);
                    // Use curfile name and '.xlsx' extension
                    // Remove current '.sc' extension if necessary
                } else if (curfile != NULL && strlen(curfile)) {
                    strcpy(filename, curfile);
                    char * ext = strrchr(filename, '.');
                    if (ext != NULL) del_range_chars(filename, strlen(filename) - strlen(ext), strlen(filename)-1);
                    sprintf(filename + strlen(filename), ".xlsx");
                } else {
                    sc_error("No filename specified !");
                }

                if (strlen(filename) > 0 && ! force_rewrite && file_exists(filename)) {
                    sc_error("File %s already exists. Use \"!\" to force rewrite.", filename);

                #ifdef AUTOBACKUP
                    // check if backup of filename exists.
                    // if it exists and '!' is set, remove it.
                    // if it exists and curfile = fname, remove it.
                    // else return.
                } else if (strlen(filename) && backup_exists(filename)
                    && ! force_rewrite && ! (strlen(curfile) && ! strcmp(curfile, filename))) {
                    sc_error("Backup file of %s exists. Use \"!\" to force the write process.", filename);
                #endif
                } else if (strlen(filename)) {
                    #ifdef AUTOBACKUP
                    if (backup_exists(filename)) remove_backup(filename);
                    #endif
                    if (export_xlsx(filename) == 0)
                        sc_info("File \"%s\" written", filename);
                }
                #endif

        } else {
            sc_error("COMMAND NOT FOUND !");
        }

#ifdef HISTORY_FILE
        /*
         * if exists in history an item with same text to the command typed
         * (counting from the second position) it is moved to the beginning of list.
         * (first element in list means last command executed)
         */
        del_item_from_history(commandline_history, 0);
        int moved = move_item_from_history_by_str(commandline_history, inputline, -1);
        if (! moved) add(commandline_history, inputline);
        commandline_history->pos = 0;
#endif

        chg_mode('.');
        inputline[0]=L'\0';
        inputline_pos = 0; //ADDED 08/10/2018
        set_comp(0); // unmark tab completion
        ui_update(TRUE);
    }
    return;
}

/**
 * \brief TODO Document ins_in_line()
 *
 * \param[in] d
 *
 * \return none
 */

void ins_in_line(wint_t d) {
    add_wchar(inputline, (wchar_t) d, real_inputline_pos++);
    inputline_pos += wcwidth((wchar_t) d);
    return;
}
