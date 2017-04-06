#include <ncurses.h>
#include <wchar.h>
#include <lua.h>
#include "color.h"

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
extern unsigned int curmode;
extern struct srange * ranges;
extern struct ent ** p;

void start_screen();
void stop_screen();
int ui_getch(wint_t * wd);
int ui_getch_b(wint_t * wd);
void ui_clr_header(int row);
void ui_print_mult_pend();
void ui_show_header();
void ui_show_celldetails();
void ui_print_mode();

void do_welcome();
void update(int header);
void show_content(WINDOW * win, int mxrow, int mxcol);
void show_sc_row_headings(WINDOW * win, int mxrow);
void show_sc_col_headings(WINDOW * win, int mxcol);
void pad_and_align (char * str_value, char * numeric_value, int col_width, int align, int padding, wchar_t * str_out);
int get_formated_value(struct ent ** p, int col, char * value);
int calc_offscr_sc_rows();
int calc_offscr_sc_cols();
int calc_cols_show();
void show_cursor(WINDOW * win);
void handle_cursor();
void add_cell_detail(char * d, struct ent * p1);
void write_j(WINDOW * win, const char * word, const unsigned int row, const unsigned int justif);

void yyerror(char *err);               // error routine for yacc (gram.y)
void show_text(char * val);
void ui_bail(lua_State *L, char * msg);
char * ui_query(char * initial_msg);
void ui_set_ucolor(WINDOW * w, struct ucolor * uc);
void ui_start_colors();
