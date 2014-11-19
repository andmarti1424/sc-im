#include <sys/types.h>
#include <string.h>
#include <curses.h>
#include <ctype.h>
#include <unistd.h>
#include "sc.h"
#include "macros.h"
#include "utils/dictionary.h"
#include "color.h"

static struct ucolor ucolors[N_INIT_PAIRS];
static struct dictionary * d_colors_param = NULL;

struct dictionary * get_d_colors_param() {
    return d_colors_param;
}

// Funcion que genera los initcolor de colores DEFAULT
void start_default_ucolors() {

    // Blanqueo los atributos de todos los colores
    int i;
    for (i=0; i< N_INIT_PAIRS; i++) {
        ucolors[ i ].bold = 0;
        ucolors[ i ].dim = 0;
        ucolors[ i ].reverse = 0;
        ucolors[ i ].stdout = 0;
        ucolors[ i ].underline = 0;
        ucolors[ i ].blink = 0;
    }

    ucolors[ HEADINGS        ].fg = WHITE;
    ucolors[ HEADINGS        ].bg = BLUE;
    ucolors[ WELCOME         ].fg = BLUE;
    ucolors[ WELCOME         ].bg = BLACK;
    ucolors[ CELL_SELECTION  ].fg = BLUE;        // cell selection in headings
    ucolors[ CELL_SELECTION  ].bg = WHITE;

    ucolors[ CELL_SELECTION_SC ].fg = BLACK;      // cell selection in spreadsheet
    ucolors[ CELL_SELECTION_SC ].bg = WHITE;

    ucolors[ NUMBER          ].fg = CYAN;
    ucolors[ NUMBER          ].bg = BLACK;

    ucolors[ STRING          ].fg = RED;
    ucolors[ STRING          ].bg = BLACK;
    ucolors[ STRING          ].bold = 1;

    ucolors[ EXPRESSION      ].fg = YELLOW;
    ucolors[ EXPRESSION      ].bg = BLACK;

    ucolors[ INFO_MSG        ].fg = CYAN;
    ucolors[ INFO_MSG        ].bg = BLACK;
    ucolors[ ERROR_MSG       ].fg = RED;
    ucolors[ ERROR_MSG       ].bg = BLACK;

    ucolors[ MODE            ].fg = WHITE;
    ucolors[ MODE            ].bg = BLACK;
    ucolors[ MODE            ].bold = 1;

    ucolors[ CELL_ID         ].fg = BLUE;
    ucolors[ CELL_ID         ].bg = BLACK;
    ucolors[ CELL_ID         ].bold = 1;
    ucolors[ CELL_FORMAT     ].fg = GREEN;
    ucolors[ CELL_FORMAT     ].bg = BLACK;
    ucolors[ CELL_CONTENT    ].fg = CYAN;
    ucolors[ CELL_CONTENT    ].bg = BLACK;
    ucolors[ CELL_CONTENT    ].bold = 1;

    ucolors[ INPUT           ].fg = WHITE;
    ucolors[ INPUT           ].bg = BLACK;

    ucolors[ NORMAL          ].fg = WHITE;
    ucolors[ NORMAL          ].bg = BLACK;
    ucolors[ CELL_ERROR      ].fg = RED;
    ucolors[ CELL_ERROR      ].bg = BLACK;
    ucolors[ CELL_NEGATIVE   ].fg = GREEN;
    ucolors[ CELL_NEGATIVE   ].bg = BLACK;

    init_pair(HEADINGS,          ucolors[HEADINGS].fg,            ucolors[HEADINGS].bg);
    init_pair(MODE,              ucolors[MODE].fg,                ucolors[MODE].bg);
    init_pair(NUMBER,            ucolors[NUMBER].fg,              ucolors[NUMBER].bg);
    init_pair(STRING,            ucolors[STRING].fg,              ucolors[STRING].bg);
    init_pair(EXPRESSION,        ucolors[EXPRESSION].fg,          ucolors[EXPRESSION].bg);
    init_pair(CELL_ERROR,        ucolors[CELL_ERROR].fg,          ucolors[CELL_ERROR].bg);
    init_pair(CELL_NEGATIVE,     ucolors[CELL_NEGATIVE].fg,       ucolors[CELL_NEGATIVE].bg);
    init_pair(CELL_SELECTION,    ucolors[CELL_SELECTION].fg,      ucolors[CELL_SELECTION].bg);
    init_pair(CELL_SELECTION_SC, ucolors[CELL_SELECTION_SC].fg,   ucolors[CELL_SELECTION_SC].bg);
    init_pair(INFO_MSG,          ucolors[INFO_MSG].fg,            ucolors[INFO_MSG].bg);
    init_pair(ERROR_MSG,         ucolors[ERROR_MSG].fg,           ucolors[ERROR_MSG].bg);
    init_pair(CELL_ID,           ucolors[CELL_ID].fg,             ucolors[CELL_ID].bg);
    init_pair(CELL_FORMAT,       ucolors[CELL_FORMAT].fg,         ucolors[CELL_FORMAT].bg);
    init_pair(CELL_CONTENT,      ucolors[CELL_CONTENT].fg,        ucolors[CELL_CONTENT].bg);
    init_pair(WELCOME,           ucolors[WELCOME].fg,             ucolors[WELCOME].bg);
    init_pair(NORMAL,            ucolors[NORMAL].fg,              ucolors[NORMAL].bg);
    init_pair(INPUT,             ucolors[INPUT].fg,               ucolors[INPUT].bg);

}

