/*
    main_win: ventana que carga la planilla
    input_win: ventana usada para stdin y como barra de estado
*/
#include <string.h>
#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "input.h"
#include "screen.h"
#include "range.h"
#include "sc.h"
#include "cmds.h"
#include "color.h"
#include "version.h"
#include "file.h"

extern struct dictionary * d_colors_param;
extern int cmd_pending;
extern int cmd_multiplier;
extern char insert_edit_submode;

unsigned int curmode;
int rescol = RESCOL;           // Columns reserved for row numbers

WINDOW * main_win;
WINDOW * input_win;

// off screen spreadsheet rows and columns
int offscr_sc_rows = 0, offscr_sc_cols = 0;

srange * ranges;

void start_screen() {
    initscr();

    main_win = newwin(0, 0, 0, 0);
    input_win = newwin(2, COLS, 0, 0); // just 2 rows

    #ifdef USECOLORS
    if (has_colors()) {
        start_color();
        
        if (get_d_colors_param() == NULL) {
            start_default_ucolors();
 
            // in case we decide to change colors
            // esto crea un diccionario y guarda en el 
            // la equivalencia entre las macros y los valores
            // de las claves que se definen en los archivos .sc
            set_colors_param_dict();
        }
        //initcolor(0);                                // Creo los colores initpair
    }   
    #endif
    //scr_lines = LINES;
    //scr_cols = COLS;
    
    wtimeout(input_win, TIMEOUT_CURSES);
    noecho();
    curs_set(0);

    if ((char *) getenv ("ESCDELAY") == NULL) set_escdelay(ESC_DELAY);
    cbreak();
    keypad(input_win, 1); 
}

void stop_screen() {
    #ifdef USECOLORS
        //if (get_d_colors_param() != NULL)
        free_colors_param_dict();
    #endif
    endwin();
    return;
}

// pantalla de bienvenida
void do_welcome() {

    char * msg_title = "SCIM - SpreadSheet Calculator Improvised";
    char * msg_by = "A SC fork by Andrés Martinelli";
    char * msg_version = rev;
    char * msg_help = "Press «:help<Enter>» to get help";

    // show headings
    int mxcol = offscr_sc_cols + calc_offscr_sc_cols() - 1;
    int mxrow = offscr_sc_rows + calc_offscr_sc_rows() - 1;
    show_sc_col_headings(main_win, mxcol, mxrow);
    show_sc_row_headings(main_win, mxrow);

    #ifdef USECOLORS
    set_ucolor(main_win, WELCOME);
    #endif

    // show message
    mvwaddstr(main_win, LINES/2-2, COLS/2-strlen(msg_title)/2  , msg_title);
    mvwaddstr(main_win, LINES/2-1, COLS/2-strlen(msg_by)/2     , msg_by);
    mvwaddstr(main_win, LINES/2  , COLS/2-strlen(msg_version)/2, msg_version);
    mvwaddstr(main_win, LINES/2+2, COLS/2-strlen(msg_help)/2   , msg_help);

    wrefresh(main_win);

    return;
}

// function that refreshs grid of screen
void update(void) {
    //if (cmd_multiplier > 0) return;
    if (cmd_multiplier > 1) return;

    // Limpio desde comienzo hacia to bottom
    wmove(main_win, RESROW, rescol);
    wclrtobot(main_win);
    
    // visible spreadsheet rows and cols in screen
    int vis_sc_cols = calc_cols_show();       // Cantidad de columnas a dibujar en pantalla: A, B, C, D
    int vis_sc_rows = LINES - RESROW - 1;     // Cantidad de filas a dibujar en pantalla: 0, 1, 2, 3, 4..

    /*
       calculo las filas y columnas que quedan ocultas
       mxcol-1 es el numero de la ultima sc_col que se ve en pantalla
       mxrow-1 es el numero de la ultima sc_row que se ve en pantalla
    */

    int mxcol = offscr_sc_cols + calc_offscr_sc_cols() - 1;
    int mxrow = offscr_sc_rows + calc_offscr_sc_rows() - 1;

    /* You can't hide the last row or col */
    while (row_hidden[currow])
        currow++;
    while (col_hidden[curcol])
        curcol++;

    // Muestro cursor
    //show_cursor(main_win);

    // Muestro en pantalla el contenido de las celdas
    // Valores numericos, strings justificados, centrados y expresiones.
    show_content(main_win, mxrow, mxcol);

    // Show sc_col headings: A, B, C, D..
    show_sc_col_headings(main_win, mxcol, mxrow);
    
    // Show sc_row headings: 0, 1, 2, 3..
    show_sc_row_headings(main_win, mxrow);

    // Limpio desde la ultima sc_row hacia to bottom
    // REALLY NEEDED?
    /*int c = mxrow - offscr_sc_rows + 1;
    if ( c + RESROW < LINES ) {
        wmove(main_win, c + RESROW, 0);
        wclrtobot(main_win);
    }*/

    // Refresco ventanas de curses
    wrefresh(main_win);

    // Muestro detalle de celda en header (primera fila)
    show_celldetails(input_win);

    // print mode
    (void) print_mode(input_win);
    wrefresh(input_win);
}

