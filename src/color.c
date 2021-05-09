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
 * @file color.c
 * @author Andrés Martinelli <andmarti@gmail.com>
 * @date 2017-07-18
 * @brief File containing color functions
 *
 */

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

struct ucolor ucolors[N_INIT_PAIRS] = {};

static struct dictionary * d_colors_param = NULL;

struct dictionary * get_d_colors_param() {
    return d_colors_param;
}

static struct custom_color * custom_colors = NULL;
/**
 * @brief Generate DEFAULT 'initcolor' colors
 *
 * Generate DEFAULT 'initcolor' colors.
 *
 * Example usage:
 * @code
 *     start_default_ucolors();
 * @endcode
 * returns: none
 */

void start_default_ucolors() {

    // Initialize colors attributes
    int i;
    for (i=0; i < N_INIT_PAIRS; i++) {
        ucolors[ i ].bold      = 0;
        ucolors[ i ].italic    = 0;
        ucolors[ i ].dim       = 0;
        ucolors[ i ].reverse   = 0;
        ucolors[ i ].standout  = 0;
        ucolors[ i ].underline = 0;
        ucolors[ i ].blink     = 0;
    }

    // Set some colors attributes
    // NOTE: always increase N_INIT_PAIRS on each color definition you add.
    ucolors[ DEFAULT         ].fg = WHITE;
    ucolors[ DEFAULT         ].bg = DEFAULT_COLOR;
    ucolors[ HEADINGS        ].bg = YELLOW;
    ucolors[ HEADINGS        ].fg = BLACK;
    ucolors[ HEADINGS_ODD    ].bg = YELLOW;
    ucolors[ HEADINGS_ODD    ].fg = BLACK;
    ucolors[ GRID_EVEN       ].fg = WHITE;
    ucolors[ GRID_EVEN       ].bg = DEFAULT_COLOR;
    ucolors[ GRID_ODD        ].fg = WHITE;
    ucolors[ GRID_ODD        ].bg = DEFAULT_COLOR;
    ucolors[ WELCOME         ].fg = YELLOW;
    ucolors[ WELCOME         ].bg = DEFAULT_COLOR;
    ucolors[ WELCOME         ].bold = 0;
    ucolors[ CELL_SELECTION  ].bg = BLACK;                 // cell selection in headings
    ucolors[ CELL_SELECTION  ].fg = YELLOW;
    ucolors[ CELL_SELECTION  ].bold = 0;
    ucolors[ CELL_SELECTION_SC ].fg = BLACK;               // cell selection in spreadsheet
    ucolors[ CELL_SELECTION_SC ].bg = YELLOW;
    ucolors[ NUMB            ].fg = CYAN;
    ucolors[ NUMB            ].bg = DEFAULT_COLOR;
    ucolors[ STRG            ].fg = MAGENTA;
    ucolors[ STRG            ].bg = DEFAULT_COLOR;
    ucolors[ STRG            ].bold = 0;
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
    ucolors[ CELL_ID         ].fg = WHITE;
    ucolors[ CELL_ID         ].bg = DEFAULT_COLOR;
    ucolors[ CELL_ID         ].bold = 1;
    ucolors[ CELL_FORMAT     ].fg = RED;
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
    ucolors[ HELP_HIGHLIGHT  ].fg = BLACK;               // cell selection in spreadsheet
    ucolors[ HELP_HIGHLIGHT  ].bg = YELLOW;

    ui_start_colors(); // call specific ui startup routine
}

/**
 * @brief Create a dictionary that stores correspondence between macros and key values
 *
 * Create a dictionary that stores the correspondence between macros and key
 * values (integers) defined in '.sc' files or through the color command.
 *
 * Example usage:
 * @code
 *     set_colors_param_dict();
 * @endcode
 * returns: none
 */

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
    sprintf(str, "%d", HEADINGS_ODD);
    put(d_colors_param, "HEADINGS_ODD", str);
    sprintf(str, "%d", GRID_EVEN);
    put(d_colors_param, "GRID_EVEN", str);
    sprintf(str, "%d", GRID_ODD);
    put(d_colors_param, "GRID_ODD", str);
    sprintf(str, "%d", WELCOME);
    put(d_colors_param, "WELCOME", str);
    sprintf(str, "%d", CELL_SELECTION);
    put(d_colors_param, "CELL_SELECTION", str);
    sprintf(str, "%d", CELL_SELECTION_SC);
    put(d_colors_param, "CELL_SELECTION_SC", str);
    sprintf(str, "%d", HELP_HIGHLIGHT);
    put(d_colors_param, "HELP_HIGHLIGHT", str);
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

