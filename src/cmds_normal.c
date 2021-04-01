/*******************************************************************************
 * Copyright (c) 2013-2021, Andrés Martinelli <andmarti@gmail.com>             *
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
 * \file cmds_normal.c
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 2017-07-18
 * \brief TODO Write a tbrief file description.
 */

#include <ctype.h>
#include <stdlib.h>

#include "yank.h"
#include "marks.h"
#include "cmds.h"
#include "conf.h"
#include "tui.h"
#include "cmds_edit.h"
#include "history.h"
#include "hide_show.h"
#include "shift.h"
#include "main.h"    // for sig_winchg
#include "interp.h"
#include "freeze.h"
#include "utils/extra.h"
#ifdef UNDO
#include "undo.h"
#endif


#include "dep_graph.h"
extern graphADT graph;
extern char valores;
extern int cmd_multiplier;
extern void start_visualmode(int tlrow, int tlcol, int brrow, int brcol);
extern void ins_in_line(wint_t d);

extern wchar_t interp_line[BUFFERSIZE];

#ifdef HISTORY_FILE
extern struct history * commandline_history;
#endif

#ifdef INS_HISTORY_FILE
extern struct history * insert_history;
extern char ori_insert_edit_submode;
#endif

/**
 * \brief TODO Document do_normalmode()
 *
 * \param[in] buf
 *
 * \return none
 */