// Habilito cursor y el echo dependiendo del MODO actual
void handle_cursor() {
    switch (curmode) {
        case COMMAND_MODE:
            noecho(); //cambiado 04/06/2014
            curs_set(1);
            break;
        case INSERT_MODE:
        case EDIT_MODE:
            noecho();
            curs_set(2);
            break;
        default:
            noecho();
            curs_set(0);
    }    
    return;
}

/* Funcion que imprime una cadena en pantalla, aplicando un justificado.
El tercer parametro 0, indica un justificado a la izquierda
El tercer parametro 1, indica un justificado a la derecha
*/
void write_j(WINDOW * win, const char * word, const unsigned int row, const unsigned int justif) {
    (justif == 0) ? (wmove(win, row, 0) && wclrtoeol(win)) : wmove(win, row, COLS - strlen(word));
    wprintw(win, "%s", word);
    return;
}

// funcion que imprime en status_bar el efecto
// multiplicador y el comando pendiente
void print_mult_pend(WINDOW * win) {

    if (curmode != NORMAL_MODE && curmode != VISUAL_MODE && curmode != EDIT_MODE) return;

    int row_orig, col_orig;
    getyx(win, row_orig, col_orig);

    #ifdef USECOLORS
    set_ucolor(win, MODE);
    #endif
    // Muestro efecto multiplicador y/o * de comando_pendiente
    char strm[COLS];
    strm[0]='\0';
    if (cmd_multiplier > 0) sprintf(strm, "%d", cmd_multiplier);
    if (cmd_pending) {
        strcat(strm, "?");
    }

    char field[rescol+1];
    field[0]='\0';
    sprintf(field, "%0*d", rescol - strlen(strm), 0);
    subst(field, '0', ' ');
    strcat(strm, field);

    mvwprintw(win, 0, 0, "%s", strm);

    // vuelvo a la posicion anterior del cursor
    wmove(win, row_orig, col_orig);
}

// Muestro primera y segunda fila (header)
// tambien maneja posicion del cursor
void show_header(WINDOW * win) {

    clr_header(win, 0);
    clr_header(win, 1);

    print_mult_pend(win);
   
    // imprimo modo
    print_mode(win);

    // imprimo texto de input
    #ifdef USECOLORS
    set_ucolor(win, INPUT);
    #endif
    switch (curmode) {
        case COMMAND_MODE:
            mvwprintw(win, 0, 0 + rescol, ":%s", inputline);
            wmove(win, 0, inputline_pos + 1 + rescol);
            break;
        case INSERT_MODE:
            mvwprintw(win, 0, 1 + rescol, "%s", inputline);
            wmove(win, 0, inputline_pos + 1 + rescol);
            break;
        case EDIT_MODE:
            mvwprintw(win, 0, 0 + rescol, " %s", inputline);
            wmove(win, 0, inputline_pos + 1 + rescol);
            break;
    }
    wrefresh(win);
    
    return;
}

// Función que limpia una fila de la pantalla.
void clr_header(WINDOW * win, int i) {
    int row_orig, col_orig;
    getyx(win, row_orig, col_orig);

    wmove(win, i, 0);
    wclrtoeol(win);

    // vuelvo a la posicion anterior del cursor
    wmove(win, row_orig, col_orig);

    return;
}