/**
 * @brief free the colors parameters dictionary
 *
 * Free the colors parameters dictionary
 *
 * Example usage:
 * @code
 *     free_colors_param_dict();
 * @endcode
 * returns: none
 */

void free_colors_param_dict() {
    destroy_dictionary(d_colors_param);
    return;
}

/**
 * @brief Function that colorize the different types shown on screen
 *
 * Change color definition with user's one STR: color definition from '.sc'
 * file. It can be obtained at run time with the ':color str' command.
 *
 * Example usage:
 * @code
 *     chg_color();
 * @endcode
 * returns: none
 */

void chg_color(char * str) {
    if (get_conf_int("nocurses")) return;

    // Create key-value dictionary for the content of the string
    struct dictionary * d = create_dictionary();

    // Remove quotes
    if (str[0]=='"') del_char(str, 0);
    if (str[strlen(str)-1]=='"') del_char(str, strlen(str)-1);

    parse_str(d, str, TRUE);
    char * cl;

    // Validate we got enough keys to change a color
    if (
        (get(d, "fg") == NULL) ||
        (get(d, "type") == NULL) ||
        (get(d, "bg") == NULL)) {
        sc_error("Color definition incomplete");
        destroy_dictionary(d);
        return;
    }

    // Validate the values for those keys are correct
    // bg, fg should have valid values BLACK(0) to WHITE(7) for ncurses stock colors
    // or a custom color name, or -1, indicating default TERMINAL color
    if (get(d_colors_param, get(d, "type")) == NULL) {
        sc_error("Error setting color. Invalid type value: %s", get(d, "type"));
        destroy_dictionary(d);
        return;
    }

    if (get(d_colors_param, get(d, "fg")) == NULL && get_custom_color(get(d, "fg")) == NULL) {
        sc_error("Error setting color. Invalid fg value: %s. It is not and ncurses color nor user defined color.", get(d, "fg"));
        destroy_dictionary(d);
        return;
    }

    if (get(d_colors_param, get(d, "bg")) == NULL && get_custom_color(get(d, "bg")) == NULL) {
        sc_error("Error setting color. Invalid bg value: %s. It is not and ncurses color nor user defined color.", get(d, "bg"));
        destroy_dictionary(d);
        return;
    }

    // Change the color
    int type = get_int(d_colors_param, get(d, "type"));
    struct custom_color * cc;
    if ((cc = get_custom_color(get(d, "bg"))) != NULL) { // bg is custom color
        ucolors[ type ].bg = 7 + cc->number;
    } else { // bg is stock ncurses color
        ucolors[ type ].bg = get_int(d_colors_param, get(d, "bg"));
    }

    if ((cc = get_custom_color(get(d, "fg"))) != NULL) { // fg is custom color
        ucolors[ type ].fg = 7 + cc->number;
    } else { // fg is stock ncurses color
        ucolors[ type ].fg = get_int(d_colors_param, get(d, "fg"));
    }

    if (((cl = get(d, "bold")) != NULL)      && cl[0] != '\0')     ucolors[ type ].bold      = get_int(d, "bold");
    if (((cl = get(d, "italic")) != NULL)    && cl[0] != '\0')     ucolors[ type ].italic    = get_int(d, "italic");
    if (((cl = get(d, "dim")) != NULL)       && cl[0] != '\0')     ucolors[ type ].dim       = get_int(d, "dim");
    if (((cl = get(d, "reverse")) != NULL)   && cl[0] != '\0')     ucolors[ type ].reverse   = get_int(d, "reverse");
    if (((cl = get(d, "standout")) != NULL)  && cl[0] != '\0')     ucolors[ type ].standout  = get_int(d, "standout");
    if (((cl = get(d, "blink")) != NULL)     && cl[0] != '\0')     ucolors[ type ].blink     = get_int(d, "blink");
    if (((cl = get(d, "underline")) != NULL) && cl[0] != '\0')     ucolors[ type ].underline = get_int(d, "underline");

    // clean temp variable
    destroy_dictionary(d);
    return;
}

