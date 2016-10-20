#include <ctype.h>
#include <stdlib.h>
#include "yank.h"
#include "marks.h"
#include "cmds.h"
#include "conf.h"
#include "screen.h"
#include "color.h"   // for set_ucolor
#include "cmds_edit.h"
#include "history.h"
#include "hide_show.h"
#include "shift.h"
#include "main.h"    // for winchg
#include "interp.h"
#include "utils/extra.h"
#ifdef UNDO
#include "undo.h"
#endif


#include "dep_graph.h"
extern graphADT graph;
extern char valores;


extern int cmd_multiplier;
extern struct history * commandline_history;
extern void start_visualmode(int tlrow, int tlcol, int brrow, int brcol);
wchar_t interp_line[BUFFERSIZE];

void do_normalmode(struct block * buf) {
    int bs = get_bufsize(buf);
    struct ent * e;

    switch (buf->value) {
        /* TEST
        case L'A':
            {
            struct ent * p = *ATBL(tbl, currow, curcol);
            if (!p) return;
            char det[20000] = "";
            sprintf(det + strlen(det), "r:%d\nc:%d\nexpr null:%d\n", p->row, p->col, p->expr == NULL);
            if  (p && p->expr != NULL && p->expr->e.v.vp != NULL) {
                sprintf(det + strlen(det), "vp null:%d\n", p->expr->e.v.vp == NULL);
                sprintf(det + strlen(det), "vp row:%d\n", p->expr->e.v.vp->row);
                sprintf(det + strlen(det), "vp col:%d\n", p->expr->e.v.vp->col);
                //sprintf(det + strlen(det), "vp vf:%f\n", p->expr->e.v.vf);
            }
            if (p && p->expr != NULL && p->expr->e.o.right != NULL && p->expr->e.o.right->e.v.vp != NULL) {
                    sprintf(det + strlen(det), "vp null:%d\n", p->expr->e.o.right->e.v.vp == NULL);
                    sprintf(det + strlen(det), "vp row:%d\n", p->expr->e.o.right->e.v.vp->row);
                    sprintf(det + strlen(det), "vp col:%d\n", p->expr->e.o.right->e.v.vp->col);
                 //   sprintf(det + strlen(det), "vp vf:%f\n", p->expr->e.o.right->e.v.vf);
            }
            show_text((char *) &det);
            }
            break;

        case L'W':
            break;

        case L'Q':
            rebuild_graph();
            break;

        */






        // MOVEMENT COMMANDS
        case L'j':
        case OKEY_DOWN:
            lastcol = curcol;
            lastrow = currow;
            currow = forw_row(1)->row;
            unselect_ranges();
            update(TRUE);
            break;

        case L'k':
        case OKEY_UP:
            lastcol = curcol;
            lastrow = currow;
            currow = back_row(1)->row;
            unselect_ranges();
            update(TRUE);
            break;

        case L'h':
        case OKEY_LEFT:
            lastrow = currow;
            lastcol = curcol;
            curcol = back_col(1)->col;
            unselect_ranges();
            update(TRUE);
            break;

        case L'l':
        case OKEY_RIGHT:
            lastrow = currow;
            lastcol = curcol;
            curcol = forw_col(1)->col;
            unselect_ranges();
            update(TRUE);
            break;

        case L'0':
            if (atoi(get_conf_value("numeric_zero")) == 1) goto numeric;
        case OKEY_HOME:
            lastrow = currow;
            lastcol = curcol;
            curcol = left_limit()->col;
            unselect_ranges();
            update(TRUE);
            break;

        case L'$':
        case OKEY_END:
            lastrow = currow;
            lastcol = curcol;
            curcol = right_limit()->col;
            unselect_ranges();
            update(TRUE);
            break;

        case L'^':
            lastcol = curcol;
            lastrow = currow;
            currow = goto_top()->row;
            unselect_ranges();
            update(TRUE);
            break;

        case L'#':
            lastcol = curcol;
            lastrow = currow;
            currow = goto_bottom()->row;
            if (currow == lastrow && curcol == lastcol) currow = go_end()->row;
            unselect_ranges();
            update(TRUE);
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
            update(TRUE);
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
            update(TRUE);
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
            update(TRUE);
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
            if (atoi(get_conf_value("half_page_scroll"))) n = n / 2;
            struct ent * e = forw_row(n);
            lastcol = curcol;
            lastrow = currow;
            currow = e->row;
            unselect_ranges();
            scroll_down(n);
            update(TRUE);
            break;
            }

        // CTRL b
        case ctl('b'):
        case OKEY_PGUP:
            {
            int n = LINES - RESROW - 1;
            if (atoi(get_conf_value("half_page_scroll"))) n = n / 2;
            lastcol = curcol;
            lastrow = currow;
            currow = back_row(n)->row;
            unselect_ranges();
            scroll_up(n);
            update(TRUE);
            break;
            }

        case L'w':
            e = go_forward();
            lastrow = currow;
            lastcol = curcol;
            currow = e->row;
            curcol = e->col;
            unselect_ranges();
            update(TRUE);
            break;

        case L'b':
            e = go_backward();
            lastrow = currow;
            lastcol = curcol;
            currow = e->row;
            curcol = e->col;
            unselect_ranges();
            update(TRUE);
            break;

        case L'H':
            lastrow = currow;
            lastcol = curcol;
            currow = vert_top()->row;
            unselect_ranges();
            update(TRUE);
            break;

        case L'M':
            lastcol = curcol;
            lastrow = currow;
            currow = vert_middle()->row;
            unselect_ranges();
            update(TRUE);
            break;

        case L'L':
            lastrow = currow;
            lastcol = curcol;
            currow = vert_bottom()->row;
            unselect_ranges();
            update(TRUE);
            break;

        case L'G': // goto end
            e = go_end();
            lastrow = currow;
            lastcol = curcol;
            currow = e->row;
            curcol = e->col;
            unselect_ranges();
            update(TRUE);
            break;

        // GOTO goto
        case ctl('a'):
            e = go_home();
            lastrow = currow;
            lastcol = curcol;
            curcol = e->col;
            currow = e->row;
            unselect_ranges();
            update(TRUE);
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
            } else {                                                      // gA4 (goto cell)
                (void) swprintf(interp_line, BUFFERSIZE, L"goto %s", parse_cell_name(1, buf));
                send_to_interp(interp_line);
            }
            unselect_ranges();
            update(TRUE);
            break;

        // repeat last goto command - backwards
        case L'N':
            go_previous();
            update(TRUE);
            break;

        // repeat last goto command
        case L'n':
            go_last();
            update(TRUE);
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
            if (atoi(get_conf_value("numeric_decimal")) == 1) goto numeric;
            copybuffer(lastcmd_buffer, buf); // nose graba en lastcmd_buffer!!
            cmd_multiplier = 1;
            exec_mult(buf, COMPLETECMDTIMEOUT);
            break;

        // enter command mode
        case L':':
            clr_header(input_win, 0);
            chg_mode(':');