// Función que imprime el modo actual en la primera fila en pantalla
// tambien imprime el : o el submodo de insert a la izq.
void print_mode(WINDOW * win) {
    unsigned int row = 0; // Print mode in first row
    char strm[22] = "";

    #ifdef USECOLORS
    set_ucolor(win, MODE);
    #endif

    if (curmode == NORMAL_MODE) {
        strcat(strm, " -- NORMAL --");
        write_j(win, strm, row, RIGHT);

    } else if (curmode == INSERT_MODE) {
        strcat(strm, " -- INSERT --");
        write_j(win, strm, row, RIGHT);

        #ifdef USECOLORS
        set_ucolor(win, INPUT);
        #endif
        // muestro submodo de modo insert
        mvwprintw(win, 0, 0 + rescol, "%c", insert_edit_submode);
        //wmove(win, 0, 1); comentado el dia 01/06

    } else if (curmode == EDIT_MODE) {
        strcat(strm, "   -- EDIT --");
        write_j(win, strm, row, RIGHT);

    } else if (curmode == VISUAL_MODE) {
        strcat(strm, " -- VISUAL --");
        write_j(win, strm, row, RIGHT);

    } else if (curmode == COMMAND_MODE) {
        strcat(strm, "-- COMMAND --");
        write_j(win, strm, row, RIGHT);
        #ifdef USECOLORS
        set_ucolor(win, INPUT);
        #endif
        // muestro ':'
        mvwprintw(win, 0, 0 + rescol, ":");
        wmove(win, 0, 1 + rescol);
    }

    return;
}

// Funcion que calcula la cantidad de columnas de planilla que entran en pantalla,
// dejando offscr_sc_cols columnas a la izquierda sin mostrar
int calc_cols_show() {
    int k, width = 0, c = 0;
    for (k = offscr_sc_cols; width < COLS; k++, c += 1) {
        if (! col_hidden[k]) width += fwidth[k];
    }
    return c-1;
}

// Show sc_row headings: 0, 1, 2, 3, 4...
void show_sc_row_headings(WINDOW * win, int mxrow) {
    int row = RESROW;
    #ifdef USECOLORS
    if (has_colors()) set_ucolor(win, HEADINGS);
    #endif
    int i;
    for (i = offscr_sc_rows; i < mxrow; i++) {
        if (row_hidden[i]) continue;

        srange * s = get_selected_range();
        if ( (s != NULL && i >= s->tlrow && i <= s->brrow) || i == currow ) {
            #ifdef USECOLORS
            if (has_colors()) set_ucolor(win, CELL_SELECTION);
            #else
            wattron(win, A_REVERSE);
            #endif
        }
        mvwprintw (win, row+1, 0, "%*d ", rescol-1, i);

        #ifdef USECOLORS
        if (has_colors()) set_ucolor(win, HEADINGS);
        #else
        wattroff(win, A_REVERSE);
        #endif
        row++;
    }
}

// Show sc_col headings: A, B, C, D...
void show_sc_col_headings(WINDOW * win, int mxcol, int mxrow) {
    int i;
    int col = rescol;

    #ifdef USECOLORS
    if (has_colors()) set_ucolor(win, HEADINGS);
    #endif

    wmove(win, RESROW, 0);
    wclrtoeol(win);

    for (i = offscr_sc_cols; i <= mxcol; i++) {
        if (col_hidden[i]) continue;
        int k = fwidth[i] / 2;

        srange * s = get_selected_range();
        if ( (s != NULL && i >= s->tlcol && i <= s->brcol) || i == curcol ) {
            #ifdef USECOLORS
            if (has_colors()) set_ucolor(win, CELL_SELECTION);
            #else
            wattron(win, A_REVERSE);
            #endif
        }
        (void) mvwprintw(win, RESROW, col, "%*s%-*s", k-1, " ", fwidth[i] - k + 1, coltoa(i));

        wclrtoeol(win);

        #ifdef USECOLORS
        if (has_colors()) set_ucolor(win, HEADINGS);
        #else
        wattroff(win, A_REVERSE);
        #endif
        col += fwidth[i];
    }
}

void show_cursor(WINDOW * win) {
    int col=0, row=0;
    int cursor_row = RESROW+1;
    for (row = offscr_sc_rows; row < currow; row++) {
        if ( ! row_hidden[row] ) cursor_row++;
    }
    int cursor_col = rescol;
    for (col = offscr_sc_cols; col < curcol; col++) {
        if ( ! col_hidden[col] ) cursor_col += fwidth[col];
    } 
    #ifdef USECOLORS
    if (has_colors())
        set_ucolor(win, CELL_SELECTION_SC);
    #endif
    wmove(win, cursor_row, cursor_col);
    col = fwidth[curcol];
    while (col-- > 0) {
        wprintw(win, " ");
    }
}

