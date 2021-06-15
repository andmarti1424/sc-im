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
 * \file cmds_visuals.c
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 2017-07-18
 * \brief TODO Write a tbrief file description.
 */

#include <stdlib.h>

#include "cmds.h"
#include "../utils/string.h"
#include "../tui.h"
#include "../buffer.h"
#include "../marks.h"
#include "../macros.h"
#include "../conf.h"
#include "../actions/hide_show.h"
#include "../actions/shift.h"
#include "../yank.h"
#include "../actions/freeze.h"
#include "../history.h"
#include "../interp.h"
#ifdef UNDO
#include "../undo.h"
#endif

extern int offscr_sc_rows, offscr_sc_cols;
extern unsigned int curmode;
extern int cmd_multiplier;
extern struct history * commandline_history;
extern struct session * session;

char visual_submode = '0';
srange * r;                       // SELECTED RANGE!
int moving = FALSE;

/**
 * \brief TODO Document start_visualmode()
 *
 * \param[in] tlrow
 * \param[in] tlcol
 * \param[in] brrow
 * \param[in] brcol
 *
 * \return none
 */

void start_visualmode(int tlrow, int tlcol, int brrow, int brcol) {
    unselect_ranges();
    struct roman * roman = session->cur_doc;
    struct sheet * sh = roman->cur_sh;

    struct srange * sr = get_range_by_marks('\t', '\t'); // visual mode selected range
    if (sr != NULL) del_ranges_by_mark('\t');

    r = (srange *) malloc (sizeof(srange));
    r->tlrow = tlrow;
    r->tlcol = tlcol;
    r->brrow = brrow;
    r->brcol = brcol;
    r->orig_row = sh->currow;         // original row before starting selection
    r->orig_col = sh->curcol;         // original col before starting selection
    r->startup_row = sh->currow;      // original row position before entering visual mode
    r->startup_col = sh->curcol;      // original col position before entering visual mode
    r->marks[0] = '\t';
    r->marks[1] = '\t';
    r->selected = 1;
    r->pnext = NULL;

    // add visual selected range at start of list
    if (ranges == NULL) ranges = r;
    else {
        r->pnext = ranges;
        ranges = r;
    }

    if (visual_submode == '0') {  // Started visual mode with 'v' command
        ui_update(TRUE);
        moving = FALSE;
    } else {                      // Started visual mode with 'C-v' command
        ui_update(TRUE);
        moving = TRUE;
    }
    return;
}

/**
 * \brief TODO Document exit_visualmode()
 *
 * \return none
 */

void exit_visualmode() {
    struct roman * roman = session->cur_doc;
    moving = FALSE;
    visual_submode = '0';
    r->selected = 0;
    roman->cur_sh->currow = r->startup_row;
    roman->cur_sh->curcol = r->startup_col;
    del_ranges_by_mark('\t');
    return;
}

/**
 * \brief TODO Document do_visualmode()
 *
 * \param[in] buf
 *
 * \return none
 */