// Funcion que setea un color
void set_ucolor(WINDOW * w, int uc) {
    long attr = A_NORMAL;
    if (ucolors[uc].bold)      attr |= A_BOLD;
    if (ucolors[uc].dim)       attr |= A_DIM;
    if (ucolors[uc].reverse)   attr |= A_REVERSE;
    if (ucolors[uc].stdout)    attr |= A_STANDOUT;
    if (ucolors[uc].blink)     attr |= A_BLINK;
    if (ucolors[uc].underline) attr |= A_UNDERLINE;
    wattrset (w, attr | COLOR_PAIR(uc) );
}

// Funcion que crea un diccionario y guarda en el 
// la equivalencia entre las macros y los valores
// de las claves (enteros) que se definen en
// los archivos .sc o a través del comando color.
void set_colors_param_dict() {
    d_colors_param = create_dictionary();

    char str[3];
    str[0]='\0';

    sprintf(str, "%d", WHITE);
    put(d_colors_param, "WHITE", str);
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

    sprintf(str, "%d", HEADINGS);
    put(d_colors_param, "HEADINGS", str);
    sprintf(str, "%d", WELCOME);
    put(d_colors_param, "WELCOME", str);
    sprintf(str, "%d", CELL_SELECTION);
    put(d_colors_param, "CELL_SELECTION", str);
    sprintf(str, "%d", CELL_SELECTION_SC);
    put(d_colors_param, "CELL_SELECTION_SC", str);
    sprintf(str, "%d", NUMBER);
    put(d_colors_param, "NUMBER", str);
    sprintf(str, "%d", STRING);
    put(d_colors_param, "STRING", str);
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
}

void free_colors_param_dict() {
    destroy_dictionary(d_colors_param);
    return;
}