void show_text_content_of_cell(WINDOW * win, struct ent ** p, int row, int col, int r, int c); void show_numeric_content_of_cell(WINDOW * win, struct ent ** p, int col, int r, int c);

// Muestra contenido de todas las celdas
void show_content(WINDOW * win, int mxrow, int mxcol) {

    register struct ent **p;
    int row, r, col, q_row_hidden = 0;

    for (row = offscr_sc_rows, r = RESROW; row < mxrow; row++) {
        if (row_hidden[row]) {
            q_row_hidden++;
            continue;
        }

        register c = rescol;
        int nextcol;
        int fieldlen;
        col = offscr_sc_cols;

        for (p = ATBL(tbl, row, offscr_sc_cols); col <= mxcol;
        p += nextcol - col, col = nextcol, c += fieldlen) {

            nextcol = col + 1;
            fieldlen = fwidth[col];
            if (col_hidden[col]) {
                c -= fieldlen;
                continue;
            }

            if ( (*p) == NULL) *p = lookat(row, col);

            // Clean format
            #ifdef USECOLORS
            if ((*p)->expr) {
                set_ucolor(win, EXPRESSION);
            } else if ((*p)->label) {             // string
                set_ucolor(win, STRING);
            } else if ((*p)->flags & is_valid) {  // numeric value
                set_ucolor(win, NUMBER);
            } else if ((*p)->cellerror) {         // cellerror
            } else {
                set_ucolor(win, NORMAL);
            }
            #endif

            // Color selected cell
            if ((currow == row) && (curcol == col)) {
                #ifdef USECOLORS
                    if (has_colors()) set_ucolor(win, CELL_SELECTION_SC);
                #else
                    wattron(win, A_REVERSE);
                #endif
            }
            
            // Color selected range
            int in_range = 0; // this is for coloring empty cells within a range
            srange * s = get_selected_range();
            if (s != NULL && row >= s->tlrow && row <= s->brrow && col >= s->tlcol && col <= s->brcol ) {
                //currow = s->tlrow; //FIXME comentado el dia 08/06/2014
                //curcol = s->tlcol; //FIXME comentado el dia 08/06/2014
                #ifdef USECOLORS
                    set_ucolor(win, CELL_SELECTION_SC);
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
                    set_ucolor(win, CELL_SELECTION_SC);
                #else
                    wattron(win, A_REVERSE);
                #endif
            }

            //if ((*p)->cellerror == CELLERROR) {
            //       (void) mvprintw(row + RESROW + 1 - offscr_sc_rows, c, "%*.*s", fwidth[col], fwidth[col], "ERROR");
            //       continue;
            //}
            
            // si hay valor numerico
            if ( (*p)->flags & is_valid) {
                show_numeric_content_of_cell(win, p, col, row + RESROW + 1 - offscr_sc_rows - q_row_hidden, c);

            // si hay solo valor de texto
            } else if ((*p) -> label) { 
                show_text_content_of_cell(win, p, row, col, row + RESROW + 1 - offscr_sc_rows - q_row_hidden, c);

            } /* repaint a blank cell. this fixes KI1 */
            else if (!((*p)->flags & is_valid) && !(*p)->label   ) {
                //if ( currow == row && curcol == col) 
                if (( currow == row && curcol == col) ||
                ( in_range && row >= ranges->tlrow && row <= ranges->brrow &&
                col >= ranges->tlcol && col <= ranges->brcol ) ) {
                    #ifdef USECOLORS
                    if (has_colors()) set_ucolor(win, CELL_SELECTION_SC);
                    #else
                        wattron(win, A_REVERSE);
                    #endif
                } else  {
                    #ifdef USECOLORS
                    set_ucolor(win, STRING); // FIXME when no colors
                    #endif
                }
                char field[1024];
                char caracter;
                chtype cht[fieldlen];
                int i;

                mvwinchnstr(win, row + RESROW + 1 - offscr_sc_rows - q_row_hidden, c, cht, fieldlen);
                for (i=0; i < fieldlen; i++) {
                    caracter = cht[i] & A_CHARTEXT;
                    if ( ! caracter ) caracter = ' '; // this is for NetBSD compatibility
                    mvwprintw(win, row + RESROW + 1 - offscr_sc_rows - q_row_hidden, c + i, "%c", caracter);
                }
            }

            // clean format
            #ifndef USECOLORS
                wattroff(win, A_REVERSE);
            #endif
        }
    }
}

