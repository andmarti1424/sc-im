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
 * \file tui.c
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 2017-07-18
 * \brief  This is the ncurses implementation of sc-im user interface,
 *
 * \details This is the ncurses implementation of sc-im user interface,
 * or called tui. It mainly consists on the following two windows:
 * main_win: window that shows the grid
 * input_win: status bar window (or called header) and stdin input
 *
 * these are the functions called outside tui.c:
 * ui_query               // function to read text from stdin
 * ui_query_opt           // function that shows a message and waits for confirmation between a couple of defined options
 * yyerror                // error routine for yacc parser
 * ui_bail                // function to print errors of lua scripts
 * ui_winchg              // function that handles SIGWINCH
 * ui_show_celldetails    // function that shows cell details in header bar
 * ui_start_colors        // exclusive ui startup routine for colors
 * ui_pause
 * ui_resume
 * ui_mv_bottom_bar
 * ui_refresh_pad
 *
 * these are local functions that might not be needed to reimplement if writing another ui:
 * ui_set_ucolor          // function called internally for setting a color
 * ui_show_content
 * ui_show_sc_col_headings
 * ui_show_sc_row_headings
 * ui_add_cell_detail     // Add details of an ent to a char * received as a parameter. used for input_win
 *
 * ANYONE WHO WANTS TO PORT THIS TO ANOTHER UI, WOULD JUST NEED TO REIMPLEMENT THIS FILE
 * AND HELP() IN HELP.C
 *
 *
 * if not working with ncurses, you should also have to define LINES and COLS macros in Xui.h as well.
 * see ui example inside /examples/gui_example folder
 */

#include <string.h>
#include <ncurses.h>
#include <stdio.h>
#include <time.h>
#include <locale.h>
#include <stdlib.h>
#include <stdarg.h>

#include "main.h"
#include "macros.h"
#include "conf.h"
#include "input.h"
#include "tui.h"
#include "range.h"
#include "interp.h"
#include "sc.h"
#include "cmds/cmds.h"
#include "cmds/cmds_visual.h"
#include "cmds/cmds_command.h"
#include "conf.h"
#include "version.h"
#include "file.h"
#include "format.h"
#include "utils/string.h"
#include "digraphs.h"

extern struct dictionary * d_colors_param;
extern int cmd_pending;
extern int cmd_multiplier;
extern char insert_edit_submode;
extern struct session * session;

WINDOW * main_win;
WINDOW * input_win;
WINDOW * input_pad;
SCREEN * sstderr;
SCREEN * sstdout;

/* for the "show_cursor" option */
int status_line_empty;
int curwinrow, curwincol;

/**
 * \brief Called to start UI
 *
 * \return none
 */

void ui_start_screen() {
    sstderr = newterm(NULL, stderr, stdin);
    noecho();
    sstdout = newterm(NULL, stdout, stdin);
    set_term(sstdout);

    main_win = newwin(LINES - RESROW, COLS, get_conf_int("input_bar_bottom") ? 0 : RESROW, 0);
    input_win = newwin(RESROW-1, COLS, get_conf_int("input_bar_bottom") ? LINES-RESROW : 0, 0);
    input_pad = newpad(RESROW-1, MAX_IB_LEN);

    status_line_empty = 1;

    #ifdef USECOLORS
    if (has_colors()) {
        start_color();

        if (get_d_colors_param() == NULL) {
            start_default_ucolors();

            // in case we decide to change colors
            // Create a dictionary and save equivalences between macros and
            // values defined in '.sc' files
            set_colors_param_dict();
        }
        //wbkgd(main_win, COLOR_PAIR((ucolors[DEFAULT].fg+1) * (COLORS) + ucolors[DEFAULT].bg + 2));
        //wbkgd(input_win, COLOR_PAIR((ucolors[DEFAULT].fg+1) * (COLORS) + ucolors[DEFAULT].bg + 2));
        //wbkgd(input_pad, COLOR_PAIR((ucolors[DEFAULT].fg+1) * (COLORS) + ucolors[DEFAULT].bg + 2));
    }
    #endif

    wtimeout(input_pad, TIMEOUT_CURSES);
    noecho();
    curs_set(0);

    // Mouse support
    #ifdef MOUSE
    mmask_t old;
    mousemask (ALL_MOUSE_EVENTS, &old);
    #endif

    #ifndef NETBSD
    if ((char *) getenv ("ESCDELAY") == NULL) set_escdelay(ESC_DELAY);
    #endif
    cbreak();
    keypad(input_pad, 1);
}

/**
 * \brief Called to stop UI
 *
 * \return none
 */

void ui_stop_screen() {
    #ifdef USECOLORS
        //if (get_d_colors_param() != NULL)
        free_colors_param_dict();
    #endif
    move(0, 0);
    clrtobot();
    refresh();

    set_term(sstdout);
    endwin();
    set_term(sstderr);
    endwin();
    return;
}

/**
 * \brief Asks the user input from stdin (non-blocking)
 *
 * \details This function asks the user for input from stdin. Should be
 * non-blocking and should return -1 when no key was pressed; 0 when
 * key was pressed. It receives wint_t as a parameter. When a valid
 * key is pressed, its value is then updated in that win_t variable.
 *
 * \param[in] wd
 *
 * \return 0 on key press; -1 otherwise
 */

int ui_getch(wint_t * wd) {
    return wget_wch(input_pad, wd);
}

/**
 * \brief Asks the user for input from stdin (blocking)
 *
 * \details This function asks the user for input from stdin. Should be
 * blocking and should return -1 when ESC was pressed; 0 otherwise. It
 * receives * wint_t as a parameter. When a valid key is pressed, its value
 * is then updated in that wint_t variable.
 *
 * \param[in] wd
 *
 * \return -1 on ESC press; 0 otherwise
 */

int ui_getch_b(wint_t * wd) {
    wint_t digraph = 0;
    wtimeout(input_pad, -1);
    move(0, inputline_pos + 1);
    wget_wch(input_pad, wd);
    if (*wd == ctl('k')) {
        wget_wch(input_pad, wd);
        if (*wd != OKEY_ESC) {
            digraph = *wd;
            wget_wch(input_pad, wd);
            if (*wd != OKEY_ESC)
                *wd = get_digraph(digraph, *wd);
        }
    }
    wtimeout(input_pad, TIMEOUT_CURSES);
    if ( *wd != OKEY_ESC ) return 0;
    return -1;
}

/**
 * \brief Used for sc_info, sc_error, and sc_debug macros
 *
 * \param[in] s
 * \param[in] type
 * \return none
 */

