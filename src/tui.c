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
#include "sc.h"
#include "cmds.h"
#include "cmds_visual.h"
#include "cmds_command.h"
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
        if (type == DEBUG_MSG || (loading && type == ERROR_MSG)) {
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
    char * msg_title = "SC-IM - SpreadSheet Calculator Improvised";
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
    int mxcol = offscr_sc_cols + calc_offscr_sc_cols() - 1;
    int mxrow = offscr_sc_rows + calc_offscr_sc_rows() - 1;
    ui_show_sc_col_headings(main_win, mxcol);
    ui_show_content(main_win, mxrow, mxcol);
    ui_show_sc_row_headings(main_win, mxrow); // show_sc_row_headings must be after show_content

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
    if (loading) return;
    if (cmd_multiplier > 1) return;
    if (get_conf_int("nocurses")) return;

    if (header) {
    #ifdef USECOLORS
    wbkgd(main_win, COLOR_PAIR((ucolors[DEFAULT].fg+1) * (COLORS) + ucolors[DEFAULT].bg + 2));
    // comment this to prevent bold to be reset
    wbkgd(input_win, COLOR_PAIR((ucolors[DEFAULT].fg+1) * (COLORS) + ucolors[DEFAULT].bg + 2));
    wbkgd(input_pad, COLOR_PAIR((ucolors[DEFAULT].fg+1) * (COLORS) + ucolors[DEFAULT].bg + 2));
    #endif

        // Clean from top to bottom
        wmove(main_win, 0, 0);
        wclrtobot(main_win);
        // comment this to prevent info message to be erased
        //wmove(input_win, 0, 0);
        //wclrtobot(input_win);
        ui_show_celldetails(); // always before ui_print_mode
        ui_print_mode();
        wrefresh(input_win);
        ui_refresh_pad(0);
    }

    /* You can't hide the last row or col */
    while (row_hidden[currow])
        currow++;
    while (col_hidden[curcol])
        curcol++;

    /*
     * Calculate offscreen rows and columns
     *
     * mxcol-1 is the last visible column in screen grid.
     * for instance, if mxcol is 8, last visible column would be I
     * mxrow-1 is the last visible row in screen grid
     *
     * offscr_sc_cols are the number of columns left at the left of start of grid.
     * for instance, if offscr_sc_cols is 4. the first visible column in grid would be column E.
     *
     * there is a special behaviour when frozen columns or rows exists.
     * center_hidden_rows and center_hidden_columns are the number of rows and columns between
     * a frozen range and the first column or row visible.
     * example: if columns A and B are frozen, and center_hidden_cols is 4,
     * your grid would start with columns A, B, G, H..
     */
    int cols = calc_offscr_sc_cols();
    int rows = calc_offscr_sc_rows();
    int mxcol = offscr_sc_cols + cols - 1;
    int mxrow = offscr_sc_rows + rows - 1;
    //sc_info("rows:%d off:%d, mxrow:%d, maxrows:%d aft:%d", rows, offscr_sc_rows, mxrow, maxrows, num_frozen_after_rows);
    //sc_info("cols:%d off:%d, mxcol:%d, maxcols:%d", cols, offscr_sc_cols, mxcol, maxcols);

    // Show sc_col headings: A, B, C, D..
    ui_show_sc_col_headings(main_win, mxcol);

    // Show the content of the cells
    // Numeric values, strings.
    ui_show_content(main_win, mxrow, mxcol);

    // Show sc_row headings: 0, 1, 2, 3..
    ui_show_sc_row_headings(main_win, mxrow); // schow_sc_row_headings must be after show_content

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

    char field[rescol+1];
    field[0]='\0';
    sprintf(field, "%0*d", rescol - (int) strlen(strm), 0);
    subst(field, '0', ' ');
    strcat(strm, field);

    mvwprintw(input_win, 0, COLS - rescol - 14, "%s", strm);
    wrefresh(input_win);

    // Return cursor to previous position
    wmove(input_pad, row_orig, col_orig);
    ui_refresh_pad(0);

    if (status_line_empty && curmode != EDIT_MODE && get_conf_int("show_cursor")) {
        // Leave cursor on selected cell when no status message
        wmove(main_win, curwinrow, curwincol);
        wrefresh(main_win);
    }
}