// funcion que agrega detalle de un ent en una cadena que recibe como parametro
// se usa para mostrar el detalle de una celda en el input_win (funcion de debajo)
void add_cell_detail(char * d, struct ent * p1) {
    if ( !p1 ) return;

    // string part of cell - ?????
    if (p1->expr && (p1->flags & is_strexpr)) {
        if (p1->flags & is_label)
            strcat(d, "|{");
        else
            strcat(d, (p1->flags & is_leftflush) ? "<{" : ">{");
        //strcat(d, line); FIXME
        strcat(d, "??? } ");        /* and this '}' is for vi % */

    } else if (p1->label) {
        /* has constant label only */
        if (p1->flags & is_label)
            strcat(d, "|\"");
        else
            strcat(d, (p1->flags & is_leftflush) ? "<\"" : ">\"");
        strcat(d, p1->label);
        strcat(d, "\" ");
    }

    // value part of cell:
    if (p1->flags & is_valid) {
        /* has value or num expr */
        if ( ( ! (p1->expr) ) || ( p1->flags & is_strexpr ) ) {
            sprintf(d, "%s%c", d, '[');
            (void) sprintf(d, "%s%.15g", d, p1->v);
            sprintf(d, "%s%s", d, "]");
        }
    }
}

// Funcion que agrega en header (la primera fila de pantalla)
// el detalle del contenido de la celda seleccionada
void show_celldetails(WINDOW * win) {
    char head[COLS];
    int inputline_pos = 0;

    // show cell in header
    #ifdef USECOLORS
        set_ucolor(win, CELL_ID);
    #endif
    sprintf(head, "%s%d ", coltoa(curcol), currow);
    mvwprintw(win, 0, 0 + rescol, "%s", head); // agregado rescol el 08/06
    inputline_pos += strlen(head) + rescol;  

    // show the current cell's format
    #ifdef USECOLORS
        set_ucolor(win, CELL_FORMAT);
    #endif
    register struct ent *p1 = *ATBL(tbl, currow, curcol);
    if ((p1) && p1->format)
        sprintf(head, "(%s) ", p1->format);
    else
        sprintf(head, "(%d %d %d) ", fwidth[curcol], precision[curcol], realfmt[curcol]);
    mvwprintw(win, 0, inputline_pos, "%s", head);
    inputline_pos += strlen(head);

    // show expr
    #ifdef USECOLORS
        set_ucolor(win, CELL_CONTENT);
    #endif
    if (p1 && p1->expr) {
        linelim = 0;
        editexp(currow, curcol);  /* set line to expr */
        linelim = -1;
        sprintf(head, "[%s] ", line);
        mvwprintw(win, 0, inputline_pos, "%s", head);
        inputline_pos += strlen(head);
    }
    // add cell content to head string 
    head[0] = '\0';
    add_cell_detail(head, p1);
    mvwprintw(win, 0, inputline_pos, "%s", head);
    wclrtoeol(win); //linea agregada el 08/06
    wrefresh(win);
}

// Calculo la cantidad de filas que quedan ocultas en la parte superior de la pantalla
int calc_offscr_sc_rows() {
    // pick up row counts
    int i, rows = 0, row = 0;
    if (offscr_sc_rows <= currow) {
        for (i = offscr_sc_rows, rows = 0, row=RESROW;
        (row < LINES) && (i <= maxrows); i++) {
            rows++;
            if (! row_hidden[i])
                row++;
        }
    }
    // get off screen rows
    while (offscr_sc_rows + rows - 2 < currow || currow < offscr_sc_rows ) {
        if (offscr_sc_rows - 1 == currow) offscr_sc_rows--;
        else if (offscr_sc_rows + rows - 1 == currow) offscr_sc_rows++;
        else  {
            // Try to put the cursor in the center of the screen
            row = (LINES - RESROW) / 2 + RESROW;
            offscr_sc_rows = currow;
            for (i=currow-1; (i >= 0) && (row - 1 > RESROW); i--) {
                offscr_sc_rows--;
                if (! row_hidden[i])
                    row--;
            }
        }
        // Now pick up the counts again
        for (i = offscr_sc_rows, rows = 0, row = RESROW;
            (row < LINES) && (i <= maxrows); i++) {
            rows++;
            if (! row_hidden[i])
                row++;
        }
    }
    return rows;
}