#ifdef HISTORY_FILE
            add(commandline_history, L"");
#endif
            print_mode(input_win);
            wrefresh(input_win);
            handle_cursor();
            inputline_pos = 0;
            real_inputline_pos = 0;
            break;

        // enter visual mode
        case L'v':
            chg_mode('v');
            handle_cursor();
            clr_header(input_win, 0);
            print_mode(input_win);
            wrefresh(input_win);
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
            clr_header(input_win, 0);
            print_mode(input_win);
            wrefresh(input_win);
            inputline_pos = 0;
            real_inputline_pos = 0;
            break;

        // EDITION COMMANDS
        // edit cell (v)
        case L'e':
            if (locked_cell(currow, curcol)) return;
            clr_header(input_win, 0);
            inputline_pos = 0;
            real_inputline_pos = 0;
            if (start_edit_mode(buf, 'v')) show_header(input_win);
            break;

        // edit cell (s)
        case L'E':
            if (locked_cell(currow, curcol)) return;
            clr_header(input_win, 0);
            inputline_pos = 0;
            real_inputline_pos = 0;
            if (start_edit_mode(buf, 's')) show_header(input_win);
            else {
                sc_info("No string value to edit");
                chg_mode('.');
                show_celldetails(input_win);
                print_mode(input_win);
                wrefresh(input_win);
            }
            break;

        // del current cell or range
        case L'x':
            del_selected_cells();
            update(TRUE);
            break;

        // format col
        case L'f':
            if (bs != 2) return;
