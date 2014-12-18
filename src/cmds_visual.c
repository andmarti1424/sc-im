#include <curses.h>
#include <stdlib.h>

#include "screen.h"
#include "buffer.h"
#include "marks.h"
#include "macros.h"
#include "cmds.h"
#include "conf.h"
#include "color.h"       // for set_ucolor
#include "hide_show.h"
#include "undo.h"
#include "shift.h"
#include "yank.h"
#include "history.h"

extern int offscr_sc_rows, offscr_sc_cols;
extern unsigned int curmode;
extern int cmd_multiplier;
extern struct history * commandline_history;

srange * r; // SELECTED RANGE!

void start_visualmode(int tlrow, int tlcol, int brrow, int brcol) {
    unselect_ranges();

    struct srange * sr = get_range_by_marks('\0', '\0');
    if (sr != NULL) del_ranges_by_mark('\0');

    r = (srange *) malloc (sizeof(srange));
    r->tlrow = tlrow;
    r->tlcol = tlcol;
    r->brrow = brrow;
    r->brcol = brcol;
    r->orig_row = currow;
    r->orig_col = curcol;
    r->marks[0] = '\0';
    r->marks[1] = '\0';
    r->selected = 1;
    r->pnext = NULL;
 
    if (ranges == NULL) ranges = r;
    else { 
        r->pnext = ranges;
        ranges = r;
    }

    update();
    return;
}

void exit_visualmode() {
    r->selected = 0;
    currow = r->orig_row;
    curcol = r->orig_col;
    del_ranges_by_mark('\0');
    return;
}