// Calculo la cantidad de columnas que quedan ocultas a la izquierda de la pantalla
int calc_offscr_sc_cols() {
    int i, cols = 0, col = 0;
    // pick up col counts
    if (offscr_sc_cols <= curcol)
    for (i = offscr_sc_cols, cols = 0, col = rescol;
            (col + fwidth[i] < COLS - 1) && (i < maxcols); i++) {
            cols++;
            if (! col_hidden[i])
                col += fwidth[i];
    }
    // get off screen cols
    while (offscr_sc_cols + cols - 1 < curcol || curcol < offscr_sc_cols ) {
        if (offscr_sc_cols - 1 == curcol) offscr_sc_cols--;
        else if (offscr_sc_cols + cols == curcol) offscr_sc_cols++;
        else {
            /* Try to put the cursor in the center of the screen */
            col = (COLS - rescol - fwidth[curcol]) / 2 + rescol;
            offscr_sc_cols = curcol;
            for (i=curcol-1; i >= 0 && col-fwidth[i] > rescol; i--) {
                offscr_sc_cols--;
                if (! col_hidden[i])
                    col -= fwidth[i];
            } 
        }

        // Now pick up the counts again
        for (i = offscr_sc_cols, cols = 0, col = rescol;
            (col + fwidth[i] < COLS - 1) && (i < maxcols); i++) {
            cols++;
            if (! col_hidden[i])
                col += fwidth[i];
        }
    }
    return cols;
}

// error routine for yacc (gram.y)
int seenerr;
void yyerror(char *err) {
    if (seenerr) return;
    seenerr++;
    mvwprintw(input_win, 1, 0, "%s: %.*s<=%s", err, linelim, line, line + linelim);
    wrefresh(input_win);
    return;
}