void do_visualmode(struct block * buf) {
    struct roman * roman = session->cur_doc;
    struct sheet * sh = roman->cur_sh;

    // we are moving (previous to a 'C-o' keypress)
    if (moving == TRUE) {
        switch (buf->value) {
            case L'j':
            case OKEY_DOWN:
                sh->currow = forw_row(sh, 1)->row;
                break;

            case L'k':
            case OKEY_UP:
                sh->currow = back_row(sh, 1)->row;
                break;

            case L'h':
            case OKEY_LEFT:
                sh->curcol = back_col(sh, 1)->col;
                break;

            case L'l':
            case OKEY_RIGHT:
                sh->curcol = forw_col(sh, 1)->col;
                break;

            case ctl('o'):
                moving = FALSE;
                r->orig_row = sh->currow;
                r->orig_col = sh->curcol;
                break;

            case OKEY_ENTER:
                sc_info("Press <C-o> to begin selection or <Esc> key to exit VISUAL MODE");
                return;

        }
        r->tlrow = sh->currow;
        r->tlcol = sh->curcol;
        r->brrow = sh->currow;
        r->brcol = sh->curcol;

        ui_update(FALSE);
        return;
    }

    // started visual mode with 'C-v'
    // ENTER or 'C-k' : Confirm selection
    // 'C-k' only works if started visualmode with 'C-v'
    if ((buf->value == OKEY_ENTER || buf->value == ctl('k')) && visual_submode != '0') {
        wchar_t cline [BUFFERSIZE];
        swprintf(cline, BUFFERSIZE, L"%ls%d", coltoa(r->tlcol), r->tlrow);
        if (r->tlrow != r->brrow || r->tlcol != r->brcol)
            swprintf(cline + wcslen(cline), BUFFERSIZE, L":%ls%d", coltoa(r->brcol), r->brrow);
        swprintf(inputline + wcslen(inputline), BUFFERSIZE, L"%ls", cline);

        real_inputline_pos += wcslen(cline);
        inputline_pos = wcswidth(inputline, real_inputline_pos);

        char c = visual_submode;
        exit_visualmode();
        chg_mode(c);

        ui_show_header();
        return;

    // moving to TRUE
    //} else if (buf->value == ctl('m')) {
    //    moving = TRUE;

    // MOVEMENT COMMANDS
    // UP - ctl(b)
    } else if (buf->value == OKEY_UP || buf->value == L'k' || buf->value == ctl('b') ) {
        int n, i;
        if (buf->value == ctl('b')) {
            n = SC_DISPLAY_ROWS;
            if (get_conf_value("half_page_scroll")) n = n / 2;
        } else n = 1;

        for (i=0; i < n; i++)
            if (r->orig_row < r->brrow && r->tlrow < r->brrow) {
                while (sh->row_hidden[-- r->brrow]);
                sh->currow = r->brrow;
            } else if (r->tlrow <= r->brrow && r->tlrow-1 >= 0) {
                int newrow = r->tlrow;
                while (newrow > 0 && sh->row_hidden[-- newrow]);
                if (! sh->row_hidden[newrow]) {
                    sh->currow = r->tlrow = newrow;
                }
            }

    // DOWN - ctl('f')
    } else if (buf->value == OKEY_DOWN || buf->value == L'j' || buf->value == ctl('f')) {
        int n, i;
        if (buf->value == ctl('f')) {
            n = SC_DISPLAY_ROWS;
            if (get_conf_value("half_page_scroll")) n = n / 2;
        } else n = 1;

        for (i=0; i < n; i++)
            if (r->orig_row <= r->tlrow && r->tlrow <= r->brrow) {
                while (r->brrow+1 < sh->maxrows && sh->row_hidden[++ r->brrow]);
                sh->currow = r->brrow;
            } else if (r->tlrow <  r->brrow) {
                while (sh->row_hidden[++ r->tlrow]);
                sh->currow = r->tlrow;
            }

    // LEFT
    } else if (buf->value == OKEY_LEFT || buf->value == L'h') {
        if (r->orig_col < r->brcol && r->tlcol < r->brcol) {
            while (sh->col_hidden[-- r->brcol]);
            sh->curcol = r->brcol;
        } else if (r->tlcol <= r->brcol && r->tlcol-1 >= 0) {
            while (sh->col_hidden[-- r->tlcol]);
            sh->curcol = r->tlcol;
        }

    // RIGHT
    } else if (buf->value == OKEY_RIGHT || buf->value == L'l') {
        if (r->orig_col <= r->tlcol && r->tlcol <= r->brcol && r->brcol+2 < sh->maxcols) {
            while (sh->col_hidden[++ r->brcol]);
            sh->curcol = r->brcol;
        } else if (r->tlcol <= r->brcol) {
            while (sh->col_hidden[++ r->tlcol]);
            sh->curcol = r->tlcol;
        }

    // 0
    } else if (buf->value == L'0') {
        r->brcol = r->tlcol;
        r->tlcol = left_limit(sh)->col;
        sh->curcol = r->tlcol;

    // $
    } else if (buf->value == L'$') {
        int s = right_limit(sh, sh->currow)->col;
        r->tlcol = r->brcol;
        r->brcol = r->brcol > s ? r->brcol : s;
        sh->curcol = r->brcol;

    // ^
    } else if (buf->value == L'^') {
        r->brrow = r->tlrow;
        r->tlrow = goto_top(sh)->row;
        sh->currow = r->tlrow;

    // #
    } else if (buf->value == L'#') {
        int s = goto_bottom(sh)->row;
        if (s == r->brrow) s = go_end(sh)->row;
        //r->tlrow = r->brrow;
        r->brrow = r->brrow > s ? r->brrow : s;
        //r->brrow = s;
        sh->currow = r->brrow;

    // ctl(a)
    } else if (buf->value == ctl('a')) {
        if (r->tlrow == 0 && r->tlcol == 0) return;
        struct ent * e = go_home(sh);
        r->tlrow = e->row;
        r->tlcol = e->col;
        r->brrow = r->orig_row;
        r->brcol = r->orig_col;
        sh->currow = r->tlrow;
        sh->curcol = r->tlcol;

    // G
    } else if (buf->value == L'G') {
        struct ent * e = go_end(sh);
        r->tlrow = r->orig_row;
        r->tlcol = r->orig_col;
        r->brrow = e->row;
        r->brcol = e->col;
        sh->currow = r->tlrow;
        sh->curcol = r->tlcol;

    // '
    } else if (buf->value == L'\'') {
        // if we receive a mark of a range, just return.
        if (get_mark(buf->pnext->value)->row == -1) {
            sc_error("That is a mark of a range. Returning.");
            return;
        }

        struct ent_ptr * ep = tick(buf->pnext->value);
        if (ep == NULL) {
            sc_error("No mark. Returning.");
            return;
        } else if (ep->sheet != NULL && ep->sheet != roman->cur_sh) {
            sc_error("Cell marked is on other sheet. Dismissing.");
            if (ep != NULL) free(ep);
            return;
        }

        if (sh->row_hidden[ep->vp->row]) {
            sc_error("Cell row is hidden");
            if (ep != NULL) free(ep);
            return;
        } else if (sh->col_hidden[ep->vp->col]) {
            sc_error("Cell column is hidden");
            if (ep != NULL) free(ep);
            return;
        }
        r->tlrow = r->tlrow < ep->vp->row ? r->tlrow : ep->vp->row;
        r->tlcol = r->tlcol < ep->vp->col ? r->tlcol : ep->vp->col;
        r->brrow = r->brrow > ep->vp->row ? r->brrow : ep->vp->row;
        r->brcol = r->brcol > ep->vp->col ? r->brcol : ep->vp->col;
        if (ep != NULL) free(ep);

    // w
    } else if (buf->value == L'w') {
        struct ent * e = go_forward(sh);
        if (e->col > r->orig_col) {
            r->brcol = e->col;
            r->tlcol = r->orig_col;
        } else {
            r->tlcol = e->col;
            r->brcol = r->orig_col;
        }
        r->brrow = e->row;
        r->tlrow = r->orig_row;
        sh->curcol = e->col;
        sh->currow = e->row;

    // b
    } else if (buf->value == L'b') {
        struct ent * e = go_backward(sh);
        if (e->col <= r->orig_col) {
            r->tlcol = e->col;
            r->brcol = r->orig_col;
        } else {
            r->brcol = e->col;
            r->tlcol = r->orig_col;
        }
        r->tlrow = e->row;
        r->brrow = r->orig_row;
        sh->curcol = e->col;
        sh->currow = e->row;

    // H
    } else if (buf->value == L'H') {
        r->brrow = r->tlrow;
        r->tlrow = vert_top(sh)->row;
        sh->currow = r->tlrow;

    // M
    } else if (buf->value == L'M') {
        r->tlrow = r->orig_row;
        int rm = vert_middle(sh)->row;
        if (r->orig_row < rm) r->brrow = rm;
        else r->tlrow = rm;
        sh->currow = r->tlrow;

    // L
    } else if (buf->value == L'L') {
        r->tlrow = r->orig_row;
        r->brrow = vert_bottom(sh)->row;
        sh->currow = r->brrow;

    // mark a range
    } else if (buf->value == L'm' && get_bufsize(buf) == 2) {
        del_ranges_by_mark(buf->pnext->value);
        srange * rn = create_range('\0', '\0', lookat(sh, r->tlrow, r->tlcol), lookat(sh, r->brrow, r->brcol));
        set_range_mark(buf->pnext->value, sh, rn);
        exit_visualmode();
        chg_mode('.');
        ui_show_header();

    // auto_fit
    } else if (buf->value == ctl('j')) {
        auto_fit(sh, r->tlcol, r->brcol, DEFWIDTH);  // auto justify columns
        exit_visualmode();
        chg_mode('.');
        ui_show_header();

    // datefmt with locale D_FMT format
    } else if (buf->value == ctl('d')) {
        #ifdef USELOCALE
            #include <locale.h>
            #include <langinfo.h>
            char * loc = NULL;
            char * f = NULL;
            loc = setlocale(LC_TIME, "");
            if (loc != NULL) {
                f = nl_langinfo(D_FMT);
            } else {
                sc_error("No locale set. Nothing changed");
            }
            if (any_locked_cells(sh, r->tlrow, r->tlcol, r->brrow, r->brcol)) {
                sc_error("Locked cells encountered. Nothing changed");
                return;
            }
            dateformat(sh, lookat(sh, r->tlrow, r->tlcol), lookat(sh, r->brrow, r->brcol), f);
        exit_visualmode();
        chg_mode('.');
        ui_show_header();
        #else
            sc_info("Build made without USELOCALE enabled");
        #endif

    // EDITION COMMANDS
    // yank
    } else if (buf->value == 'y') {
        yank_area(sh, r->tlrow, r->tlcol, r->brrow, r->brcol, 'a', 1);

        exit_visualmode();
        chg_mode('.');
        ui_show_header();

   // 'p' normal paste
   // 'P' Works like 'p' except that all cell references are adjusted.
    } else if (buf->value == 'P' || buf->value == 'p') {
        struct ent_ptr * yl = get_yanklist();
        int type_paste = (buf->value == 'P') ? 'c' : 'a' ;
        int row, col;
        if( yl != NULL) {
            int colsize = -(yl->vp->col); //calculate colsize for correct repeating if paste area is bigger than yank area
            int rowsize = -(yl->vp->row); //calculate rowsize
            while (yl->next != NULL) { yl = yl->next; } //get the last one to calculated size of yank_area
            colsize += (yl->vp->col +1); //calculate size
            rowsize += (yl->vp->row +1); //calculate size
#ifdef DEBUG
            char str[20];
            sprintf(str, "RowSize:%d ColSize:%d Type Paste:%d", rowsize, colsize, type_paste);
#endif
            for (row = r->tlrow; row <= r->brrow; row += rowsize) {
                for (col = r->tlcol; col <= r->brcol; col += colsize) {
                    sh->currow = row;
                    sh->curcol = col;
                    paste_yanked_ents(sh, 0, type_paste);
                }
            }
            exit_visualmode();
            chg_mode('.');
            ui_show_header();
#ifdef DEBUG
            sc_info(str);
#endif
#ifndef DEBUG
            sc_info("Nice Pasting :-)");
#endif
        }
        else{
            exit_visualmode();
            chg_mode('.');
            ui_show_header();
            sc_info("Nothing to Paste");
        }

        // left / right / center align
    } else if (buf->value == L'{' || buf->value == L'}' || buf->value == L'|') {
        if (any_locked_cells(sh, r->tlrow, r->tlcol, r->brrow, r->brcol)) {
            sc_error("Locked cells encountered. Nothing changed");
            return;
        }
        extern wchar_t interp_line[BUFFERSIZE];
        if (buf->value == L'{')      swprintf(interp_line, BUFFERSIZE, L"leftjustify %s", v_name(r->tlrow, r->tlcol));
        else if (buf->value == L'}') swprintf(interp_line, BUFFERSIZE, L"rightjustify %s", v_name(r->tlrow, r->tlcol));
        else if (buf->value == L'|') swprintf(interp_line, BUFFERSIZE, L"center %s", v_name(r->tlrow, r->tlcol));
        swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L":%s", v_name(r->brrow, r->brcol));
#ifdef UNDO
        create_undo_action();
        copy_to_undostruct(sh, r->tlrow, r->tlcol, r->brrow, r->brcol, UNDO_DEL, IGNORE_DEPS, NULL);
#endif
        send_to_interp(interp_line);
#ifdef UNDO
        copy_to_undostruct(sh, r->tlrow, r->tlcol, r->brrow, r->brcol, UNDO_ADD, IGNORE_DEPS, NULL);
        end_undo_action();
#endif
        cmd_multiplier = 0;

        exit_visualmode();
        chg_mode('.');
        ui_show_header();

    // freeze a range
    } else if (buf->value == L'f') {
        handle_freeze(sh, lookat(sh, r->tlrow, r->tlcol), lookat(sh, r->brrow, r->brcol), 1, 'r');
        handle_freeze(sh, lookat(sh, r->tlrow, r->tlcol), lookat(sh, r->brrow, r->brcol), 1, 'c');

        cmd_multiplier = 0;
        exit_visualmode();
        chg_mode('.');
        ui_show_header();
        sc_info("Area frozen");

    // range lock / unlock // valueize
    } else if ( buf->value == L'r' && (buf->pnext->value == L'l' || buf->pnext->value == L'u' ||
            buf->pnext->value == L'v' )) {
        if (buf->pnext->value == L'l') {
            lock_cells(sh, lookat(sh, r->tlrow, r->tlcol), lookat(sh, r->brrow, r->brcol));
        } else if (buf->pnext->value == L'u') {
            unlock_cells(sh, lookat(sh, r->tlrow, r->tlcol), lookat(sh, r->brrow, r->brcol));
        } else if (buf->pnext->value == L'v') {
            valueize_area(sh, r->tlrow, r->tlcol, r->brrow, r->brcol);
        }
        cmd_multiplier = 0;

        exit_visualmode();
        chg_mode('.');
        ui_show_header();

    // Zr Zc - Zap col or row
    } else if ( (buf->value == L'Z' || buf->value == L'S') && (buf->pnext->value == L'c' || buf->pnext->value == L'r')) {
        int arg = buf->pnext->value == L'r' ? r->brrow - r->tlrow + 1 : r->brcol - r->tlcol + 1;
        if (buf->value == L'Z' && buf->pnext->value == L'r') {
            hide_row(r->tlrow, arg);
        } else if (buf->value == L'Z' && buf->pnext->value == L'c') {
            hide_col(r->tlcol, arg);
        } else if (buf->value == L'S' && buf->pnext->value == L'r') {
            show_row(r->tlrow, arg);
        } else if (buf->value == L'S' && buf->pnext->value == L'c') {
            show_col(r->tlcol, arg);
        }
        cmd_multiplier = 0;

        exit_visualmode();
        chg_mode('.');
        ui_show_header();

    // delete selected range
    } else if (buf->value == L'x' || (buf->value == L'd' && buf->pnext->value == L'd') ) {
        del_selected_cells(sh);
        exit_visualmode();
        chg_mode('.');
        ui_show_header();

    // shift range
    } else if (buf->value == L's') {
        shift(sh, r->tlrow, r->tlcol, r->brrow, r->brcol, buf->pnext->value);
        exit_visualmode();
        chg_mode('.');
        ui_show_header();

    } else if (buf->value == L':') {
        chg_mode(':');
        ui_show_header();
#ifdef HISTORY_FILE
        add(commandline_history, L"");
#endif
        ui_handle_cursor();
        inputline_pos = 0;
        real_inputline_pos = 0;
        return;
    }

    if (visual_submode == '0')
        ui_update(TRUE);
    else {
        ui_update(FALSE);
    }
}