/*
 * this functions is for coloring a cell, or a range of cells.
 * it also applies a format such as bold or underline.
 * supports undo / redo
 */
/**
 * @brief
 *
 * Changes coloring and format for cell or range of cells.
 *
 * Format options: bold, underline.
 *
 * This function supports undo/redo.
 *
 * Example usage:
 * @code
 *     color_cell(<var1>,<var2>,<var3>,<var4>,<var5>);
 * @endcode
 * returns: none
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

    parse_str(d, str, TRUE);
    char * cl;

    // Validations
    if (((cl = get(d, "fg")) != NULL && cl[0] != '\0' && get(d_colors_param, get(d, "fg")) == NULL && get_custom_color(cl) == NULL)) {
            sc_error("One of the values specified is wrong: %s. Please check the values of type, fg and bg.", cl);
            destroy_dictionary(d);
            return;
    } else if ((cl = get(d, "bg")) != NULL && cl[0] != '\0' && get(d_colors_param, get(d, "bg")) == NULL && get_custom_color(cl) == NULL) {
            sc_error("One of the values specified is wrong: %s. Please check the values of type, fg and bg.", cl);
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
                copy_to_undostruct(i, j, i, j, UNDO_DEL, IGNORE_DEPS, NULL);
                #endif
            }

            // action
            n = lookat(i, j);
            if (n->ucolor == NULL) {
                n->ucolor = (struct ucolor *) malloc(sizeof(struct ucolor));
                n->ucolor->fg = NONE_COLOR;
                n->ucolor->bg = NONE_COLOR;
                n->ucolor->bold = 0;
                n->ucolor->italic = 0;
                n->ucolor->dim = 0;
                n->ucolor->reverse = 0;
                n->ucolor->standout = 0;
                n->ucolor->underline = 0;
                n->ucolor->blink = 0;
            }

            struct custom_color * cc;
            if ((cl = get(d, "bg")) != NULL && cl[0] != '\0') {
                if (get(d_colors_param, get(d, "bg")) != NULL) {
                    n->ucolor->bg = get_int(d_colors_param, get(d, "bg"));
                } else if ((cc = get_custom_color(get(d, "bg"))) != NULL) {
                    n->ucolor->bg = 7 + cc->number;
                } else {
                    sc_error("error setting bg color. we should not be here.");
                    n->ucolor->bg = DEFAULT_COLOR;
                }
            }
            if ((cl = get(d, "fg")) != NULL && cl[0] != '\0') {
                if (get(d_colors_param, get(d, "fg")) != NULL) {
                    n->ucolor->fg = get_int(d_colors_param, get(d, "fg"));
                } else if ((cc = get_custom_color(get(d, "fg"))) != NULL) {
                    n->ucolor->fg = 7 + cc->number;
                } else {
                    sc_error("error setting fg color. we should not be here.");
                    n->ucolor->fg = DEFAULT_COLOR;
                }
            }

            if ((cl = get(d, "bold"))      != NULL && cl[0] != '\0')   n->ucolor->bold      = get_int(d, "bold");
            if ((cl = get(d, "italic"))    != NULL && cl[0] != '\0')   n->ucolor->italic    = get_int(d, "italic");
            if ((cl = get(d, "dim") )      != NULL && cl[0] != '\0')   n->ucolor->dim       = get_int(d, "dim");
            if ((cl = get(d, "reverse"))   != NULL && cl[0] != '\0')   n->ucolor->reverse   = get_int(d, "reverse");
            if ((cl = get(d, "standout"))  != NULL && cl[0] != '\0')   n->ucolor->standout  = get_int(d, "standout");
            if ((cl = get(d, "blink"))     != NULL && cl[0] != '\0')   n->ucolor->blink     = get_int(d, "blink");
            if ((cl = get(d, "underline")) != NULL && cl[0] != '\0')   n->ucolor->underline = get_int(d, "underline");

            if (! loading) {
                #ifdef UNDO
                copy_to_undostruct(i, j, i, j, UNDO_ADD, IGNORE_DEPS, NULL);
                end_undo_action();
                #endif
            }
        }
    }

    destroy_dictionary(d);
    if (! loading) ui_update(TRUE);
    return;
}

/**
 * @brief
 *
 * Cleans format from a range of cells
 *
 * Example usage:
 * @code
 *     unformat(<var1>,<var2>,<var3>,<var4>);
 * @endcode
 * returns: none
 */

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
        copy_to_undostruct(r, rf, c, cf, UNDO_DEL, IGNORE_DEPS, NULL);
        #endif
    }

    // we remove format in the range
    struct ent * n;
    int i, j;
    for (i=r; i<=rf; i++) {
        for (j=c; j<=cf; j++) {

            // action
            if ( (n = *ATBL(tbl, i, j)) && n->ucolor != NULL) {
                free(n->ucolor);
                n->ucolor = NULL;
            }

       }
    }
    if (! loading) {
        #ifdef UNDO
        copy_to_undostruct(r, rf, c, cf, UNDO_ADD, IGNORE_DEPS, NULL);
        end_undo_action();
        #endif
        ui_update(TRUE);
    }
    return;
}