/**
 * \brief Show first and second row (header). Handle cursor position.
 *
 * \return none
 */

void ui_show_header() {
    //ui_clr_header(0);
    ui_clr_header(1);

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

    strm[0] = '\0';
    if (get_conf_int("filename_with_mode") && curfile[0]) {
        sprintf(strm, "%s ", curfile);
    }

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
 * \param[in] mxrow
 *
 * \return none
 */

void ui_show_sc_row_headings(WINDOW * win, int mxrow) {
    #ifdef USECOLORS
    if (has_colors()) ui_set_ucolor(win, &ucolors[HEADINGS], DEFAULT_COLOR);
    #endif
    int i, j;
    int count = 0;
    int frozen_after_rows_shown = 0;

    for (i = 0; i <= mxrow || frozen_after_rows_shown != num_frozen_after_rows; i++) {
        if (i >= maxrows) {
            sc_debug("i:%d >= maxrows:%d in ui_show_sc_row_headings. please check calc_offscr_sc_rows.", i, maxrows);
            break;
        }
        // print rows in case frozen rows are before offscr_sc_rows
        if ( (i < offscr_sc_rows && ! row_frozen[i]) || row_hidden[i] ) continue;

        if (i > mxrow) { while (! row_frozen[i]) i++; frozen_after_rows_shown++; }

        srange * s = get_selected_range();
        if ( (s != NULL && i >= s->tlrow && i <= s->brrow) || i == currow ) {
            #ifdef USECOLORS
            if (has_colors()) ui_set_ucolor(win, &ucolors[CELL_SELECTION], DEFAULT_COLOR);
            #else
            wattron(win, A_REVERSE);
            #endif
        }
        mvwprintw (win, count+1, 0, "%*d ", rescol-1, i);
        if (row_frozen[i]) mvwprintw (win, count+1, rescol-1, "!");

        for (j = 1; j < row_format[i]; j++)
            mvwprintw (win, count+1+j, 0, "%*c ", rescol-1, ' ');

        count += row_format[i];

        #ifdef USECOLORS
        if (has_colors()) ui_set_ucolor(win, &ucolors[HEADINGS], DEFAULT_COLOR);
        #else
        wattroff(win, A_REVERSE);
        #endif
    }
}

/**
 * \brief Show sc_col headings: A, B, C...
 *
 * \param[in] win
 * \param[in] mxcol last column printed to the screen
 *
 * \return none
 */

void ui_show_sc_col_headings(WINDOW * win, int mxcol) {
    int i, col = rescol;
    int frozen_after_cols_shown = 0;

    wmove(win, 0, 0);
    wclrtoeol(win);

    for (i = 0; i <= mxcol || frozen_after_cols_shown != num_frozen_after_cols; i++) {
        //sc_info("mxcol:%d %s num_frozen_after_cols: %d", mxcol, coltoa(mxcol), num_frozen_after_cols);
        if (i >= maxcols) {
            sc_debug("i:%d >= maxcols:%d in ui_show_sc_col_headings. please check calc_offscr_sc_cols.", i, maxcols);
            break;
        }
        // print cols in case freezen columns are before offscr_sc_cols
        if ( (i < offscr_sc_cols && ! col_frozen[i]) || col_hidden[i] ) continue;

        if (i > mxcol) { while (! col_frozen[i]) i++; frozen_after_cols_shown++; }

        #ifdef USECOLORS
        if (has_colors() && i % 2 == 0)
            ui_set_ucolor(win, &ucolors[HEADINGS], DEFAULT_COLOR);
        else if (i % 2 == 1)
            ui_set_ucolor(win, &ucolors[HEADINGS_ODD], DEFAULT_COLOR);
        #endif

        srange * s = get_selected_range();
        if ( (s != NULL && i >= s->tlcol && i <= s->brcol) || i == curcol ) {
            #ifdef USECOLORS
            if (has_colors()) ui_set_ucolor(win, &ucolors[CELL_SELECTION], DEFAULT_COLOR);
            #else
            wattron(win, A_REVERSE);
            #endif
        }


        // if we want ! after column name:
        int k = (fwidth[i] - 1) / 2;
        mvwprintw(win, 0, col, "%*s%s%s%*s", k, "", coltoa(i),
        col_frozen[i] ? "!" : "", fwidth[i] - k - (col_frozen[i] ? strlen(coltoa(i))+1 : strlen(coltoa(i))), "");

        col += fwidth[i];
        if (i == mxcol && COLS - col > 0) wclrtoeol(win);

        #ifdef USECOLORS
        if (has_colors() && i % 2 == 0)
            ui_set_ucolor(win, &ucolors[HEADINGS], DEFAULT_COLOR);
        else if (i % 2 == 1)
            ui_set_ucolor(win, &ucolors[HEADINGS_ODD], DEFAULT_COLOR);
        #else
        wattroff(win, A_REVERSE);
        #endif
    }
}



/**
 * \brief Show the content of the cell
 *
 * \param[in] win
 * \param[in] mxrow
 * \param[in] mxcol
 *
 * \return none
 */

void ui_show_content(WINDOW * win, int mxrow, int mxcol) {
    register struct ent ** p;
    int row, col, count = 0;
    int frozen_after_rows_shown = 0;

    for (row = 0; row <= mxrow || frozen_after_rows_shown != num_frozen_after_rows; row++) {
         if (row >= maxrows) {
             sc_debug("row:%d >= maxrows:%d in show_content. please check calc_offscr_sc_rows.", row, maxrows);
             break;
         }

        // print rows in case frozen rows are before offscr_sc_rows
        if (row < offscr_sc_rows && ! row_frozen[row]) continue;

        if (row_hidden[row]) continue;

        if (row > mxrow) { while (! row_frozen[row]) row++; frozen_after_rows_shown++; }

        int r = row + offscr_sc_rows;
        int c = rescol;
        int nextcol;
        int fieldlen;
        col = 0;

        for (p = ATBL(tbl, row, col); col <= mxcol; p += nextcol - col, col = nextcol, c += fieldlen) {
            if (col >= maxcols) {
                sc_debug("col:%d >= maxcols:%d in show_content. please check calc_offscr_sc_cols.", col, maxcols);
                break;
            }

            nextcol = col + 1;
            fieldlen = fwidth[col];

            // show cols in case frozen cols are before offscr_sc_cols
            // and ignore col_hidden
            if ( (col < offscr_sc_cols && ! col_frozen[col]) || col_hidden[col] ) { c -= fieldlen; continue; }

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
            if ((currow == row) && (curcol == col)) {
#ifdef USECOLORS
                    if (has_colors()) ui_set_ucolor(win, &ucolors[CELL_SELECTION_SC], ucolors[CELL_SELECTION_SC].bg != DEFAULT_COLOR ? DEFAULT_COLOR : col % 2 == 0 ? ucolors[GRID_EVEN].bg : ucolors[GRID_ODD].bg);
#else
                    wattron(win, A_REVERSE);
#endif
            }

            // setup color for selected range
            int in_range = 0; // this is for coloring empty cells within a range
            srange * s = get_selected_range();
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
                    sprintf(num, "%.*f", precision[col], (*p)->v);
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
               (void) mvprintw(row + RESROW + 1 - offscr_sc_rows, c, "%*.*s", fieldlen, fieldlen, "ERROR");
               align = 0;
               strcpy(text, "ERROR");
               num[0]='\0';
            }
            if ((*p) && (*p)->cellerror == CELLREF) {
               (void) mvprintw(row + RESROW + 1 - offscr_sc_rows, c, "%*.*s", fieldlen, fieldlen, "REF");
               align = 0;
               strcpy(text, "REF");
               num[0]='\0';
            }

            // repaint a blank cell or range
            if ( (currow == row && curcol == col) ||
               ( in_range && row >= ranges->tlrow && row <= ranges->brrow &&
                 col >= ranges->tlcol && col <= ranges->brcol ) ) {
#ifdef USECOLORS
                if (has_colors()) ui_set_ucolor(win, &ucolors[CELL_SELECTION_SC], ucolors[CELL_SELECTION_SC].bg != DEFAULT_COLOR ? DEFAULT_COLOR : col % 2 == 0 ? ucolors[GRID_EVEN].bg : ucolors[GRID_ODD].bg);
#else
                wattron(win, A_REVERSE);
#endif
            }

#ifdef USECOLORS
            if (has_colors() && get_conf_int("underline_grid")) {
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

            for (k=0; k < row_format[row]; k++) {
                for (i = 0; i < fieldlen; ) {
                    w = L' ';
                    j = mvwin_wchnstr (win, count+k+1, c + i, cht, 1);
                    if (j == OK && cht[0].chars[0] != L'\0')
                        w = cht[0].chars[0];
                    //mvwprintw(win, count+k+1, c+i, "%lc", L'*');//w
                    mvwprintw(win, count+k+1, c+i, "%lc", w);
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
                pad_and_align(text, num, fieldlen, align, (*p)->pad, out, row_format[row]);

                // auto wrap
                if (! get_conf_int("truncate") && ! get_conf_int("overlap") && get_conf_int("autowrap")) {
                    int newheight = ceil(wcslen(out) * 1.0 / fwidth[col]);
                    if (row_format[row] < newheight) row_format[row] = newheight;
                }

#ifdef USECOLORS
                if (has_colors() && get_conf_int("underline_grid")) {
                    attr_t attr;
                    short color;
                    wattr_get(win, &attr, &color, NULL);
                    wattr_set(win, attr | A_UNDERLINE, color, NULL);
                }
#endif
                // 'out' may contain the output to fill multiple rows. not just one.
                int count_row = 0;
                wchar_t new[wcslen(out)+1];
                for (count_row = 0; count_row < row_format[row]; count_row++) {
                    int cw = count_width_widestring(out, fieldlen);
                    wcscpy(new, out);
                    if (get_conf_int("truncate") || !get_conf_int("overlap")) new[cw] = L'\0';

                    int whites = fieldlen - cw;
                    while (whites-- > 0) add_wchar(new, L' ', wcslen(new));
                    if (wcslen(new)) mvwprintw(win, count+1+count_row, c, "%ls", new);
                    //wclrtoeol(win);
                    if (cw) del_range_wchars(out, 0, cw-1);
                    if (get_conf_int("overlap")) break;
                }
            }

            if (currow == row && curcol == col) {
                curwinrow = r;
                curwincol = c;
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
        }
        count += row_format[row];
    }
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
 * \return none
 */
void ui_show_celldetails() {
    char head[FBUFLEN];
    int il_pos = 0;

    // show cell in header
    #ifdef USECOLORS
        ui_set_ucolor(input_win, &ucolors[CELL_ID], DEFAULT_COLOR);
    #endif
    sprintf(head, "%s%d ", coltoa(curcol), currow);
    mvwprintw(input_win, 0, 0, "%s", head);
    il_pos += strlen(head);

    // show the current cell's format
    #ifdef USECOLORS
        ui_set_ucolor(input_win, &ucolors[CELL_FORMAT], DEFAULT_COLOR);
    #endif

    register struct ent *p1 = *ATBL(tbl, currow, curcol);

    // show padding
    if (p1 != NULL && p1->pad)
        sprintf(head, "(%d) ", p1->pad);
    else
        head[0]='\0';

    // show format
    if ((p1) && p1->format)
        sprintf(head + strlen(head), "(%s) ", p1->format);
    else
        sprintf(head + strlen(head), "(%d %d %d) ", fwidth[curcol], precision[curcol], realfmt[curcol]);
    mvwprintw(input_win, 0, il_pos, "%s", head);
    il_pos += strlen(head);

    // show expr
    #ifdef USECOLORS
        ui_set_ucolor(input_win, &ucolors[CELL_CONTENT], DEFAULT_COLOR);
    #endif
    if (p1 && p1->expr) {
        linelim = 0;
        editexp(currow, curcol);  /* set line to expr */
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
        head[COLS - il_pos - 1 - 15]='>';
        head[COLS - il_pos - 1 - 14]='>';
        head[COLS - il_pos - 1 - 13]='>';
        head[COLS - il_pos - 1 - 12]='\0';
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
    //char * cfmt = (*p)->format ? (*p)->format : NULL;
    char * cfmt = (*p)->format ? (*p)->format : (realfmt[col] >= 0 && realfmt[col] < COLFORMATS && colformat[realfmt[col]] != NULL) ? colformat[realfmt[col]] : NULL;

    if (cfmt) {
        if (*cfmt == 'd') {
            time_t v = (time_t) ((*p)->v);
            strftime(value, sizeof(char) * FBUFLEN, cfmt + 1, localtime(&v));
            return 0;
        } else {
            format(cfmt, precision[col], (*p)->v, value, sizeof(char) * FBUFLEN);
            return 1;
        }
    } else { // there is no format
        // FIXME: error with number and text in same cell and no overlap
        engformat(realfmt[col], fwidth[col], precision[col], (*p)->v, value, sizeof(char) * FBUFLEN);
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
    if (loading) {
        loading_o=loading;
        loading=0;
        ui_update(0);
        loading=loading_o;
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
//TODO handle frozen rows and columns after "rows" "cols"
#ifdef MOUSE
void ui_handle_mouse(MEVENT event) {
    if (isendwin()) return;

    // if out of range return
    int i, j, r = 0, c = 0;
    if ( event.x < RESCOL || ( get_conf_int("input_bar_bottom") && (event.y == 0 || event.y >= LINES - RESROW)) ||
       ( !get_conf_int("input_bar_bottom") && (event.y <= RESROW))) return;

    // if mode is not handled return
    if (curmode != NORMAL_MODE && curmode != INSERT_MODE && curmode != COMMAND_MODE) return;

    // scroll in normal mode
#ifdef BUTTON5_PRESSED
    if (curmode == NORMAL_MODE && (event.bstate & BUTTON4_PRESSED || // scroll up
        event.bstate & BUTTON5_PRESSED)) { // scroll down
            int n = LINES - RESROW - 1;
            if (get_conf_int("half_page_scroll")) n = n / 2;
            lastcol = curcol;
            lastrow = currow;
            currow = event.bstate & BUTTON5_PRESSED ? forw_row(n)->row : back_row(n)->row;
            if (event.bstate & BUTTON5_PRESSED) scroll_down(n);
            else scroll_up(n);
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

    c = event.x - RESCOL;
    r = event.y - RESROW + (get_conf_int("input_bar_bottom") ? 1 : - 1);

    int mxcol = calc_offscr_sc_cols() + offscr_sc_cols - 1;
    int mxrow = calc_offscr_sc_rows() + offscr_sc_rows - 1;
    int col = 0;
    int row = 0;

    for (i = 0; i <= mxcol; i++) {
        if (i >= maxcols) {
            sc_debug("ui_handle_mouse - i:%d >= maxcols:%d in show_content. please check calc_offscr_sc_cols.", i, maxcols);
            break;
        }
        if (col_hidden[i]) continue;
        if (col_frozen[i] || i >= offscr_sc_cols) col += fwidth[i];
        if (col >= c + 1) break;
    }
    if (i > mxcol) i = mxcol;

    // same for rows
    for (j = 0; j <= mxrow; j++) {
        if (j >= maxrows) {
            sc_debug("ui_handle_mouse - j:%d >= maxrows:%d in ui_show_sc_row_headings. please check calc_offscr_sc_rows.", j, maxrows);
            break;
        }
        if (row_hidden[j]) continue;
        if (row_frozen[j] || j >= offscr_sc_rows) row += row_format[j];
        if (row >= r + 1) break;
    }
    if (j > mxrow) j = mxrow;

    // if in normal mode, change currow and curcol
    if (curmode == NORMAL_MODE) {
        curcol = i;
        currow = j;
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
