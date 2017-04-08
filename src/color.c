#include <sys/types.h>
#include <string.h>
#include <stdlib.h>      // for atoi
#include <ctype.h>
#include <unistd.h>

#include "sc.h"
#include "macros.h"
#include "utils/dictionary.h"
#include "utils/string.h"
#include "range.h"
#include "tui.h"
#include "undo.h"
#include "conf.h"
#include "cmds.h"

static struct dictionary * d_colors_param = NULL;

struct dictionary * get_d_colors_param() {
    return d_colors_param;
}

// Generate DEFAULT 'initcolor' colors
void start_default_ucolors() {

    // Initialize colors attributes
    int i;
    for (i=0; i < N_INIT_PAIRS; i++) {
        ucolors[ i ].bold      = 0;
        ucolors[ i ].dim       = 0;
        ucolors[ i ].reverse   = 0;
        ucolors[ i ].standout  = 0;
        ucolors[ i ].underline = 0;
        ucolors[ i ].blink     = 0;
    }

    // Set some colors attributes
    ucolors[ DEFAULT         ].fg = WHITE;
    ucolors[ DEFAULT         ].bg = DEFAULT_COLOR;
    ucolors[ HEADINGS        ].fg = WHITE;
    ucolors[ HEADINGS        ].bg = RED;
    ucolors[ WELCOME         ].fg = WHITE;
    ucolors[ WELCOME         ].bg = DEFAULT_COLOR;
    ucolors[ WELCOME         ].bold = 1;
    ucolors[ CELL_SELECTION  ].fg = BLUE;         // cell selection in headings
    ucolors[ CELL_SELECTION  ].bg = WHITE;
    ucolors[ CELL_SELECTION_SC ].fg = BLACK;      // cell selection in spreadsheet
    ucolors[ CELL_SELECTION_SC ].bg = WHITE;
    ucolors[ NUMB            ].fg = CYAN;
    ucolors[ NUMB            ].bg = DEFAULT_COLOR;
    ucolors[ STRG            ].fg = BLUE;
    ucolors[ STRG            ].bg = DEFAULT_COLOR;
    ucolors[ STRG            ].bold = 1;
    ucolors[ DATEF           ].fg = YELLOW;
    ucolors[ DATEF           ].bg = DEFAULT_COLOR;
    ucolors[ EXPRESSION      ].fg = RED;
    ucolors[ EXPRESSION      ].bg = DEFAULT_COLOR;
    ucolors[ INFO_MSG        ].fg = CYAN;
    ucolors[ INFO_MSG        ].bg = DEFAULT_COLOR;
    ucolors[ INFO_MSG        ].bold = 1;
    ucolors[ ERROR_MSG       ].bg = RED;
    ucolors[ ERROR_MSG       ].fg = WHITE;
    ucolors[ ERROR_MSG       ].bold = 1;
    ucolors[ MODE            ].fg = WHITE;
    ucolors[ MODE            ].bg = DEFAULT_COLOR;
    ucolors[ MODE            ].bold = 1;
    ucolors[ CELL_ID         ].fg = RED;
    ucolors[ CELL_ID         ].bg = DEFAULT_COLOR;
    ucolors[ CELL_ID         ].bold = 1;
    ucolors[ CELL_FORMAT     ].fg = CYAN;
    ucolors[ CELL_FORMAT     ].bg = DEFAULT_COLOR;
    ucolors[ CELL_CONTENT    ].fg = CYAN;
    ucolors[ CELL_CONTENT    ].bg = DEFAULT_COLOR;
    ucolors[ CELL_CONTENT    ].bold = 1;
    ucolors[ INPUT           ].fg = WHITE;
    ucolors[ INPUT           ].bg = DEFAULT_COLOR;
    ucolors[ NORMAL          ].fg = WHITE;
    ucolors[ NORMAL          ].bg = DEFAULT_COLOR;
    ucolors[ CELL_ERROR      ].fg = RED;
    ucolors[ CELL_ERROR      ].bg = DEFAULT_COLOR;
    ucolors[ CELL_ERROR      ].bold = 1;
    ucolors[ CELL_NEGATIVE   ].fg = GREEN;
    ucolors[ CELL_NEGATIVE   ].bg = DEFAULT_COLOR;

    ui_start_colors(); // call specific ui startup routine
}