// returns a string that represents the formated value of the cell, if a format exists
// returns 0  format of label - datetime en label - format "d" - no hay cadena en label más que la fecha - no puede haber numero en p->v
// returns 1  format of number - (numbers with format) - puede haber label.
// returns -1 if there is no format in the cell.
int get_formated_value(struct ent ** p, int col, char * value) {
    char * cfmt = (*p)->format ? (*p)->format : (realfmt[col] >= 0 && realfmt[col] < COLFORMATS && colformat[realfmt[col]]) ? colformat[realfmt[col]] : NULL;

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

void show_text_content_of_cell(WINDOW * win, struct ent ** p, int row, int col, int r, int c) {
    char value[FBUFLEN];      // the value to be printed without padding
    char field[FBUFLEN] = ""; // the value with padding and alignment    
    int col_width = fwidth[col];    
    int flen;                 // current length of field
    int left;

    int str_len  = strlen((*p)->label);
    strcpy(value, (*p)->label);

    // in case there is a format
    char s[FBUFLEN] = "";
    int res = get_formated_value(p, col, s);
    if (res == 0) {           // datetime format
        strcpy(value, s);
        str_len  = strlen(value);
    }

    // si no entra en pantalla
    if (str_len > col_width) {
        sprintf(field, "%0*d", col_width, 0);
        subst(field, '0', '*');

       // Color selected cell
        if ((currow == row) && (curcol == col)) {
            #ifdef USECOLORS
                if (has_colors()) set_ucolor(win, CELL_SELECTION_SC);
            #else
                wattron(win, A_REVERSE);
            #endif
        }
        strncpy(field, value, col_width);
        field[col_width]='\0';
        mvwprintw(win, r, c, "%s", field);

        char ex[str_len+1];
        strcpy(ex, value);
        del_range_chars(ex, 0, col_width-1);
            #ifdef USECOLORS
                if (has_colors()) set_ucolor(win, STRING);
            #else
                wattroff(win, A_REVERSE);
            #endif
        mvwprintw(win, r, c + col_width, "%s", ex);
        wclrtoeol(win);
        return;

    // izquierda
    } else if ( (*p)->label && (*p)->flags & is_leftflush ) {
        strcpy(field, value);
        left = col_width - str_len;
        left = left < 0 ? 0 : left;
        flen = str_len;
        while (left-- && flen++) add_char(field, ' ', flen-1);

    // centrado
    } else if ( (*p)->label && (*p)->flags & is_label) {
        left = (col_width - str_len )/2;
        left = left < 0 ? 0 : left;
        flen = 0;
        while (left-- && ++flen) add_char(field, ' ', flen-1);
        strcat(field, value);
        flen += str_len;
        left = (col_width - flen);
        left = left < 0 ? 0 : left;
        while (left-- && ++flen) add_char(field, ' ', flen-1);

    // derecha
    } else if ( (*p)->label || res == 0) {
        left = col_width - str_len;
        left = left < 0 ? 0 : left;
        flen = 0;
        while (left-- && ++flen) add_char(field, ' ', flen-1);
        strcat(field, value);
    }

    mvwprintw(win, r, c, "%s", field);
    wclrtoeol(win);

    return;
}

// this functions shows:
// 1. numeric value of a cell
// 2. numeric value and label value if both exists in a cell
void show_numeric_content_of_cell(WINDOW * win, struct ent ** p, int col, int r, int c) {
    char field[1024]="";
    char fieldaux[1024]="";
    char value[1024]="";
    int col_width = fwidth[col];
    int tlen = 0, nlen = 0;

    // save number with default format of column
    sprintf(field, "%.*f", precision[col], (*p)->v);
    nlen = strlen(field);

    // we get cell format, in case there is one defined for that cell
    char s[FBUFLEN] = "";
    int res = get_formated_value(p, col, s);
    if (res == 0) {
        //nlen = 0;
        strcpy(field, s);            
        tlen = strlen(s); //format in label
    } else if (res == 1) {
        strcpy(field, s);
        nlen = strlen(field);
    }
    
    // if content is larger than column width
    if (nlen > col_width || tlen > col_width) {
        sprintf(field, "%0*d", col_width, 0);
        subst(field, '0', '*');

    // if there is (text centered or left aligned) and (numeric value without format or numeric value with format) 
    } else if ( (*p)->label && (*p)->flags & (is_leftflush | is_label ) && ( res == -1 || res == 1) ) {
        int tlen = strlen((*p)->label);

        if ( nlen + tlen > col_width) {            // content doesnt fit
            sprintf(field, "%0*d", col_width, 0);
            subst(field, '0', '*');
        } else if ((*p)->flags & (is_leftflush)) { // left label
            int left = col_width - nlen - tlen;
            strcpy(field, strlen(s) ? s : (*p)->label);
            while (left--) add_char(field, ' ', strlen(field));
            sprintf(field, "%s%.*f", field, precision[col], (*p)->v);
        } else {                                   // center label
            field[0]='\0';
            int left = (col_width - tlen) / 2;
            while (left--) add_char(field, ' ', strlen(field));
            strcat(field, strlen(s) ? s : (*p)->label);
            left = col_width - strlen(field);
            int s = nlen < left ? left - nlen : 0;
            while (s > 0 && s--) add_char(field, ' ', strlen(field));
            sprintf(fieldaux, "%.*f", precision[col], (*p)->v);
            int i = col_width - strlen(field) - strlen(fieldaux);
            sprintf(field, "%s%s", field, &fieldaux[-i]);
        }
    // label with format (datetime) + numeric value w/o format and no label + number with format.
    // -> fill leftsize
    } else if (
         //(res == -1 || res == 1 || res == 0) &&
         strlen(field) < col_width) {
         int left = col_width - strlen(field) + 1;
         while (left-- && left > 0) add_char(field, ' ', 0);
    }
    mvwprintw(win, r, c, "%s", field);
    wclrtoeol(win);
    return;
}

// function that shows text in a child process
void show_text(char * val) {
    int pid;
    char px[MAXCMD];
    char *pager;

    (void) strcpy(px, "| ");
    if (!(pager = getenv("PAGER")))
        pager = DFLT_PAGER;
    (void) strcat(px, pager);
    FILE * f = openfile(px, &pid, NULL);
    if (!f) {
        error("Can't open pipe to %s", pager);
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
    update();
}
