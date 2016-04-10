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
#include "shift.h"
#include "yank.h"
#include "history.h"
#include "interp.h"
#ifdef UNDO
#include "undo.h"
#endif

extern int offscr_sc_rows, offscr_sc_cols;
extern unsigned int curmode;
extern int cmd_multiplier;
extern struct history * commandline_history;

char visual_submode = '0';

srange * r;                       // SELECTED RANGE!
int moving = FALSE;

void start_visualmode(int tlrow, int tlcol, int brrow, int brcol) {
    unselect_ranges();

    struct srange * sr = get_range_by_marks('\t', '\t'); // visual mode selected range
    if (sr != NULL) del_ranges_by_mark('\t');

    r = (srange *) malloc (sizeof(srange));
    r->tlrow = tlrow;
    r->tlcol = tlcol;
    r->brrow = brrow;
    r->brcol = brcol;
    r->orig_row = currow;         // original row before starting selection
    r->orig_col = curcol;         // original col before starting selection
    r->startup_row = currow;      // original row position before entering visual mode
    r->startup_col = curcol;      // original col position before entering visual mode
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
        update(TRUE);
    } else {                      // Started visual mode with 'C-v' command
        update(FALSE);
        moving = TRUE;
    }
    return;
}

void exit_visualmode() {
    moving = FALSE;
    visual_submode = '0';
    r->selected = 0;
    currow = r->startup_row;
    curcol = r->startup_col;
    del_ranges_by_mark('\t');
    return;
}

