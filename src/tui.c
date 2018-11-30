/*******************************************************************************
 * Copyright (c) 2013-2017, Andrés Martinelli <andmarti@gmail.com              *
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
 * \brief TODO Write a brief file description.
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
 *
 * these are local functions that might not be needed to reimplement if writing another ui:
 * ui_set_ucolor          // function called internally for setting a color
 * ui_show_content
 * ui_show_sc_col_headings
 * ui_add_cell_detail     // Add details of an ent to a char * received as a parameter. used for input_win
 *
 * ANYONE WHO WANTS TO PORT THIS TO ANOTHER UI, WOULD JUST NEED TO REIMPLEMENT THIS FILE
 * AND HELP() IN HELP.C
 *
 *
 * if not working with ncurses, you should also have to define LINES and COLS macros in Xui.h as well.
 * see ui example inside /files folder
 */

#include <string.h>
#include <ncurses.h>
#include <stdio.h>
#include <time.h>
#include <locale.h>
#include <stdlib.h>
#include <stdarg.h>

#include "main.h"
#include "conf.h"
#include "input.h"
#include "tui.h"
#include "range.h"
#include "sc.h"
#include "cmds.h"
#include "cmds_visual.h"
#include "conf.h"
#include "version.h"
#include "file.h"
#include "format.h"
#include "utils/string.h"

extern struct dictionary * d_colors_param;
extern int cmd_pending;
extern int cmd_multiplier;
extern char insert_edit_submode;