#ifdef UNDO

            create_undo_action();
            add_undo_col_format(curcol, 'R', fwidth[curcol], precision[curcol], realfmt[curcol]);
#endif
            formatcol(buf->pnext->value);
#ifdef UNDO
            add_undo_col_format(curcol, 'A', fwidth[curcol], precision[curcol], realfmt[curcol]);
            end_undo_action();
#endif
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
                struct ent * p = *ATBL(tbl, get_mark(buf->pnext->value)->row, get_mark(buf->pnext->value)->col);
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
                    copy_to_undostruct(currow, c1, currow, c1, 'd');
#endif
                    copyent(n, p, currow - get_mark(buf->pnext->value)->row, c1 - get_mark(buf->pnext->value)->col, 0, 0, maxrow, maxcol, 0);

                    n->row += currow - get_mark(buf->pnext->value)->row;
                    n->col += c1 - get_mark(buf->pnext->value)->col;

                    n->flags |= is_changed;
#ifdef UNDO
                    copy_to_undostruct(currow, c1, currow, c1, 'a');
#endif
                }
#ifdef UNDO
                end_undo_action();
#endif
            }

            if (atoi(get_conf_value("autocalc"))) EvalAll();
            update(TRUE);
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
            } else if (buf->pnext->value == L'u') {
                unlock_cells(lookat(r, c), lookat(rf, cf));
            } else if (buf->pnext->value == L'v') {
                valueize_area(r, c, rf, cf);
            }
            update(TRUE);
            break;
            }

        // create range with two marks
        case L'R':
            if (bs == 3) {
                create_range(buf->pnext->value, buf->pnext->pnext->value, NULL, NULL);
                update(TRUE);
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
            update(TRUE);
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
            if ( any_locked_cells(r, c, rf, cf) && (buf->pnext->value == L'h' || buf->pnext->value == L'k') ) {
                sc_error("Locked cells encountered. Nothing changed");
                return;
            }
#ifdef UNDO
            create_undo_action();
#endif
            int ic = cmd_multiplier + 1;
            switch (buf->pnext->value) {
                case L'j':
                    fix_marks(  (rf - r + 1) * cmd_multiplier, 0, r, maxrow, c, cf);
#ifdef UNDO
                    save_undo_range_shift(cmd_multiplier, 0, r, c, rf + (rf-r+1) * (cmd_multiplier - 1), cf);
#endif
                    while (ic--) shift_range(ic, 0, r, c, rf, cf);
                    break;
                case L'k':
                    fix_marks( -(rf - r + 1) * cmd_multiplier, 0, r, maxrow, c, cf);
                    yank_area(r, c, rf + (rf-r+1) * (cmd_multiplier - 1), cf, 'a', cmd_multiplier); // keep ents in yanklist for sk
#ifdef UNDO
                    copy_to_undostruct(r, c, rf + (rf-r+1) * (cmd_multiplier - 1), cf, 'd');
                    save_undo_range_shift(-cmd_multiplier, 0, r, c, rf + (rf-r+1) * (cmd_multiplier - 1), cf);
#endif
                    while (ic--) shift_range(-ic, 0, r, c, rf, cf);
#ifdef UNDO
                    copy_to_undostruct(r, c, rf + (rf-r+1) * (cmd_multiplier - 1), cf, 'a');
#endif
                    if (atoi(get_conf_value("autocalc")) && ! loading) EvalAll();
                    break;
                case L'h':
                    fix_marks(0, -(cf - c + 1) * cmd_multiplier, r, rf, c, maxcol);
                    yank_area(r, c, rf, cf + (cf-c+1) * (cmd_multiplier - 1), 'a', cmd_multiplier); // keep ents in yanklist for sk
#ifdef UNDO
                    copy_to_undostruct(r, c, rf, cf + (cf-c+1) * (cmd_multiplier - 1), 'd');
                    save_undo_range_shift(0, -cmd_multiplier, r, c, rf, cf + (cf-c+1) * (cmd_multiplier - 1));
#endif
                    while (ic--) shift_range(0, -ic, r, c, rf, cf);
#ifdef UNDO
                    copy_to_undostruct(r, c, rf, cf + (cf-c+1) * (cmd_multiplier - 1), 'a');
#endif
                    if (atoi(get_conf_value("autocalc")) && ! loading) EvalAll();
                    break;
                case L'l':
                    fix_marks(0,  (cf - c + 1) * cmd_multiplier, r, rf, c, maxcol);
#ifdef UNDO
                    save_undo_range_shift(0, cmd_multiplier, r, c, rf, cf + (cf-c+1) * (cmd_multiplier - 1));
#endif
                    while (ic--) shift_range(0, ic, r, c, rf, cf);
                    break;
            }
#ifdef UNDO
            end_undo_action();
#endif
            cmd_multiplier = 0;
            unselect_ranges();
            update(TRUE);
            break;
            }

        // delete row or column, or selected cell or range
        case L'd':
            {
            if (bs != 2) return;
            int ic = cmd_multiplier; // orig

            if (buf->pnext->value == L'r') {
                if (any_locked_cells(currow, 0, currow + cmd_multiplier, maxcol)) {
                    sc_error("Locked cells encountered. Nothing changed");
                    return;
                }
#ifdef UNDO
                create_undo_action();
                copy_to_undostruct(currow, 0, currow + ic - 1, maxcol, 'd');
                save_undo_range_shift(-ic, 0, currow, 0, currow - 1 + ic, maxcol);
#endif
                fix_marks(-ic, 0, currow + ic - 1, maxrow, 0, maxcol);
                yank_area(currow, 0, currow - 1 + cmd_multiplier, maxcol, 'r', ic);
                while (ic--) deleterow();
                EvalAll();
#ifdef UNDO
                copy_to_undostruct(currow, 0, currow - 1 + cmd_multiplier, maxcol, 'a');
                end_undo_action();
#endif
                if (cmd_multiplier > 0) cmd_multiplier = 0;

            } else if (buf->pnext->value == L'c') {
                if (any_locked_cells(0, curcol, maxrow, curcol + cmd_multiplier)) {
                    sc_error("Locked cells encountered. Nothing changed");
                    return;
                }
#ifdef UNDO
                create_undo_action();
                copy_to_undostruct(0, curcol, maxrow, curcol - 1 + ic, 'd');
                save_undo_range_shift(0, -ic, 0, curcol, maxrow, curcol - 1 + ic);
#endif
                fix_marks(0, -ic, 0, maxrow,  curcol - 1 + ic, maxcol); // FIXME
                yank_area(0, curcol, maxrow, curcol + cmd_multiplier - 1, 'c', ic);
                while (ic--) {
#ifdef UNDO
                    add_undo_col_format(curcol-ic+1, 'R', fwidth[curcol], precision[curcol], realfmt[curcol]);
#endif
                    deletecol();
                }
#ifdef UNDO
                copy_to_undostruct(0, curcol, maxrow, curcol + cmd_multiplier - 1, 'a');
                end_undo_action();
#endif
                if (cmd_multiplier > 0) cmd_multiplier = 0;

            } else if (buf->pnext->value == L'd') {
                del_selected_cells();
            }
            if (atoi(get_conf_value("autocalc")) && ! loading) EvalAll();
            update(TRUE);
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
            update(TRUE);
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
            update(TRUE);
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
            update(TRUE);
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
                update(TRUE);
            }
            break;

        // paste cell above or right
        case L't':
            if (paste_yanked_ents(1, 'a') == -1) {
                sc_error("Locked cells encountered. Nothing changed");
                break;
            }
            update(TRUE);
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
                update(TRUE);
            }
            break;

        // scroll
        case L'z':
            if ( bs != 2 ) break;
            int scroll = 0;

            switch (buf->pnext->value) {
                case L'l':
                    scroll_right(1);
                    //unselect_ranges();
                    break;

                case L'h':
                    scroll_left(1);
                    //unselect_ranges();
                    break;

                case L'H':
                    scroll = calc_offscr_sc_cols();
                    if (atoi(get_conf_value("half_page_scroll"))) scroll /= 2;
                    scroll_left(scroll);
                    //unselect_ranges();
                    break;

                case L'L':
                    scroll = calc_offscr_sc_cols();
                    if (atoi(get_conf_value("half_page_scroll"))) scroll /= 2;
                    scroll_right(scroll);
                    //unselect_ranges();
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

                case L'z':
                case L'.':
                case L't':
                case L'b':
                    if (buf->pnext->value == L'z' || buf->pnext->value == L'.')
                        scroll = currow - offscr_sc_rows + LINES - RESROW - 2 - (LINES - RESROW - 2)/2; // zz
                    else if (buf->pnext->value == L't')
                        scroll = currow - offscr_sc_rows + 1;
                    else if (buf->pnext->value == L'b')
                        scroll = currow - offscr_sc_rows - LINES + RESROW + 2;

                    if (scroll > 0)
                        scroll_down(scroll);
//                    else if (scroll > offscr_sc_rows)
//                        scroll_up(-scroll);
                    else if (scroll < 0)
                        scroll_up(-scroll);
//                    else if (offscr_sc_rows > 0)
//                        scroll_up(offscr_sc_rows);
                    break;

            }
            update(TRUE);
            break;

        // scroll up a line
        case ctl('y'):
            scroll_up(1);
            update(TRUE);
            break;

        // scroll down a line
        case ctl('e'):
            scroll_down(1);
            update(TRUE);
            break;

        // undo
        case L'u':
            #ifdef UNDO
            do_undo();
            // sync_refs();
            EvalAll();
            update(TRUE);
            break;
            #else
            sc_error("Build was done without UNDO support");
            #endif

        // redo
        case ctl('r'):
            #ifdef UNDO
            do_redo();
            // sync_refs();
            EvalAll();
            update(TRUE);
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
            copy_to_undostruct(r, c, rf, cf, 'd');
