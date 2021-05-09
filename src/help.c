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
 * \file help.c
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 2017-07-18
 * \brief Help functions
 */

#ifdef NCURSES
#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <limits.h>

#include "sc.h"
#include "macros.h"
#include "tui.h"
#include "string.h"
#include "utils/string.h"
#include "help.h"
#include "conf.h"

static char ** long_help;
static int delta = 0;
static int max;
static int look_result = -1;
static char word_looked[50] = "";

extern WINDOW * main_win;
extern WINDOW * input_win;
extern WINDOW * input_pad;

/**
 * \brief Load the contents of help_doc into memory
 *
 * \return 0 on success; -1 on error
 */

int load_help () {
    register FILE * f;
    int line;
    int c = 0, count = 0, max_width = COLS;
    char helpfile_path[BUFFERSIZE];

    // we try to read help file in HELP_PATH
    sprintf(helpfile_path, "%s/%s_help", HELP_PATH, SNAME);
    f = fopen(helpfile_path, "r");

    // we try to read help file in current dir
    if (! f ) {
        f = fopen("./doc", "r");
    }

    // last change to read the help file !
    if (! f ) {
        char cwd[PATH_MAX];
        extern char *exepath;
        if (realpath(exepath, cwd) == NULL) return -1;
        char * str_pos = strrchr(cwd, '/');
        if (str_pos == NULL) return -1;
        cwd[str_pos - cwd + 1] = '\0';
        strcat(cwd, "doc");
        f = fopen(cwd, "r");
    }

    if (! f ) return -1;

    // Calculate number of elements in long_help
    while ( (line = getc(f)) != -1) {
        c++;
        if (c >= COLS || line == '\n') {
            c = 0;
            if (max_width < c) max_width = c;
            count++;
        }
    }
    max = count; //max lines in entire help

    rewind(f);

    // Allocate memory for pointers and lines
    long_help = (char **) malloc (sizeof(char *) * (count + 1));

    // Load long_help
    char word[max_width+1];
    word[0] = '\0';
    count = 0;
    while ( (line = getc(f)) != -1) {
        c++;
        add_char(word, line, strlen(word));
        if (c >= COLS || line == '\n') {
            c = 0;
            long_help[count] = (char *) malloc (sizeof(char) * (strlen(word) + 1));
            strcpy(long_help[count], word);
            count++;
            word[0]='\0';
        }
    }
    long_help[count] = (char *) malloc (sizeof(char) * (strlen(word) + 1));
    strcpy(long_help[count], word);

    fclose(f);
    return 0;
}

/**
 * \brief Main function of help
 *
 * \return none
 */

void help() {
    if (load_help() == -1) {
        sc_error("Cannot open help file");
        return;
    }
    delta = 0;

    wmove(input_win, 0,0);
    wclrtobot(input_win);
    wrefresh(input_win);
    wmove(input_pad, 0, 0);
    wclrtoeol(input_pad);
    ui_refresh_pad(0);

    wbkgd(main_win, COLOR_PAIR((ucolors[DEFAULT].fg+1) * (COLORS) + ucolors[DEFAULT].bg + 2));
    ui_set_ucolor(main_win, &ucolors[NORMAL], DEFAULT_COLOR);
    wtimeout(input_win, -1);
    noecho();
    curs_set(0);
    int quit_help_now = 0;

    int option;

    while( ! quit_help_now ) {
        option = show_lines();

        // clear input_win and input_pad
        wmove(input_win, 0,0);
        wclrtobot(input_win);
        wrefresh(input_win);
        wmove(input_pad, 0, 0);
        wclrtoeol(input_pad);
        ui_refresh_pad(0);

        look_result = -1;

        switch (option) {

        case OKEY_ENTER:
        case OKEY_DOWN:
        case 'j':
            if (max >= delta + LINES - 1) delta++;
            break;

        case OKEY_DEL:
        case OKEY_UP:
        case 'k':
            if (delta) delta--;
            break;

        case ' ':
            if (max > delta + LINES + LINES) delta += LINES;
            else if (max > delta + LINES) delta = max - 1 - LINES;
            break;

        case ctl('b'):
        case OKEY_PGUP:
            if (delta - LINES/2 > 0) delta -= LINES/2;
            else if (delta) delta = 0;
            break;

        case ctl('f'):
        case OKEY_PGDOWN:
            if (delta + LINES + LINES/2 < max) delta += LINES/2;
            else if (max > delta + LINES) delta = max - 1 - LINES;
            break;

        case OKEY_END:
        case 'G':
            delta = max - LINES + RESROW;
            break;

        case 'g':
            wtimeout(input_win, TIMEOUT_CURSES);
            char c = wgetch(input_win);
            wtimeout(input_win, -1);
            if (c != 'g') break;

        case ctl('a'):
        case OKEY_HOME:
            delta = 0;
            break;

        case 'n':
            if (strlen(word_looked)) find_word(word_looked, 'f');
            break;

        case 'N':
            if (strlen(word_looked)) find_word(word_looked, 'b');
            break;

        case ':':
            curs_set(1);
            char hline [100];
            hline[0]='\0';
            mvwprintw(input_win, 0, 0, ":%s", hline);
            wclrtoeol(input_win);
            wrefresh(input_win);

            int d = wgetch(input_win);
            while (d != OKEY_ENTER && d != OKEY_ESC) {
                if (d == OKEY_BS || d == OKEY_BS2) {
                    del_char(hline, strlen(hline) - 1);
                } else {
                    sprintf(hline + strlen(hline), "%c", d);
                }
                mvwprintw(input_win, 0, 0, ":%s", hline);
                wclrtoeol(input_win);
                wrefresh(input_win);
                d = wgetch(input_win);
            }
            if (d == OKEY_ENTER && ( ! strcmp(hline, "q") || ! strcmp(hline, "quit") || ! strcmp(hline, "q!") )) {
                quit_help_now = TRUE;
            } else if (d == OKEY_ESC) {
                wmove(input_win, 0, 0);
                wclrtoeol(input_win);
                wrefresh(input_win);
                //d = wgetch(input_win);
            }
            curs_set(0);
            break;

        case '/':
            curs_set(1);
            word_looked[0]='\0';
            mvwprintw(input_win, 0, 0, "/%s", word_looked);
            wrefresh(input_win);
            d = wgetch(input_win);
            while (d != OKEY_ENTER && d != OKEY_ESC) {
                if (d == OKEY_BS || d == OKEY_BS2) {
                    del_char(word_looked, strlen(word_looked) - 1);
                } else {
                    //sprintf(word_looked, "%s%c", word_looked, d);
                    sprintf(word_looked + strlen(word_looked), "%c", d);
                }
                mvwprintw(input_win, 0, 0, "/%s", word_looked);
                wclrtoeol(input_win);
                wrefresh(input_win);
                d = wgetch(input_win);
            }

            if (d == OKEY_ENTER && strlen(word_looked)) {
                find_word(word_looked, 'f');
            }
            mvwprintw(input_win, 0, 0, "");
            wclrtoeol(input_win);
            wrefresh(input_win);
            curs_set(0);
            break;

        case 'q':
            quit_help_now = TRUE;
            curs_set(0);
            break;

        }
    }

    // free memory allocated
    for (delta = 0; delta <= max; delta++) {
       free(long_help[delta]);
    }
    free(long_help);

    // go back to spreadsheet
    wtimeout(input_win, TIMEOUT_CURSES);
    wmove(input_win, 0,0);
    wclrtobot(input_win);
    wmove(main_win, 0,0);
    wclrtobot(main_win);
    wrefresh(main_win);
}