void ui_sc_msg(char * s, int type, ...) {
    if (get_conf_int("quiet")) return;
    if (type == DEBUG_MSG && ! get_conf_int("debug")) return;
    char t[BUFFERSIZE];
    va_list args;
    va_start(args, type);
    vsprintf (t, s, args);
    if ( ! get_conf_int("nocurses")) {
#ifdef USECOLORS
        if (type == ERROR_MSG)
            ui_set_ucolor(input_pad, &ucolors[ERROR_MSG], DEFAULT_COLOR);
        else
            ui_set_ucolor(input_pad, &ucolors[INFO_MSG], DEFAULT_COLOR);
#endif
        mvwprintw(input_pad, 0, 0, "%s", t);
        wclrtoeol(input_pad);
        wmove(input_pad, 0, 0);
        status_line_empty = 0;

        ui_refresh_pad(0);
#ifdef USECOLORS
        ui_set_ucolor(input_win, &ucolors[INPUT], DEFAULT_COLOR);
#endif
        if (type == DEBUG_MSG || (session != NULL && session->cur_doc != NULL && session->cur_doc->loading && type == ERROR_MSG)) {
            wtimeout(input_pad, -1);
            wgetch(input_pad);
            wtimeout(input_pad, TIMEOUT_CURSES);
        }

    } else if (type == VALUE_MSG && get_conf_value("output") != NULL && fdoutput != NULL) {
        fwprintf(fdoutput, L"%s\n", t);
    } else if (type == VALUE_MSG) {
        if (fwide(stdout, 0) >0)
            wprintf(L"%s\n", t);
        else
            printf("%s\n", t);
        fflush(stdout);
    } else {
        if (fwide(stderr, 0) >0)
            fwprintf(stderr, L"%s\n", t);
        else
            fprintf(stderr, "%s\n", t);
        fflush(stderr);
    }
    va_end(args);
    return;
}

/**
 * \brief Welcome screen function used when starting sc-im without a file
 * as a parameter
 *
 * \return none
 */
void ui_do_welcome() {
    char * msg_title = "sc-im - SpreadSheet Calculator Improvised";
    char * msg_by = "An SC fork by Andrés Martinelli";
    char * msg_version = rev;
    char * msg_help  = "Press  :help<Enter>  to get help         ";
    char * msg_help2 = "Press  <Enter>       to enter NORMAL mode";
    int i;

    #ifdef USECOLORS
    wbkgd(main_win, COLOR_PAIR((ucolors[DEFAULT].fg+1) * (COLORS) + ucolors[DEFAULT].bg + 2));
    wbkgd(input_win, COLOR_PAIR((ucolors[DEFAULT].fg+1) * (COLORS) + ucolors[DEFAULT].bg + 2));
    wbkgd(input_pad, COLOR_PAIR((ucolors[DEFAULT].fg+1) * (COLORS) + ucolors[DEFAULT].bg + 2));
    #endif

    // show headings
    int nbcols = calc_mobile_cols(session->cur_doc->cur_sh, NULL);
    int nbrows = calc_mobile_rows(session->cur_doc->cur_sh, NULL);
    ui_show_sc_col_headings(main_win, nbcols);
    ui_show_content(main_win, nbrows, nbcols);
    ui_show_sc_row_headings(main_win, nbrows); // show_sc_row_headings must be after show_content

    #ifdef USECOLORS
    ui_set_ucolor(main_win, &ucolors[WELCOME], DEFAULT_COLOR);
    #endif

    // show message
    mvwaddstr(main_win, LINES/2-3, COLS/2-strlen(msg_title)/2  , msg_title);
    mvwaddstr(main_win, LINES/2-2, COLS/2-strlen(msg_by)/2     , msg_by);
    mvwaddstr(main_win, LINES/2-1, COLS/2-strlen(msg_version)/2, msg_version);

    #ifdef USECOLORS
    ui_set_ucolor(main_win, &ucolors[WELCOME], DEFAULT_COLOR);
    #endif
    for (i=0; msg_help[i] != '\0'; i++) {
        if (msg_help[i] == '<') {
            #ifdef USECOLORS
            ui_set_ucolor(main_win, &ucolors[NUMB], DEFAULT_COLOR);
            #endif
        }
        mvwaddstr(main_win, LINES/2, COLS/2-strlen(msg_help)/2+i, &msg_help[i]);
        if (msg_help[i] == '>') {
            #ifdef USECOLORS
            ui_set_ucolor(main_win, &ucolors[WELCOME], DEFAULT_COLOR);
            #endif
        }
    }
    for (i=0; msg_help2[i] != '\0'; i++) {
        if (msg_help2[i] == '<') {
            #ifdef USECOLORS
            ui_set_ucolor(main_win, &ucolors[NUMB], DEFAULT_COLOR);
            #endif
        }
        mvwaddstr(main_win, LINES/2+1, COLS/2-strlen(msg_help2)/2+i, &msg_help2[i]);
        if (msg_help2[i] == '>') {
            #ifdef USECOLORS
            ui_set_ucolor(main_win, &ucolors[WELCOME], DEFAULT_COLOR);
            #endif
        }
    }
    wrefresh(main_win);

    if (get_conf_int("show_cursor")) {
        /* land cursor next to the help line */
        curwinrow = LINES / 2;
        curwincol = COLS / 2 - strlen(msg_help) / 2 - 1;
        status_line_empty = 1;
    }

    // refresh pad
    ui_refresh_pad(0);
    return;
}


/**
 * \brief Refreshes screen grid
 *
 * \details This function is used to refresh the screen content. If 
 * the header flag is set, the first column of the screen gets
 * refreshed.
 *
 * \param[in] header
 *
 * \return none
 */
void ui_update(int header) {
    struct roman * roman = session->cur_doc;
    struct sheet * sh = roman->cur_sh;

    if (roman->loading) return;
    if (cmd_multiplier > 1) return;
    if (get_conf_int("nocurses")) return;

    #ifdef USECOLORS
    wbkgd(main_win, COLOR_PAIR((ucolors[DEFAULT].fg+1) * (COLORS) + ucolors[DEFAULT].bg + 2));
    // comment this to prevent bold to be reset
    wbkgd(input_win, COLOR_PAIR((ucolors[DEFAULT].fg+1) * (COLORS) + ucolors[DEFAULT].bg + 2));
    wbkgd(input_pad, COLOR_PAIR((ucolors[DEFAULT].fg+1) * (COLORS) + ucolors[DEFAULT].bg + 2));
    #endif

    if (header) {
        ui_show_celldetails(); // always before ui_print_mode
        ui_print_mode();
        wrefresh(input_win);
        ui_refresh_pad(0);
    }

    /* You can't hide the last row or col */
    while (sh->row_hidden[sh->currow])
        sh->currow++;
    while (sh->col_hidden[sh->curcol])
        sh->curcol++;

    /*
     * Calculate offscreen rows and columns
     *
     * offscr_sc_cols are the number of columns left at the left of start
     * of grid. For instance, if offscr_sc_cols is 4. the first visible
     * column in grid would be column E.
     *
     * Mobile rows and columns are not frozen. They can move in and out of
     * the display screen. Here nb_mobile_cols and nb_mobile_rows contain
     * the number of visible mobile columns and rows. The more there are
     * frozen rows/cols, the fewer mobile rows/cols.
     */
    int nb_mobile_cols = calc_mobile_cols(sh, NULL);

    // Show sc_col headings: A, B, C, D..
    ui_show_sc_col_headings(main_win, nb_mobile_cols);

    int nb_mobile_rows;
    int redraw_needed;
    do {
        nb_mobile_rows = calc_mobile_rows(sh, NULL);

        // Show the content of the cells
        // Numeric values, strings.
        redraw_needed = ui_show_content(main_win, nb_mobile_rows, nb_mobile_cols);
    } while (redraw_needed);

    // Show sc_row headings: 0, 1, 2, 3..
    ui_show_sc_row_headings(main_win, nb_mobile_rows); // schow_sc_row_headings must be after show_content

    if (status_line_empty && get_conf_int("show_cursor")) {
        // Leave cursor on selected cell when no status message
        wmove(main_win, curwinrow, curwincol);
    }

    // Refresh curses windows
    wrefresh(main_win);

    return;
}

