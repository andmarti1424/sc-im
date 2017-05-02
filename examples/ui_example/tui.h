#include <ncurses.h>
#include <wchar.h>
#include <lua.h>
#include "color.h"

#define LINES 25
#define COLS 80

#define DEFAULT_COLOR     -1
#define BLACK             COLOR_BLACK
#define RED               COLOR_RED
#define GREEN             COLOR_GREEN
#define YELLOW            COLOR_YELLOW
#define BLUE              COLOR_BLUE
#define MAGENTA           COLOR_MAGENTA
#define CYAN              COLOR_CYAN
#define WHITE             COLOR_WHITE

extern int offscr_sc_rows, offscr_sc_cols;
extern int center_hidden_rows, center_hidden_cols;
extern unsigned int curmode;
extern struct srange * ranges;
extern struct ent ** p;

void ui_start_screen();
void ui_stop_screen();
int ui_getch(wint_t * wd);
int ui_getch_b(wint_t * wd);
void ui_clr_header(int row);
void ui_print_mult_pend();
void ui_show_header();
void ui_show_celldetails();
void ui_print_mode();
void ui_do_welcome();
void ui_update(int header);
int ui_get_formated_value(struct ent ** p, int col, char * value);
void ui_handle_cursor();
void yyerror(char *err);               // error routine for yacc (gram.y)
void ui_show_text(char * val);
void ui_bail(lua_State *L, char * msg);
char * ui_query(char * initial_msg);
void ui_start_colors();
void ui_sc_msg(char * s, int type, ...);

void ui_set_ucolor(WINDOW * w, struct ucolor * uc);
void ui_show_content(WINDOW * win, int mxrow, int mxcol);
void ui_show_sc_row_headings(WINDOW * win, int mxrow);
void ui_show_sc_col_headings(WINDOW * win, int mxcol);
void ui_add_cell_detail(char * d, struct ent * p1);
void ui_write_j(WINDOW * win, const char * word, const unsigned int row, const unsigned int justif);
void ui_show_cursor(WINDOW * win);