WINDOW * main_win;
WINDOW * input_win;
SCREEN * sstderr;
SCREEN * sstdout;
srange * ranges;

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

    main_win = newwin(LINES - RESROW, COLS, RESROW, 0);
    input_win = newwin(RESROW, COLS, 0, 0); // just 2 rows (RESROW = 2)

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
        wbkgd(main_win, COLOR_PAIR((ucolors[DEFAULT].fg+1) * 9 + ucolors[DEFAULT].bg + 2));
        wbkgd(input_win, COLOR_PAIR((ucolors[DEFAULT].fg+1) * 9 + ucolors[DEFAULT].bg + 2));
    }
    #endif

    wtimeout(input_win, TIMEOUT_CURSES);
    noecho();
    curs_set(0);

    #ifndef NETBSD
    if ((char *) getenv ("ESCDELAY") == NULL) set_escdelay(ESC_DELAY);
    #endif
    cbreak();
    keypad(input_win, 1);
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
    return wget_wch(input_win, wd);
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
    wtimeout(input_win, -1);
    move(0, rescol + inputline_pos + 1);
    wget_wch(input_win, wd);
    wtimeout(input_win, TIMEOUT_CURSES);
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
    if (type == DEBUG_MSG && ! atoi(get_conf_value("debug"))) return;
    char t[BUFFERSIZE];
    va_list args;
    va_start(args, type);
    vsprintf (t, s, args);
    if ( ! atoi(get_conf_value("nocurses"))) {
#ifdef USECOLORS
        if (type == ERROR_MSG)
            ui_set_ucolor(input_win, &ucolors[ERROR_MSG]);
        else
            ui_set_ucolor(input_win, &ucolors[INFO_MSG]);
#endif
        mvwprintw(input_win, 1, 0, "%s", t);
        wclrtoeol(input_win);

        if (type == DEBUG_MSG) {
            wtimeout(input_win, -1);
            wgetch(input_win);
            wtimeout(input_win, TIMEOUT_CURSES);
        }
        wrefresh(input_win);

    } else if (type == VALUE_MSG && get_conf_value("output") != NULL && fdoutput != NULL) {
        fwprintf(fdoutput, L"%s\n", t);
    } else if (type == VALUE_MSG) {
        if (fwide(stdout, 0) >0)
            //wprintf(L"wide %s\n", t);
            wprintf(L"%s\n", t);
        else
            //printf("nowide %s\n", t);
            printf("%s\n", t);
        fflush(stdout);
    } else {
        if (fwide(stderr, 0) >0)
            //wprintf(L"wide %s\n", t);
            fwprintf(stderr, L"%s\n", t);
        else
            //printf("nowide %s\n", t);
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
    wbkgd(main_win, COLOR_PAIR((ucolors[DEFAULT].fg+1) * 9 + ucolors[DEFAULT].bg + 2));
    wbkgd(input_win, COLOR_PAIR((ucolors[DEFAULT].fg+1) * 9 + ucolors[DEFAULT].bg + 2));
    #endif

    // show headings
    int mxcol = offscr_sc_cols + calc_offscr_sc_cols() - 1;
    int mxrow = offscr_sc_rows + calc_offscr_sc_rows() - 1;
    ui_show_sc_col_headings(main_win, mxcol);
    ui_show_sc_row_headings(main_win, mxrow);

    #ifdef USECOLORS
    ui_set_ucolor(main_win, &ucolors[WELCOME]);
    #endif

    // show message
    mvwaddstr(main_win, LINES/2-3, COLS/2-strlen(msg_title)/2  , msg_title);
    mvwaddstr(main_win, LINES/2-2, COLS/2-strlen(msg_by)/2     , msg_by);
    mvwaddstr(main_win, LINES/2-1, COLS/2-strlen(msg_version)/2, msg_version);

    #ifdef USECOLORS
    ui_set_ucolor(main_win, &ucolors[WELCOME]);
    #endif
    for (i=0; msg_help[i] != '\0'; i++) {
        if (msg_help[i] == '<') {
            #ifdef USECOLORS
            ui_set_ucolor(main_win, &ucolors[NUMB]);
            #endif
        }
        mvwaddstr(main_win, LINES/2, COLS/2-strlen(msg_help)/2+i, &msg_help[i]);
        if (msg_help[i] == '>') {
            #ifdef USECOLORS
            ui_set_ucolor(main_win, &ucolors[WELCOME]);
            #endif
        }
    }
    for (i=0; msg_help2[i] != '\0'; i++) {
        if (msg_help2[i] == '<') {
            #ifdef USECOLORS
            ui_set_ucolor(main_win, &ucolors[NUMB]);
            #endif
        }
        mvwaddstr(main_win, LINES/2+1, COLS/2-strlen(msg_help2)/2+i, &msg_help2[i]);
        if (msg_help2[i] == '>') {
            #ifdef USECOLORS
            ui_set_ucolor(main_win, &ucolors[WELCOME]);
            #endif
        }
    }
    wrefresh(main_win);
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
    if (atoi(get_conf_value("nocurses"))) return;

    if (header) {
    #ifdef USECOLORS
        wbkgd(main_win, COLOR_PAIR((ucolors[DEFAULT].fg+1) * 9 + ucolors[DEFAULT].bg + 2));
        // comment this to prevent bold to be reset
        //wbkgd(input_win, COLOR_PAIR((ucolors[DEFAULT].fg+1) * 9 + ucolors[DEFAULT].bg + 2));
    #endif
        // Clean from top to bottom
        wmove(main_win, 0, rescol);
        wclrtobot(main_win);
        // comment this to prevent info message to be erased
        //wmove(input_win, 0, 0);
        //wclrtobot(input_win);
        ui_show_celldetails(); // always before ui_print_mode
        ui_print_mode();
    }

    /*
     * Calculate offscreen rows and columns
     *
     * mxcol is the last visible column in screen grid.
     * for instance, if mxcol is 8, last visible column would be I
     * mxrow is the last visible row in screen grid
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
    int off_cols = calc_offscr_sc_cols();
    int off_rows = calc_offscr_sc_rows();
    int mxcol = offscr_sc_cols + off_cols - 1;
    int mxrow = offscr_sc_rows + off_rows - 1;
    //sc_debug("out: off:%d, center:%d, mxcol:%d %s, maxcols:%d", offscr_sc_cols, center_hidden_cols, mxcol, coltoa(mxcol), maxcols);

    /* You can't hide the last row or col */
    while (row_hidden[currow])
        currow++;
    while (col_hidden[curcol])
        curcol++;

    // Show the content of the cells
    // Numeric values, strings.
    ui_show_content(main_win, mxrow, mxcol);

    // Show sc_col headings: A, B, C, D..
    ui_show_sc_col_headings(main_win, mxcol);

    // Show sc_row headings: 0, 1, 2, 3..
    ui_show_sc_row_headings(main_win, mxrow);

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
    getyx(input_win, row_orig, col_orig);

    #ifdef USECOLORS
    ui_set_ucolor(input_win, &ucolors[MODE]);
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

    mvwprintw(input_win, 0, 0, "%s", strm);

    // Return cursor to previous position
    wmove(input_win, row_orig, col_orig);
    wrefresh(input_win);
}

