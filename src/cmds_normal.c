#include "cmds_normal.h"
#include "yank.h"
#include "marks.h"
#include "cmds.h"
#include "screen.h"

extern int cmd_multiplier;
char interp_line[100];

void do_normalmode(struct block * buf) {
    int bs = get_bufsize(buf);
    struct ent * e;

    switch (buf->value) {

        // Movement commands
        case 'j':
        case OKEY_DOWN:
            currow = forw_row(1)->row; 
            unselect_ranges();
            update();
            break;

        case 'k':
        case OKEY_UP:
            currow = back_row(1)->row;
            unselect_ranges();
            update();
            break;

        case 'h':
        case OKEY_LEFT:
            curcol = back_col(1)->col;
            unselect_ranges();
            update();
            break;

        case 'l':
        case OKEY_RIGHT:
            curcol = forw_col(1)->col;
            unselect_ranges();
            update();
            break;

        case '0':
        case OKEY_HOME:
            curcol = left_limit()->col;
            unselect_ranges();
            update();
            break;

        case '$':
        case OKEY_END:
            curcol = right_limit()->col;
            unselect_ranges();
            update();
            break;

        case '^':
            currow = goto_top()->row;
            unselect_ranges();
            update();
            break;

        case '#':
            currow = goto_bottom()->row;
            unselect_ranges();
            update();
            break;

        // Tick
        case '\'':
            if (bs != 2) break;
            unselect_ranges();
            e = tick(buf->pnext->value);
            if (row_hidden[e->row]) {
                error("Cell row is hidden");
                break;
            }
            if (col_hidden[e->col]) {
                error("Cell column is hidden");
                break;
            }
            currow = e->row;
            curcol = e->col;
            update();
            break;

        // CTRL f
        case ctl('f'):
            {
            int n = LINES - RESROW - 1;
            if (atoi(get_conf_value("half_page_scroll"))) n = n / 2;
            currow = forw_row(n)->row; 
            unselect_ranges();
            scroll_down(n);
            update();
            break;
            }

        // CTRL b
        case ctl('b'):
            {
            int n = LINES - RESROW - 1;
            if (atoi(get_conf_value("half_page_scroll"))) n = n / 2;
            currow = back_row(n)->row; 
            unselect_ranges();
            scroll_up(n);
            update();
            break;
            }

        case 'w':
            e = go_forward();  
            currow = e->row;
            curcol = e->col;
            unselect_ranges();
            update();
            break;

        case 'b':
            e = go_backward();  
            currow = e->row;
            curcol = e->col;
            unselect_ranges();
            update();
            break;

        case 'H':
            currow = vert_top()->row;
            unselect_ranges();
            update();
            break;

        case 'M':
            currow = vert_middle()->row;
            unselect_ranges();
            update();
            break;

        case 'L':
            currow = vert_bottom()->row;
            unselect_ranges();
            update();
            break;

        case 'G': // goto end
            e = go_end();  
            currow = e->row;
            curcol = e->col;
            unselect_ranges();
            update();
            break;

        // GOTO goto
        case ctl('a'):
            e = go_home(); 
            curcol = e->col;
            currow = e->row;
            unselect_ranges();
            update();
            break;

        case 'g':
            if (buf->pnext->value == '0') {                               // g0
                curcol = go_bol()->col;

            } else if (buf->pnext->value == '$') {                        // g$
                curcol = go_eol()->col;

            } else if (buf->pnext->value == 'g') {                        // gg
                e = go_home(); 
                curcol = e->col;
                currow = e->row;

            } else if (buf->pnext->value == 'G') {                        // gG
                e = go_end();  
                currow = e->row;
                curcol = e->col;

            } else if (buf->pnext->value == 'M') {                        // gM
                curcol = horiz_middle()->col;

            } else {                                                      // gA4 (goto cell)
                (void) sprintf(interp_line, "goto %s", parse_cell_name(1, buf));
                send_to_interp(interp_line);
            }
            unselect_ranges();
            update();
            break;

        // repeat last command
        case '.':
            copybuffer(lastcmd_buffer, buf); // nose graba en lastcmd_buffer!!
            cmd_multiplier = 1;
            exec_mult(buf, COMPLETECMDTIMEOUT);
            break;

        // enter command mode
        case ':':
            clr_header(input_win, 0);
            chg_mode(':');
            add(commandline_history, "");
            print_mode(input_win);
            wrefresh(input_win);
            handle_cursor();
            inputline_pos = 0;
            break;

        // enter visual mode
        case 'v':
            chg_mode('v');
            handle_cursor();
            clr_header(input_win, 0);
            print_mode(input_win);
            wrefresh(input_win);
            start_visualmode();
            break;

        // edit cell (v)
        case 'e':
            clr_header(input_win, 0);
            inputline_pos = 0;
            if (start_edit_mode(buf, 'v')) show_header(input_win);
            break;

        // edit cell (s)
        case 'E':
            clr_header(input_win, 0);
            inputline_pos = 0;
            if (start_edit_mode(buf, 's')) show_header(input_win);
            else {
                info("No string value to edit");
                chg_mode('.');
                show_celldetails(input_win);
                print_mode(input_win);
                wrefresh(input_win);
            }
            break;

        case '=':
        case '\\':
        case '<':
        case '>':
            insert_edit_submode = buf->value;
            chg_mode(insert_edit_submode);
            clr_header(input_win, 0);
            print_mode(input_win);
            wrefresh(input_win);
            inputline_pos = 0;
            break;

        // del current cell or range
        case 'x': 
            del_selected_cells();
            update();
            break;

        // format col
        case 'f': 
            if (bs != 2) return;
            formatcol(buf->pnext->value);
            break;

        // mark cell or range
        case 'm': 
            if (bs != 2) break;                
            int p = is_range_selected();
            if (p != -1) { // mark range
                struct srange * sr = get_range_by_pos(p); 
                set_range_mark(buf->pnext->value, sr);                   
            } else         // mark cell 
                set_cell_mark(buf->pnext->value, currow, curcol);
            break;

        case 'c': 
            {
            if (bs != 2) break;
            struct ent * p = *ATBL(tbl, get_mark(buf->pnext->value)->row, get_mark(buf->pnext->value)->col);
            int c1;
            struct ent * n;
            cmd_multiplier++;

            for (c1 = curcol; cmd_multiplier-- && c1 < maxcols; c1++) {
                if ((n = * ATBL(tbl, currow, c1))) {
                    if (n->flags & is_locked)
                        continue;
                    if (!p) {
                        clearent(n); //fixme?
                        continue;
                    }
                } else {
                    if (!p) break;
                    n = lookat(currow, c1);
                }
                copyent(n, p, currow - get_mark(buf->pnext->value)->row, c1 - get_mark(buf->pnext->value)->col, 0, 0, maxrow, maxcol, 0);

                n->row += currow - get_mark(buf->pnext->value)->row;
                n->col += c1 - get_mark(buf->pnext->value)->col;

                n->flags |= is_changed;
            }
            update();
            break;
            }

        // create range with two marks
        case 'r':  
            if (bs == 3) {
                create_range(buf->pnext->value, buf->pnext->pnext->value);
                update();
            }
            break;

        // Zr Zc - Zap col or row
        case 'Z': 
             // limpio el efecto multiplicador del buffer.
             if (cmd_multiplier > 0) {
                 int i, cm = cmd_multiplier + 1;
                 for (i = bs / cm; i < bs; i++) {
                     del_buf(buf, bs / cm);
                 }
                 bs = get_bufsize(buf);
             }
             if (buf->pnext->value == 'r') {
                hide_row(currow, cmd_multiplier + 1);
             } else if (buf->pnext->value == 'c') {
                hide_col(curcol, cmd_multiplier + 1);
             }
             send_to_interp(interp_line);
             cmd_multiplier = 0;
             update();
             break;

        // shift range or cell
        case 's':
            {
            struct srange * sr;
            struct srange * srn = NULL;
            int p = is_range_selected();
            if (p != -1) sr = get_range_by_pos(p);
            else {
                srn = create_custom_range(currow, curcol, currow, curcol);
                sr = srn;
            }
            create_undo_action();
            int ic = cmd_multiplier + 1;
            switch (buf->pnext->value) {
                case 'j':
                    fix_marks(  sr->brrow - sr->tlrow + 1  , 0, sr->tlrow, maxrow, sr->tlcol, sr->brcol);
                    save_undo_range_shift(sr->brrow - sr->tlrow + 1, 0   , sr->tlrow, sr->tlcol, sr->brrow, sr->brcol);
                    shift_range(sr->brrow - sr->tlrow + 1, 0             , sr->tlrow, sr->tlcol, sr->brrow, sr->brcol);
                    break;
                case 'k':
                    fix_marks( -(sr->brrow - sr->tlrow + 1), 0, sr->tlrow, maxrow, sr->tlcol, sr->brcol);
                    yank_area(sr->tlrow, sr->tlcol, sr->brrow, sr->brcol, 'a', ic); // keep ents in yanklist for sk
                    copy_to_undostruct(sr->tlrow, sr->tlcol, sr->brrow, sr->brcol, 'd');
                    save_undo_range_shift(-(sr->brrow - sr->tlrow + 1), 0, sr->tlrow, sr->tlcol, sr->brrow, sr->brcol);
                    shift_range(-(sr->brrow - sr->tlrow + 1), 0          , sr->tlrow, sr->tlcol, sr->brrow, sr->brcol);
                    //copy_to_undostruct(sr->tlrow, sr->tlcol, sr->brrow, sr->brcol, 'a');
                    break;
                case 'h':
                    fix_marks(0, -(sr->brcol - sr->tlcol + 1), sr->tlrow, sr->brrow, sr->tlcol, maxcol);
                    yank_area(sr->tlrow, sr->tlcol, sr->brrow, sr->brcol, 'a', ic); // keep ents in yanklist for sh
                    copy_to_undostruct(sr->tlrow, sr->tlcol, sr->brrow   , sr->brcol, 'd');
                    save_undo_range_shift(0, -(sr->brcol - sr->tlcol + 1), sr->tlrow, sr->tlcol, sr->brrow, sr->brcol);
                    shift_range(0, - (sr->brcol - sr->tlcol + 1)         , sr->tlrow, sr->tlcol, sr->brrow, sr->brcol);
                    //copy_to_undostruct(sr->tlrow, sr->tlcol, sr->brrow   , sr->brcol, 'a');
                    break;
                case 'l':
                    fix_marks(0, sr->brcol - sr->tlcol + 1, sr->tlrow, sr->brrow, sr->tlcol, maxcol);
                    save_undo_range_shift(0, sr->brcol - sr->tlcol + 1   , sr->tlrow, sr->tlcol, sr->brrow, sr->brcol);
                    shift_range(0, sr->brcol - sr->tlcol + 1             , sr->tlrow, sr->tlcol, sr->brrow, sr->brcol);
                    break;
            }
            if (cmd_multiplier > 0) cmd_multiplier = 0;
            end_undo_action();
            if (srn != NULL) free_custom_range(srn);
            unselect_ranges();
            update();
            break;
            }

        // delete row or column, or selected cell or range
        case 'd':
            {
            if (bs != 2) return;
            int ic = cmd_multiplier + 1;

            if (buf->pnext->value == 'r') {
                create_undo_action();
                copy_to_undostruct(currow, 0, currow - 1 + ic, maxcol, 'd');
                save_undo_range_shift(-ic, 0, currow, 0, currow -1 + ic, maxcol);
                fix_marks(-ic, 0, currow - 1 + ic, maxrow, 0, maxcol);
                yank_area(currow, 0, currow + cmd_multiplier, maxcol, 'r', ic);
                while (ic--) deleterow();
                copy_to_undostruct(currow, 0, currow + cmd_multiplier, maxcol, 'a');
                if (cmd_multiplier > 0) cmd_multiplier = 0;
                end_undo_action();

            } else if (buf->pnext->value == 'c') {
                create_undo_action();
                copy_to_undostruct(0, curcol, maxrow, curcol - 1 + ic, 'd');
                save_undo_range_shift(0, -ic, 0, curcol, maxrow, curcol - 1 + ic);
                fix_marks(0, -ic, 0, maxrow,  curcol - 1 + ic, maxcol);
                yank_area(0, curcol, maxrow, curcol + cmd_multiplier, 'c', ic);
                while (ic--) deletecol();
                copy_to_undostruct(0, curcol, maxrow, curcol + cmd_multiplier, 'a');
                if (cmd_multiplier > 0) cmd_multiplier = 0;
                end_undo_action();

            } else if (buf->pnext->value == 'd') {
                del_selected_cells(); 
            }
            update();
            break;
            }

        // insert row or column
        case 'i':
            {
            if (bs != 2) return;
            create_undo_action();
            if (buf->pnext->value == 'r') {
                save_undo_range_shift(1, 0, currow, 0, currow, maxcol);
                fix_marks(1, 0, currow, maxrow, 0, maxcol);
                insert_row(0);

            } else if (buf->pnext->value == 'c') {
                save_undo_range_shift(0, 1, 0, curcol, maxrow, curcol);
                fix_marks(0, 1, 0, maxrow, curcol, maxcol);
                insert_col(0);
            }
            end_undo_action();
            update();
            break;
            }
 
        case 'y':
            // yank row
            if ( bs == 2 && buf->pnext->value == 'r') {        
                yank_area(currow, 0, currow + cmd_multiplier, maxcol, 'r', cmd_multiplier + 1);
                if (cmd_multiplier > 0) cmd_multiplier = 0;

            // yank col
            } else if ( bs == 2 && buf->pnext->value == 'c') {
                yank_area(0, curcol, maxrow, curcol + cmd_multiplier, 'c', cmd_multiplier + 1);
                if (cmd_multiplier > 0) cmd_multiplier = 0;

            // yank cell
            } else if ( bs == 2 && buf->pnext->value == 'y' && is_range_selected() == -1) {
                yank_area(currow, curcol, currow, curcol, 'e', cmd_multiplier + 1);

            // yank range
            } else if ( bs == 1 && is_range_selected() != -1) {
                srange * r = get_selected_range();
                yank_area(r->tlrow, r->tlcol, r->brrow, r->brcol, 'a', cmd_multiplier + 1);
            }
            break;

        // paste cell below or left
        case 'p':
            paste_yanked_ents(0);
            update();
            break;

        // paste cell above or right
        case 'P':
            paste_yanked_ents(1);
            update();
            break;

        // scroll
        case 'z':
            {
            if ( bs == 2 && buf->pnext->value == 'l') {
                scroll_right(1);
                //unselect_ranges();

            } else if ( bs == 2 && buf->pnext->value == 'h') {
                scroll_left(1);
                //unselect_ranges();

            } else if ( bs == 2 && buf->pnext->value == 'H') {
                int z = calc_offscr_sc_cols();
                if (atoi(get_conf_value("half_page_scroll"))) z /= 2;
                scroll_left(z);
                //unselect_ranges();

            } else if ( bs == 2 && buf->pnext->value == 'L') {
                int z = calc_offscr_sc_cols();
                if (atoi(get_conf_value("half_page_scroll"))) z /= 2;
                scroll_right(z);
                //unselect_ranges();
           
            } else if ( bs == 2 && buf->pnext->value == 'z') {            
                int scroll = currow - offscr_sc_rows + LINES - RESROW - 2 - (LINES - RESROW - 2)/2;

                if (scroll > 0) {
                    scroll_down(scroll);
                } else if (scroll > offscr_sc_rows) {
                    scroll_up(-scroll);
                } else if (offscr_sc_rows > 0) {
                    scroll_up(offscr_sc_rows);
                }

            } else if ( bs == 2 && buf->pnext->value == 'm') {                 
                 int ancho = rescol;
                 offscr_sc_cols = 0;
                 int i = 0, c = 0;
 
                 for (i = 0; i < curcol; i++) {
                     for (c = i; c < curcol; c++) {
                         ancho += fwidth[c];
                         if (ancho >= (COLS - rescol)/ 2) {
                             ancho = rescol;
                             break;
                         } 
                     }
                     if (c == curcol) break;
                 }
                 offscr_sc_cols = i;
            }
            update();
            break;
            }
 
        // scroll up a line
        case ctl('y'):
            scroll_up(1);
            update();
            break;

        // scroll down a line
        case ctl('e'):
            scroll_down(1);
            update();
            break;

        // undo
        case 'u':
            do_undo();
            // sync_refs();
            update();
            break;

        // redo
        case ctl('r'):
            do_redo();
            // sync_refs();
            update();
            break;

        // left align
        case '{':
            create_undo_action();
            copy_to_undostruct(currow, curcol, currow, curcol, 'd');
            sprintf(interp_line, "leftjustify %s", v_name(currow, curcol));
            send_to_interp(interp_line);
            copy_to_undostruct(currow, curcol, currow, curcol, 'a');
            end_undo_action();
            update();
            break;

        // right align
        case '}':
            create_undo_action();
            copy_to_undostruct(currow, curcol, currow, curcol, 'd');
            sprintf(interp_line, "rightjustify %s", v_name(currow, curcol));
            send_to_interp(interp_line);
            copy_to_undostruct(currow, curcol, currow, curcol, 'a');
            end_undo_action();
            update();
            break;

        // center align
        case '|':
            create_undo_action();
            copy_to_undostruct(currow, curcol, currow, curcol, 'd');
            sprintf(interp_line, "center %s", v_name(currow, curcol));
            send_to_interp(interp_line);
            copy_to_undostruct(currow, curcol, currow, curcol, 'a');
            end_undo_action();
            update();
            break;

        case ctl('l'):
            endwin();
            start_screen();
            clearok(stdscr, TRUE);
            update();
            flushinp();
            show_header(input_win);
            show_celldetails(input_win);
            wrefresh(input_win);
            update();
            break;

        case '@':
            EvalAll();
            update();
            break;

        default:
            if (isdigit(buf->value) && atoi(get_conf_value("numeric")) ) { // input of numbers
                insert_edit_submode='=';
                chg_mode(insert_edit_submode);
                inputline_pos = 0;
                ins_in_line(buf->value);
                show_header(input_win);
            }
    }
    return;
}