// Create a dictionary that stores the correspondence between macros and key
// values (integers) defined in '.sc' files or through the color command.
void set_colors_param_dict() {
    d_colors_param = create_dictionary();
    char str[3];
    str[0]='\0';

    sprintf(str, "%d", DEFAULT_COLOR);
    put(d_colors_param, "DEFAULT_COLOR", str);
    sprintf(str, "%d", BLACK);
    put(d_colors_param, "BLACK", str);
    sprintf(str, "%d", RED);
    put(d_colors_param, "RED", str);
    sprintf(str, "%d", GREEN);
    put(d_colors_param, "GREEN", str);
    sprintf(str, "%d", YELLOW);
    put(d_colors_param, "YELLOW", str);
    sprintf(str, "%d", BLUE);
    put(d_colors_param, "BLUE", str);
    sprintf(str, "%d", MAGENTA);
    put(d_colors_param, "MAGENTA", str);
    sprintf(str, "%d", CYAN);
    put(d_colors_param, "CYAN", str);
    sprintf(str, "%d", WHITE);
    put(d_colors_param, "WHITE", str);
    sprintf(str, "%d", HEADINGS);
    put(d_colors_param, "HEADINGS", str);
    sprintf(str, "%d", WELCOME);
    put(d_colors_param, "WELCOME", str);
    sprintf(str, "%d", CELL_SELECTION);
    put(d_colors_param, "CELL_SELECTION", str);
    sprintf(str, "%d", CELL_SELECTION_SC);
    put(d_colors_param, "CELL_SELECTION_SC", str);
    sprintf(str, "%d", NUMB);
    put(d_colors_param, "NUMB", str);
    sprintf(str, "%d", STRG);
    put(d_colors_param, "STRG", str);
    sprintf(str, "%d", DATEF);
    put(d_colors_param, "DATEF", str);
    sprintf(str, "%d", EXPRESSION);
    put(d_colors_param, "EXPRESSION", str);
    sprintf(str, "%d", INFO_MSG);
    put(d_colors_param, "INFO_MSG", str);
    sprintf(str, "%d", ERROR_MSG);
    put(d_colors_param, "ERROR_MSG", str);
    sprintf(str, "%d", MODE);
    put(d_colors_param, "MODE", str);
    sprintf(str, "%d", CELL_ID);
    put(d_colors_param, "CELL_ID", str);
    sprintf(str, "%d", CELL_FORMAT);
    put(d_colors_param, "CELL_FORMAT", str);
    sprintf(str, "%d", CELL_CONTENT);
    put(d_colors_param, "CELL_CONTENT", str);
    sprintf(str, "%d", INPUT);
    put(d_colors_param, "INPUT", str);
    sprintf(str, "%d", NORMAL);
    put(d_colors_param, "NORMAL", str);
    sprintf(str, "%d", CELL_ERROR);
    put(d_colors_param, "CELL_ERROR", str);
    sprintf(str, "%d", CELL_NEGATIVE);
    put(d_colors_param, "CELL_NEGATIVE", str);
    sprintf(str, "%d", DEFAULT);
    put(d_colors_param, "DEFAULT", str);
}

void free_colors_param_dict() {
    destroy_dictionary(d_colors_param);
    return;
}

/*
 * Change color definition with users's one
 * STR: color definition read from '.sc' file
 * It can also be obtained at run time with the `:color str` command
 */
void chg_color(char * str) {

    // Create key-value dictionary for the content of the string
    struct dictionary * d = create_dictionary();

    // Remove quotes
    if (str[0]=='"') del_char(str, 0);
    if (str[strlen(str)-1]=='"') del_char(str, strlen(str)-1);

    parse_str(d, str);

    // Validate we got enough keys to change a color
    if (get(d, "fg") == '\0' || get(d, "bg") == '\0' || get(d, "type") == '\0') {
        sc_error("Color definition incomplete");
        destroy_dictionary(d);
        return;
    }

    // Validate the values for those keys are correct
    if (
        (get(d_colors_param, get(d, "fg")) == NULL) ||
        (get(d_colors_param, get(d, "bg")) == NULL) ||
        (get(d_colors_param, get(d, "type")) == NULL) ||
        (atoi(get(d_colors_param, get(d, "fg"))) > WHITE) ||
        (atoi(get(d_colors_param, get(d, "bg"))) > WHITE)
    ) {
        sc_error("One of the values specified is wrong. Please check the values of type, fg and bg.");
        destroy_dictionary(d);
        return;
    }

    // Change the color
    int type = atoi(get(d_colors_param, get(d, "type")));
    ucolors[ type ].fg = atoi(get(d_colors_param, get(d, "fg")));
    ucolors[ type ].bg = atoi(get(d_colors_param, get(d, "bg")));
    if (get(d, "bold")      != '\0')     ucolors[ type ].bold      = atoi(get(d, "bold"));
    if (get(d, "dim")       != '\0')     ucolors[ type ].dim       = atoi(get(d, "dim"));
    if (get(d, "reverse")   != '\0')     ucolors[ type ].reverse   = atoi(get(d, "reverse"));
    if (get(d, "standout")  != '\0')     ucolors[ type ].standout  = atoi(get(d, "standout"));
    if (get(d, "blink")     != '\0')     ucolors[ type ].blink     = atoi(get(d, "blink"));
    if (get(d, "underline") != '\0')     ucolors[ type ].underline = atoi(get(d, "underline"));

    // clean temp variable
    destroy_dictionary(d);
    return;
}

/*
 * this functions is for coloring a cell, or a range of cells.
 * it also applies a format such as bold or underline.
 * supports undo / redo
 */
