#include <sys/types.h>
#include <string.h>
#include <stdlib.h>      // for atoi
#include <ncurses.h>
#include <ctype.h>
#include <unistd.h>

#include "sc.h"
#include "macros.h"
#include "utils/dictionary.h"
#include "utils/string.h"
#include "range.h"
#include "color.h"
#include "screen.h"
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
    int i, j;
    for (i=0; i< N_INIT_PAIRS; i++) {
        ucolors[ i ].bold      = 0;
        ucolors[ i ].dim       = 0;
        ucolors[ i ].reverse   = 0;
        ucolors[ i ].standout  = 0;
        ucolors[ i ].underline = 0;
        ucolors[ i ].blink     = 0;
    }

    // Set some colors attributes
    ucolors[ DEFAULT         ].fg = get_config_color("color.default.fg", WHITE);
    ucolors[ DEFAULT         ].bg = get_config_color("color.default.bg", BLACK);
    ucolors[ HEADINGS        ].fg = get_config_color("color.headings.fg", WHITE);
    ucolors[ HEADINGS        ].bg = get_config_color("color.headings.bg", RED);
    fprintf(stderr, "headings fg: %d\n", ucolors[HEADINGS].fg);
    fprintf(stderr, "headings bg: %d\n", ucolors[HEADINGS].bg);
    ucolors[ WELCOME         ].fg = get_config_color("color.welcome.fg", CYAN);
    ucolors[ WELCOME         ].bg = get_config_color("color.welcome.bg", BLACK);
    ucolors[ CELL_SELECTION  ].fg = get_config_color("color.cell_selection.fg", BLUE);         // cell selection in headings
    ucolors[ CELL_SELECTION  ].bg = get_config_color("color.cell_selection.bg", WHITE);

    ucolors[ CELL_SELECTION_SC ].fg = get_config_color("color.cell_selection_sc.fg", BLACK);      // cell selection in spreadsheet
    ucolors[ CELL_SELECTION_SC ].bg = get_config_color("color.cell_selection_sc.bg", WHITE);

    ucolors[ NUMB            ].fg = get_config_color("color.numb.fg", CYAN);
    ucolors[ NUMB            ].bg = get_config_color("color.numb.bg", BLACK);

    ucolors[ STRG            ].fg = get_config_color("color.strg.fg", RED);
    ucolors[ STRG            ].bg = get_config_color("color.strg.bg", BLACK);
    ucolors[ STRG            ].bold = 1;

    ucolors[ DATEF           ].fg = get_config_color("color.datef.fg", YELLOW);
    ucolors[ DATEF           ].bg = get_config_color("color.datef.bg", BLACK);

    ucolors[ EXPRESSION      ].fg = get_config_color("color.expression.fg", YELLOW);
    ucolors[ EXPRESSION      ].bg = get_config_color("color.expression.bg", BLACK);

    ucolors[ INFO_MSG        ].fg = get_config_color("color.info_msg.fg", CYAN);
    ucolors[ INFO_MSG        ].bg = get_config_color("color.info_msg.bg", BLACK);
    ucolors[ INFO_MSG        ].bold = 1;
    ucolors[ ERROR_MSG       ].fg = get_config_color("color.error_msg.fg", RED);
    ucolors[ ERROR_MSG       ].bg = get_config_color("color.error_msg.bg", WHITE);
    ucolors[ ERROR_MSG       ].reverse = 1;
    ucolors[ ERROR_MSG       ].bold = 1;

    ucolors[ MODE            ].fg = get_config_color("color.mode.fg", WHITE);
    ucolors[ MODE            ].bg = get_config_color("color.mode.bg", BLACK);
    ucolors[ MODE            ].bold = 1;

    ucolors[ CELL_ID         ].fg = get_config_color("color.cell_id.fg", BLUE);
    ucolors[ CELL_ID         ].bg = get_config_color("color.cell_id.bg", BLACK);
    ucolors[ CELL_ID         ].bold = 1;
    ucolors[ CELL_FORMAT     ].fg = get_config_color("color.cell_format.fg", GREEN);
    ucolors[ CELL_FORMAT     ].bg = get_config_color("color.cell_format.bg", BLACK);
    ucolors[ CELL_CONTENT    ].fg = get_config_color("color.cell_content.fg", CYAN);
    ucolors[ CELL_CONTENT    ].bg = get_config_color("color.cell_content.bg", BLACK);
    ucolors[ CELL_CONTENT    ].bold = 1;

    ucolors[ INPUT           ].fg = get_config_color("color.input.fg", WHITE);
    ucolors[ INPUT           ].bg = get_config_color("color.input.bg", BLACK);

    ucolors[ NORMAL          ].fg = get_config_color("color.normal.fg", WHITE);
    ucolors[ NORMAL          ].bg = get_config_color("color.normal.bg", BLACK);

    ucolors[ CELL_ERROR      ].fg = get_config_color("color.cell_error.fg", RED);
    ucolors[ CELL_ERROR      ].bg = get_config_color("color.cell_error.bg", BLACK);
    ucolors[ CELL_ERROR      ].bold = 1;

    ucolors[ CELL_NEGATIVE   ].fg = get_config_color("color.cell_negative.fg", GREEN);
    ucolors[ CELL_NEGATIVE   ].bg = get_config_color("color.cell_negative.bg", BLACK);

    // Initialize all possible 64 init pairs
    for (i=0; i < 8; i++)      // fg
        for (j=0; j < 8; j++)  // bg
            init_pair( (i*8) + j + 1, i, j); // i is fg and j is bg

}

// Set a color
void set_ucolor(WINDOW * w, struct ucolor * uc) {
    long attr = A_NORMAL;
    if (uc->bold)      attr |= A_BOLD;
    if (uc->dim)       attr |= A_DIM;
    if (uc->reverse)   attr |= A_REVERSE;
    if (uc->standout)  attr |= A_STANDOUT;
    if (uc->blink)     attr |= A_BLINK;
    if (uc->underline) attr |= A_UNDERLINE;
    wattrset (w, attr | COLOR_PAIR(uc->fg * 8 + uc->bg + 1));
}

// Create a dictionary that stores the correspondence between macros and key
// values (integers) defined in '.sc' files or through the color command.
void set_colors_param_dict() {
    d_colors_param = create_dictionary();

    char str[3];
    str[0]='\0';

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

// Change color definition with users's one
// STR: color definition read from '.sc' file
// It can also be obtained at run time with the `:color str` command
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
        (get(d_colors_param, get(d, "type")) == NULL)
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

// this functions is for coloring a cell, or a range of cells.
// it also applies a format such as bold or underline.
// supports undo / redo
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
                n->ucolor->bg = BLACK;
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

            if (get(d, "bold")      != '\0')     n->ucolor->bold = atoi(get(d, "bold"));
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
    if (! loading)
        update(TRUE);
    return;
}

// this function receives two ucolors variables and returns 1 if both have the same values
// returns 0 otherwise
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
    void winchg();

#ifdef USECOLORS
    if (
        ! atoi(get_conf_value("nocurses")) &&
        has_colors() && can_change_color()
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

#endif
       if (! loading) sc_error("Could not redefine color");
       return -1;
}