/**
 * \brief Enable cursor and echo depending on the current screen mode
 *
 * \return none
 */
void ui_handle_cursor() {
    switch (curmode) {
        case COMMAND_MODE:
            noecho();
            curs_set(1);
            break;
        case INSERT_MODE:
        case EDIT_MODE:
            noecho();
            curs_set(1); // changed for NETBSD compatibility
            break;
        default:
            noecho();
            curs_set(0);
    }
    return;
}


/**
 * \brief Print string with alignment
 *
 * \details Internal function to print a string with alignment.
 *
 * JUSTIF: 0 left shift
 *
 * JUSTIF: 1 right shift
 *
 * \param[in] win
 * \param[in] word
 * \param[in] row
 * \param[in] justify
 *
 * \return none
 */
void ui_write_j(WINDOW * win, const char * word, const unsigned int row, const unsigned int justif) {
    (justif == 0) ? (wmove(win, row, 0) && wclrtoeol(win)) : wmove(win, row, COLS - strlen(word));
    wprintw(win, "%s", word);
    return;
}


/**
 * \brief Print multiplier and pending operator to the status bar
 *
 * \return none
 */
void ui_print_mult_pend() {
    struct roman * roman = session->cur_doc;
    struct sheet * sh = roman->cur_sh;
    if (curmode != NORMAL_MODE && curmode != VISUAL_MODE && curmode != EDIT_MODE) return;

    int row_orig, col_orig;
    getyx(input_pad, row_orig, col_orig);

    #ifdef USECOLORS
    ui_set_ucolor(input_win, &ucolors[MODE], DEFAULT_COLOR);
    #endif
    // Show multiplier and pending operator
    char strm[COLS];
    strm[0]='\0';
    if (cmd_multiplier > 0) sprintf(strm, "%d", cmd_multiplier);
    if (cmd_pending) {
        strcat(strm, "?");
    }

    char field[sh->rescol+1];
    field[0]='\0';
    sprintf(field, "%0*d", sh->rescol - (int) strlen(strm), 0);
    subst(field, '0', ' ');
    strcat(strm, field);

    mvwprintw(input_win, 0, COLS - sh->rescol - 14, "%s", strm);
    wrefresh(input_win);

    // Return cursor to previous position
    wmove(input_pad, row_orig, col_orig);
    int scroll = 0;
    if (inputline_pos > COLS - 14) scroll = inputline_pos - COLS + 14;
    ui_refresh_pad(scroll);

    if (status_line_empty && curmode != EDIT_MODE && get_conf_int("show_cursor")) {
        // Leave cursor on selected cell when no status message
        wmove(main_win, curwinrow, curwincol);
        wrefresh(main_win);
    }
}


/**
 * \brief Show first and second row (header). Handle cursor position.
 * \return none
 */
void ui_show_header() {
    //ui_clr_header(0);
    //ui_clr_header(1); // the clr stuff should be called outside this function

    // print multiplier
    ui_print_mult_pend();

    // Show current mode
    ui_print_mode();

    // Print input text over the input pad
    switch (curmode) {
        case COMMAND_MODE:
            mvwprintw(input_pad, 0, 0, ":%ls", inputline);
            wmove(input_pad, 0, inputline_pos + 1);
            break;
        case INSERT_MODE:
            mvwprintw(input_pad, 0, 1, "%ls", inputline);
            wmove(input_pad, 0, inputline_pos + 1);
            break;
        case EDIT_MODE:
            mvwprintw(input_pad, 0, 1, "%ls", inputline);
            wmove(input_pad, 0, inputline_pos + 1);
    }
    int scroll = 0;
    if (inputline_pos > COLS - 14) scroll += inputline_pos - COLS + 14;
    wrefresh(input_win);
    ui_refresh_pad(scroll);
    return;
}


/**
 * \brief ui_clr_header
 * \details clr a line
 * \param[in] i (row)
 *
 * \return none
 */
void ui_clr_header(int i) {
    int row_orig, col_orig;
    getyx(i == 0 ? input_win : input_pad, row_orig, col_orig);
    if (col_orig > COLS) col_orig = COLS - 1;

    if (i == 0) {
        wbkgd(input_win, COLOR_PAIR((ucolors[DEFAULT].fg+1) * (COLORS) + ucolors[DEFAULT].bg + 2));
        wmove(input_win, 0, 0);
        wclrtoeol(input_win);
    } else {
        wbkgd(input_pad, COLOR_PAIR((ucolors[DEFAULT].fg+1) * (COLORS) + ucolors[DEFAULT].bg + 2));
        getyx(input_pad, row_orig, col_orig);
        wmove(input_pad, 0, 0);
        wclrtoeol(input_pad);
        status_line_empty = 1;
    }
    // Return cursor to previous position
    wmove(i == 0 ? input_win : input_pad, row_orig, col_orig);
    if (! i) ui_refresh_pad(0);
}


/**
 * \brief Print current mode in the first row
 *
 * \details Print the current mode in the first row. Print ':' (colon)
 * or submode indicator.
 *
 * \return none
 */
void ui_print_mode() {
    unsigned int row = 0; // Print mode in first row
    char strm[PATHLEN+22] = "";

    #ifdef USECOLORS
    ui_set_ucolor(input_win, &ucolors[MODE], DEFAULT_COLOR);
    #endif

    if (curmode == NORMAL_MODE) {
        strcat(strm, " -- NORMAL --");
        ui_write_j(input_win, strm, row, RIGHT);

    } else if (curmode == INSERT_MODE) {
        strcat(strm, " -- INSERT --");
        ui_write_j(input_win, strm, row, RIGHT);

        #ifdef USECOLORS
        ui_set_ucolor(input_win, &ucolors[INPUT], DEFAULT_COLOR);
        #endif
        // Show submode (INSERT)
        mvwprintw(input_pad, 0, 0, "%c", insert_edit_submode);

    } else if (curmode == EDIT_MODE) {
        strcat(strm, "   -- EDIT --");
        ui_write_j(input_win, strm, row, RIGHT);
        // Show ^ in 0,0 position of pad
        mvwprintw(input_pad, 0, 0, "^");

    } else if (curmode == VISUAL_MODE) {
        strcat(strm, " -- VISUAL --");
        if (visual_submode != '0')
            strcpy(strm, " << VISUAL >>");
        ui_write_j(input_win, strm, row, RIGHT);

    } else if (curmode == COMMAND_MODE) {
        strcat(strm, "-- COMMAND --");

        ui_write_j(input_win, strm, row, RIGHT);
        #ifdef USECOLORS
        ui_set_ucolor(input_win, &ucolors[INPUT], DEFAULT_COLOR);
        #endif
        // show ':'
        mvwprintw(input_pad, 0, 0, ":");
        wmove(input_pad, 0, 1);
    }

    return;
}


/**
 * \brief Show sc_row headings: 0, 1, 2...
 *
 * \param[in] win
 * \param[in] row_boundary
 *
 * \return none
 */