void do_visualmode(struct block * sb) {
    
     // UP - ctl(b)
    if (sb->value == OKEY_UP || sb->value == 'k' || sb->value == ctl('b') ) {
        int n, i;
        if (sb->value == ctl('b')) { 
            n = LINES - RESROW - 1;
            if (get_conf_value("half_page_scroll")) n = n / 2;
        } else n = 1;

        for (i=0; i < n; i++)
            if (r->orig_row < r->brrow && r->tlrow < r->brrow) {
                while (row_hidden[-- r->brrow]);
                currow = r->brrow;
            } else if (r->tlrow <= r->brrow && r->tlrow-1 >= 0) {
                while (row_hidden[-- r->tlrow]);
                currow = r->tlrow;
            }

    // DOWN - ctl('f')
    } else if (sb->value == OKEY_DOWN || sb->value == 'j' || sb->value == ctl('f')) {
        int n, i;
        if (sb->value == ctl('f')) { 
            n = LINES - RESROW - 1;
            if (get_conf_value("half_page_scroll")) n = n / 2;
        } else n = 1;

        for (i=0; i < n; i++)
            if (r->orig_row <= r->tlrow && r->tlrow <= r->brrow && r->brrow+1 < maxrows) {
                while (row_hidden[++ r->brrow]);
                currow = r->brrow;
            } else if (r->tlrow <  r->brrow) {
                while (row_hidden[++ r->tlrow]);
                currow = r->tlrow;
            }

    // LEFT
    } else if (sb->value == OKEY_LEFT || sb->value == 'h') {
        if (r->orig_col < r->brcol && r->tlcol < r->brcol) {
            while (col_hidden[-- r->brcol]);
            curcol = r->brcol;
        } else if (r->tlcol <= r->brcol && r->tlcol-1 >= 0) {
            while (col_hidden[-- r->tlcol]);
            curcol = r->tlcol;
        }

    // RIGHT
    } else if (sb->value == OKEY_RIGHT || sb->value == 'l') {
        if (r->orig_col <= r->tlcol && r->tlcol <= r->brcol && r->brcol+2 < maxcols) {
            while (col_hidden[++ r->brcol]);
            curcol = r->brcol;
        } else if (r->tlcol <= r->brcol) {
            while (col_hidden[++ r->tlcol]);
            curcol = r->tlcol;
        }

    // 0
    } else if (sb->value == '0') {
        r->brcol = r->tlcol;
        r->tlcol = left_limit()->col;
        curcol = r->tlcol;

    // $
    } else if (sb->value == '$') {
        int s = right_limit()->col;
        r->tlcol = r->brcol;
        r->brcol = r->brcol > s ? r->brcol : s;
        curcol = r->brcol;

    // ^
    } else if (sb->value == '^') {
        r->brrow = r->tlrow;
        r->tlrow = goto_top()->row;
        currow = r->tlrow;         

    // #
    } else if (sb->value == '#') {
        int s = goto_bottom()->row;
        r->tlrow = r->brrow;
        r->brrow = r->brrow > s ? r->brrow : s;
        currow = r->brrow;         

    // ctl(a)
    } else if (sb->value == ctl('a')) {
        if (r->tlrow == 0 && r->tlcol == 0) return;
        struct ent * e = go_home();
        r->tlrow = e->row;
        r->tlcol = e->col;
        r->brrow = r->orig_row;
        r->brcol = r->orig_col;
        currow = r->tlrow;         
        curcol = r->tlcol;

    // G
    } else if (sb->value == 'G') {
        struct ent * e = go_end();
        r->tlrow = r->orig_row;
        r->tlcol = r->orig_col;
        r->brrow = e->row;
        r->brcol = e->col;
        currow = r->tlrow;         
        curcol = r->tlcol;

    // '
    } else if (sb->value == '\'') {
        // if we receive a mark of a range, just return.
        if (get_mark(sb->pnext->value)->row == -1) return;

        struct ent * e = tick(sb->pnext->value);
        if (row_hidden[e->row]) {
            error("Cell row is hidden");
            return;
        } else if (col_hidden[e->col]) {
            error("Cell column is hidden");
            return;
        }
        r->tlrow = r->tlrow < e->row ? r->tlrow : e->row;
        r->tlcol = r->tlcol < e->col ? r->tlcol : e->col;
        r->brrow = r->brrow > e->row ? r->brrow : e->row;
        r->brcol = r->brcol > e->col ? r->brcol : e->col;

    // w
    } else if (sb->value == 'w') {
        struct ent * e = go_forward();  
        if (e->col > r->orig_col) {
            r->brcol = e->col;
            r->tlcol = r->orig_col;
        } else {
            r->tlcol = e->col;
            r->brcol = r->orig_col;
        }
        r->brrow = e->row;
        r->tlrow = r->orig_row;
        curcol = e->col;
        currow = e->row;

    // b
    } else if (sb->value == 'b') {
        struct ent * e = go_backward();  
        if (e->col <= r->orig_col) {
            r->tlcol = e->col;
            r->brcol = r->orig_col;
        } else {
            r->brcol = e->col;
            r->tlcol = r->orig_col;
        }
        r->tlrow = e->row;
        r->brrow = r->orig_row;
        curcol = e->col;
        currow = e->row;

    // H
    } else if (sb->value == 'H') {
        r->brrow = r->tlrow;
        r->tlrow = vert_top()->row;
        currow = r->tlrow;

    // M
    } else if (sb->value == 'M') {
        r->tlrow = r->orig_row;
        int rm = vert_middle()->row;
        if (r->orig_row < rm) r->brrow = rm;
        else r->tlrow = rm;
        currow = r->tlrow;

    // L
    } else if (sb->value == 'L') {
        r->tlrow = r->orig_row;
        r->brrow = vert_bottom()->row;
        currow = r->brrow;         

    // yank
    } else if (sb->value == 'y') {
        yank_area(r->tlrow, r->tlcol, r->brrow, r->brcol, 'a', 1);
        exit_visualmode();
        curmode = NORMAL_MODE;
        clr_header(input_win, 0);
        show_header(input_win);

    // left / right / center align
    } else if (sb->value == '{' || sb->value == '}' || sb->value == '|') {
        char interp_line[100];
        int p, r = currow, c = curcol, rf = currow, cf = curcol;
        struct srange * sr;
        if ( (p = is_range_selected()) != -1) {
            sr = get_range_by_pos(p);
            r = sr->tlrow;
            c = sr->tlcol;
            rf = sr->brrow;
            cf = sr->brcol;
        }
        create_undo_action();
        if (sb->value == '{')      sprintf(interp_line, "leftjustify %s", v_name(r, c));
        else if (sb->value == '}') sprintf(interp_line, "rightjustify %s", v_name(r, c));
        else if (sb->value == '|') sprintf(interp_line, "center %s", v_name(r, c));
        if (p != -1) sprintf(interp_line, "%s:%s", interp_line, v_name(rf, cf));
        copy_to_undostruct(r, c, rf, cf, 'd');
        send_to_interp(interp_line);
        copy_to_undostruct(r, c, rf, cf, 'a');
        end_undo_action();            
        cmd_multiplier = 0;

    // Zr Zc - Zap col or row
    } else if ( (sb->value == 'Z' || sb->value == 'S') && (sb->pnext->value == 'c' || sb->pnext->value == 'r')) {
        int rs, r = currow, c = curcol, arg = cmd_multiplier + 1;
        struct srange * sr;
        if ( (rs = is_range_selected()) != -1) {
            sr = get_range_by_pos(rs);
            cmd_multiplier = 1;
            r = sr->tlrow;
            c = sr->tlcol;
            arg = sb->pnext->value == 'r' ? sr->brrow - sr->tlrow + 1 : sr->brcol - sr->tlcol + 1;
        }
        if (sb->value == 'Z' && sb->pnext->value == 'r') {
            hide_row(r, arg);
        } else if (sb->value == 'Z' && sb->pnext->value == 'c') {
            hide_col(c, arg);
        } else if (sb->value == 'S' && sb->pnext->value == 'r') {
            show_row(r, arg);
        } else if (sb->value == 'S' && sb->pnext->value == 'c') {
            show_col(c, arg);
        }
        cmd_multiplier = 0;

    // delete selected range
    } else if (sb->value == 'x' || (sb->value == 'd' && sb->pnext->value == 'd') ) {
        del_selected_cells();
        exit_visualmode();
        curmode = NORMAL_MODE;
        clr_header(input_win, 0);
        show_header(input_win);

    // shift range
    } else if (sb->value == 's') {
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
        switch (sb->pnext->value) {
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
                copy_to_undostruct(sr->tlrow, sr->tlcol, sr->brrow, sr->brcol, 'a');
                break;
            case 'h':
                fix_marks(0, -(sr->brcol - sr->tlcol + 1), sr->tlrow, sr->brrow, sr->tlcol, maxcol);
                yank_area(sr->tlrow, sr->tlcol, sr->brrow, sr->brcol, 'a', ic); // keep ents in yanklist for sh
                copy_to_undostruct(sr->tlrow, sr->tlcol, sr->brrow   , sr->brcol, 'd');
                save_undo_range_shift(0, -(sr->brcol - sr->tlcol + 1), sr->tlrow, sr->tlcol, sr->brrow, sr->brcol);
                shift_range(0, - (sr->brcol - sr->tlcol + 1)         , sr->tlrow, sr->tlcol, sr->brrow, sr->brcol);
                copy_to_undostruct(sr->tlrow, sr->tlcol, sr->brrow   , sr->brcol, 'a');
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
        exit_visualmode();
        curmode = NORMAL_MODE;
        clr_header(input_win, 0);
        show_header(input_win);

    } else if (sb->value == ':') {
        clr_header(input_win, 0);
        wrefresh(input_win);
        chg_mode(':');
        add(commandline_history, "");
        print_mode(input_win);
        wrefresh(input_win);
        handle_cursor();
        inputline_pos = 0;
        return;
    }
    update();
}
