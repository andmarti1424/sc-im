/* dumb ui */
#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>

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

#define NB_ENABLE 0
#define NB_DISABLE 1

void ui_start_screen() {
    printf("started screen\n");
}

void ui_stop_screen() {
    printf("stopped screen\n");
}

void nonblock(int state) {
    struct termios ttystate;
    tcgetattr(STDIN_FILENO, &ttystate);
    if (state==NB_ENABLE) {
        ttystate.c_lflag &= ~ICANON;
        ttystate.c_cc[VMIN] = 1;
    } else if (state==NB_DISABLE) {
        ttystate.c_lflag |= ICANON;
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
    return;
}

int kbhit() {
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds); //STDIN_FILENO is 0
    select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
    return FD_ISSET(STDIN_FILENO, &fds);
}

/* this function asks user for input from stdin.
 * should be non blocking and should
 * return -1 when no key was press
 * return 0 when key was press.
 * it receives * wint_t as a parameter.
 * when a valid key is press, its value its then updated in that wint_t variable.
 */
int ui_getch(wint_t * wd) {
    char c;
    int i=0;
    nonblock(NB_ENABLE);
    while (!i) {
        usleep(1);
        i=kbhit();
        if (i!=0) {
            c = fgetc(stdin);
            //printf("\n you hit %c. \n",c);
            nonblock(NB_DISABLE);
            *wd = (wint_t) c;
            return 0;
        }
    }
    return -1;
}

/* this function asks user for input from stdin.
 * should be blocking and should
 * return -1 when ESC was pressed
 * return 0 otherwise.
 * it receives * wint_t as a parameter.
 * when a valid key is press, its value its then updated in that wint_t variable.
 */
int ui_getch_b(wint_t * wd) {
    printf("calling uigetch_b\n");

    char c;
    int i=0;
    nonblock(NB_DISABLE);
    while (!i) {
        usleep(1);
        i=kbhit();
        if (i!=0) {
            c = fgetc(stdin);
            if (c == 27) {
                nonblock(NB_ENABLE);
                return -1;
            }
            printf("you hit %d %c\n", c, c);
            *wd = (wint_t) c;
            nonblock(NB_ENABLE);
            return 0;
        }
    }
    nonblock(NB_ENABLE);
    return -1;
}

// sc_msg - used for sc_info, sc_error and sc_debug macros
void ui_sc_msg(char * s, int type, ...) {
    char t[BUFFERSIZE];
    va_list args;
    va_start(args, type);
    vsprintf (t, s, args);
    printf("%s\n", t);
    va_end(args);
    return;
}

// Welcome screen
void ui_do_welcome() {
    printf("welcome screen\n");
    return;
}

// function that refreshes grid of screen
// if header flag is set, the first column of screen gets refreshed
void ui_update(int header) {
    printf("update\n");
    printf("value of current cell: %d %d %f\n", currow, curcol, lookat(currow, curcol)->v);
    return;
}

// Enable cursor and echo depending on the current mode
void ui_handle_cursor() {
}

// Print multiplier and pending operator on the status bar
void ui_print_mult_pend() {
    return;
}

void ui_show_header() {
    return;
}

void ui_clr_header(int i) {
    return;
}

void ui_print_mode() {
    return;
}

// Draw cell content detail in header
void ui_show_celldetails() {
    return;
}

// error routine for yacc (gram.y)
void yyerror(char * err) {
    printf("%s: %.*s<=%s\n", err, linelim, line, line + linelim);
    return;
}

int ui_get_formated_value(struct ent ** p, int col, char * value) {
    return -1;
}

void ui_show_text(char * val) {
    printf("%s", val);
    return;
}

void winchg() {
    return;
}

#ifdef XLUA
/* function to print errors of lua scripts */
void ui_bail(lua_State *L, char * msg) {
    return;
}
#endif

/* function to read text from stdin */
char * ui_query(char * initial_msg) {
    return NULL;
}

void ui_start_colors() {
    return;
}