// Funcion que cambia la definicion de un color
// por una definida por el usuario.
// str = es la definicion que se lee del archivo .sc
// o bien ingresada en tiempo de ejecucion a través
// del comando :color str
void chg_color(char * str) {
    
    // creo un diccionario para guardar las claves y valores que figuran en el string
    struct dictionary * d = create_dictionary();

    parse_str(d, str);

    // valido que existan las minimas claves necesarias para cambiar un color
    // TODO validar tambien que los valores que tengan esas claves sean correctos
    if (get(d, "fg") == '\0' || get(d, "bg") == '\0' || get(d, "type") == '\0') {
        error("Color definition incomplete");
        return;
    }

    // cambio el color
    int type = atoi(get(d_colors_param, get(d, "type")));
    ucolors[ type ].fg = atoi(get(d_colors_param, get(d, "fg")));
    ucolors[ type ].bg = atoi(get(d_colors_param, get(d, "bg")));
    if (get(d, "bold")      != '\0')     ucolors[ type ].bold      = atoi(get(d, "bold"));
    if (get(d, "dim")       != '\0')     ucolors[ type ].dim       = atoi(get(d, "dim"));
    if (get(d, "reverse")   != '\0')     ucolors[ type ].reverse   = atoi(get(d, "reverse"));
    if (get(d, "stdout")    != '\0')     ucolors[ type ].stdout    = atoi(get(d, "stdout"));
    if (get(d, "blink")     != '\0')     ucolors[ type ].blink     = atoi(get(d, "blink"));
    if (get(d, "underline") != '\0')     ucolors[ type ].underline = atoi(get(d, "underline"));

    init_pair(type, ucolors[type].fg, ucolors[type].bg);

    destroy_dictionary(d);

    return;
}

