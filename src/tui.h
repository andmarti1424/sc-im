#include <ncurses.h>
#include <wchar.h>

extern WINDOW * main_win;
extern WINDOW * input_win;
extern int offscr_sc_rows, offscr_sc_cols;
extern unsigned int curmode;
extern struct srange * ranges;
extern struct ent ** p;

void start_screen();
void stop_screen();
void do_welcome();
void update(int header);
void show_content(WINDOW * win, int mxrow, int mxcol);
void show_sc_row_headings(WINDOW * win, int mxrow);
void show_sc_col_headings(WINDOW * win, int mxcol);
void show_celldetails(WINDOW * win);
void pad_and_align (char * str_value, char * numeric_value, int col_width, int align, int padding, wchar_t * str_out);
int get_formated_value(struct ent ** p, int col, char * value);
int calc_offscr_sc_rows();
int calc_offscr_sc_cols();
int calc_cols_show();
void clr_header(WINDOW * win, int row);
void print_mode(WINDOW * win);
void print_mult_pend(WINDOW * win);
void show_cursor(WINDOW * win);
void show_header(WINDOW * win);
void handle_cursor();
void add_cell_detail(char * d, struct ent * p1);
void write_j(WINDOW * win, const char * word, const unsigned int row, const unsigned int justif);
void yyerror(char *err);               // error routine for yacc (gram.y)
void show_text(char * val);