void ui_show_sc_row_headings(WINDOW * win, int nb_mobile_rows) {
    struct roman * roman = session->cur_doc;
    struct sheet * sh = roman->cur_sh;
    #ifdef USECOLORS
    if (has_colors()) ui_set_ucolor(win, &ucolors[HEADINGS], DEFAULT_COLOR);
    #endif

    int i, j;
    srange * s = get_selected_range();

    int winrow = RESCOLHEADER;
    int mobile_rows = nb_mobile_rows;
    int total_rows = nb_mobile_rows + sh->nb_frozen_rows;

    for (i = 0; i < sh->maxrows && total_rows > 0; i++) {
        if (sh->row_hidden[i])
            continue;
        if (! sh->row_frozen[i]) {
            if (i < sh->offscr_sc_rows)
                continue;
            if (--mobile_rows < 0)
                continue;
        }
        --total_rows;

        if ( (s != NULL && i >= s->tlrow && i <= s->brrow) || i == sh->currow ) {
            #ifdef USECOLORS
            if (has_colors()) ui_set_ucolor(win, &ucolors[CELL_SELECTION], DEFAULT_COLOR);
            #else
            wattron(win, A_REVERSE);
            #endif
        }
        mvwprintw (win, winrow, 0, "%*d ", sh->rescol-1, i);
        if (sh->row_frozen[i]) mvwprintw (win, winrow, sh->rescol-1, "!");

        for (j = 1; j < sh->row_format[i]; j++)
            mvwprintw (win, winrow+j, 0, "%*c ", sh->rescol-1, ' ');

        #ifdef USECOLORS
        if (has_colors()) ui_set_ucolor(win, &ucolors[HEADINGS], DEFAULT_COLOR);
        #else
        wattroff(win, A_REVERSE);
        #endif

        winrow += sh->row_format[i];
    }
}


/**
 * \brief Show sc_col headings: A, B, C...
 *
 * \param[in] win
 * \param[in] col_boundary
 *
 * \return none
 */
void ui_show_sc_col_headings(WINDOW * win, int nb_mobile_cols) {
    struct roman * roman = session->cur_doc;
    struct sheet * sh = roman->cur_sh;
    int i;
    srange * s = get_selected_range();

    wmove(win, 0, 0);
    wclrtoeol(win);

    int wincol = sh->rescol;
    int mobile_cols = nb_mobile_cols;
    int total_cols = nb_mobile_cols + sh->nb_frozen_cols;

    for (i = 0; i < sh->maxcols && total_cols > 0; i++) {
        if (sh->col_hidden[i])
            continue;
        if (! sh->col_frozen[i]) {
            if (i < sh->offscr_sc_cols)
                continue;
            if (--mobile_cols < 0)
                continue;
        }
        --total_cols;

        #ifdef USECOLORS
        if (has_colors() && i % 2 == 0)
            ui_set_ucolor(win, &ucolors[HEADINGS], DEFAULT_COLOR);
        else if (i % 2 == 1)
            ui_set_ucolor(win, &ucolors[HEADINGS_ODD], DEFAULT_COLOR);
        #endif

        if ( (s != NULL && i >= s->tlcol && i <= s->brcol) || i == sh->curcol ) {
            #ifdef USECOLORS
            if (has_colors()) ui_set_ucolor(win, &ucolors[CELL_SELECTION], DEFAULT_COLOR);
            #else
            wattron(win, A_REVERSE);
            #endif
        }

        char *a = coltoa(i);
        int l1 = (i < 26) ? 1 : 2;
        int l2 = sh->col_frozen[i] ? 1 : 0;
        int f1 = (sh->fwidth[i] - l1 - l2)/2 + l1;
        int f2 = sh->fwidth[i] - f1;
        mvwprintw(win, 0, wincol, "%*s%*s", f1, a, -f2, sh->col_frozen[i] ? "!" : "");

        #ifdef USECOLORS
        if (has_colors() && i % 2 == 0)
            ui_set_ucolor(win, &ucolors[HEADINGS], DEFAULT_COLOR);
        else if (i % 2 == 1)
            ui_set_ucolor(win, &ucolors[HEADINGS_ODD], DEFAULT_COLOR);
        #else
        wattroff(win, A_REVERSE);
        #endif

        wincol += sh->fwidth[i];
    }
}


/**
 * \brief Show the content of the cell
 *
 * \param[in] win
 * \param[in] row_boundary
 * \param[in] col_boundary
 *
 * \return none
 */