void do_normalmode(struct block * buf) {
    int bs = get_bufsize(buf);
    struct ent * e;

    switch (buf->value) {
        // FOR TEST PURPOSES
        case L'A':
            //;
            //wchar_t t = ui_query_opt(L"show a message. q / a / d to quit", L"qad");
            //sc_info("char: %lc.", t);
            break;

        case L'W':
            break;

        case L'Q':
            break;

        // MOVEMENT COMMANDS
        case L'j':
        case OKEY_DOWN:
            lastcol = curcol;
            lastrow = currow;
            currow = forw_row(1)->row;
            unselect_ranges();
            ui_update(TRUE);
            break;

        case L'k':
        case OKEY_UP:
            lastcol = curcol;
            lastrow = currow;
            currow = back_row(1)->row;
            unselect_ranges();
            ui_update(TRUE);
            break;

        case L'h':
        case OKEY_LEFT:
            lastrow = currow;
            lastcol = curcol;
            curcol = back_col(1)->col;
            unselect_ranges();
            ui_update(TRUE);
            break;

        case L'l':
        case OKEY_RIGHT:
            lastrow = currow;
            lastcol = curcol;
            curcol = forw_col(1)->col;
            unselect_ranges();
            ui_update(TRUE);
            break;

        case L'0':
            if (get_conf_int("numeric_zero") == 1 && get_conf_int("numeric") == 1) goto numeric;
        case OKEY_HOME:
            ;
            int freeze = freeze_ranges && (freeze_ranges->type == 'c' ||  freeze_ranges->type == 'a') ? 1 : 0;
            int tlcol = freeze ? freeze_ranges->tl->col : 0;
            int brcol = freeze ? freeze_ranges->br->col : 0;
            extern int center_hidden_cols;
            lastrow = currow;
            lastcol = curcol;
            if (freeze && curcol > brcol && tlcol >= offscr_sc_cols && curcol != brcol + center_hidden_cols + 1) curcol = brcol + center_hidden_cols + 1;
            else curcol = left_limit()->col;

            unselect_ranges();
            ui_update(TRUE);
            break;

        case L'$':
        case OKEY_END:
            lastrow = currow;
            lastcol = curcol;
            curcol = right_limit()->col;
            unselect_ranges();
            ui_update(TRUE);
            break;

        case L'^':
            lastcol = curcol;
            lastrow = currow;
            currow = goto_top()->row;
            unselect_ranges();
            ui_update(TRUE);
            break;

        case L'#':
            lastcol = curcol;
            lastrow = currow;
            currow = goto_bottom()->row;
            if (currow == lastrow && curcol == lastcol) currow = go_end()->row;
            unselect_ranges();
            ui_update(TRUE);
            break;

        // Tick
        case L'\'':
            if (bs != 2) break;
            unselect_ranges();
            e = tick(buf->pnext->value);
            if (row_hidden[e->row]) {
                sc_error("Cell row is hidden");
                break;
            }
            if (col_hidden[e->col]) {
                sc_error("Cell column is hidden");
                break;
            }
            lastrow = currow;
            lastcol = curcol;
            currow = e->row;
            curcol = e->col;
            ui_update(TRUE);
            break;

        // CTRL j
        case ctl('j'):
            {
            int p, c = curcol, cf = curcol;
            if ( (p = is_range_selected()) != -1) {
                struct srange * sr = get_range_by_pos(p);
                c = sr->tlcol;
                cf = sr->brcol;
            }
            auto_justify(c, cf, DEFWIDTH);  // auto justify columns
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
            int p, r = currow, c = curcol, rf = currow, cf = curcol;
            if ( (p = is_range_selected()) != -1) {
                struct srange * sr = get_range_by_pos(p);
                r = sr->tlrow;
                c = sr->tlcol;
                rf = sr->brrow;
                cf = sr->brcol;
            }
            if (any_locked_cells(r, c, rf, cf)) {
                sc_error("Locked cells encountered. Nothing changed");
                return;
            }
            dateformat(lookat(r, c), lookat(rf, cf), f);
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
            int n = LINES - RESROW - 1;
            if (get_conf_int("half_page_scroll")) n = n / 2;
            lastcol = curcol;
            lastrow = currow;
            currow = forw_row(n)->row;
            unselect_ranges();
            scroll_down(n);
            ui_update(TRUE);
            break;
            }

        // CTRL b
        case ctl('b'):
        case OKEY_PGUP:
            {
            int n = LINES - RESROW - 1;
            if (get_conf_int("half_page_scroll")) n = n / 2;
            lastcol = curcol;
            lastrow = currow;
            currow = back_row(n)->row;
            unselect_ranges();
            scroll_up(n);
            ui_update(TRUE);
            break;
            }

        case L'w':
            e = go_forward();
            lastrow = currow;
            lastcol = curcol;
            currow = e->row;
            curcol = e->col;
            unselect_ranges();
            ui_update(TRUE);
            break;

        case L'b':
            e = go_backward();
            lastrow = currow;
            lastcol = curcol;
            currow = e->row;
            curcol = e->col;
            unselect_ranges();
            ui_update(TRUE);
            break;

        case L'H':
            lastrow = currow;
            int currow_h = vert_top()->row;
            if (currow_h < center_hidden_rows) break;
            currow = currow_h;
            unselect_ranges();
            ui_update(TRUE);
            break;

        case L'M':
            lastrow = currow;
            currow = vert_middle()->row;
            unselect_ranges();
            ui_update(TRUE);
            break;

        case L'L':
            lastrow = currow;
            currow = vert_bottom()->row;
            unselect_ranges();
            ui_update(TRUE);
            break;

        case L'G': // goto end
            e = go_end();
            lastrow = currow;
            lastcol = curcol;
            currow = e->row;
            curcol = e->col;
            unselect_ranges();
            ui_update(TRUE);
            break;

        // GOTO goto
        case ctl('a'):
            e = go_home();
            lastrow = currow;
            lastcol = curcol;
            curcol = e->col;
            currow = e->row;
            unselect_ranges();
            extern int center_hidden_rows;
            extern int center_hidden_cols;
            center_hidden_rows=0;
            center_hidden_cols=0;
            offscr_sc_rows = 0;
            offscr_sc_cols = 0;
            ui_update(TRUE);
            break;

        case L'g':
            if (buf->pnext->value == L'0') {                               // g0
                lastcol = curcol;
                lastrow = currow;
                curcol = go_bol()->col;

            } else if (buf->pnext->value == L'$') {                        // g$
                lastcol = curcol;
                lastrow = currow;
                curcol = go_eol()->col;

            } else if (buf->pnext->value == L'g') {                        // gg
                e = go_home();
                lastcol = curcol;
                lastrow = currow;
                curcol = e->col;
                currow = e->row;
                extern int center_hidden_rows;
                extern int center_hidden_cols;
                center_hidden_rows=0;
                center_hidden_cols=0;
                offscr_sc_rows = 0;
                offscr_sc_cols = 0;

            } else if (buf->pnext->value == L'G') {                        // gG
                e = go_end();
                lastcol = curcol;
                lastrow = currow;
                currow = e->row;
                curcol = e->col;

            } else if (buf->pnext->value == L'M') {                        // gM
                lastcol = curcol;
                lastrow = currow;
                curcol = horiz_middle()->col;

            // goto last cell position
            } else if (buf->pnext->value == L'l') {                        // gl
                int newlr = currow;
                int newlc = curcol;
                curcol = lastcol;
                currow = lastrow;
                lastrow = newlr;
                lastcol = newlc;
            } else if (buf->pnext->value == L't') {                        // gtA4 (goto cell A4)
                (void) swprintf(interp_line, BUFFERSIZE, L"goto %s", parse_cell_name(2, buf));
                send_to_interp(interp_line);
            }
            unselect_ranges();
            ui_update(TRUE);
            break;

        // repeat last goto command - backwards
        case L'N':
            go_previous();
            ui_update(TRUE);
            break;

        // repeat last goto command
        case L'n':
            go_last();
            ui_update(TRUE);
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
            start_visualmode(currow, curcol, currow, curcol);
            break;

        // INPUT COMMANDS
        case L'=':
        case L'\\':
        case L'<':
        case L'>':
            if (locked_cell(currow, curcol)) return;
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
            if (locked_cell(currow, curcol)) return;
            ui_clr_header(0);
            inputline_pos = 0;
            real_inputline_pos = 0;
            if (start_edit_mode(buf, 'v')) ui_show_header();
            break;

        // edit cell (s)
        case L'E':
            if (locked_cell(currow, curcol)) return;
            ui_clr_header(0);
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
            del_selected_cells();
            ui_update(TRUE);
            break;

        // format col or freeze range
        case L'f':
            if (bs != 2) return;

            // freeze row / column or area
            if (buf->pnext->value == 'r' || buf->pnext->value == 'c' || buf->pnext->value == 'a') {
                int p = is_range_selected(), r = currow, c = curcol, rf = currow, cf = curcol;

                if (p != -1) { // mark range
                    struct srange * sr = get_range_by_pos(p);
                    r = sr->tlrow;
                    c = sr->tlcol;
                    rf = sr->brrow;
                    cf = sr->brcol;
                }

                if (buf->pnext->value == 'r') {
                    add_frange(lookat(r, c), lookat(rf, cf), 'r');
                } else if (buf->pnext->value == 'c') {
                    add_frange(lookat(r, c), lookat(rf, cf), 'c');
                } else if (buf->pnext->value == 'a') {
                    add_frange(lookat(r, c), lookat(rf, cf), 'a');
                }

            // change in format
            } else {
#ifdef UNDO

            create_undo_action();
            add_undo_col_format(curcol, 'R', fwidth[curcol], precision[curcol], realfmt[curcol]);
#endif
            formatcol(buf->pnext->value);
#ifdef UNDO
            add_undo_col_format(curcol, 'A', fwidth[curcol], precision[curcol], realfmt[curcol]);
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
                set_range_mark(buf->pnext->value, sr);
            } else         // mark cell
                set_cell_mark(buf->pnext->value, currow, curcol);
            modflg++;
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
                yank_area(r->tlrow, r->tlcol, r->brrow, r->brcol, 'a', cmd_multiplier);
                if (paste_yanked_ents(0, 'c') == -1) {
                    sc_error("Locked cells encountered. Nothing changed");
                    break;
                }

            // if m represents just one cell
            } else {
                struct mark * m = get_mark(buf->pnext->value);
                struct ent * p = lookat(m->row, m->col);
                struct ent * n;
                int c1;

#ifdef UNDO
                create_undo_action();
#endif
                for (c1 = curcol; cmd_multiplier-- && cmd_multiplier > -1 && c1 < maxcols; c1++) {
                    if ((n = * ATBL(tbl, currow, c1))) {
                        if (n->flags & is_locked)
                            continue;
                        if (! p) {
                            clearent(n);
                            continue;
                        }
                    } else {
                        if (! p) break;
                        n = lookat(currow, c1);
                    }
#ifdef UNDO
                    // added for #244 - 22/03/2018
                    ents_that_depends_on_range(n->row, n->col, n->row, n->col);
                    copy_to_undostruct(currow, c1, currow, c1, UNDO_DEL, HANDLE_DEPS, NULL);
#endif
                    copyent(n, p, currow - get_mark(buf->pnext->value)->row, c1 - get_mark(buf->pnext->value)->col, 0, 0, maxrow, maxcol, 0);

                    n->row += currow - get_mark(buf->pnext->value)->row;
                    n->col += c1 - get_mark(buf->pnext->value)->col;

                    n->flags |= is_changed;
                    if (n->expr) EvalJustOneVertex(n, n->row, n->col, 1);

#ifdef UNDO
                    copy_to_undostruct(currow, c1, currow, c1, UNDO_ADD, HANDLE_DEPS, NULL);
#endif
                }
#ifdef UNDO
                extern struct ent_ptr * deps;
                if (deps != NULL) free(deps);
                deps = NULL;
                end_undo_action();
#endif
            }

            //if (get_conf_int("autocalc")) EvalAll();
            ui_update(TRUE);
            break;
            }

        // range lock / unlock / valueize
        case L'r':
            {
            int p, r = currow, c = curcol, rf = currow, cf = curcol;
            if ( (p = is_range_selected()) != -1) {
                struct srange * sr = get_range_by_pos(p);
                r = sr->tlrow;
                c = sr->tlcol;
                rf = sr->brrow;
                cf = sr->brcol;
            }
            if (buf->pnext->value == L'l') {
                lock_cells(lookat(r, c), lookat(rf, cf));
            } else if (buf->pnext->value == L'u') { // watch out if you do C-r and u too quickly !
                unlock_cells(lookat(r, c), lookat(rf, cf));
            } else if (buf->pnext->value == L'v') {
                valueize_area(r, c, rf, cf);
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
            int rs, r = currow, c = curcol, arg = cmd_multiplier;
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
            int p, r = currow, c = curcol, rf = currow, cf = curcol;
            if ( (p = is_range_selected()) != -1) {
                struct srange * sr = get_range_by_pos(p);
                r = sr->tlrow;
                c = sr->tlcol;
                rf = sr->brrow;
                cf = sr->brcol;
            }
            shift(r, c, rf, cf, buf->pnext->value);
            unselect_ranges();
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
                deleterow(currow, ic);
                if (cmd_multiplier > 0) cmd_multiplier = 0;

            // deletecol
            } else if (buf->pnext->value == L'c') {
                deletecol(curcol, ic);
                if (cmd_multiplier > 0) cmd_multiplier = 0;

            } else if (buf->pnext->value == L'd') {
                del_selected_cells();
                if (get_conf_int("autocalc") && ! loading) EvalAll();
            }

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
                save_undo_range_shift(1, 0, currow, 0, currow, maxcol);
#endif
                fix_marks(1, 0, currow, maxrow, 0, maxcol);
                insert_row(0);

            } else if (buf->pnext->value == L'c') {
#ifdef UNDO
                save_undo_range_shift(0, 1, 0, curcol, maxrow, curcol);
#endif
                fix_marks(0, 1, 0, maxrow, curcol, maxcol);
                insert_col(0);
#ifdef UNDO
                add_undo_col_format(curcol, 'A', fwidth[curcol], precision[curcol], realfmt[curcol]);
#endif
            }