#endif
            send_to_interp(interp_line);
#ifdef UNDO
            copy_to_undostruct(r, c, rf, cf, 'a');
            end_undo_action();
#endif
            cmd_multiplier = 0;
            update(TRUE);
            break;
            }

        case ctl('l'):
            /*
            endwin();
            start_screen();
            clearok(stdscr, TRUE);
            update(TRUE);
            flushinp();
            show_header(input_win);
            show_celldetails(input_win);
            wrefresh(input_win);
            update(TRUE);
            */
            winchg();
            break;

        case L'@':
            EvalAll();
            update(TRUE);
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
            if (atoi(get_conf_value("numeric")) == 1) goto numeric;
            struct ent * p;
#ifdef UNDO
            create_undo_action();
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
#ifdef UNDO
                        copy_to_undostruct(r, c, r, c, 'd');
#endif
                        p->v += buf->value == L'+' ? (double) arg : - 1 * (double) arg;
#ifdef UNDO
                        copy_to_undostruct(r, c, r, c, 'a');
#endif
                        if (mf == modflg) modflg++; // increase just one time
                    }
                }
            }
#ifdef UNDO
            end_undo_action();
#endif
            if (atoi(get_conf_value("autocalc"))) EvalAll();
            cmd_multiplier = 0;
            update(TRUE);
            }
            break;

        // input of numbers
        default:
        numeric:
            if ( (isdigit(buf->value) || buf->value == L'-' || buf->value == L'+' || 
                  ( buf->value == L'.' &&  atoi(get_conf_value("numeric_decimal")) )) &&
                atoi(get_conf_value("numeric")) ) {
                insert_edit_submode='=';
                chg_mode(insert_edit_submode);
                inputline_pos = 0;
                real_inputline_pos = 0;
                ins_in_line(buf->value);
                show_header(input_win);
            }
    }
    return;
}