void color_cell(int r, int c, int rf, int cf, char * str) {
    if (any_locked_cells(r, c, rf, cf)) {
        sc_error("Locked cells encountered. Nothing changed");
        return;
    }

    // parse detail
    // Create key-value dictionary for the content of the string
    struct dictionary * d = create_dictionary();

    // Remove quotes
    if (str[0]=='"') del_char(str, 0);
    if (str[strlen(str)-1]=='"') del_char(str, strlen(str)-1);

    parse_str(d, str);

    // Validations
    if (
        ((get(d, "fg") != '\0' && get(d_colors_param, get(d, "fg")) == NULL) ||
        (get(d, "bg") != '\0' && get(d_colors_param, get(d, "bg")) == NULL))) {
            sc_error("One of the values specified is wrong. Please check the values of type, fg and bg.");
            destroy_dictionary(d);
            return;
    }

    // we apply format in the range
    struct ent * n;
    int i, j;
    for (i=r; i<=rf; i++) {
        for (j=c; j<=cf; j++) {

            // if we are not loading the file
            if (! loading) {
                modflg++;

                #ifdef UNDO
                create_undo_action();
                copy_to_undostruct(i, j, i, j, 'd');
                #endif
            }

            // action
            n = lookat(i, j);
            if (n->ucolor == NULL) {
                n->ucolor = (struct ucolor *) malloc(sizeof(struct ucolor));
                n->ucolor->fg = WHITE;
                n->ucolor->bg = DEFAULT_COLOR;
                n->ucolor->bold = 0;
                n->ucolor->dim = 0;
                n->ucolor->reverse = 0;
                n->ucolor->standout = 0;
                n->ucolor->underline = 0;
                n->ucolor->blink = 0;
            }

            if (get(d, "bg") != '\0')
                n->ucolor->bg = atoi(get(d_colors_param, get(d, "bg")));

            if (get(d, "fg") != '\0')
                n->ucolor->fg = atoi(get(d_colors_param, get(d, "fg")));

            if (get(d, "bold")      != '\0')     n->ucolor->bold      = atoi(get(d, "bold"));
            if (get(d, "dim")       != '\0')     n->ucolor->dim       = atoi(get(d, "dim"));
            if (get(d, "reverse")   != '\0')     n->ucolor->reverse   = atoi(get(d, "reverse"));
            if (get(d, "standout")  != '\0')     n->ucolor->standout  = atoi(get(d, "standout"));
            if (get(d, "blink")     != '\0')     n->ucolor->blink     = atoi(get(d, "blink"));
            if (get(d, "underline") != '\0')     n->ucolor->underline = atoi(get(d, "underline"));

            if (! loading) {
                #ifdef UNDO
                copy_to_undostruct(i, j, i, j, 'a');
                end_undo_action();
                #endif
            }
        }
    }

    destroy_dictionary(d);
    if (! loading) ui_update(TRUE);
    return;
}

void unformat(int r, int c, int rf, int cf) {
    if (any_locked_cells(r, c, rf, cf)) {
        sc_error("Locked cells encountered. Nothing changed");
        return;
    }

    // if we are not loading the file
    if (! loading) {
        modflg++;
        #ifdef UNDO
        create_undo_action();
        #endif
    }

    // we remove format in the range
    struct ent * n;
    int i, j;
    for (i=r; i<=rf; i++) {
        for (j=c; j<=cf; j++) {

            // action
            if ( (n = *ATBL(tbl, i, j)) && n->ucolor != NULL) {
                if (! loading) {
                    #ifdef UNDO
                    copy_to_undostruct(i, j, i, j, 'd');
                    #endif
                }

                free(n->ucolor);
                n->ucolor = NULL;

                if (! loading) {
                    #ifdef UNDO
                    copy_to_undostruct(i, j, i, j, 'a');
                    #endif
                }
            }

       }
    }
    if (! loading) {
        #ifdef UNDO
        end_undo_action();
        #endif
        ui_update(TRUE);
    }
    return;
}

/*
 * this function receives two ucolors variables and returns 1 if both have the same values
 * returns 0 otherwise
 */
int same_ucolor(struct ucolor * u, struct ucolor * v) {
    if (u == NULL || v == NULL)       return 0;

    if (u->fg != v->fg)               return 0;
    if (u->bg != v->bg)               return 0;
    if (u->bold != v->bold)           return 0;
    if (u->dim != v->dim)             return 0;
    if (u->reverse != v->reverse)     return 0;
    if (u->standout != v->standout)   return 0;
    if (u->underline != v->underline) return 0;
    if (u->blink != v->blink)         return 0;

    return 1;
}

int redefine_color(char * color, int r, int g, int b) {
    #if defined(NCURSES) && defined(USECOLORS)
    extern void winchg();
    if (
        ! atoi(get_conf_value("nocurses"))
        && has_colors() && can_change_color()
       ) {
           char * s = get(d_colors_param, color);
           if (s == NULL) {
               sc_error("Color not found");
               return -1;
           }
           if (init_color(atoi(s), r, g, b) == 0) {
               winchg();
               if (! loading) sc_info("Color %s redefined to %d %d %d.", color, r, g, b);
               return 0;
           }
       }
       if (! loading) sc_error("Could not redefine color");
    #endif
    return -1;
}