int ui_show_content(WINDOW * win, int nb_mobile_rows, int nb_mobile_cols) {
    struct roman * roman = session->cur_doc;
    struct sheet * sh = roman->cur_sh;
    int row, col;
    int redraw_needed = FALSE;

    srange * s = get_selected_range();
    int conf_underline_grid = get_conf_int("underline_grid");
    int conf_truncate = get_conf_int("truncate");
    int conf_overlap = get_conf_int("overlap");
    int conf_autowrap = get_conf_int("autowrap");

    int winrow = RESCOLHEADER;
    int mobile_rows = nb_mobile_rows;
    int total_rows = nb_mobile_rows + sh->nb_frozen_rows;

    wmove(win, winrow, 0);
    wclrtobot(win);

    for (row = 0; row < sh->maxrows && total_rows > 0; row++) {
        if (sh->row_hidden[row])
            continue;
        if (! sh->row_frozen[row]) {
            if (row < sh->offscr_sc_rows)
                continue;
            if (--mobile_rows < 0)
                continue;
        }
        --total_rows;

        int wincol = sh->rescol;
        int mobile_cols = nb_mobile_cols;
        int total_cols = nb_mobile_cols + sh->nb_frozen_cols;

        for (col = 0; col < sh->maxcols && total_cols > 0; col++) {
            if (sh->col_hidden[col])
                continue;
            if (! sh->col_frozen[col]) {
                if (col < sh->offscr_sc_cols)
                    continue;
                if (--mobile_cols < 0)
                    continue;
            }
            --total_cols;

            struct ent ** p = ATBL(sh, sh->tbl, row, col);
            int fieldlen = sh->fwidth[col];

            // Clean format
#ifdef USECOLORS
            if ((*p) && (*p)->cellerror) {                                  // cellerror
                ui_set_ucolor(win, &ucolors[CELL_ERROR], ucolors[CELL_ERROR].bg != DEFAULT_COLOR ? DEFAULT_COLOR : col % 2 == 0 ? ucolors[GRID_EVEN].bg : ucolors[GRID_ODD].bg);
            } else if ((*p) && (*p)->v < 0) {                               // cell negative
                ui_set_ucolor(win, &ucolors[CELL_NEGATIVE], ucolors[CELL_NEGATIVE].bg != DEFAULT_COLOR ? DEFAULT_COLOR : col % 2 == 0 ? ucolors[GRID_EVEN].bg : ucolors[GRID_ODD].bg);
            } else if ((*p) && (*p)->expr) {
                ui_set_ucolor(win, &ucolors[EXPRESSION], ucolors[EXPRESSION].bg != DEFAULT_COLOR ? DEFAULT_COLOR : col % 2 == 0 ? ucolors[GRID_EVEN].bg : ucolors[GRID_ODD].bg);
            } else if ((*p) && (*p)->label) {                               // string
                ui_set_ucolor(win, &ucolors[STRG], ucolors[STRG].bg != DEFAULT_COLOR ? DEFAULT_COLOR : col % 2 == 0 ? ucolors[GRID_EVEN].bg : ucolors[GRID_ODD].bg);
            } else if ((*p) && (*p)->flags & is_valid && ! (*p)->format) {  // numeric value
                ui_set_ucolor(win, &ucolors[NUMB], ucolors[NUMB].bg != DEFAULT_COLOR ? DEFAULT_COLOR : col % 2 == 0 ? ucolors[GRID_EVEN].bg : ucolors[GRID_ODD].bg);
            } else if ((*p) && (*p)->format && (*p)->format[0] == 'd') {    // date format
                ui_set_ucolor(win, &ucolors[DATEF], ucolors[DATEF].bg != DEFAULT_COLOR ? DEFAULT_COLOR : col % 2 == 0 ? ucolors[GRID_EVEN].bg : ucolors[GRID_ODD].bg);
            } else {
                ui_set_ucolor(win, &ucolors[NORMAL], ucolors[NORMAL].bg != DEFAULT_COLOR ? DEFAULT_COLOR : col % 2 == 0 ? ucolors[GRID_EVEN].bg : ucolors[GRID_ODD].bg);
            }
#endif

            // Cell color!
            if ((*p) && (*p)->ucolor != NULL) {
                ui_set_ucolor(win, (*p)->ucolor, (*p)->ucolor->bg != DEFAULT_COLOR ? DEFAULT_COLOR : col % 2 == 0 ? ucolors[GRID_EVEN].bg : ucolors[GRID_ODD].bg);
            }

            // setup color for selected cell
            if ((sh->currow == row) && (sh->curcol == col)) {
#ifdef USECOLORS
                    if (has_colors()) ui_set_ucolor(win, &ucolors[CELL_SELECTION_SC], ucolors[CELL_SELECTION_SC].bg != DEFAULT_COLOR ? DEFAULT_COLOR : col % 2 == 0 ? ucolors[GRID_EVEN].bg : ucolors[GRID_ODD].bg);
#else
                    wattron(win, A_REVERSE);
#endif
            }

            // setup color for selected range
            int in_range = 0; // this is for coloring empty cells within a range
            if (s != NULL && row >= s->tlrow && row <= s->brrow && col >= s->tlcol && col <= s->brcol ) {
#ifdef USECOLORS
                    if (has_colors()) ui_set_ucolor(win, &ucolors[CELL_SELECTION_SC], ucolors[CELL_SELECTION_SC].bg != DEFAULT_COLOR ? DEFAULT_COLOR : col % 2 == 0 ? ucolors[GRID_EVEN].bg : ucolors[GRID_ODD].bg);
#else
                    wattron(win, A_REVERSE);
#endif
                in_range = 1; // local variable. this is for coloring empty cells within a range
            }

            /* Color empty cells inside a range */
            if ( in_range && row >= ranges->tlrow && row <= ranges->brrow &&
                 col >= ranges->tlcol && col <= ranges->brcol
               ) {
#ifdef USECOLORS
                    if (has_colors()) ui_set_ucolor(win, &ucolors[CELL_SELECTION_SC], ucolors[CELL_SELECTION_SC].bg != DEFAULT_COLOR ? DEFAULT_COLOR : col % 2 == 0 ? ucolors[GRID_EVEN].bg : ucolors[GRID_ODD].bg);
#else
                    wattron(win, A_REVERSE);
#endif
            }

            char num [FBUFLEN] = "";
            char text[FBUFLEN] = "";
            wchar_t out [FBUFLEN] = L"";
            char formated_s[FBUFLEN] = "";
            int res = -1;
            int align = 1;

            // If a numeric value exists
            if ( (*p) && (*p)->flags & is_valid) {
                res = ui_get_formated_value(p, col, formated_s);
                // res = 0, indicates that in num we store a date
                // res = 1, indicates a format is applied in num
                if (res == 0 || res == 1) {
                    strcpy(num, formated_s);
                } else if (res == -1) {
                    sprintf(num, "%.*f", sh->precision[col], (*p)->v);
                }
            }

            // If a string exists
            if ((*p) && (*p)->label) {
                strcpy(text, (*p)->label);
                align = 1;                               // right alignment
                if ((*p)->flags & is_label) {            // center alignment
                    align = 0;
                } else if ((*p)->flags & is_leftflush) { // left alignment
                    align = -1;
                } else if (res == 0) {                   // res must ¿NOT? be zero for label to be printed // TODO CHECK!
                    text[0] = '\0';
                }
            }

            if ((*p) && (*p)->cellerror == CELLERROR) {
               (void) mvwprintw(win, winrow, wincol, "%*.*s", fieldlen, fieldlen, "ERROR");
               align = 0;
               strcpy(text, "ERROR");
               num[0]='\0';
            }
            if ((*p) && (*p)->cellerror == CELLREF) {
               (void) mvwprintw(win, winrow, wincol, "%*.*s", fieldlen, fieldlen, "REF");
               align = 0;
               strcpy(text, "REF");
               num[0]='\0';
            }

            // repaint a blank cell or range
            if ( (sh->currow == row && sh->curcol == col) ||
               ( in_range && row >= ranges->tlrow && row <= ranges->brrow &&
                 col >= ranges->tlcol && col <= ranges->brcol ) ) {
#ifdef USECOLORS
                if (has_colors()) ui_set_ucolor(win, &ucolors[CELL_SELECTION_SC], ucolors[CELL_SELECTION_SC].bg != DEFAULT_COLOR ? DEFAULT_COLOR : col % 2 == 0 ? ucolors[GRID_EVEN].bg : ucolors[GRID_ODD].bg);
#else
                wattron(win, A_REVERSE);
#endif
            }

#ifdef USECOLORS
            if (has_colors() && conf_underline_grid) {
                attr_t attr;
                short color;
                wattr_get(win, &attr, &color, NULL);
                wattr_set(win, attr | A_UNDERLINE, color, NULL);
            }
#endif

            // new implementation for wide char support
            cchar_t cht[fieldlen];
            wchar_t w;
            int i, j, k;

            for (k=0; k < sh->row_format[row]; k++) {
                for (i = 0; i < fieldlen; ) {
                    w = L' ';
                    j = mvwin_wchnstr (win, winrow+k, wincol+i, cht, 1);
                    if (j == OK && cht[0].chars[0] != L'\0')
                        w = cht[0].chars[0];
                    //mvwprintw(win, winrow+k, wincol+i, "%lc", L'*');//w
                    mvwprintw(win, winrow+k, wincol+i, "%lc", w);
                    i += wcwidth(w);
                }
            }

            // we print text and number
            if ( (*p) && ( ((*p)->flags & is_valid) || (*p)->label ) ) {
                // hide text or number from num & text combined cells
                // if ((strlen(text) && strlen(num)) {
                //    if get_conf_int("hide_text_from_combined"))
                //        text[0]='\0';
                //    else if get_conf_int("hide_number_from_combined"))
                //        num[0]='\0';
                // }
                pad_and_align(text, num, fieldlen, align, (*p)->pad, out, sh->row_format[row]);

#ifdef USECOLORS
                if (has_colors() && conf_underline_grid) {
                    attr_t attr;
                    short color;
                    wattr_get(win, &attr, &color, NULL);
                    wattr_set(win, attr | A_UNDERLINE, color, NULL);
                }
#endif

                if (!conf_truncate && !conf_overlap && conf_autowrap) {
                    // auto wrap
                    int newheight = (wcslen(out) + sh->fwidth[col] - 1) / sh->fwidth[col];
                    if (sh->row_format[row] < newheight) {
                        sh->row_format[row] = newheight;
                        /* calc_mobile_rows() didn't know about this */
                        redraw_needed = TRUE;
                    }
                }

                if (!conf_overlap || sh->row_format[row] != 1) {
                    int k;
                    wchar_t *p_out = out;
                    for (k = 0; k < sh->row_format[row]; k++) {
                        int eol = count_width_widestring(p_out, fieldlen);
                        wchar_t save = p_out[eol];
                        p_out[eol] = L'\0';
                        mvwprintw(win, winrow+k, wincol, "%*ls", -fieldlen, p_out);
                        p_out[eol] = save;
                        p_out += eol;
                    }
                } else {
                    //sh->row_format[row] = 1;
                    mvwprintw(win, winrow, wincol, "%*ls", -fieldlen, out);
                }
            }

            if (sh->currow == row && sh->curcol == col) {
                curwinrow = winrow;
                curwincol = wincol;
                if (out[0] == L'\0') {
                    // center the cursor on empty cells
                    curwincol += (fieldlen - 1)/2;;
                } else if (out[0] == L' ') {
                    // cursor on last space but not beyond the middle
                    int i;
                    for (i = 0; i < (fieldlen - 1)/2 && out[i+1] == L' '; i++);
                    curwincol += i;
                }
            }

            // clean format
#ifndef USECOLORS
            wattroff(win, A_REVERSE);
#endif

            wincol += fieldlen;
        }
        winrow += sh->row_format[row];
    }

    return redraw_needed;
}


