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
 * \file cmds_normal.c
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 26/05/2021
 * \brief TODO Write a tbrief file description.
 */

#include <ctype.h>
#include <stdlib.h>

#include "cmds.h"
#include "cmds_edit.h"
#include "../yank.h"
#include "../marks.h"
#include "../conf.h"
#include "../tui.h"
#include "../history.h"
#include "../actions/freeze.h"
#include "../actions/hide_show.h"
#include "../actions/shift.h"
#include "../main.h"    // for sig_winchg
#include "../interp.h"
#include "../utils/extra.h"
#ifdef UNDO
#include "../undo.h"
#endif


#include "../graph.h"
extern graphADT graph;
extern char valores;
extern int cmd_multiplier;
extern void start_visualmode(int tlrow, int tlcol, int brrow, int brcol);
extern void ins_in_line(wint_t d);
extern void openfile_under_cursor(int r, int c);

extern wchar_t interp_line[BUFFERSIZE];
extern struct session * session;

#ifdef HISTORY_FILE
extern struct history * commandline_history;
#endif

#ifdef INS_HISTORY_FILE
extern struct history * insert_history;
extern char ori_insert_edit_submode;
#endif

/**
 * \brief TODO Document do_normalmode()
 * \param[in] buf
 * \return none
 */
void do_normalmode(struct block * buf) {
    struct roman * roman = session->cur_doc;
    struct sheet * sh = roman->cur_sh;
    int bs = get_bufsize(buf);
    struct ent * e;

    switch (buf->value) {
        // FOR TEST PURPOSES
        case L'A':
            //;
            //wchar_t t = ui_query_opt(L"show a message. q / a / d to quit", L"qad");
            break;

        case L'W':
            break;

        case L'Q':
            break;

        // MOVEMENT COMMANDS
        case L'j':
        case OKEY_DOWN:
            sh->lastcol = sh->curcol;
            sh->lastrow = sh->currow;
            sh->currow = forw_row(sh, 1)->row;
            unselect_ranges();
            ui_update(TRUE);
            break;

        case L'k':
        case OKEY_UP:
            sh->lastcol = sh->curcol;
            sh->lastrow = sh->currow;
            sh->currow = back_row(sh, 1)->row;
            unselect_ranges();
            ui_update(TRUE);
            break;

        case L'h':
        case OKEY_LEFT:
            sh->lastrow = sh->currow;
            sh->lastcol = sh->curcol;
            sh->curcol = back_col(sh, 1)->col;
            unselect_ranges();
            ui_update(TRUE);
            break;

        case L'l':
        case OKEY_RIGHT:
            sh->lastrow = sh->currow;
            sh->lastcol = sh->curcol;
            sh->curcol = forw_col(sh, 1)->col;
            unselect_ranges();
            ui_update(TRUE);
            break;

        case L'0':
            if (get_conf_int("numeric_zero") == 1 && get_conf_int("numeric") == 1) goto numeric;
        case OKEY_HOME:
            sh->lastrow = sh->currow;
            sh->lastcol = sh->curcol;
            sh->curcol = left_limit(sh)->col;
            unselect_ranges();
            ui_update(TRUE);
            break;

        case L'$':
        case OKEY_END:
            sh->lastrow = sh->currow;
            sh->lastcol = sh->curcol;
            sh->curcol = right_limit(sh, sh->currow)->col;
            unselect_ranges();
            ui_update(TRUE);
            break;

        case L'^':
            sh->lastcol = sh->curcol;
            sh->lastrow = sh->currow;
            sh->currow = goto_top(sh)->row;
            unselect_ranges();
            ui_update(TRUE);
            break;

        case L'#':
            sh->lastcol = sh->curcol;
            sh->lastrow = sh->currow;
            sh->currow = goto_bottom(sh)->row;
            if (sh->currow == sh->lastrow && sh->curcol == sh->lastcol) sh->currow = go_end(sh)->row;
            unselect_ranges();
            ui_update(TRUE);
            break;

        // Tick
        case L'\'':
            if (bs != 2) break;
            unselect_ranges();
            struct ent_ptr * ep = tick(buf->pnext->value);
            if (sh->row_hidden[ep->vp->row]) {
                sc_error("Cell row is hidden");
                if (ep != NULL) free(ep);
                break;
            }
            if (sh->col_hidden[ep->vp->col]) {
                sc_error("Cell column is hidden");
                if (ep != NULL) free(ep);
                break;
            }
            if (ep->sheet != NULL) roman->cur_sh = ep->sheet;
            roman->cur_sh->lastrow = roman->cur_sh->currow;
            roman->cur_sh->lastcol = roman->cur_sh->curcol;
            roman->cur_sh->currow = ep->vp->row;
            roman->cur_sh->curcol = ep->vp->col;
            if (ep != NULL) free(ep);
            ui_update(TRUE);
            break;

        // CTRL j
        case ctl('j'):
            {
            int p, c = sh->curcol, cf = sh->curcol;
            if ( (p = is_range_selected()) != -1) {
                struct srange * sr = get_range_by_pos(p);
                c = sr->tlcol;
                cf = sr->brcol;
            }
            auto_fit(sh, c, cf, DEFWIDTH);  // auto justify columns
            ui_update(TRUE);
            break;
            }

        // CTRL d
        case ctl('d'):                      // set date format using current locate D_FMT format
            {
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
            int p, r = sh->currow, c = sh->curcol, rf = sh->currow, cf = sh->curcol;
            if ( (p = is_range_selected()) != -1) {
                struct srange * sr = get_range_by_pos(p);
                r = sr->tlrow;
                c = sr->tlcol;
                rf = sr->brrow;
                cf = sr->brcol;
            }
            if (any_locked_cells(sh, r, c, rf, cf)) {
                sc_error("Locked cells encountered. Nothing changed");
                return;
            }
            dateformat(sh, lookat(sh, r, c), lookat(sh, rf, cf), f);
            ui_update(TRUE);
            break;
        #else
            sc_info("Build made without USELOCALE enabled");
        #endif
            }

        // CTRL f
        case ctl('f'):
        case OKEY_PGDOWN:
            {
            int n = calc_mobile_rows(sh, NULL);
            if (get_conf_int("half_page_scroll")) n = n / 2;
            sh->lastcol = sh->curcol;
            sh->lastrow = sh->currow;
            sh->currow = forw_row(sh, n)->row;
            unselect_ranges();
            scroll_down(sh, n);
            ui_update(TRUE);
            break;
            }

        // CTRL b
        case ctl('b'):
        case OKEY_PGUP:
            {
            int n = calc_mobile_rows(sh, NULL);
            if (get_conf_int("half_page_scroll")) n = n / 2;
            sh->lastcol = sh->curcol;
            sh->lastrow = sh->currow;
            sh->currow = back_row(sh, n)->row;
            unselect_ranges();
            scroll_up(sh, n);
            ui_update(TRUE);
            break;
            }

        case L'w':
            e = go_forward(sh);
            sh->lastrow = sh->currow;
            sh->lastcol = sh->curcol;
            sh->currow = e->row;
            sh->curcol = e->col;
            unselect_ranges();
            ui_update(TRUE);
            break;

        case L'b':
            e = go_backward(sh);
            sh->lastrow = sh->currow;
            sh->lastcol = sh->curcol;
            sh->currow = e->row;
            sh->curcol = e->col;
            unselect_ranges();
            ui_update(TRUE);
            break;

        case L'H':
            sh->lastrow = sh->currow;
            int currow_h = vert_top(sh)->row;
            sh->currow = currow_h;
            unselect_ranges();
            ui_update(TRUE);
            break;

        case L'M':
            sh->lastrow = sh->currow;
            sh->currow = vert_middle(sh)->row;
            unselect_ranges();
            ui_update(TRUE);
            break;

        case L'L':
            sh->lastrow = sh->currow;
            sh->currow = vert_bottom(sh)->row;
            unselect_ranges();
            ui_update(TRUE);
            break;

        case L'G': // goto end
            e = go_end(sh);
            sh->lastrow = sh->currow;
            sh->lastcol = sh->curcol;
            sh->currow = e->row;
            sh->curcol = e->col;
            unselect_ranges();
            ui_update(TRUE);
            break;

        // GOTO goto
        case ctl('a'):
            e = go_home(sh);
            sh->lastrow = sh->currow;
            sh->lastcol = sh->curcol;
            sh->curcol = e->col;
            sh->currow = e->row;
            unselect_ranges();
            sh->offscr_sc_rows = 0;
            sh->offscr_sc_cols = 0;
            ui_update(TRUE);
            break;

        case L'g':
            if (buf->pnext->value == L'0') {                               // g0
                sh->lastcol = sh->curcol;
                sh->lastrow = sh->currow;
                sh->curcol = go_bol(sh)->col;

            } else if (buf->pnext->value == L'$') {                        // g$
                sh->lastcol = sh->curcol;
                sh->lastrow = sh->currow;
                sh->curcol = go_eol(sh)->col;

            } else if (buf->pnext->value == L'f') {                        // gf
                unselect_ranges();
                ui_update(TRUE);
                ui_stop_screen();
                openfile_under_cursor(sh->currow, sh->curcol);
                ui_start_screen();
                start_default_ucolors();
                set_colors_param_dict();

            } else if (buf->pnext->value == L'g') {                        // gg
                e = go_home(sh);
                sh->lastcol = sh->curcol;
                sh->lastrow = sh->currow;
                sh->curcol = e->col;
                sh->currow = e->row;
                sh->offscr_sc_rows = 0;
                sh->offscr_sc_cols = 0;

            } else if (buf->pnext->value == L'G') {                        // gG
                e = go_end(sh);
                sh->lastcol = sh->curcol;
                sh->lastrow = sh->currow;
                sh->currow = e->row;
                sh->curcol = e->col;

            } else if (buf->pnext->value == L'M') {                        // gM
                sh->lastcol = sh->curcol;
                sh->lastrow = sh->currow;
                sh->curcol = horiz_middle(sh)->col;

            // goto last cell position
            } else if (buf->pnext->value == L'l') {                        // gl
                int newlr = sh->currow;
                int newlc = sh->curcol;
                sh->curcol = sh->lastcol;
                sh->currow = sh->lastrow;
                sh->lastrow = newlr;
                sh->lastcol = newlc;

            } else if (buf->pnext->value == L't') {                        // gt
                (void) swprintf(interp_line, BUFFERSIZE, L"nextsheet");
                send_to_interp(interp_line);

            } else if (buf->pnext->value == L'T') {                        // gT
                (void) swprintf(interp_line, BUFFERSIZE, L"prevsheet");
                send_to_interp(interp_line);

            } else if (buf->pnext->value == L'o') {                        // goA4 (goto cell A4)
                (void) swprintf(interp_line, BUFFERSIZE, L"goto %s", parse_cell_name(2, buf));
                send_to_interp(interp_line);
            }
            unselect_ranges();
            ui_update(TRUE);
            break;

        // repeat last goto command - backwards
        case L'N':
            {
            struct roman * roman = session->cur_doc;
            struct sheet * sh = roman->cur_sh;
            extern struct go_save gs;
            if (gs.g_sheet == sh) {
                go_previous();
            } else if (gs.g_type == G_NUM) {
                num_search(sh, gs.g_n, 0, 0, sh->maxrow, sh->maxcol, 0, gs.g_flow);
            } else if (gs.g_type == G_STR) {
                gs.g_type = G_NONE;    /* Don't free the string */
                str_search(sh, gs.g_s, 0, 0, sh->maxrow, sh->maxcol, 0, gs.g_flow);
            }
            ui_update(TRUE);
            }
            break;

        // repeat last goto command
        case L'n':
            {
            struct roman * roman = session->cur_doc;
            struct sheet * sh = roman->cur_sh;
            extern struct go_save gs;
            if (gs.g_sheet == sh) {
                go_last();
            } else if (gs.g_type == G_NUM) {
                num_search(sh, gs.g_n, 0, 0, sh->maxrow, sh->maxcol, 0, gs.g_flow);
            } else if (gs.g_type == G_STR) {
                gs.g_type = G_NONE;    /* Don't free the string */
                str_search(sh, gs.g_s, 0, 0, sh->maxrow, sh->maxcol, 0, gs.g_flow);
            }
            ui_update(TRUE);
            }
            break;

        // END OF MOVEMENT COMMANDS
        case L'/':
            {
            char cadena[] = ":int goto ";
            int i;
            for (i=0; i<strlen(cadena); i++) {
                flush_buf(buf);
                addto_buf(buf, cadena[i]);
                exec_single_cmd(buf);
            }
            break;
            }

        case L'?':
            {
            char cadena[] = ":int gotob ";
            int i;
            for (i=0; i<strlen(cadena); i++) {
                flush_buf(buf);
                addto_buf(buf, cadena[i]);
                exec_single_cmd(buf);
            }
            break;
            }

        // repeat last command
        case L'.':
            if (get_conf_int("numeric_decimal") == 1 && get_conf_int("numeric") == 1) goto numeric;
            copybuffer(lastcmd_buffer, buf); // nose graba en lastcmd_buffer!!
            cmd_multiplier = 1;
            exec_mult(buf, COMPLETECMDTIMEOUT);
            break;

        // enter command mode
        case L':':
            chg_mode(':');
#ifdef HISTORY_FILE
            add(commandline_history, L"");
#endif
            ui_handle_cursor();
            inputline_pos = 0;
            real_inputline_pos = 0;
            ui_show_header();
            break;

        // enter visual mode
        case L'v':
            chg_mode('v');
            ui_show_header();
            ui_handle_cursor();
            start_visualmode(sh->currow, sh->curcol, sh->currow, sh->curcol);
            break;

        // INPUT COMMANDS
        case L'=':
        case L'\\':
        case L'<':
        case L'>':
            if (locked_cell(sh, sh->currow, sh->curcol)) return;
            insert_edit_submode = buf->value;
            chg_mode(insert_edit_submode);
#ifdef INS_HISTORY_FILE
            ori_insert_edit_submode = buf->value;
            add(insert_history, L"");
#endif
            inputline_pos = 0;
            real_inputline_pos = 0;
            ui_show_header();
            break;

        // EDITION COMMANDS
        // edit cell (v)
        case L'e':
            if (locked_cell(sh, sh->currow, sh->curcol)) return;
            inputline_pos = 0;
            real_inputline_pos = 0;
            if (start_edit_mode(buf, 'v')) ui_show_header();
            break;

        // edit cell (s)
        case L'E':
            if (locked_cell(sh, sh->currow, sh->curcol)) return;
            inputline_pos = 0;
            real_inputline_pos = 0;
            if (start_edit_mode(buf, 's')) ui_show_header();
            else {
                sc_info("No string value to edit");
                chg_mode('.');
                ui_print_mode();
                ui_show_celldetails();
            }
            break;

        // del current cell or range
        case L'x':
            del_selected_cells(sh);
            ui_update(TRUE);
            break;

        // format col or freeze range
        case L'f':
            if (bs != 2) return;

            // freeze row / column or area
            if (buf->pnext->value == 'r' || buf->pnext->value == 'c' || buf->pnext->value == 'a') {
                int p = is_range_selected(), r = sh->currow, c = sh->curcol, rf = sh->currow, cf = sh->curcol;

                if (p != -1) { // mark range
                    struct srange * sr = get_range_by_pos(p);
                    r = sr->tlrow;
                    c = sr->tlcol;
                    rf = sr->brrow;
                    cf = sr->brcol;
                }

                if (buf->pnext->value == 'r') {
                    handle_freeze(sh, lookat(sh, r, c), lookat(sh, rf, cf), 1, 'r');
                    sc_info("Row%s frozen", r != rf ? "s" : "");
                } else if (buf->pnext->value == 'c') {
                    handle_freeze(sh, lookat(sh, r, c), lookat(sh, rf, cf), 1, 'c');
                    sc_info("Column%s frozen", c != cf ? "s" : "");
                } else if (buf->pnext->value == 'a') {
                    handle_freeze(sh, lookat(sh, r, c), lookat(sh, rf, cf), 1, 'r');
                    handle_freeze(sh, lookat(sh, r, c), lookat(sh, rf, cf), 1, 'c');
                    sc_info("Area frozen");
                }
                ui_update(FALSE);
                break;

            // decrease row height
            } else if (buf->pnext->value == 'k' || buf->pnext->value == OKEY_UP) {

#ifdef UNDO
                create_undo_action();
                int fmt_ori = sh->row_format[sh->currow];
                add_undo_row_format(sh->currow, 'R', sh->row_format[sh->currow]);
#endif
                swprintf(interp_line, BUFFERSIZE, L"format %d %d", sh->currow, sh->row_format[sh->currow]-1);
                send_to_interp(interp_line);
#ifdef UNDO
                if (sh->row_format[sh->currow] != fmt_ori) {
                    add_undo_row_format(sh->currow, 'A', sh->row_format[sh->currow]);
                    end_undo_action();
                } else dismiss_undo_item(NULL);
#endif
                ui_update(TRUE);
                break;

            // increase row height
            } else if (buf->pnext->value == 'j' || buf->pnext->value == OKEY_DOWN) {

#ifdef UNDO
                create_undo_action();
                int fmt_ori = sh->row_format[sh->currow];
                add_undo_row_format(sh->currow, 'R', sh->row_format[sh->currow]);
#endif
                swprintf(interp_line, BUFFERSIZE, L"format %d %d", sh->currow, sh->row_format[sh->currow]+1);
                send_to_interp(interp_line);
#ifdef UNDO
                if (sh->row_format[sh->currow] != fmt_ori) {
                    add_undo_row_format(sh->currow, 'A', sh->row_format[sh->currow]);
                    end_undo_action();
                } else dismiss_undo_item(NULL);
#endif
                ui_update(TRUE);
                break;

            // change in format
            } else {
#ifdef UNDO
                create_undo_action();
                add_undo_col_format(sh->curcol, 'R', sh->fwidth[sh->curcol], sh->precision[sh->curcol], sh->realfmt[sh->curcol]);
#endif
            formatcol(sh, buf->pnext->value);
#ifdef UNDO
            add_undo_col_format(sh->curcol, 'A', sh->fwidth[sh->curcol], sh->precision[sh->curcol], sh->realfmt[sh->curcol]);
            end_undo_action();
#endif
            }
            break;

        // mark cell or range
        case L'm':
            if (bs != 2) break;
            int p = is_range_selected();
            if (p != -1) { // mark range
                struct srange * sr = get_range_by_pos(p);
                set_range_mark(buf->pnext->value, sh, sr);
            } else         // mark cell
                set_cell_mark(buf->pnext->value, sh, sh->currow, sh->curcol);
            roman->modflg++;
            break;

        // copy
        case L'c':
            {
            if (bs != 2) break;
            struct mark * m = get_mark(buf->pnext->value);
            if ( m == NULL) return;


            // if m represents a range
            if ( m->row == -1 && m->col == -1) {
                srange * r = m->rng;
                yank_area(sh, r->tlrow, r->tlcol, r->brrow, r->brcol, 'a', cmd_multiplier);
                if (paste_yanked_ents(sh, 0, 'c') == -1) {
                    sc_error("Locked cells encountered. Nothing changed");
                    break;
                }

            // if m represents just one cell
            } else {
                struct mark * m = get_mark(buf->pnext->value);
                struct ent * p = lookat(sh, m->row, m->col);
                struct ent * n;
                int c1;

#ifdef UNDO
                create_undo_action();
#endif
                for (c1 = sh->curcol; cmd_multiplier-- && cmd_multiplier > -1 && c1 < sh->maxcols; c1++) {
                    if ((n = * ATBL(sh, sh->tbl, sh->currow, c1))) {
                        if (n->flags & is_locked)
                            continue;
                        if (! p) {
                            clearent(n);
                            continue;
                        }
                    } else {
                        if (! p) break;
                        n = lookat(sh, sh->currow, c1);
                    }
#ifdef UNDO
                    // added for #244 - 22/03/2018
                    ents_that_depends_on_range(sh, n->row, n->col, n->row, n->col);
                    copy_to_undostruct(sh, sh->currow, c1, sh->currow, c1, UNDO_DEL, HANDLE_DEPS, NULL);
#endif
                    copyent(n, sh, p, sh->currow - get_mark(buf->pnext->value)->row, c1 - get_mark(buf->pnext->value)->col, 0, 0, sh->maxrow, sh->maxcol, 0);

                    n->row += sh->currow - get_mark(buf->pnext->value)->row;
                    n->col += c1 - get_mark(buf->pnext->value)->col;

                    n->flags |= is_changed;
                    if (n->expr) EvalJustOneVertex(sh, n, 1);

#ifdef UNDO
                    copy_to_undostruct(sh, sh->currow, c1, sh->currow, c1, UNDO_ADD, HANDLE_DEPS, NULL);
#endif
                }
#ifdef UNDO
                extern struct ent_ptr * deps;
                if (deps != NULL) free(deps);
                deps = NULL;
                end_undo_action();
#endif
            }

            ui_update(TRUE);
            break;
            }

        // range lock / unlock / valueize
        case L'r':
            {
            int p, r = sh->currow, c = sh->curcol, rf = sh->currow, cf = sh->curcol;
            if ( (p = is_range_selected()) != -1) {
                struct srange * sr = get_range_by_pos(p);
                r = sr->tlrow;
                c = sr->tlcol;
                rf = sr->brrow;
                cf = sr->brcol;
            }
            if (buf->pnext->value == L'l') {
                lock_cells(sh, lookat(sh, r, c), lookat(sh, rf, cf));
            } else if (buf->pnext->value == L'u') { // watch out if you do C-r and u too quickly !
                unlock_cells(sh, lookat(sh, r, c), lookat(sh, rf, cf));
            } else if (buf->pnext->value == L'v') {
                valueize_area(sh, r, c, rf, cf);
            }
            ui_update(TRUE);
            break;
            }

        // create range with two marks
        case L'R':
            if (bs == 3) {
                create_range(buf->pnext->value, buf->pnext->pnext->value, NULL, NULL);
                ui_update(TRUE);
            }
            break;

        // Zr Zc - Zap col or row - Show col or row - Sr Sc
        case L'Z':
        case L'S':
            {
            int rs, r = sh->currow, c = sh->curcol, arg = cmd_multiplier;
            struct srange * sr;
            if ( (rs = is_range_selected()) != -1) {
                sr = get_range_by_pos(rs);
                cmd_multiplier = 1;
                r = sr->tlrow;
                c = sr->tlcol;
                arg = buf->pnext->value == L'r' ? sr->brrow - sr->tlrow + 1 : sr->brcol - sr->tlcol + 1;
            }
            if (buf->value == L'Z' && buf->pnext->value == L'r') {
                hide_row(r, arg);
            } else if (buf->value == L'Z' && buf->pnext->value == L'c') {
                hide_col(c, arg);
            } else if (buf->value == L'S' && buf->pnext->value == L'r') {
                show_row(r, arg);
            } else if (buf->value == L'S' && buf->pnext->value == L'c') {
                show_col(c, arg);
            }
            cmd_multiplier = 0;
            ui_update(TRUE);
            break;
            }

        // shift range or cell
        case L's':
            {
            int p, r = sh->currow, c = sh->curcol, rf = sh->currow, cf = sh->curcol;
            if ( (p = is_range_selected()) != -1) {
                struct srange * sr = get_range_by_pos(p);
                r = sr->tlrow;
                c = sr->tlcol;
                rf = sr->brrow;
                cf = sr->brcol;
            }
            shift(sh, r, c, rf, cf, buf->pnext->value);
            unselect_ranges();
            rebuild_graph();
            ui_update(TRUE);
            break;
            }

        // delete row or column, or selected cell or range
        case L'd':
            {
            if (bs != 2) return;
            int ic = cmd_multiplier; // orig

            // deleterow
            if (buf->pnext->value == L'r') {
                deleterow(sh, sh->currow, ic);
                if (cmd_multiplier > 1) cmd_multiplier = 0;

            // deletecol
            } else if (buf->pnext->value == L'c') {
                deletecol(sh, sh->curcol, ic);
                if (cmd_multiplier > 1) cmd_multiplier = 0;

            } else if (buf->pnext->value == L'd') {
                del_selected_cells(sh);
            }

            rebuild_graph();
            ui_update(TRUE);
            break;
            }

        // insert row or column
        case L'i':
            {
            if (bs != 2) return;
#ifdef UNDO
            create_undo_action();
#endif

            if (buf->pnext->value == L'r') {
#ifdef UNDO
                save_undo_range_shift(1, 0, sh->currow, 0, sh->currow, sh->maxcol);
#endif
                fix_marks(sh, 1, 0, sh->currow, sh->maxrow, 0, sh->maxcol);
                insert_row(sh, 0);
#ifdef UNDO
                add_undo_row_format(sh->currow, 'A', sh->row_format[sh->currow]);
#endif

            } else if (buf->pnext->value == L'c') {
#ifdef UNDO
                save_undo_range_shift(0, 1, 0, sh->curcol, sh->maxrow, sh->curcol);
#endif
                fix_marks(sh, 0, 1, 0, sh->maxrow, sh->curcol, sh->maxcol);
                insert_col(sh, 0);
#ifdef UNDO
                add_undo_col_format(sh->curcol, 'A', sh->fwidth[sh->curcol], sh->precision[sh->curcol], sh->realfmt[sh->curcol]);
#endif
            }
#ifdef UNDO
            end_undo_action();
#endif
            rebuild_graph();
            ui_update(TRUE);
            break;
            }

        // open row or column
        case L'o':
            {
            if (bs != 2) return;
#ifdef UNDO
            create_undo_action();
#endif
            if (buf->pnext->value == L'r') {
#ifdef UNDO
                save_undo_range_shift(1, 0, sh->currow+1, 0, sh->currow+1, sh->maxcol);
#endif
                fix_marks(sh, 1, 0, sh->currow+1, sh->maxrow, 0, sh->maxcol);
                insert_row(sh, 1);
#ifdef UNDO
                add_undo_row_format(sh->currow, 'A', sh->row_format[sh->currow]);
#endif

            } else if (buf->pnext->value == L'c') {
#ifdef UNDO
                save_undo_range_shift(0, 1, 0, sh->curcol+1, sh->maxrow, sh->curcol+1);
#endif
                fix_marks(sh, 0, 1, 0, sh->maxrow, sh->curcol+1, sh->maxcol);
                insert_col(sh, 1);
#ifdef UNDO
                add_undo_col_format(sh->curcol, 'A', sh->fwidth[sh->curcol], sh->precision[sh->curcol], sh->realfmt[sh->curcol]);
#endif
            }
#ifdef UNDO
            end_undo_action();
#endif
            rebuild_graph();
            ui_update(TRUE);
            break;
            }

        case L'y':
            // yank row
            if ( bs == 2 && buf->pnext->value == L'r') {
                yank_area(sh, sh->currow, 0, sh->currow + cmd_multiplier - 1, sh->maxcol, 'r', cmd_multiplier);
                if (cmd_multiplier > 1) cmd_multiplier = 0;

            // yank col
            } else if ( bs == 2 && buf->pnext->value == L'c') {
                yank_area(sh, 0, sh->curcol, sh->maxrow, sh->curcol + cmd_multiplier - 1, 'c', cmd_multiplier);
                if (cmd_multiplier > 1) cmd_multiplier = 0;

            // yank cell
            } else if ( bs == 2 && buf->pnext->value == L'y' && is_range_selected() == -1) {
                yank_area(sh, sh->currow, sh->curcol, sh->currow, sh->curcol, 'e', cmd_multiplier);

            // yank range
            } else if ( bs == 1 && is_range_selected() != -1) {
                srange * r = get_selected_range();
                yank_area(sh, r->tlrow, r->tlcol, r->brrow, r->brcol, 'a', cmd_multiplier);
            }
            break;

        // paste cell below or left
        case L'p':
            if (paste_yanked_ents(sh, 0, 'a') == -1) {
                sc_error("Locked cells encountered. Nothing changed");
                break;
            }
            ui_update(TRUE);
            break;

        case L'P':
        case L'T':
            if (bs != 2) break;
            if (buf->pnext->value == L'v' || buf->pnext->value == L'f' || buf->pnext->value == L'c') {
                int res = buf->value == L'P' ? paste_yanked_ents(sh, 0, buf->pnext->value) : paste_yanked_ents(sh, 1, buf->pnext->value); // paste cell above or right
                if (res == -1) {
                    sc_error("Locked cells encountered. Nothing changed");
                    break;
                }
                ui_update(TRUE);
            }
            break;

        // paste cell above or right
        case L't':
            if (paste_yanked_ents(sh, 1, 'a') == -1) {
                sc_error("Locked cells encountered. Nothing changed");
                break;
            }
            ui_update(TRUE);
            break;

        // select inner range - Vir
        case L'V':
            if (buf->value == L'V' && bs == 3 &&
            buf->pnext->value == L'i' && buf->pnext->pnext->value == L'r') {
                int tlrow = sh->currow;
                int brrow = sh->currow;
                int tlcol = sh->curcol;
                int brcol = sh->curcol;
                int * tlr = &tlrow;
                int * brr = &brrow;
                int * tlc = &tlcol;
                int * brc = &brcol;
                select_inner_range(sh, tlr, tlc, brr, brc);
                start_visualmode(*tlr, *tlc, *brr, *brc);
            }
            break;

        // autofit
        case L'a':
            if ( bs != 2 ) break;

            if (buf->pnext->value == L'a') {
                int p, r = sh->currow, c = sh->curcol, rf = sh->currow, cf = sh->curcol;
                if ( (p = is_range_selected()) != -1) {
                    struct srange * sr = get_range_by_pos(p);
                    r = sr->tlrow;
                    c = sr->tlcol;
                    rf = sr->brrow;
                    cf = sr->brcol;
                }
                if (any_locked_cells(sh, r, c, rf, cf)) {
                    sc_error("Locked cells encountered. Nothing changed");
                    return;
                }
                wchar_t cline [BUFFERSIZE];
                swprintf(cline, BUFFERSIZE, L"autofit %s:", coltoa(c));
                swprintf(cline + wcslen(cline), BUFFERSIZE, L"%s", coltoa(cf));
                send_to_interp(cline);
                ui_update(TRUE);
            }
            break;

        // scroll
        case L'z':
            if ( bs != 2 ) break;
            int scroll = 0;

            switch (buf->pnext->value) {
                case L'l':
                    scroll_right(sh, 1);
                    break;

                case L'h':
                    scroll_left(sh, 1);
                    break;

                case L'H':
                    scroll = calc_mobile_cols(sh, NULL);
                    if (get_conf_int("half_page_scroll")) scroll /= 2;
                    scroll_left(sh, scroll);
                    break;

                case L'L':
                    scroll = calc_mobile_cols(sh, NULL);
                    if (get_conf_int("half_page_scroll")) scroll /= 2;
                    scroll_right(sh, scroll);
                    break;

                case L'm':
                    ;
                    int i = 0, c = 0, ancho = sh->rescol;
                    sh->offscr_sc_cols = 0;
                    for (i = 0; i < sh->curcol; i++) {
                        for (c = i; c < sh->curcol; c++) {
                            if (! sh->col_hidden[c]) ancho += sh->fwidth[c];
                            if (ancho >= SC_DISPLAY_COLS/2) {
                                ancho = sh->rescol;
                                break;
                            }
                        }
                        if (c == sh->curcol) break;
                    }
                    sh->offscr_sc_cols = i;
                    break;

                case L't':
                case L'b':
                case L'z':
                case L'.':
                    {
                    int i = 0, r = sh->offscr_sc_rows-1;

                    if (buf->pnext->value == L't') {
                        while (i < SC_DISPLAY_ROWS && r < sh->currow) {
                            r++;
                            if (sh->row_frozen[r]) continue;
                            i++;
                        }
                        scroll_down(sh, --i);

                    } else if (buf->pnext->value == L'b') {
                        int hidden = 0;
                        while (i < SC_DISPLAY_ROWS) {
                            r++;
                            if (sh->row_hidden[r]) { hidden++; continue; }
                            else if (r < sh->offscr_sc_rows && ! (sh->row_frozen[r])) continue;
                            else if (sh->row_frozen[r]) continue;
                            i++;
                        }
                        scroll_up(sh, r-sh->currow-hidden);

                    } else if (buf->pnext->value == L'z' || buf->pnext->value == L'.') {
                        while (i < SC_DISPLAY_ROWS && r <= sh->currow) {
                            r++;
                            if (sh->row_frozen[r]) continue;
                            i++;
                        }
                        int top = --i;
                        i = 0, r = sh->offscr_sc_rows-1;
                        while (i < SC_DISPLAY_ROWS) {
                            r++;
                            if (r < sh->offscr_sc_rows && ! (sh->row_frozen[r])) continue;
                            i++;
                        }
                        int bottom = r-sh->currow;
                        int scroll = (-top + bottom)/2;
                        if (scroll < 0)
                            scroll_down(sh, -scroll);
                        else if (scroll > 0)
                            scroll_up(sh, scroll);
                    }
                    break;
                    }
            }
            ui_update(TRUE);
            break;

        // scroll up a line
        case ctl('y'):
            scroll_up(sh, 1);
            ui_update(TRUE);
            break;

        // scroll down a line
        case ctl('e'):
            scroll_down(sh, 1);
            ui_update(TRUE);
            break;

        // undo
        case L'u':
            #ifdef UNDO
            do_undo();
            ui_update(TRUE);
            #else
            sc_error("Build was done without UNDO support");
            #endif
            break;

        // redo
        case ctl('r'):
            #ifdef UNDO
            do_redo();
            ui_update(TRUE);
            #else
            sc_error("Build was done without UNDO support");
            #endif
            break;

        case L'{': // left align
        case L'}': // right align
        case L'|': // center align
            {
            int p, r = sh->currow, c = sh->curcol, rf = sh->currow, cf = sh->curcol;
            struct srange * sr;
            if ( (p = is_range_selected()) != -1) {
                sr = get_range_by_pos(p);
                r = sr->tlrow;
                c = sr->tlcol;
                rf = sr->brrow;
                cf = sr->brcol;
            }
            if (any_locked_cells(sh, r, c, rf, cf)) {
                sc_error("Locked cells encountered. Nothing changed");
                return;
            }
#ifdef UNDO
            create_undo_action();
#endif
            if (buf->value == L'{')      swprintf(interp_line, BUFFERSIZE, L"leftjustify %s", v_name(r, c));
            else if (buf->value == L'}') swprintf(interp_line, BUFFERSIZE, L"rightjustify %s", v_name(r, c));
            else if (buf->value == L'|') swprintf(interp_line, BUFFERSIZE, L"center %s", v_name(r, c));
            if (p != -1) swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L":%s", v_name(rf, cf));
#ifdef UNDO
            copy_to_undostruct(sh, r, c, rf, cf, UNDO_DEL, IGNORE_DEPS, NULL);
#endif
            send_to_interp(interp_line);
#ifdef UNDO
            copy_to_undostruct(sh, r, c, rf, cf, UNDO_ADD, IGNORE_DEPS, NULL);
            end_undo_action();
#endif
            cmd_multiplier = 0;
            ui_update(TRUE);
            break;
            }

        case ctl('l'):
            sig_winchg();
            break;

        case L'@':
            EvalAll();
            ui_update(TRUE);
            break;

        // increase or decrease numeric value of cell or range
        case L'-':
        case L'+':
            {
            int r, c, tlrow = sh->currow, tlcol = sh->curcol, brrow = sh->currow, brcol = sh->curcol;
            if ( is_range_selected() != -1 ) {
                struct srange * sr = get_selected_range();
                tlrow = sr->tlrow;
                tlcol = sr->tlcol;
                brrow = sr->brrow;
                brcol = sr->brcol;
            }
            if (any_locked_cells(sh, tlrow, tlcol, brrow, brcol)) {
                sc_error("Locked cells encountered. Nothing changed");
                return;
            }
            if (get_conf_int("numeric") == 1) goto numeric;
            struct ent * p;
#ifdef UNDO
            create_undo_action();
            copy_to_undostruct(sh, tlrow, tlcol, brrow, brcol, UNDO_DEL, IGNORE_DEPS, NULL);
#endif
            int arg = cmd_multiplier;
            int mf = roman->modflg; // keep original modflg
            for (r = tlrow; r <= brrow; r++) {
                for (c = tlcol; c <= brcol; c++) {
                    p = *ATBL(sh, sh->tbl, r, c);
                    if ( ! p )  {
                        continue;
                    } else if (p->expr && !(p->flags & is_strexpr)) {
                        //sc_error("Can't increment / decrement a formula");
                        continue;
                    } else if (p->flags & is_valid) {
                        p->v += buf->value == L'+' ? (double) arg : - 1 * (double) arg;
                        if (mf == roman->modflg) roman->modflg++; // increase just one time
                    }
                }
            }
#ifdef UNDO
            copy_to_undostruct(sh, tlrow, tlcol, brrow, brcol, UNDO_ADD, IGNORE_DEPS, NULL);
            end_undo_action();
#endif
            if (get_conf_int("autocalc")) EvalRange(sh, tlrow, tlcol, brrow, brcol);
            cmd_multiplier = 0;
            ui_update(TRUE);
            }
            break;

        // input of numbers
        default:
        numeric:
            if ( (isdigit(buf->value) || buf->value == L'-' || buf->value == L'+' ||
                  ( buf->value == L'.' &&  get_conf_int("numeric_decimal") )) &&
                get_conf_int("numeric") ) {
                if (locked_cell(sh, sh->currow, sh->curcol)) return;
                insert_edit_submode='=';
                chg_mode(insert_edit_submode);
#ifdef INS_HISTORY_FILE
                ori_insert_edit_submode = buf->value;
                add(insert_history, L"");
#endif
                inputline_pos = 0;
                real_inputline_pos = 0;
                ins_in_line(buf->value);
                ui_show_header();
            }
    }
    return;
}