void do_visualmode(struct block * buf) {
    if (moving == TRUE) {
        switch (buf->value) {
            case L'j':
            case OKEY_DOWN:
                currow = forw_row(1)->row;
                break;

            case L'k':
            case OKEY_UP:
                currow = back_row(1)->row;
                break;

            case L'h':
            case OKEY_LEFT:
                curcol = back_col(1)->col;
                break;

            case L'l':
            case OKEY_RIGHT:
                curcol = forw_col(1)->col;
                break;

            case ctl('o'):
                moving = FALSE;
                r->orig_row = currow;
                r->orig_col = curcol;
                break;

            case OKEY_ENTER:
                scinfo("Press <C-o> to begin selection or <Esc> key to exit VISUAL MODE");
                return;

        }
        r->tlrow = currow;
        r->tlcol = curcol;
        r->brrow = currow;
        r->brcol = curcol;

        update(FALSE);
        return;
    }

    // ENTER - ctl(k) - Confirm selection
    if (buf->value == OKEY_ENTER || buf->value == ctl('k')) {
        wchar_t cline [BUFFERSIZE];
        swprintf(cline, BUFFERSIZE, L"%s%d", coltoa(r->tlcol), r->tlrow);
        if (r->tlrow != r->brrow || r->tlcol != r->brcol)
            swprintf(cline + wcslen(cline), BUFFERSIZE, L":%s%d", coltoa(r->brcol), r->brrow);
        swprintf(inputline + wcslen(inputline), BUFFERSIZE, L"%s", cline);

        char c = visual_submode;
        exit_visualmode();
        chg_mode(c);

        inputline_pos += wcslen(cline);
        show_header(input_win);
        return;

    // moving to TRUE
    //} else if (buf->value == ctl('m')) {
    //    moving = TRUE;

    // MOVEMENT COMMANDS
    // UP - ctl(b)
    } else if (buf->value == OKEY_UP || buf->value == L'k' || buf->value == ctl('b') ) {
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
    } else if (buf->value == OKEY_DOWN || buf->value == L'j' || buf->value == ctl('f')) {
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
    } else if (buf->value == OKEY_LEFT || buf->value == L'h') {
        if (r->orig_col < r->brcol && r->tlcol < r->brcol) {
            while (col_hidden[-- r->brcol]);
            curcol = r->brcol;
        } else if (r->tlcol <= r->brcol && r->tlcol-1 >= 0) {
            while (col_hidden[-- r->tlcol]);
            curcol = r->tlcol;
        }

    // RIGHT
    } else if (buf->value == OKEY_RIGHT || buf->value == L'l') {
        if (r->orig_col <= r->tlcol && r->tlcol <= r->brcol && r->brcol+2 < maxcols) {
            while (col_hidden[++ r->brcol]);
            curcol = r->brcol;
        } else if (r->tlcol <= r->brcol) {
            while (col_hidden[++ r->tlcol]);
            curcol = r->tlcol;
        }

    // 0
    } else if (buf->value == L'0') {
        r->brcol = r->tlcol;
        r->tlcol = left_limit()->col;
        curcol = r->tlcol;

    // $
    } else if (buf->value == L'$') {
        int s = right_limit()->col;
        r->tlcol = r->brcol;
        r->brcol = r->brcol > s ? r->brcol : s;
        curcol = r->brcol;

    // ^
    } else if (buf->value == L'^') {
        r->brrow = r->tlrow;
        r->tlrow = goto_top()->row;
        currow = r->tlrow;

    // #
    } else if (buf->value == L'#') {
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
    } else if (buf->value == L'G') {
        struct ent * e = go_end();
        r->tlrow = r->orig_row;
        r->tlcol = r->orig_col;
        r->brrow = e->row;
        r->brcol = e->col;
        currow = r->tlrow;
        curcol = r->tlcol;

    // '
    } else if (buf->value == L'\'') {
        // if we receive a mark of a range, just return.
        if (get_mark(buf->pnext->value)->row == -1) return;

        struct ent * e = tick(buf->pnext->value);
        if (row_hidden[e->row]) {
            scerror("Cell row is hidden");
            return;
        } else if (col_hidden[e->col]) {
            scerror("Cell column is hidden");
            return;
        }
        r->tlrow = r->tlrow < e->row ? r->tlrow : e->row;
        r->tlcol = r->tlcol < e->col ? r->tlcol : e->col;
        r->brrow = r->brrow > e->row ? r->brrow : e->row;
        r->brcol = r->brcol > e->col ? r->brcol : e->col;

    // w
    } else if (buf->value == L'w') {
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
    } else if (buf->value == L'b') {
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
    } else if (buf->value == L'H') {
        r->brrow = r->tlrow;
        r->tlrow = vert_top()->row;
        currow = r->tlrow;

    // M
    } else if (buf->value == L'M') {
        r->tlrow = r->orig_row;
        int rm = vert_middle()->row;
        if (r->orig_row < rm) r->brrow = rm;
        else r->tlrow = rm;
        currow = r->tlrow;

    // L
    } else if (buf->value == L'L') {
        r->tlrow = r->orig_row;
        r->brrow = vert_bottom()->row;
        currow = r->brrow;

    // mark a range
    } else if (buf->value == L'm' && get_bufsize(buf) == 2) {
        del_ranges_by_mark(buf->pnext->value);
        srange * rn = create_range('\0', '\0', lookat(r->tlrow, r->tlcol), lookat(r->brrow, r->brcol));
        set_range_mark(buf->pnext->value, rn);
        exit_visualmode();
        curmode = NORMAL_MODE;
        clr_header(input_win, 0);
        show_header(input_win);

    // auto_justify
    } else if (buf->value == ctl('j')) {
        auto_justify(r->tlcol, r->brcol, DEFWIDTH);  // auto justify columns
        exit_visualmode();
        curmode = NORMAL_MODE;
        clr_header(input_win, 0);
        show_header(input_win);

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
                scerror("No locale set. Nothing changed");
            }
            if (any_locked_cells(r->tlrow, r->tlcol, r->brrow, r->brcol)) {
                scerror("Locked cells encountered. Nothing changed");
                return;
            }
            dateformat(lookat(r->tlrow, r->tlcol), lookat(r->brrow, r->brcol), f);
        exit_visualmode();
        curmode = NORMAL_MODE;
        clr_header(input_win, 0);
        show_header(input_win);
        #else
            scinfo("Build made without USELOCALE enabled");
        #endif

    // EDITION COMMANDS
    // yank
    } else if (buf->value == 'y') {
        yank_area(r->tlrow, r->tlcol, r->brrow, r->brcol, 'a', 1);

        exit_visualmode();
        curmode = NORMAL_MODE;
        clr_header(input_win, 0);
        show_header(input_win);

    // left / right / center align
    } else if (buf->value == L'{' || buf->value == L'}' || buf->value == L'|') {
        if (any_locked_cells(r->tlrow, r->tlcol, r->brrow, r->brcol)) {
            scerror("Locked cells encountered. Nothing changed");
            return;
        }
        wchar_t interp_line[BUFFERSIZE];
        if (buf->value == L'{')      swprintf(interp_line, BUFFERSIZE, L"leftjustify %s", v_name(r->tlrow, r->tlcol));
        else if (buf->value == L'}') swprintf(interp_line, BUFFERSIZE, L"rightjustify %s", v_name(r->tlrow, r->tlcol));
        else if (buf->value == L'|') swprintf(interp_line, BUFFERSIZE, L"center %s", v_name(r->tlrow, r->tlcol));
        swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L":%s", v_name(r->brrow, r->brcol));
#ifdef UNDO
        create_undo_action();
        copy_to_undostruct(r->tlrow, r->tlcol, r->brrow, r->brcol, 'd');
#endif
        send_to_interp(interp_line);
#ifdef UNDO
        copy_to_undostruct(r->tlrow, r->tlcol, r->brrow, r->brcol, 'a');
        end_undo_action();