/**
 * \brief TODO Document find_word()
 *
 * \param[in] word
 * \param[in] order
 *
 * \return none
 */

void find_word(char * word, char order) {
    int i;
    if (order == 'f') {        // forward
        for (i = delta + 1; i < max - 1; i++) {
            if ((look_result = str_in_str(long_help[i], word)) >= 0) {
                delta = i;
                //sc_info("");
                break;
            }
        }
    } else if (order == 'b') { // backwards
        for (i = delta - 1; i > 0; i--) {
            if ((look_result = str_in_str(long_help[i], word)) >= 0) {
                delta = i;
                //sc_info("");
                break;
            }
        }
    }

    if (look_result == -1) {
        sc_info("Pattern not found.");
    }
    return;
}

/**
 * \brief show_lines() show text and ask for input from stdin
 * \return int - wgetch
 */

int show_lines() {
    int lineno, i, k, key = 0, bold = 0 ;

    for (lineno = 0; lineno + delta <= max && long_help[lineno + delta] && lineno < LINES - RESROW; lineno++) {
        if (strlen(word_looked))
            look_result =
                str_in_str(
                        long_help[lineno + delta],
                        word_looked);

        wmove(main_win, lineno, 0);
        wclrtoeol(main_win);

        for (i=0; long_help[lineno + delta][i] != '\0'; i++) {

            if (long_help[lineno + delta][i] == '&') bold = ! bold;

            #ifdef USECOLORS
            bold && ! key?
            ui_set_ucolor(main_win, &ucolors[HELP_HIGHLIGHT], DEFAULT_COLOR) :
            ui_set_ucolor(main_win, &ucolors[NORMAL], DEFAULT_COLOR);
            #endif

            if (long_help[lineno + delta][i] == '<' || long_help[lineno + delta][i] == '{') {
                // do not colorize if not '>' or '}' in line
                for (key = 1, k=i; long_help[lineno + delta][k] != '\0'; k++) {
                    if (long_help[lineno + delta][k] == '\'')        { key = 0; break; }
                    else if (long_help[lineno + delta][k] == ';')    { key = 0; break; }
                    else if (long_help[lineno + delta][k] == '>')    { break; }
                    else if (long_help[lineno + delta][k] == '}')    { break; }
                    else if (long_help[lineno + delta][k+1] == '\0') { key = 0; break; }
                }
            }

            if (long_help[lineno + delta][i] == '&') {
                #ifdef USECOLORS
                ui_set_ucolor(main_win, &ucolors[NORMAL], DEFAULT_COLOR);
                #endif
                continue;
            } else if (look_result != -1 && i >= look_result &&
                i < look_result + strlen(word_looked) ) {
                #ifdef USECOLORS
                ui_set_ucolor(main_win, &ucolors[HELP_HIGHLIGHT], DEFAULT_COLOR);
                #endif
            } else if (key) {
                #ifdef USECOLORS
                ui_set_ucolor(main_win, &ucolors[NUMB], DEFAULT_COLOR);
                #endif
            }

            mvwprintw(main_win, lineno, i, "%c", long_help[lineno + delta][i]);
            if (long_help[lineno + delta][i] == '>' || long_help[lineno + delta][i] == '}') key = 0;
        }
        wclrtoeol(main_win);
    }
    if (lineno < LINES) {
        wmove(main_win, lineno+1, 0);
        wclrtobot(main_win);
    }

    wmove(main_win, 0, 0);
    (void) wrefresh(main_win);
    return wgetch(input_win);
}
#else
// implement this function if want to create another UI

/**
 * @brief Template for a second help() function
 *
 * returns: none
 */

void help() {
}
#endif