/*
// Implementación anterior
struct colorpair    *cpairs[8];
static struct crange    *color_base;

void initcolor(int colornum) {
    use_default_colors();

    if ( ! colornum ) {
    int i;
    for (i = 0; i < 8; i++)
            cpairs[i] = (struct colorpair *) scxmalloc((unsigned)sizeof(struct colorpair));
    }

// default colors (filas y columnas)
    if ( ! colornum || colornum == 1 ) {
    cpairs[0]->fg = COLOR_YELLOW;
    cpairs[0]->bg = COLOR_BLACK;
    cpairs[0]->expr = NULL;
    init_pair(1, cpairs[0]->fg, cpairs[0]->bg);
    }

// PANTALLA DE BIENVENIDA
    if ( ! colornum || colornum == 5 ) {
    cpairs[4]->fg = COLOR_YELLOW;
    cpairs[4]->bg = COLOR_BLACK;
    cpairs[4]->expr = NULL;
    init_pair(5, cpairs[4]->fg, cpairs[4]->bg);
    }

// Expresiones
    if ( ! colornum || colornum == 3 ) {
    cpairs[2]->fg = COLOR_RED;
    cpairs[2]->bg = COLOR_BLACK;
    cpairs[2]->expr = NULL;
    init_pair(3, cpairs[2]->fg, cpairs[2]->bg);
    }

// string value y
// MODO en parte superior derecha
    if ( ! colornum || colornum == 6 ) {
    cpairs[5]->fg = COLOR_CYAN;
    cpairs[5]->bg = COLOR_BLACK;
    cpairs[5]->expr = NULL;
    init_pair(6, cpairs[5]->fg, cpairs[5]->bg);
    }

// numeric value
    if ( ! colornum || colornum == 7 ) {
    cpairs[6]->fg = COLOR_BLUE;
    cpairs[6]->bg = COLOR_BLACK;
    cpairs[6]->expr = NULL;
    init_pair(7, cpairs[6]->fg, cpairs[6]->bg);
    }

// default for cells with errors and error messages
    if ( ! colornum || colornum == 8 ) {
    cpairs[7]->fg = COLOR_GREEN;
    cpairs[7]->bg = COLOR_BLACK;
    cpairs[7]->expr = NULL;
    init_pair(8, cpairs[7]->fg, cpairs[7]->bg);
    }

    if ( ! colornum || colornum == 4 ) {
    cpairs[3]->fg = COLOR_YELLOW;
    cpairs[3]->bg = COLOR_BLACK;
    cpairs[3]->expr = NULL;
    init_pair(4, cpairs[3]->fg, cpairs[3]->bg);
    }

// default for negative numbers ???
    if ( ! colornum || colornum == 2 ) {
    cpairs[1]->fg = COLOR_CYAN;
    cpairs[1]->bg = COLOR_BLUE;
    cpairs[1]->expr = NULL;
    init_pair(2, cpairs[1]->fg, cpairs[1]->bg);
    }


// default for '*' marking cells with attached notes ???

    if ( color && has_colors() )
    color_set(1, NULL);
}

void change_color(int pair, struct enode *e) {
    int v;

    if ((--pair) < 0 || pair > 7) {
    error("Invalid color number");
    return;
    }

    v = (int) eval(e);

    if (!cpairs[pair])
    cpairs[pair] = (struct colorpair *)scxmalloc((unsigned)sizeof(struct colorpair));
    cpairs[pair]->fg = v & 7;
    cpairs[pair]->bg = (v >> 3) & 7;
    cpairs[pair]->expr = e;
    if (color && has_colors())
    init_pair(pair + 1, cpairs[pair]->fg, cpairs[pair]->bg);

    //modflg++;
}

void add_crange(struct ent *r_left, struct ent *r_right, int pair) {
    struct crange *r;
    int minr, minc, maxr, maxc;

    minr = r_left->row < r_right->row ? r_left->row : r_right->row;
    minc = r_left->col < r_right->col ? r_left->col : r_right->col;
    maxr = r_left->row > r_right->row ? r_left->row : r_right->row;
    maxc = r_left->col > r_right->col ? r_left->col : r_right->col;

    if (!pair) {
    if (color_base)
        for (r = color_base; r; r = r->r_next)
        if (    (r->r_left->row == r_left->row) &&
            (r->r_left->col == r_left->col) &&
            (r->r_right->row == r_right->row) &&
            (r->r_right->col == r_right->col)) {
            if (r->r_next)
            r->r_next->r_prev = r->r_prev;
            if (r->r_prev)
            r->r_prev->r_next = r->r_next;
            else
            color_base = r->r_next;
            scxfree((char *)r);
            modflg++;
            FullUpdate++;
            return;
        }
    error("Color range not defined");
    return;
    }

    r = (struct crange *)scxmalloc((unsigned)sizeof(struct crange));
    r->r_left = lookat(minr, minc);
    r->r_right = lookat(maxr, maxc);
    r->r_color = pair;

    r->r_next = color_base;
    r->r_prev = (struct crange *)0;
    if (color_base)
    color_base->r_prev = r;
    color_base = r;

    modflg++;
}

////////////////////////////////////////////////////////////


void clean_crange() {
    register struct crange *cr;
    register struct crange *nextcr;

    cr = color_base;
    color_base = (struct crange *)0;

    while (cr) {
    nextcr = cr->r_next;
    scxfree((char *)cr);
    cr = nextcr;
    }
}

void write_cranges(FILE *f) {
    register struct crange *r;
    register struct crange *nextr;

    for (r = nextr = color_base; nextr; r = nextr, nextr = r->r_next) ;
    while (r) {
    fprintf(f, "color %s", v_name(r->r_left->row, r->r_left->col));
    fprintf(f, ":%s", v_name(r->r_right->row, r->r_right->col));
    fprintf(f, " %d\n", r->r_color);

    r = r->r_prev;
    }
}

void write_colors(FILE *f, int indent) {
    int i, c = 0;

    for (i = 0; i < 8; i++) {
    if (cpairs[i] && cpairs[i]->expr) {
        sprintf(line, "color %d = ", i + 1);
        linelim = strlen(line);
        decompile(cpairs[i]->expr, 0);
        line[linelim] = '\0';
        fprintf(f, "%*s%s\n", indent, "", line);
        if (brokenpipe) return;
        c++;
    }
    }
    if (indent && c) fprintf(f, "\n");
}

int are_colors() {
    return (color_base != 0);
}

void fix_colors(int row1, int col1, int row2, int col2, int delta1, int delta2) {
    int r1, c1, r2, c2;
    struct crange *cr;

    if (color_base)
    for (cr = color_base; cr; cr = cr->r_next) {
        r1 = cr->r_left->row;
        c1 = cr->r_left->col;
        r2 = cr->r_right->row;
        c2 = cr->r_right->col;

    }
}
*/