/**
 * \brief Add details of an ent to a char*
 *
 * \details Add details of an ent to a char * received as a parameter.
 * Used for 'input_win'
 *
 * \param[in] d
 * \param[in] p1
 *
 * \return none
 */
void ui_add_cell_detail(char * d, struct ent * p1) {
    if ( ! p1 ) return;

    /* string expressions
    if (p1->expr && (p1->flags & is_strexpr)) {
        if (p1->flags & is_label)
            strcat(d, "|{");
        else
            strcat(d, (p1->flags & is_leftflush) ? "<{" : ">{");
        strcat(d, "??? } ");        // and this '}' is for vi %

    } else*/

    if (p1->label) {
        /* has constant label only */
        if (p1->flags & is_label)
            strcat(d, "|\"");
        else
            strcat(d, (p1->flags & is_leftflush) ? "<\"" : ">\"");
        strcat(d, p1->label);
        strcat(d, "\" ");
    }

    /* Display if cell is locked */
    if (p1 && p1->flags & is_locked)
         strcat(d, "[locked] ");

    // value part of cell:
    if (p1->flags & is_valid) {
        /* has value or num expr */
        if ( ( ! (p1->expr) ) || ( p1->flags & is_strexpr ) ) {
            sprintf(d + strlen(d), "%c", '[');
            (void) sprintf(d + strlen(d), "%.15g", p1->v);
            sprintf(d + strlen(d), "%c", ']');
        }
    }
}


/**
 * \brief Draw cell content detail in header
 * Also show current sheet name
 * \return none
 */
void ui_show_celldetails() {
    // return if no open file
    if (session->cur_doc == NULL) return;

    struct sheet * sh = session->cur_doc->cur_sh;

    char head[FBUFLEN];
    int il_pos = 0;

    // show filenames
    for (struct roman * rom = session->first_doc; rom != NULL; rom = rom->next) {
#ifdef USECOLORS
        ui_set_ucolor(input_win, &ucolors[FILENM], DEFAULT_COLOR);
#endif
        char * file_name = rom->name == NULL ? "[No Name]" : rom->name;
        mvwprintw(input_win, 0, il_pos, "%s:", file_name);
        il_pos += strlen(file_name) + 1;

        // show sheets
        for (struct sheet * sh = rom->first_sh; sh != NULL; sh = sh->next) {
            if (sh == session->cur_doc->cur_sh) {
#ifdef USECOLORS
                ui_set_ucolor(input_win, &ucolors[CURRENT_SHEET], DEFAULT_COLOR);
#endif
                if (get_conf_int("show_cursor")) mvwprintw(input_win, 0, il_pos++, "*");
            } else {
#ifdef USECOLORS
                ui_set_ucolor(input_win, &ucolors[SHEET], DEFAULT_COLOR);
#endif
                if (get_conf_int("show_cursor")) mvwprintw(input_win, 0, il_pos++, " ");
            }
            mvwprintw(input_win, 0, il_pos, "{%s}", sh->name);
            il_pos += strlen(sh->name) + 2;
        }
        mvwprintw(input_win, 0, il_pos, "  ");
        il_pos += 2;
    }

    // show cell in header
#ifdef USECOLORS
        ui_set_ucolor(input_win, &ucolors[CELL_ID], DEFAULT_COLOR);
#endif
    sprintf(head, "%s%d ", coltoa(sh->curcol), sh->currow);
    mvwprintw(input_win, 0, il_pos, "%s", head);
    il_pos += strlen(head);

    // show the current cell's format
#ifdef USECOLORS
        ui_set_ucolor(input_win, &ucolors[CELL_FORMAT], DEFAULT_COLOR);
#endif

    struct ent *p1 = *ATBL(sh, sh->tbl, sh->currow, sh->curcol);

    // show padding
    if (p1 != NULL && p1->pad)
        sprintf(head, "(%d) ", p1->pad);
    else
        head[0]='\0';

    // show format
    if ((p1) && p1->format)
        sprintf(head + strlen(head), "(%s) ", p1->format);
    else
        sprintf(head + strlen(head), "(%d %d %d) ", sh->fwidth[sh->curcol], sh->precision[sh->curcol], sh->realfmt[sh->curcol]);
    mvwprintw(input_win, 0, il_pos, "%s", head);
    il_pos += strlen(head);

    // show expr
#ifdef USECOLORS
        ui_set_ucolor(input_win, &ucolors[CELL_CONTENT], DEFAULT_COLOR);
#endif
    if (p1 && p1->expr) {
        linelim = 0;
        editexp(sh, sh->currow, sh->curcol);  /* set line to expr */
        linelim = -1;
        sprintf(head, "[%.*s] ", FBUFLEN-4, line);
        mvwprintw(input_win, 0, il_pos, "%s", head);
        il_pos += strlen(head);
    }
    // add cell content to head string
    head[0] = '\0';
    ui_add_cell_detail(head, p1);

    // cut string if its too large!
    if (strlen(head) > COLS - il_pos - 1) {
        head[COLS - il_pos - 1 - 19]='>';
        head[COLS - il_pos - 1 - 18]='>';
        head[COLS - il_pos - 1 - 17]='>';
        head[COLS - il_pos - 1 - 16]='\0';
    }

    mvwprintw(input_win, 0, il_pos, "%s", head);
    wclrtoeol(input_win);
    wrefresh(input_win);
}


