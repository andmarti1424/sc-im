#include <curses.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include "sc.h"
#include "macros.h"
#include "screen.h"
#include "string.h"
#include "color.h"
#include "utils/string.h"


static char ** long_help;
static int delta = 0;
static int max; 
static int look_result = -1;
static char word_looked[50] = "";

int show_lines();
void find_word(char * word, char order);

// cargo el contenido del archivo help_doc a memoria
void load_help () {
    register FILE * f;
    char line;
    int c = 0, count = 0, max_width = COLS;

    f = fopen("help_doc", "r");
    if (!f) return;

    // cuento cantidad de elementos que tendra long_help
    while ( (line = getc(f)) != -1) {
        c++;
        if (c > COLS || line == '\n') {
            c = 0;
            if (max_width < c) max_width = c;
            count++;
        }
    }
    max = count; //max lines in entire help

    rewind(f);
    
    // guardo memoria para tantos punteros como lineas
    long_help = (char **) malloc (sizeof(char *) * (count + 2));
    
    // cargo long_help
    char word[max_width+1];
    word[0] = '\0';
    count = 0;
    while ( (line = getc(f)) != -1) {
        c++;
        add_char(word, line, strlen(word));
        if (c > COLS || line == '\n') {
            c = 0;
            long_help[count] = (char *) malloc (sizeof(char) * (strlen(word) + 1));
            //long_help[count][0]='\0';
            strcpy(long_help[count], word);
            count++;
            word[0]='\0';
        }
    }
    long_help[count] = (char *) 0;

    fclose(f);
    return;
}

// main function of help
void help() {
    load_help();
    delta = 0;

    wmove(input_win, 0,0);
    wclrtobot(input_win);
    wrefresh(input_win);

    set_ucolor(main_win, NORMAL);
    wtimeout(input_win, -1);
    noecho();
    curs_set(0);
    int quit_help_now = 0;

    int option;

    while( ! quit_help_now ) {
        option = show_lines();
        look_result = -1;

        switch (option) {

        case OKEY_ENTER:
        case OKEY_DOWN:
        case 'j':
            if (max > delta + LINES + 1) delta++;
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
            if (delta - LINES/2 > 0) delta -= LINES/2;
            else if (delta) delta = 0;
            break;

        case ctl('f'):
            if (delta + LINES + LINES/2 < max) delta += LINES/2;
            else if (max > delta + LINES) delta = max - 1 - LINES;
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
            mvwprintw(input_win, 0, rescol, ":%s", hline);
            wclrtoeol(input_win);
            wrefresh(input_win);

            int d = wgetch(input_win);
            while (d != OKEY_ENTER && d != OKEY_ESC) {
                if (d == OKEY_BS) {
                    del_char(hline, strlen(hline) - 1);
                } else {
                    sprintf(hline, "%s%c", hline, d);
                }
                mvwprintw(input_win, 0, rescol, ":%s", hline);
                wclrtoeol(input_win);
                wrefresh(input_win);
                d = wgetch(input_win);
            }
            if (d == OKEY_ENTER && ( strcmp(hline, "q") == 0 || strcmp(hline, "quit") == 0 )) {
                quit_help_now = TRUE;
            }
            break;

        case '/':
            curs_set(1);
            word_looked[0]='\0';
            mvwprintw(input_win, 0, rescol, "/%s", word_looked);
            wrefresh(input_win);
            d = wgetch(input_win);
            while (d != OKEY_ENTER && d != OKEY_ESC) {
                if (d == OKEY_BS) {
                    del_char(word_looked, strlen(word_looked) - 1);
                } else {
                    sprintf(word_looked, "%s%c", word_looked, d);
                }
                mvwprintw(input_win, 0, rescol, "/%s", word_looked);
                wclrtoeol(input_win);
                wrefresh(input_win);
                d = wgetch(input_win);
            }

            if (d == OKEY_ENTER && strlen(word_looked)) {
                find_word(word_looked, 'f');
            }
            mvwprintw(input_win, 0, rescol, "");
            wclrtoeol(input_win);
            wrefresh(input_win);
            curs_set(0);
            break;

        case 'G':
            delta = max - 1 - LINES;
            break;

        case ctl('a'):
            delta = 0;
            break;
        }
    }

    // free memory allocated // FIXME
    for (delta = 0; delta < max; delta++) {
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

void find_word(char * word, char order) {
    int i;
    if (order == 'f') {        // forward
        for (i = delta + 1; i < max - 1; i++) {
            if ((look_result = str_in_str(long_help[i], word)) >= 0) {
                delta = i;
                info("");
                break;
            }
        }
    } else if (order == 'b') { // backwards
        for (i = delta - 1; i > 0; i--) {
            if ((look_result = str_in_str(long_help[i], word)) >= 0) {
                delta = i;
                info("");
                break;
            }
        }
    }

    if (look_result == -1) {
        info("Pattern not found.");
    }
    set_ucolor(input_win, NORMAL);
    return;
}

int show_lines() {
    int lineno, c = 0, bold = 0;

    for (lineno = 0; long_help[lineno + delta] && lineno < LINES; lineno++) {
        if (strlen(word_looked)) look_result = str_in_str(long_help[lineno + delta], word_looked);

        wmove(main_win, lineno, 0);
        wclrtoeol(main_win);

        for (c = 0; c < strlen(long_help[lineno + delta]); c++)  {
            if (long_help[lineno + delta][c] == '*') bold = ! bold;
            bold ? set_ucolor(main_win, CELL_SELECTION_SC) : set_ucolor(main_win, NORMAL);

            if (long_help[lineno + delta][c] == '*') {
                  set_ucolor(main_win, NORMAL);
            //    //mvwprintw(main_win, lineno+2, c, " ");
                continue;
            } else if (lineno == 0 && look_result != -1 && c >= look_result &&
                c < look_result + strlen(word_looked) ) {
                  set_ucolor(main_win, CELL_SELECTION_SC);
            }
            mvwprintw(main_win, lineno, c, "%c", long_help[lineno + delta][c]);
        }
        
        //mvwprintw(main_win, lineno, 0, "%s", (long_help[lineno + delta]));
        wclrtoeol(main_win);
    }
    if (lineno < LINES) {
        wmove(main_win, lineno+1, 0);
        wclrtobot(main_win);
    }
     
    (void) wrefresh(main_win);
    return wgetch(input_win);
}