#ifdef UNDO
            end_undo_action();
#endif
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
                save_undo_range_shift(1, 0, currow+1, 0, currow+1, maxcol);
#endif
                fix_marks(1, 0, currow+1, maxrow, 0, maxcol);
                insert_row(1);

            } else if (buf->pnext->value == L'c') {
#ifdef UNDO
                save_undo_range_shift(0, 1, 0, curcol+1, maxrow, curcol+1);
#endif
                fix_marks(0, 1, 0, maxrow, curcol+1, maxcol);
                insert_col(1);
#ifdef UNDO
                add_undo_col_format(curcol, 'A', fwidth[curcol], precision[curcol], realfmt[curcol]);
#endif
            }
#ifdef UNDO
            end_undo_action();
#endif
            ui_update(TRUE);
            break;
            }

        case L'y':
            // yank row
            if ( bs == 2 && buf->pnext->value == L'r') {
                yank_area(currow, 0, currow + cmd_multiplier - 1, maxcol, 'r', cmd_multiplier);
                if (cmd_multiplier > 0) cmd_multiplier = 0;

            // yank col
            } else if ( bs == 2 && buf->pnext->value == L'c') {
                yank_area(0, curcol, maxrow, curcol + cmd_multiplier - 1, 'c', cmd_multiplier);
                if (cmd_multiplier > 0) cmd_multiplier = 0;

            // yank cell
            } else if ( bs == 2 && buf->pnext->value == L'y' && is_range_selected() == -1) {
                yank_area(currow, curcol, currow, curcol, 'e', cmd_multiplier);

            // yank range
            } else if ( bs == 1 && is_range_selected() != -1) {
                srange * r = get_selected_range();
                yank_area(r->tlrow, r->tlcol, r->brrow, r->brcol, 'a', cmd_multiplier);
            }
            break;

        // paste cell below or left
        case L'p':
            if (paste_yanked_ents(0, 'a') == -1) {
                sc_error("Locked cells encountered. Nothing changed");
                break;
            }
            ui_update(TRUE);
            break;

        case L'P':
        case L'T':
            if (bs != 2) break;
            if (buf->pnext->value == L'v' || buf->pnext->value == L'f' || buf->pnext->value == L'c') {
                int res = buf->value == L'P' ? paste_yanked_ents(0, buf->pnext->value) : paste_yanked_ents(1, buf->pnext->value); // paste cell above or right
                if (res == -1) {
                    sc_error("Locked cells encountered. Nothing changed");
                    break;
                }
                ui_update(TRUE);
            }
            break;

        // paste cell above or right
        case L't':
            if (paste_yanked_ents(1, 'a') == -1) {
                sc_error("Locked cells encountered. Nothing changed");
                break;
            }
            ui_update(TRUE);
            break;

        // select inner range - Vir
        case L'V':
            if (buf->value == L'V' && bs == 3 &&
            buf->pnext->value == L'i' && buf->pnext->pnext->value == L'r') {
                int tlrow = currow;
                int brrow = currow;
                int tlcol = curcol;
                int brcol = curcol;
                int * tlr = &tlrow;
                int * brr = &brrow;
                int * tlc = &tlcol;
                int * brc = &brcol;
                select_inner_range(tlr, tlc, brr, brc);
                start_visualmode(*tlr, *tlc, *brr, *brc);
            }
            break;

        // autojus
        case L'a':
            if ( bs != 2 ) break;

            if (buf->pnext->value == L'a') {
                int p, r = currow, c = curcol, rf = currow, cf = curcol;
                if ( (p = is_range_selected()) != -1) {
                    struct srange * sr = get_range_by_pos(p);
                    r = sr->tlrow;
                    c = sr->tlcol;
                    rf = sr->brrow;
                    cf = sr->brcol;
                }
                if (any_locked_cells(r, c, rf, cf)) {
                    sc_error("Locked cells encountered. Nothing changed");
                    return;
                }
                wchar_t cline [BUFFERSIZE];
                swprintf(cline, BUFFERSIZE, L"autojus %s:", coltoa(c));
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
                    scroll_right(1);
                    break;

                case L'h':
                    scroll_left(1);
                    break;

                case L'H':
                    scroll = calc_offscr_sc_cols() - center_hidden_cols;
                    if (get_conf_int("half_page_scroll")) scroll /= 2;
                    scroll_left(scroll);
                    break;

                case L'L':
                    scroll = calc_offscr_sc_cols() - center_hidden_cols;
                    if (get_conf_int("half_page_scroll")) scroll /= 2;
                    scroll_right(scroll);
                    break;

                case L'm':
                    ;
                    int i = 0, c = 0, ancho = rescol;
                    offscr_sc_cols = 0;
                    for (i = 0; i < curcol; i++) {
                        for (c = i; c < curcol; c++) {
                            if (!col_hidden[c]) ancho += fwidth[c];
                            if (ancho >= (COLS - rescol)/ 2) {
                                ancho = rescol;
                                break;
                            }
                        }
                        if (c == curcol) break;
                    }
                    offscr_sc_cols = i;
                    break;

                case L't':
                case L'b':
                case L'z':
                case L'.':
                    {
                    int freezer = freeze_ranges && (freeze_ranges->type == 'r' ||  freeze_ranges->type == 'a') ? 1 : 0;
                    int tlrow = freezer ? freeze_ranges->tl->row : 0;
                    int brrow = freezer ? freeze_ranges->br->row : 0;
                    int i = 0, r = offscr_sc_rows-1;


                    if (buf->pnext->value == L't') {
                        while (i < LINES - RESROW - 1 && r < currow) {
                            r++;
                            if (freezer && r >= tlrow && r <= brrow) continue;
                            else if (freezer && r > brrow && r <= brrow + center_hidden_rows) continue;
                            else if (freezer && r < tlrow && r >= tlrow - center_hidden_rows) continue;
                            i++;
                        }
                        scroll_down(--i);

                    } else if (buf->pnext->value == L'b') {
                        int hidden = 0;
                        while (i < LINES - RESROW - 1) {
                            r++;
                            if (row_hidden[r]) { hidden++; continue; }
                            else if (r < offscr_sc_rows && ! (freezer && r >= tlrow && r <= brrow)) continue;
                            else if (freezer && r >= tlrow && r <= brrow) continue;
                            else if (freezer && r > brrow && r <= brrow + center_hidden_rows) continue;
                            else if (freezer && r < tlrow && r >= tlrow - center_hidden_rows) continue;
                            i++;
                        }
                        scroll_up(r-currow-hidden);

                    } else if (buf->pnext->value == L'z' || buf->pnext->value == L'.') {
                        while (i < LINES - RESROW - 1 && r <= currow) {
                            r++;
                            //if (freezer && r >= tlrow && r <= brrow) continue;
                            //else
                            if (freezer && r > brrow && r <= brrow + center_hidden_rows) continue;
                            else if (freezer && r < tlrow && r >= tlrow - center_hidden_rows) continue;
                            i++;
                        }
                        int top = --i;
                        i = 0, r = offscr_sc_rows-1;
                        while (i < LINES - RESROW - 1) {
                            r++;
                            if (r < offscr_sc_rows && ! (freezer && r >= tlrow && r <= brrow)) continue;
                            else if (freezer && r > brrow && r <= brrow + center_hidden_rows) continue;
                            else if (freezer && r < tlrow && r >= tlrow - center_hidden_rows) continue;
                            i++;
                        }
                        int bottom = r-currow;
                        int scroll = (-top + bottom)/2;
                        if (scroll < 0)
                            scroll_down(-scroll);
                        else if (scroll > 0)
                            scroll_up(scroll);
                    }
                    break;
                    }
            }
            ui_update(TRUE);
            break;

        // scroll up a line
        case ctl('y'):
            scroll_up(1);
            ui_update(TRUE);
            break;

        // scroll down a line
        case ctl('e'):
            scroll_down(1);
            ui_update(TRUE);
            break;

        // undo
        case L'u':
            #ifdef UNDO
            do_undo();
            ui_update(TRUE);
            break;
            #else
            sc_error("Build was done without UNDO support");
            #endif

        // redo
        case ctl('r'):
            #ifdef UNDO
            do_redo();
            ui_update(TRUE);
            break;
            #else
            sc_error("Build was done without UNDO support");
            #endif

        case L'{': // left align
        case L'}': // right align
        case L'|': // center align
            {
            int p, r = currow, c = curcol, rf = currow, cf = curcol;
            struct srange * sr;
            if ( (p = is_range_selected()) != -1) {
                sr = get_range_by_pos(p);
                r = sr->tlrow;
                c = sr->tlcol;
                rf = sr->brrow;
                cf = sr->brcol;
            }
            if (any_locked_cells(r, c, rf, cf)) {
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
            copy_to_undostruct(r, c, rf, cf, UNDO_DEL, IGNORE_DEPS, NULL);
#endif
            send_to_interp(interp_line);
#ifdef UNDO
            copy_to_undostruct(r, c, rf, cf, UNDO_ADD, IGNORE_DEPS, NULL);
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
            int r, c, tlrow = currow, tlcol = curcol, brrow = currow, brcol = curcol;
            if ( is_range_selected() != -1 ) {
                struct srange * sr = get_selected_range();
                tlrow = sr->tlrow;
                tlcol = sr->tlcol;
                brrow = sr->brrow;
                brcol = sr->brcol;
            }
            if (any_locked_cells(tlrow, tlcol, brrow, brcol)) {
                sc_error("Locked cells encountered. Nothing changed");
                return;
            }
            if (get_conf_int("numeric") == 1) goto numeric;
            struct ent * p;
#ifdef UNDO
            create_undo_action();
            copy_to_undostruct(tlrow, tlcol, brrow, brcol, UNDO_DEL, IGNORE_DEPS, NULL);
#endif
            int arg = cmd_multiplier;
            int mf = modflg; // keep original modflg
            for (r = tlrow; r <= brrow; r++) {
                for (c = tlcol; c <= brcol; c++) {
                    p = *ATBL(tbl, r, c);
                    if ( ! p )  {
                        continue;
                    } else if (p->expr && !(p->flags & is_strexpr)) {
                        //sc_error("Can't increment / decrement a formula");
                        continue;
                    } else if (p->flags & is_valid) {
                        p->v += buf->value == L'+' ? (double) arg : - 1 * (double) arg;
                        if (mf == modflg) modflg++; // increase just one time
                    }
                }
            }
#ifdef UNDO
            copy_to_undostruct(tlrow, tlcol, brrow, brcol, UNDO_ADD, IGNORE_DEPS, NULL);
            end_undo_action();
#endif
            if (get_conf_int("autocalc")) EvalAll();
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
                if (locked_cell(currow, curcol)) return;
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