/**
 * \brief Error routine for yacc (gram.y)
 *
 * \param[in] err

 * \return none
 */

void yyerror(char * err) {
    mvwprintw(input_pad, 0, 0, "%s: %.*s<=%s", err, linelim, line, line + linelim);
    ui_refresh_pad(0);
    return;
}


/**
 * \brief Create a string that represents the formatted value of the cell
 *
 * \details This function creates a string (value) that represents
 * the formatted value of the cell.
 *
 * \param[in] err

 * \return 0 datetime format - number in p->v represents a date - format "d"
 * \return 1 if format of number - (numbers with format) - puede harber label.
 * \return -1 if there is no format in the cell
 */
int ui_get_formated_value(struct ent ** p, int col, char * value) {
    struct roman * roman = session->cur_doc;
    struct sheet * sh = roman->cur_sh;
    //char * cfmt = (*p)->format ? (*p)->format : NULL;
    char * cfmt = (*p)->format ? (*p)->format : (sh->realfmt[col] >= 0 && sh->realfmt[col] < COLFORMATS && colformat[sh->realfmt[col]] != NULL) ? colformat[sh->realfmt[col]] : NULL;

    if (cfmt) {
        if (*cfmt == 'd') {
            time_t v = (time_t) ((*p)->v);
            strftime(value, sizeof(char) * FBUFLEN, cfmt + 1, localtime(&v));
            return 0;
        } else {
            format(cfmt, sh->precision[col], (*p)->v, value, sizeof(char) * FBUFLEN);
            return 1;
        }
    } else { // there is no format
        // FIXME: error with number and text in same cell and no overlap
        engformat(sh->realfmt[col], sh->fwidth[col], sh->precision[col], (*p)->v, value, sizeof(char) * FBUFLEN);
        ltrim(value, ' ');
        return 1;
    }
}


/**
 * \brief Shows text in child process
 *
 * \details Shows text in child process. Used for set, version,
 * showmaps, print_graph, showfilters, hiddenrows, and hiddencols commands.
 *
 * \param[in] val
 */
void ui_show_text(char * val) {
    int pid;
    char px[MAXCMD];
    char * pager;

    (void) strcpy(px, "| ");
    if ( !(pager = getenv("PAGER")) )
        pager = DFLT_PAGER;
    (void) strcat(px, pager);
    FILE * f = openfile(px, &pid, NULL);
    if ( !f ) {
        sc_error("Can't open pipe to %s", pager);
        return;
    }
    def_prog_mode();
    endwin();
    fprintf(f, "%s\n", val);
    fprintf(f, "Press 'q' and then ENTER to return.");
    closefile(f, pid, 0);
    getchar();
    reset_prog_mode();
    refresh();
    ui_update(TRUE);
    wmove(input_pad, 0, 0);
    wclrtoeol(input_pad);
    ui_refresh_pad(0);
}


/**
 * \brief
 * UI function thats called after SIGWINCH signal.
 * \return none
 */
void sig_winchg() {
    if (isendwin()) return;
    endwin();
    refresh();
    clear();
    set_term(sstdout);

    wresize(main_win, LINES - RESROW, COLS);
    wresize(input_win, RESROW, COLS);
    ui_mv_bottom_bar();
    ui_update(TRUE);
    flushinp();

    return;
}


#ifdef XLUA
/**
 * \brief Print error of lua scripts
 *
 * \param[in] L
 * \param[in] msg
 *
 * \return none
 */
void ui_bail(lua_State *L, char * msg) {
    extern char stderr_buffer[1024];
    fprintf(stderr,"FATAL ERROR: %s: %s\n", msg, lua_tostring(L, -1));
    move(0, 0);
    clrtobot();
    wrefresh(stdscr);
    set_term(sstderr);
    move(0, 0);
    clrtobot();
    status_line_empty = 1;
    clearok(stdscr, TRUE);
    mvprintw(0, 0, "%s", stderr_buffer);
    stderr_buffer[0]='\0';
    fseek(stderr, 0, SEEK_END);
    refresh();
    getch();
    set_term(sstdout);
    clearok(stdscr, TRUE);
    ui_show_header();
    refresh();
    ui_update(TRUE);
}
#endif


/**
 * \brief Show a message in the screen
 *
 * \details This function shows a message on the screen and waits for user
 * confirmation between a couple of options defined on valid (wchar *)
 * parameter.
 * \return wchar_t indicating user answer
 */
wchar_t ui_query_opt(wchar_t * initial_msg, wchar_t * valid) {
    if (! wcslen(initial_msg)) return L'\0';
    sc_info("%ls", initial_msg);
    wint_t wd = -1;
    wchar_t wdc[2] = L"";
    int res;

    curs_set(1);
    wtimeout(input_pad, -1);
    move(0, wcslen(initial_msg) + 1);
    while ((res = wstr_in_wstr(valid, wdc)) == -1) {
        wget_wch(input_pad, &wd);
        swprintf(wdc, FBUFLEN, L"%lc", wd);
    }
    wtimeout(input_pad, TIMEOUT_CURSES);
    curs_set(0);
    return wdc[0];
}


/**
 * \brief Read text from stdin
 * \param[in] initial_msg
 * \return user input
 */
char * ui_query(char * initial_msg) {
    struct roman * roman = session->cur_doc;
    char * hline = (char *) malloc(sizeof(char) * BUFFERSIZE);
    hline[0]='\0';

    // curses is not enabled
    if ( get_conf_int("nocurses")) {
        if (strlen(initial_msg)) wprintf(L"%s", initial_msg);

        if (fgets(hline, BUFFERSIZE-1, stdin) == NULL)
            hline[0]='\0';

        clean_carrier(hline);
        return hline;
    }

    // curses is enabled
    int loading_o;
    if (roman->loading) {
        loading_o = roman->loading;
        roman->loading = 0;
        ui_update(0);
        roman->loading = loading_o;
    }
    curs_set(1);

    // show initial message
    if (strlen(initial_msg)) sc_info(initial_msg);

    // ask for input
    wtimeout(input_pad, -1);
    notimeout(input_pad, TRUE);
    //wmove(input_pad, 0, 0);
    wmove(input_pad, 0, strlen(initial_msg));
    //wclrtoeol(input_pad);
    ui_refresh_pad(0);

    int d = wgetch(input_pad);

    while (d != OKEY_ENTER && d != OKEY_ESC) {
        if (d == OKEY_BS || d == OKEY_BS2) {
            del_char(hline, strlen(hline) - 1);
        } else {
            sprintf(hline + strlen(hline), "%c", d);
        }

        mvwprintw(input_pad, 0, 0, "%s", hline);
        wclrtoeol(input_pad);
        ui_refresh_pad(0);
        d = wgetch(input_pad);
    }
    if (d == OKEY_ESC) hline[0]='\0';

    // go back to spreadsheet
    noecho();
    curs_set(0);
    wtimeout(input_pad, TIMEOUT_CURSES);
    wmove(input_win, 0,0);
    wclrtoeol(input_win);
    wmove(input_pad, 0,0);
    wclrtoeol(input_pad);
    status_line_empty = 1;
    ui_refresh_pad(0);
    return hline;
}