/**
 * @brief
 *
 * This function receives two ucolor variables and returns 1 if both have the
 * same values, returns 0 otherwise.
 *
 * Example usage:
 * @code
 *     same_color(<ucolor>,<ucolor>);
 * @endcode
 * returns: 1 if colors are the same, 0 otherwise
 */

int same_ucolor(struct ucolor * u, struct ucolor * v) {
    if (u == NULL || v == NULL)       return 0;

    if (u->fg != v->fg)               return 0;
    if (u->bg != v->bg)               return 0;
    if (u->bold != v->bold)           return 0;
    if (u->italic != v->italic)       return 0;
    if (u->dim != v->dim)             return 0;
    if (u->reverse != v->reverse)     return 0;
    if (u->standout != v->standout)   return 0;
    if (u->underline != v->underline) return 0;
    if (u->blink != v->blink)         return 0;

    return 1;
}

/**
 * @brief Redefine one of the 8 ncurses colors to a new RGB value.
 *
 * Redefine stock ncurses color to a new RGB value. Only if terminal supports it.
 * RGB values must be between 0 and 255.
 *
 * Example usage:
 * @code
 *     redefine_color(<color>,<r>,<g>,<b>);
 * @endcode
 * returns: 0 on success, -1 on error
 */

int redefine_color(char * color, int r, int g, int b) {
    #if defined(NCURSES) && defined(USECOLORS)
    extern void sig_winchg();
    if (
        ! get_conf_int("nocurses")
        && has_colors() && can_change_color()
       ) {
           char * s = get(d_colors_param, color);
           if (s == NULL) {
               sc_error("Color not found");
               return -1;
           }
#if defined(NCURSES_VERSION_MAJOR) && (( NCURSES_VERSION_MAJOR > 5 && defined(NCURSES_VERSION_MINOR) && NCURSES_VERSION_MINOR > 0) || NCURSES_VERSION_MAJOR > 6)
           if (init_extended_color(atoi(s), RGB(r, g, b)) == 0) {
#else
           if (init_color(atoi(s), RGB(r, g, b)) == 0) {
#endif
               sig_winchg();
               if (! loading) sc_info("Color %s redefined to %d %d %d.", color, r, g, b);
               return 0;
           }
       }
       if (! loading) sc_error("Could not redefine color");
    #endif
    return -1;
}

/**
 * @brief Define a custom color
 *
 * Define a custom color. If terminal does not support 256 colors, return.
 *
 * Example usage:
 * @code
 *     define_color(<color>,<r>,<g>,<b>);
 * @endcode
 * returns: 0 on success, -1 on error
 */
int define_color(char * color, int r, int g, int b) {

#if defined(NCURSES) && defined(USECOLORS)
    if (get_conf_int("nocurses")) {
        // this should not be alerted.
        //sc_error("Could not define color %s. Not using NCURSES.", color);
        return -1;
    } else if (! has_colors () || ! can_change_color() || COLORS < 9) {
        sc_error("Could not define color %s. Not supported by terminal.", color);
        return -1;

    } else if (get(d_colors_param, color) != NULL) {
        sc_error("Could not define custom color %s. That is an ncurses color. Use :redefine for that purpose.", color);
        return -1;

    } else if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) {
        sc_error("Could not define color %s. One of the RGB values is invalid.", color);
        return -1;
    }

    int number_defined_colors;
    struct custom_color * cc;

    // update color value if it already exists
    if ((cc = get_custom_color(color)) != NULL) {
        sc_debug("Color '%s' already defined. Updating its RGB values.", color);
    } else if ((number_defined_colors = count_custom_colors()) == 24) {
        sc_error("Could not define color %s. There are already 24 custom colors defined.", color);
        return -1;
    } else { // we create a new custom color
        cc = malloc (sizeof(struct custom_color));
        cc->number = number_defined_colors+1;
        cc->name =  (char *) malloc (sizeof(char) * (strlen(color) + 1));
        strcpy(cc->name, color);
        cc->p_next = custom_colors;
        custom_colors = cc;
    }
    cc->r = r;
    cc->g = g;
    cc->b = b;
#if defined(NCURSES_VERSION_MAJOR) && (( NCURSES_VERSION_MAJOR > 5 && defined(NCURSES_VERSION_MINOR) && NCURSES_VERSION_MINOR > 0) || NCURSES_VERSION_MAJOR > 6)
    init_extended_color(7 + cc->number, RGB(r, g, b));
#else
    init_color(7 + cc->number, RGB(r, g, b));
#endif

    if (! loading) sc_info("Defined custom color #%d with name '%s' and RGB values %d %d %d", cc->number, cc->name, cc->r, cc->g, cc->b);
    return 0;
#endif
    sc_error("Could not define color %s", color);
    return -1;
}

