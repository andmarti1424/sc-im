/* gtk ui */
#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <wchar.h>

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

static wint_t buf[1000];

static void on_key_press(GtkWidget *widget, GdkEventKey * event, gpointer data) {
    if (event->hardware_keycode == 0 ||
        event->hardware_keycode == 50 ||
        event->hardware_keycode == 66 ||
        event->hardware_keycode == 62 ||
        event->hardware_keycode == 105 ||
        event->hardware_keycode == 37 ||
        event->hardware_keycode == 64) return;
    buf[0] = event->keyval;
    g_print("key pressed: %d %c\n", event->hardware_keycode, event->keyval);
    return;
}

/* this function asks user for input from stdin.
 * should be non blocking and should
 * return -1 when no key was press
 * return 0 when key was press.
 * it receives * wint_t as a parameter.
 * when a valid key is press, its value its then updated in that wint_t variable.
 */
int ui_getch(wint_t * wd) {
    gtk_main_iteration();
    *wd = buf[0];

    if (buf[0] == GDK_KEY_Return) {
        *wd = 10;
    } else if (buf[0] == GDK_KEY_Escape)
        *wd = OKEY_ESC;

    //printf("char: %d\n", *wd);
    buf[0]=0;
    return 0;
}
void ui_start_screen() {
    buf[0]=0;
    GtkWidget *window;
    GtkWidget *t;

    gtk_init(0, NULL);
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 250);

    t = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(t), 0);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(t, "key_press_event", G_CALLBACK(on_key_press), NULL);

    gtk_container_add(GTK_CONTAINER (window), t);
    gtk_widget_show(t);
    gtk_widget_show(window);
    printf("started screen\n");
}

void ui_stop_screen() {
    gtk_main_quit();
    printf("stopped screen\n");
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