/**
 * \brief Show first and second row (header). Handle cursor position.
 *
 * \return none
 */

void ui_show_header() {
    ui_clr_header(0);
    ui_clr_header(1);

    ui_print_mult_pend();

    // Show current mode
    ui_print_mode();

    // Print input text
    switch (curmode) {
        case COMMAND_MODE:
            mvwprintw(input_win, 0, rescol, ":%ls", inputline);
            wmove(input_win, 0, inputline_pos + 1 + rescol);
            break;
        case INSERT_MODE:
            mvwprintw(input_win, 0, 1 + rescol, "%ls", inputline);
            wmove(input_win, 0, inputline_pos + 1 + rescol);
            break;
        case EDIT_MODE:
            mvwprintw(input_win, 0, rescol, " %ls", inputline);
            wmove(input_win, 0, inputline_pos + 1 + rescol);
    }
    wrefresh(input_win);
    return;
}

/**
 * \brief Clean a whole row
 *
 * \param[in] i Line to clean
 *
 * \return none
 */

void ui_clr_header(int i) {
    int row_orig, col_orig;
    getyx(input_win, row_orig, col_orig);
    if (col_orig > COLS) col_orig = COLS - 1;

    wmove(input_win, i, 0);
    wclrtoeol(input_win);

    // Return cursor to previous position
    wmove(input_win, row_orig, col_orig);

    return;
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
    char strm[22] = "";

    #ifdef USECOLORS
    ui_set_ucolor(input_win, &ucolors[MODE]);
    #endif

    if (curmode == NORMAL_MODE) {
        strcat(strm, " -- NORMAL --");
        ui_write_j(input_win, strm, row, RIGHT);

    } else if (curmode == INSERT_MODE) {
        strcat(strm, " -- INSERT --");
        ui_write_j(input_win, strm, row, RIGHT);

        #ifdef USECOLORS
        ui_set_ucolor(input_win, &ucolors[INPUT]);
        #endif
        // Show submode (INSERT)
        mvwprintw(input_win, 0, 0 + rescol, "%c", insert_edit_submode);
        //wmove(input_win, 0, 1); commented on 01/06

    } else if (curmode == EDIT_MODE) {
        strcat(strm, "   -- EDIT --");
        ui_write_j(input_win, strm, row, RIGHT);

    } else if (curmode == VISUAL_MODE) {
        strcat(strm, " -- VISUAL --");
        if (visual_submode != '0')
            strcpy(strm, " << VISUAL >>");
        ui_write_j(input_win, strm, row, RIGHT);

    } else if (curmode == COMMAND_MODE) {
        strcat(strm, "-- COMMAND --");

        ui_write_j(input_win, strm, row, RIGHT);
        #ifdef USECOLORS
        ui_set_ucolor(input_win, &ucolors[INPUT]);
        #endif
        // muestro ':'
        mvwprintw(input_win, 0, 0 + rescol, ":");
        wmove(input_win, 0, 1 + rescol);
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
    int row = 0;
    #ifdef USECOLORS
    if (has_colors()) ui_set_ucolor(win, &ucolors[HEADINGS]);
    #endif
    int i;
    int freeze = freeze_ranges && (freeze_ranges->type == 'r' ||  freeze_ranges->type == 'a') ? 1 : 0;

    //for (i = 0; i < mxrow && i < maxrows; i++) {
    for (i = 0; i <= mxrow; i++) {
        if (i >= maxrows) {
            sc_error("i >= maxrows in ui_show_sc_row_headings. please check calc_offscr_sc_rows.");
            break;
        }
        // print rows in case freezen rows are before offscr_sc_rows
        if (i < offscr_sc_rows && !(freeze && i >= freeze_ranges->tl->row && i <= freeze_ranges->br->row)) continue;

        if (row_hidden[i]) continue;

        // skip center_hidden_rows
        if (freeze && ((
         i > freeze_ranges->br->row && i <= freeze_ranges->br->row + center_hidden_rows) || (
         i < freeze_ranges->tl->row && i >= freeze_ranges->tl->row - center_hidden_rows))) continue;

        srange * s = get_selected_range();
        if ( (s != NULL && i >= s->tlrow && i <= s->brrow) || i == currow ) {
            #ifdef USECOLORS
            if (has_colors()) ui_set_ucolor(win, &ucolors[CELL_SELECTION]);
            #else
            wattron(win, A_REVERSE);
            #endif
        }
        mvwprintw (win, row+1, 0, "%*d ", rescol-1, i);

        #ifdef USECOLORS
        if (has_colors()) ui_set_ucolor(win, &ucolors[HEADINGS]);
        #else
        wattroff(win, A_REVERSE);
        #endif
        row++;
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
    int freeze = freeze_ranges && (freeze_ranges->type == 'c' ||  freeze_ranges->type == 'a') ? 1 : 0;

    #ifdef USECOLORS
    if (has_colors()) ui_set_ucolor(win, &ucolors[HEADINGS]);
    #endif

    wmove(win, 0, 0);
    wclrtoeol(win);

    //for (i = 0; i <= mxcol && i < maxcols; i++) {
    for (i = 0; i <= mxcol; i++) {
        if (i >= maxcols) {
            sc_error("i >= maxcols in ui_show_sc_col_headings. please check calc_offscr_sc_cols.");
            break;
        }
        // print cols in case freezen columns are before offscr_sc_cols
        if (i < offscr_sc_cols && !(freeze && i >= freeze_ranges->tl->col && i <= freeze_ranges->br->col)) continue;

        if (col_hidden[i]) continue;

        // skip center_hidden_cols
        if (freeze && ((
         i > freeze_ranges->br->col && i <= freeze_ranges->br->col + center_hidden_cols) || (
         i < freeze_ranges->tl->col && i >= freeze_ranges->tl->col - center_hidden_cols))) continue;

        int k = fwidth[i] / 2;
        srange * s = get_selected_range();
        if ( (s != NULL && i >= s->tlcol && i <= s->brcol) || i == curcol ) {
            #ifdef USECOLORS
            if (has_colors()) ui_set_ucolor(win, &ucolors[CELL_SELECTION]);
            #else
            wattron(win, A_REVERSE);
            #endif
        }
        (void) mvwprintw(win, 0, col, "%*s%-*s", k-1, " ", fwidth[i] - k + 1, coltoa(i));

        col += fwidth[i];
        if (i == mxcol && COLS - col > 0)
            wclrtoeol(win);

        #ifdef USECOLORS
        if (has_colors()) ui_set_ucolor(win, &ucolors[HEADINGS]);
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
    int row, col;
    int q_row_hidden = 0;
    int freezer = freeze_ranges && (freeze_ranges->type == 'r' ||  freeze_ranges->type == 'a') ? 1 : 0;
    int freezec = freeze_ranges && (freeze_ranges->type == 'c' ||  freeze_ranges->type == 'a') ? 1 : 0;

    //for (row = 0; row <= mxrow && row < maxrows; row++) {
    for (row = 0; row <= mxrow; row++) {
        if (row >= maxrows) {
            sc_error("i >= maxrows in show_content. please check calc_offscr_sc_rows.");
            break;
        }

        if (row < offscr_sc_rows
            && (freezer
            && row >= freeze_ranges->tl->row
            && row <= freeze_ranges->br->row)) {
            q_row_hidden--;
            continue;
        } else if (row < offscr_sc_rows
            && !(freezer
            && row >= freeze_ranges->tl->row
            && row <= freeze_ranges->br->row)) {
            //q_row_hidden++;
            continue;
        }
        if (row_hidden[row]) {
            q_row_hidden++;
            continue;
        }

        // skip center_hidden_cols
        if (freezer && (
        (row >= freeze_ranges->br->row && row > freeze_ranges->br->row && row <= freeze_ranges->br->row + center_hidden_rows) ||
        (row <= freeze_ranges->tl->row && row < freeze_ranges->tl->row && row >= freeze_ranges->tl->row - center_hidden_rows))) {
            q_row_hidden++;
            continue;
        }

        register int c = rescol;
        int nextcol;
        int fieldlen;
        col = 0;

        for (p = ATBL(tbl, row, col); col <= mxcol; p += nextcol - col, col = nextcol, c += fieldlen) {
        //for (p = ATBL(tbl, row, col); col <= mxcol && col < maxcols; p += nextcol - col, col = nextcol, c += fieldlen) {
            if (col >= maxcols) {
                sc_error("i >= maxcols in show_content. please check calc_offscr_sc_cols.");
                break;
            }

            nextcol = col + 1;
            fieldlen = fwidth[col];

            // print cols in case freezen columns are before offscr_sc_cols
            if (col < offscr_sc_cols
                && !(freezec
                && col >= freeze_ranges->tl->col
                && col <= freeze_ranges->br->col)) {
                c -= fieldlen;
                continue;
            }

            if (col_hidden[col]) {
                c -= fieldlen;
                continue;
            }

            // skip center_hidden_cols
            if (freezec &&
            ((col >= freeze_ranges->br->col && col > freeze_ranges->br->col && col <= freeze_ranges->br->col + center_hidden_cols) ||
             (col <= freeze_ranges->tl->col && col < freeze_ranges->tl->col && col >= freeze_ranges->tl->col - center_hidden_cols))) {
                c -= fieldlen;
                continue;
            }

            //if ( (*p) == NULL) *p = lookat(row, col);

            // Clean format
            #ifdef USECOLORS
            if ((*p) && (*p)->cellerror) {                                  // cellerror
                ui_set_ucolor(win, &ucolors[CELL_ERROR]);
            } else if ((*p) && (*p)->v < 0) {     // cell negative
                ui_set_ucolor(win, &ucolors[CELL_NEGATIVE]);
            } else if ((*p) && (*p)->expr) {
                ui_set_ucolor(win, &ucolors[EXPRESSION]);
            } else if ((*p) && (*p)->label) {                               // string
                ui_set_ucolor(win, &ucolors[STRG]);
            } else if ((*p) && (*p)->flags & is_valid && ! (*p)->format) {  // numeric value
                ui_set_ucolor(win, &ucolors[NUMB]);
            } else if ((*p) && (*p)->format && (*p)->format[0] == 'd') {    // date format
                ui_set_ucolor(win, &ucolors[DATEF]);
            } else {
                ui_set_ucolor(win, &ucolors[NORMAL]);
            }
            #endif

            // Cell color!
            if ((*p) && (*p)->ucolor != NULL) {
                ui_set_ucolor(win, (*p)->ucolor);
            }

            // Color selected cell
            if ((currow == row) && (curcol == col)) {
                #ifdef USECOLORS
                    if (has_colors()) ui_set_ucolor(win, &ucolors[CELL_SELECTION_SC]);
                #else
                    wattron(win, A_REVERSE);
                #endif
            }

            // Color selected range
            int in_range = 0; // this is for coloring empty cells within a range
            srange * s = get_selected_range();
            if (s != NULL && row >= s->tlrow && row <= s->brrow && col >= s->tlcol && col <= s->brcol ) {
                #ifdef USECOLORS
                    ui_set_ucolor(win, &ucolors[CELL_SELECTION_SC]);
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
                    ui_set_ucolor(win, &ucolors[CELL_SELECTION_SC]);
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
                //show_numeric_content_of_cell(win, p, col, row + 1 - offscr_sc_rows - q_row_hidden, c);

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
               (void) mvprintw(row + RESROW + 1 - offscr_sc_rows, c, "%*.*s", fwidth[col], fwidth[col], "ERROR");
               align = 0;
               strcpy(text, "ERROR");
               num[0]='\0';
            }
            if ((*p) && (*p)->cellerror == CELLREF) {
               (void) mvprintw(row + RESROW + 1 - offscr_sc_rows, c, "%*.*s", fwidth[col], fwidth[col], "REF");
               align = 0;
               strcpy(text, "REF");
               num[0]='\0';
            }


            // repaint a blank cell, because of in range, or because we have a coloured empty cell!
            if ( !(*p) || (( !((*p)->flags & is_valid) && !(*p)->label ) && !((*p)->cellerror == CELLERROR)) ) {
                if ( (currow == row && curcol == col) ||
                ( in_range && row >= ranges->tlrow && row <= ranges->brrow &&
                col >= ranges->tlcol && col <= ranges->brcol ) ) {
                    #ifdef USECOLORS
                    if (has_colors()) ui_set_ucolor(win, &ucolors[CELL_SELECTION_SC]);
                    #else
                    wattron(win, A_REVERSE);
                    #endif
                } else if ( !(*p) || (*p)->ucolor == NULL) {
                    #ifdef USECOLORS
                    ui_set_ucolor(win, &ucolors[STRG]); // When a long string does not fit in column.
                    #endif
                }

                // new implementation for wide char support
                cchar_t cht[fieldlen];
                wchar_t w;
                int i, j;
                for (i = 0; i < fieldlen; ) {
                    w = L' ';
                    j = mvwin_wchnstr (win,  row + 1 - offscr_sc_rows - q_row_hidden, c + i, cht, 1);
                    if (j == OK && cht[0].chars[0] != L'\0')
                        w = cht[0].chars[0];
                    mvwprintw(win, row + 1 - offscr_sc_rows - q_row_hidden, c+i, "%lc", w);
                    i+= wcwidth(w);
                }

            // we print text and number
            } else {
                pad_and_align(text, num, fwidth[col], align, (*p)->pad, out);
                if (col == mxcol && wcswidth(out, wcslen(out)) > fwidth[col])
                    out[ count_width_widestring(out, fwidth[col]) ] = L'\0';

                mvwprintw(win, row + 1 - offscr_sc_rows - q_row_hidden, c, "%ls", out);
                wclrtoeol(win);
            }

            // clean format
            #ifndef USECOLORS
                wattroff(win, A_REVERSE);
            #endif
        }
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
 *
 * \return none
 */

void ui_show_celldetails() {
    char head[FBUFLEN];
    int inputline_pos = 0;

    // show cell in header
    #ifdef USECOLORS
        ui_set_ucolor(input_win, &ucolors[CELL_ID]);
    #endif
    sprintf(head, "%s%d ", coltoa(curcol), currow);
    mvwprintw(input_win, 0, 0 + rescol, "%s", head);
    inputline_pos += strlen(head) + rescol;

    // show the current cell's format
    #ifdef USECOLORS
        ui_set_ucolor(input_win, &ucolors[CELL_FORMAT]);
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
    mvwprintw(input_win, 0, inputline_pos, "%s", head);
    inputline_pos += strlen(head);

    // show expr
    #ifdef USECOLORS
        ui_set_ucolor(input_win, &ucolors[CELL_CONTENT]);
    #endif
    if (p1 && p1->expr) {
        linelim = 0;
        editexp(currow, curcol);  /* set line to expr */
        linelim = -1;
        sprintf(head, "[%.*s] ", FBUFLEN-4, line);
        mvwprintw(input_win, 0, inputline_pos, "%s", head);
        inputline_pos += strlen(head);
    }
    // add cell content to head string
    head[0] = '\0';
    ui_add_cell_detail(head, p1);

    // cut string if its too large!
    if (strlen(head) > COLS - inputline_pos - 1) {
        head[COLS - inputline_pos - 1 - 15]='>';
        head[COLS - inputline_pos - 1 - 14]='>';
        head[COLS - inputline_pos - 1 - 13]='>';
        head[COLS - inputline_pos - 1 - 12]='\0';
    }

    mvwprintw(input_win, 0, inputline_pos, "%s", head);
    wclrtoeol(input_win);
    //wrefresh(input_win);
}

/**
 * \brief Error routine for yacc (gram.y)
 *
 * \param[in] err

 * \return none
 */

void yyerror(char * err) {
    mvwprintw(input_win, 1, 0, "%s: %.*s<=%s", err, linelim, line, line + linelim);
    wrefresh(input_win);
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
        return -1;
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
}



/**
 * \brief
 * UI function thats called after SIGWINCH signal.
 * \return none
 */
void sig_winchg() {
    endwin();
    refresh();
    clear();
    set_term(sstdout);

    wresize(main_win, LINES - RESROW, COLS);
    wresize(input_win, RESROW, COLS);
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
 * confirmation between a coupld of options defined on valie (wchar *).
 *
 * \return wchar_t indicating user answer
 */

wchar_t ui_query_opt(wchar_t * initial_msg, wchar_t * valid) {
    if (! wcslen(initial_msg)) return L'\0';
    sc_info("%ls", initial_msg);
    wint_t wd = -1;
    wchar_t wdc[2] = L"";
    int res;

    curs_set(1);
    wtimeout(input_win, -1);
    move(0, rescol + wcslen(initial_msg) + 1);
    while ((res = wstr_in_wstr(valid, wdc)) == -1) {
        wget_wch(input_win, &wd);
        swprintf(wdc, FBUFLEN, L"%lc", wd);
    }
    wtimeout(input_win, TIMEOUT_CURSES);
    curs_set(0);
    return wdc[0];
}

/**
 * \brief Read text from stdin
 *
 * \param[in] initial_msg
 *
 * \return user input
 */

char * ui_query(char * initial_msg) {
    char * hline = (char *) malloc(sizeof(char) * BUFFERSIZE);
    hline[0]='\0';

    // curses is not enabled
    if ( atoi(get_conf_value("nocurses"))) {
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
    wtimeout(input_win, -1);
    notimeout(input_win, TRUE);
    wmove(input_win, 0, rescol);
    wclrtoeol(input_win);
    wrefresh(input_win);
    int d = wgetch(input_win);

    while (d != OKEY_ENTER && d != OKEY_ESC) {
        if (d == OKEY_BS || d == OKEY_BS2) {
            del_char(hline, strlen(hline) - 1);
        } else {
            sprintf(hline + strlen(hline), "%c", d);
        }

        mvwprintw(input_win, 0, rescol, "%s", hline);
        wclrtoeol(input_win);
        wrefresh(input_win);
        d = wgetch(input_win);
    }
    if (d == OKEY_ESC) hline[0]='\0';

    // go back to spreadsheet
    noecho();
    curs_set(0);
    wtimeout(input_win, TIMEOUT_CURSES);
    wmove(input_win, 0,0);
    wclrtoeol(input_win);
    wmove(input_win, 1,0);
    wclrtoeol(input_win);
    wrefresh(input_win);
    return hline;
}

/**
 * \brief Set a color
 *
 * \param[in] w
 * \param[in] uc
 *
 * \return none
 */

void ui_set_ucolor(WINDOW * w, struct ucolor * uc) {
    short color;
    long attr = A_NORMAL;
    if (uc->bold)      attr |= A_BOLD;
    if (uc->dim)       attr |= A_DIM;
    if (uc->reverse)   attr |= A_REVERSE;
    if (uc->standout)  attr |= A_STANDOUT;
    if (uc->blink)     attr |= A_BLINK;
    if (uc->underline) attr |= A_UNDERLINE;
    if (uc->bg == NONE_COLOR || uc->fg == NONE_COLOR) {
        // get current window color;
        attr_t a;
        wattr_get(w, &a, &color, NULL);
    } else {
        color = (uc->fg+1)*9 + uc->bg + 2;
    }
    wattrset (w, attr | COLOR_PAIR(color));
}

/**
 * \brief TODO Document ui_start_colors()
 *
 * \return none
 */
void ui_start_colors() {
    if (! has_colors()) return;
    int i, j;
    // Initialize all possible 81 init pairs
    use_default_colors();
    for (i=0; i < 9; i++)      // fg
        for (j=0; j < 9; j++)  // bg
            init_pair( i*9+j+1, i-1, j-1); // i is fg and j is bg
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
    refresh();
    clearok(stdscr, TRUE);
    ui_show_header();
    ui_update(TRUE);
    return;
}

