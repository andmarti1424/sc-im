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
#include "interp.h"

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

void do_visualmode(struct block * buf) {
    
    // MOVEMENT COMMANDS
    // UP - ctl(b)
    if (buf->value == OKEY_UP || buf->value == 'k' || buf->value == ctl('b') ) {
        int n, i;
        if (buf->value == ctl('b')) { 
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
    } else if (buf->value == OKEY_DOWN || buf->value == 'j' || buf->value == ctl('f')) {
        int n, i;
        if (buf->value == ctl('f')) { 
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
    } else if (buf->value == OKEY_LEFT || buf->value == 'h') {
        if (r->orig_col < r->brcol && r->tlcol < r->brcol) {
            while (col_hidden[-- r->brcol]);
            curcol = r->brcol;
        } else if (r->tlcol <= r->brcol && r->tlcol-1 >= 0) {
            while (col_hidden[-- r->tlcol]);
            curcol = r->tlcol;
        }

    // RIGHT
    } else if (buf->value == OKEY_RIGHT || buf->value == 'l') {
        if (r->orig_col <= r->tlcol && r->tlcol <= r->brcol && r->brcol+2 < maxcols) {
            while (col_hidden[++ r->brcol]);
            curcol = r->brcol;
        } else if (r->tlcol <= r->brcol) {
            while (col_hidden[++ r->tlcol]);
            curcol = r->tlcol;
        }

    // 0
    } else if (buf->value == '0') {
        r->brcol = r->tlcol;
        r->tlcol = left_limit()->col;
        curcol = r->tlcol;

    // $
    } else if (buf->value == '$') {
        int s = right_limit()->col;
        r->tlcol = r->brcol;
        r->brcol = r->brcol > s ? r->brcol : s;
        curcol = r->brcol;

    // ^
    } else if (buf->value == '^') {
        r->brrow = r->tlrow;
        r->tlrow = goto_top()->row;
        currow = r->tlrow;         

    // #
    } else if (buf->value == '#') {
        int s = goto_bottom()->row;
        r->tlrow = r->brrow;
        r->brrow = r->brrow > s ? r->brrow : s;
        currow = r->brrow;         

    // ctl(a)
    } else if (buf->value == ctl('a')) {
        if (r->tlrow == 0 && r->tlcol == 0) return;
        struct ent * e = go_home();
        r->tlrow = e->row;
        r->tlcol = e->col;
        r->brrow = r->orig_row;
        r->brcol = r->orig_col;
        currow = r->tlrow;         
        curcol = r->tlcol;

    // G
    } else if (buf->value == 'G') {
        struct ent * e = go_end();
        r->tlrow = r->orig_row;
        r->tlcol = r->orig_col;
        r->brrow = e->row;
        r->brcol = e->col;
        currow = r->tlrow;         
        curcol = r->tlcol;

    // '
    } else if (buf->value == '\'') {
        // if we receive a mark of a range, just return.
        if (get_mark(buf->pnext->value)->row == -1) return;

        struct ent * e = tick(buf->pnext->value);
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
    } else if (buf->value == 'w') {
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
    } else if (buf->value == 'b') {
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
    } else if (buf->value == 'H') {
        r->brrow = r->tlrow;
        r->tlrow = vert_top()->row;
        currow = r->tlrow;

    // M
    } else if (buf->value == 'M') {
        r->tlrow = r->orig_row;
        int rm = vert_middle()->row;
        if (r->orig_row < rm) r->brrow = rm;
        else r->tlrow = rm;
        currow = r->tlrow;

    // L
    } else if (buf->value == 'L') {
        r->tlrow = r->orig_row;
        r->brrow = vert_bottom()->row;
        currow = r->brrow;         

    // EDITION COMMANDS
    // yank
    } else if (buf->value == 'y') {
        yank_area(r->tlrow, r->tlcol, r->brrow, r->brcol, 'a', 1);

        exit_visualmode();
        curmode = NORMAL_MODE;
        clr_header(input_win, 0);
        show_header(input_win);

    // left / right / center align
    } else if (buf->value == '{' || buf->value == '}' || buf->value == '|') {
        if (any_locked_cells(r->tlrow, r->tlcol, r->brrow, r->brcol)) {
            error("Locked cells encountered. Nothing changed");           
            return;
        }
        char interp_line[100];
        if (buf->value == '{')      sprintf(interp_line, "leftjustify %s", v_name(r->tlrow, r->tlcol));
        else if (buf->value == '}') sprintf(interp_line, "rightjustify %s", v_name(r->tlrow, r->tlcol));
        else if (buf->value == '|') sprintf(interp_line, "center %s", v_name(r->tlrow, r->tlcol));
        sprintf(interp_line, "%s:%s", interp_line, v_name(r->brrow, r->brcol));
        create_undo_action();
        copy_to_undostruct(r->tlrow, r->tlcol, r->brrow, r->brcol, 'd');
        send_to_interp(interp_line);
        copy_to_undostruct(r->tlrow, r->tlcol, r->brrow, r->brcol, 'a');
        end_undo_action();            
        cmd_multiplier = 0;

        exit_visualmode();
        curmode = NORMAL_MODE;
        clr_header(input_win, 0);
        show_header(input_win);

    // range lock / unlock
    } else if ( buf->value == 'r' && (buf->pnext->value == 'l' || buf->pnext->value == 'u')) {
        if (buf->pnext->value == 'l') {
            lock_cells(lookat(r->tlrow, r->tlcol), lookat(r->brrow, r->brcol));
        } else if (buf->pnext->value == 'u') {
            unlock_cells(lookat(r->tlrow, r->tlcol), lookat(r->brrow, r->brcol));
        }
        cmd_multiplier = 0;

        exit_visualmode();
        curmode = NORMAL_MODE;
        clr_header(input_win, 0);
        show_header(input_win);

    // Zr Zc - Zap col or row
    } else if ( (buf->value == 'Z' || buf->value == 'S') && (buf->pnext->value == 'c' || buf->pnext->value == 'r')) {
        int arg = buf->pnext->value == 'r' ? r->brrow - r->tlrow + 1 : r->brcol - r->tlcol + 1;
        if (buf->value == 'Z' && buf->pnext->value == 'r') {
            hide_row(r->tlrow, arg);
        } else if (buf->value == 'Z' && buf->pnext->value == 'c') {
            hide_col(r->tlcol, arg);
        } else if (buf->value == 'S' && buf->pnext->value == 'r') {
            show_row(r->tlrow, arg);
        } else if (buf->value == 'S' && buf->pnext->value == 'c') {
            show_col(r->tlcol, arg);
        }
        cmd_multiplier = 0;

        exit_visualmode();
        curmode = NORMAL_MODE;
        clr_header(input_win, 0);
        show_header(input_win);

    // delete selected range
    } else if (buf->value == 'x' || (buf->value == 'd' && buf->pnext->value == 'd') ) {
        del_selected_cells();

        exit_visualmode();
        curmode = NORMAL_MODE;
        clr_header(input_win, 0);
        show_header(input_win);

    // shift range
    } else if (buf->value == 's') {
        int ic = cmd_multiplier + 1;
        if ( any_locked_cells(r->tlrow, r->tlcol, r->brrow, r->brcol) &&
           (buf->pnext->value == 'h' || buf->pnext->value == 'k') ) {
            error("Locked cells encountered. Nothing changed");           
            return;
        }
        create_undo_action();
        switch (buf->pnext->value) {
            case 'j':
                fix_marks(  r->brrow - r->tlrow + 1  , 0           , r->tlrow, maxrow,   r->tlcol, r->brcol);
                save_undo_range_shift(r->brrow - r->tlrow + 1, 0   , r->tlrow, r->tlcol, r->brrow, r->brcol);
                shift_range(r->brrow - r->tlrow + 1, 0             , r->tlrow, r->tlcol, r->brrow, r->brcol);
                break;
            case 'k':
                fix_marks( -(r->brrow - r->tlrow + 1), 0           , r->tlrow, maxrow, r->tlcol, r->brcol);
                yank_area(r->tlrow, r->tlcol, r->brrow, r->brcol, 'a', ic); // keep ents in yanklist for sk
                copy_to_undostruct(r->tlrow, r->tlcol, r->brrow    , r->brcol, 'd');
                save_undo_range_shift(-(r->brrow - r->tlrow + 1), 0, r->tlrow, r->tlcol, r->brrow, r->brcol);
                shift_range(-(r->brrow - r->tlrow + 1), 0          , r->tlrow, r->tlcol, r->brrow, r->brcol);
                copy_to_undostruct(r->tlrow, r->tlcol, r->brrow    , r->brcol, 'a');
                break;
            case 'h':
                fix_marks(0, -(r->brcol - r->tlcol + 1), r->tlrow  , r->brrow, r->tlcol, maxcol);
                yank_area(r->tlrow, r->tlcol, r->brrow, r->brcol, 'a', ic); // keep ents in yanklist for sh
                copy_to_undostruct(r->tlrow, r->tlcol, r->brrow    , r->brcol, 'd');
                save_undo_range_shift(0, -(r->brcol - r->tlcol + 1), r->tlrow, r->tlcol, r->brrow, r->brcol);
                shift_range(0, - (r->brcol - r->tlcol + 1)         , r->tlrow, r->tlcol, r->brrow, r->brcol);
                copy_to_undostruct(r->tlrow, r->tlcol, r->brrow    , r->brcol, 'a');
                break;
            case 'l':
                fix_marks(0, r->brcol - r->tlcol + 1               , r->tlrow, r->brrow, r->tlcol, maxcol);
                save_undo_range_shift(0, r->brcol - r->tlcol + 1   , r->tlrow, r->tlcol, r->brrow, r->brcol);
                shift_range(0, r->brcol - r->tlcol + 1             , r->tlrow, r->tlcol, r->brrow, r->brcol);
                break;
        }
        cmd_multiplier = 0;
        end_undo_action();

        exit_visualmode();
        curmode = NORMAL_MODE;
        clr_header(input_win, 0);
        show_header(input_win);

    } else if (buf->value == ':') {
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