/**
 * @brief Free memory of custom_colors
 *
 * Free custom_colors memory
 *
 * Example usage:
 * @code
 *     free_custom_colors();
 * @endcode
 * returns 0
 */
int free_custom_colors() {
    struct custom_color * aux;
    while (custom_colors != NULL) {
        aux = custom_colors->p_next;
        free(custom_colors->name);
        free(custom_colors);
        custom_colors = aux;
    }
    return 0;
}

/**
 * @brief get custom color by name
 *
 * get custom color by name
 *
 * Example usage:
 * @code
       get_custom_color("skyblue")
 * @endcode
 * returns struct custom_color * if found
 * returns NULL otherwise
 */
struct custom_color * get_custom_color(char * name) {
    struct custom_color * aux = custom_colors;
    while (aux != NULL) {
        if (! strcasecmp(name, aux->name)) return aux;
        aux=aux->p_next;
    }
    return NULL;
}

/**
 * @brief count the defined custom colors
 *
 * count the defined custom colors
 *
 * Example usage:
 * @code
       count_custom_colors()
 * @endcode
 * returns int
 */
int count_custom_colors() {
    int c = 0;
    struct custom_color * aux = custom_colors;
    while (aux != NULL) {
        aux=aux->p_next;
        c++;
    }
    return c;
}

/**
 * @brief get custom color by number
 *
 * get custom color by number
 *
 * Example usage:
 * @code
       get_custom_color_by_number(2)
 * @endcode
 * returns struct custom_color * if found
 * returns NULL otherwise
 */
struct custom_color * get_custom_color_by_number(int number) {
    struct custom_color * aux = custom_colors;
    while (aux != NULL) {
        if (number == aux->number) return aux;
        aux=aux->p_next;
    }
    return NULL;
}