#endif
        cmd_multiplier = 0;

        exit_visualmode();
        curmode = NORMAL_MODE;
        clr_header(input_win, 0);
        show_header(input_win);

    // range lock / unlock // valueize
    } else if ( buf->value == L'r' && (buf->pnext->value == L'l' || buf->pnext->value == L'u' ||
            buf->pnext->value == L'v' )) {
        if (buf->pnext->value == L'l') {
            lock_cells(lookat(r->tlrow, r->tlcol), lookat(r->brrow, r->brcol));
        } else if (buf->pnext->value == L'u') {
            unlock_cells(lookat(r->tlrow, r->tlcol), lookat(r->brrow, r->brcol));
        } else if (buf->pnext->value == L'v') {
            valueize_area(r->tlrow, r->tlcol, r->brrow, r->brcol);
        }
        cmd_multiplier = 0;

        exit_visualmode();
        curmode = NORMAL_MODE;
        clr_header(input_win, 0);
        show_header(input_win);

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
        curmode = NORMAL_MODE;
        clr_header(input_win, 0);
        show_header(input_win);

    // delete selected range
    } else if (buf->value == L'x' || (buf->value == L'd' && buf->pnext->value == L'd') ) {
        del_selected_cells();

        exit_visualmode();
        curmode = NORMAL_MODE;
        clr_header(input_win, 0);
        show_header(input_win);

    // shift range
    } else if (buf->value == L's') {
        int ic = cmd_multiplier + 1;
        if ( any_locked_cells(r->tlrow, r->tlcol, r->brrow, r->brcol) &&
           (buf->pnext->value == L'h' || buf->pnext->value == L'k') ) {
            scerror("Locked cells encountered. Nothing changed");
            return;
        }
#ifdef UNDO
        create_undo_action();
#endif
        switch (buf->pnext->value) {
            case L'j':
                    fix_marks(  (r->brrow - r->tlrow + 1) * cmd_multiplier, 0, r->tlrow, maxrow, r->tlcol, r->brcol);
#ifdef UNDO
                    save_undo_range_shift(cmd_multiplier, 0, r->tlrow, r->tlcol, r->brrow + (r->brrow-r->tlrow+1) * (cmd_multiplier - 1), r->brcol);
#endif
                    while (ic--) shift_range(ic, 0, r->tlrow, r->tlcol, r->brrow, r->brcol);
                    break;
            case L'k':
                    fix_marks( -(r->brrow - r->tlrow + 1) * cmd_multiplier, 0, r->tlrow, maxrow, r->tlcol, r->brcol);
                    yank_area(r->tlrow, r->tlcol, r->brrow + (r->brrow-r->tlrow+1) * (cmd_multiplier - 1), r->brcol, 'a', cmd_multiplier); // keep ents in yanklist for sk
#ifdef UNDO
                    copy_to_undostruct(r->tlrow, r->tlcol, r->brrow + (r->brrow-r->tlrow+1) * (cmd_multiplier - 1), r->brcol, 'd');
                    save_undo_range_shift(-cmd_multiplier, 0, r->tlrow, r->tlcol, r->brrow + (r->brrow-r->tlrow+1) * (cmd_multiplier - 1), r->brcol);
#endif
                    while (ic--) shift_range(-ic, 0, r->tlrow, r->tlcol, r->brrow, r->brcol);
#ifdef UNDO
                    copy_to_undostruct(r->tlrow, r->tlcol, r->brrow + (r->brrow-r->tlrow+1) * (cmd_multiplier - 1), r->brcol, 'a');
#endif
                    break;
            case L'h':
                    fix_marks(0, -(r->brcol - r->tlcol + 1) * cmd_multiplier, r->tlrow, r->brrow, r->tlcol, maxcol);
                    yank_area(r->tlrow, r->tlcol, r->brrow, r->brcol + (r->brcol-r->tlcol+1) * (cmd_multiplier - 1), 'a', cmd_multiplier); // keep ents in yanklist for sk
#ifdef UNDO
                    copy_to_undostruct(r->tlrow, r->tlcol, r->brrow, r->brcol + (r->brcol-r->tlcol+1) * (cmd_multiplier - 1), 'd');
                    save_undo_range_shift(0, -cmd_multiplier, r->tlrow, r->tlcol, r->brrow, r->brcol + (r->brcol-r->tlcol+1) * (cmd_multiplier - 1));
#endif
                    while (ic--) shift_range(0, -ic, r->tlrow, r->tlcol, r->brrow, r->brcol);
#ifdef UNDO
                    copy_to_undostruct(r->tlrow, r->tlcol, r->brrow, r->brcol + (r->brcol-r->tlcol+1) * (cmd_multiplier - 1), 'a');
#endif
                    break;
            case L'l':
                    fix_marks(0,  (r->brcol - r->tlcol + 1) * cmd_multiplier, r->tlrow, r->brrow, r->tlcol, maxcol);
#ifdef UNDO
                    save_undo_range_shift(0, cmd_multiplier, r->tlrow, r->tlcol, r->brrow, r->brcol + (r->brcol-r->tlcol+1) * (cmd_multiplier - 1));
#endif
                    while (ic--) shift_range(0, ic, r->tlrow, r->tlcol, r->brrow, r->brcol);
                    break;
        }
        cmd_multiplier = 0;
#ifdef UNDO
        end_undo_action();
#endif

        exit_visualmode();
        curmode = NORMAL_MODE;
        clr_header(input_win, 0);
        show_header(input_win);

    } else if (buf->value == L':') {
        clr_header(input_win, 0);
        wrefresh(input_win);
        chg_mode(':');
#ifdef HISTORY_FILE
        add(commandline_history, L"");
#endif
        print_mode(input_win);
        wrefresh(input_win);
        handle_cursor();
        inputline_pos = 0;
        return;
    }

    if (visual_submode == '0')
        update(TRUE);
    else {
        update(FALSE);
    }
}