/**
 * \brief Set a color
 * if bg_override != -1 (DEFAULT_COLOR) set this instead of uc->bg
 *
 * \param[in] w
 * \param[in] uc
 * \param[in} bg_override
 * \return none
 */
void ui_set_ucolor(WINDOW * w, struct ucolor * uc, int bg_override) {
    short color;
    long attr = A_NORMAL;
    if (uc->bold)      attr |= A_BOLD;

#ifdef A_ITALIC
    if (uc->italic)    attr |= A_ITALIC;
#endif
    if (uc->dim)       attr |= A_DIM;
    if (uc->reverse)   attr |= A_REVERSE;
    if (uc->standout)  attr |= A_STANDOUT;
    if (uc->blink)     attr |= A_BLINK;
    if (uc->underline) attr |= A_UNDERLINE;

    // see in ui_start_colors() the explanation of this
    int def = 9;
    if (COLORS > 8) def = 33;
    //

    if (uc->bg == NONE_COLOR || uc->fg == NONE_COLOR) {
        // leave colors intact
        // just apply other ncurses attributes
        attr_t a;
        wattr_get(w, &a, &color, NULL);
    } else if (bg_override == DEFAULT_COLOR) {
        color = (uc->fg+1)*def + uc->bg + 2;
    } else {
        color = (uc->fg+1)*def + bg_override + 2;
    }
    wattr_set (w, attr | COLOR_PAIR(color), color, NULL);
}


/**
 * \brief ui_start_colors()
 *
 * \return none
 */
void ui_start_colors() {
    if (! has_colors()) return;
    int i, j;

    // Initialize all possible 81 init pairs
    use_default_colors();

    /* ncurses has 8 colors, but we need to keep 1 slot more for
     * default terminal background and foreground.
     * bg can be other than black.
     * we also have 24 custom colors.
     * that makes 33 x 33 combinations
     */
    int def = 9;
    if (COLORS > 8) def = 33;
    for (i=0; i < def; i++) {     // fg
        for (j=0; j < def; j++) { // bg
            /*
             * NOTE: calling init_pair with -1 sets it with default
             * terminal foreground and background colors
             */
#if defined(NCURSES_VERSION_MAJOR) && (( NCURSES_VERSION_MAJOR > 5 && defined(NCURSES_VERSION_MINOR) && NCURSES_VERSION_MINOR > 0) || NCURSES_VERSION_MAJOR > 6)
            init_extended_pair( i*def+j+1, i-1, j-1); // i is fg and j is bg
#else
            init_pair(i*def+j+1, i-1, j-1); // i is fg and j is bg
#endif
        }
    }
}


/**
 * \brief
 * UI function thats called after SIGTSTP signal.
 * \return none
 */
void ui_pause() {
    def_prog_mode();
    set_term(sstderr);
    endwin();
    return;
}


/**
 * \brief
 * UI function thats called after SIGCONT signal.
 * \return none
 */
void ui_resume() {
    set_term(sstdout);
    reset_prog_mode();
    clearok(stdscr, TRUE);
    sig_winchg();

    return;
}


/**
 * \brief
 * UI function thats moves input bar to bottom or back to the top
 * \return none
 */
void ui_mv_bottom_bar() {
    mvwin(main_win, get_conf_int("input_bar_bottom") ? 0 : RESROW, 0);
    mvwin(input_win, get_conf_int("input_bar_bottom") ? LINES-RESROW : 0, 0);
    return;
}


/**
 * \brief
 * UI function thats refresh input_pad
 * \return none
 */
void ui_refresh_pad(int scroll) {
    prefresh(input_pad, 0, scroll, get_conf_int("input_bar_bottom") ? LINES-RESROW+1: RESROW-1, 0,
             get_conf_int("input_bar_bottom") ? LINES-RESROW+1: RESROW-1, COLS-1);
}


/**
 * \brief
 * function thats handles mouse movements
 * \return none
 */
#ifdef MOUSE
void ui_handle_mouse(MEVENT event) {
    struct roman * roman = session->cur_doc;
    struct sheet * sh = roman->cur_sh;
    if (isendwin()) return;

    // if mode is not handled return
    if (curmode != NORMAL_MODE && curmode != INSERT_MODE && curmode != COMMAND_MODE) return;

    // scroll in normal mode
#ifdef BUTTON5_PRESSED
    if (curmode == NORMAL_MODE && (event.bstate & BUTTON4_PRESSED || // scroll up
        event.bstate & BUTTON5_PRESSED)) { // scroll down
            int n = calc_mobile_rows(sh, NULL);
            if (get_conf_int("half_page_scroll")) n = n / 2;
            sh->lastcol = sh->curcol;
            sh->lastrow = sh->currow;
            sh->currow = event.bstate & BUTTON5_PRESSED ? forw_row(sh, n)->row : back_row(sh, n)->row;
            if (event.bstate & BUTTON5_PRESSED) scroll_down(sh, n);
            else scroll_up(sh, n);
            unselect_ranges();
            ui_update(TRUE);
        return;
    }
#else
    sc_error("Cannot handle mouse scroll. Please update your ncurses library");
    return;
#endif

    // return if not a single click
    if (! (event.bstate & BUTTON1_CLICKED)) return;

    // get coordinates corresponding to the grid area
    int c = event.x - sh->rescol;
    int r = event.y - RESROW + (get_conf_int("input_bar_bottom") ? 1 : -1);

    // if out of range return
    if ( c < 0 || c >= SC_DISPLAY_COLS ||
         r < 0 || r >= SC_DISPLAY_ROWS ) return;

    int i, j;
    int mobile_cols = calc_mobile_cols(sh, NULL);
    int mobile_rows = calc_mobile_rows(sh, NULL);
    int scr_col = 0;
    int scr_row = 0;

    for (i = 0; i < sh->maxcols; i++) {
        if (sh->col_hidden[i])
            continue;
        if (! sh->col_frozen[i]) {
            if (i < sh->offscr_sc_cols)
                continue;
            if (--mobile_cols < 0)
                continue;
        }

        scr_col += sh->fwidth[i];
        if (scr_col >= c + 1) break;
    }

    // same for rows
    for (j = 0; j < sh->maxrows; j++) {
        if (sh->row_hidden[j])
            continue;
        if (! sh->row_frozen[j]) {
            if (j < sh->offscr_sc_rows)
                continue;
            if (--mobile_rows < 0)
                continue;
        }

        scr_row += sh->row_format[j];
        if (scr_row >= r + 1) break;
    }

    if (i >= sh->maxcols || j >= sh->maxrows) return;

    // if in normal mode, change currow and curcol
    if (curmode == NORMAL_MODE) {
        sh->curcol = i;
        sh->currow = j;
        unselect_ranges();

    // if in insert or command mode, we add the selected cell to inputbar
    } else if (curmode == COMMAND_MODE || curmode == INSERT_MODE) {
        wchar_t cline [BUFFERSIZE];
        int count;
        swprintf(cline, BUFFERSIZE, L"%s%d", coltoa(i), j);
        for(count = 0; count < wcslen(cline); count++) ins_in_line(cline[count]);
        ui_show_header();
        return;
    }

    ui_update(TRUE);
}
#endif
