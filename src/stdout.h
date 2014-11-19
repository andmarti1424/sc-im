#include <ncurses.h>
extern WINDOW * main_win;
extern WINDOW * input_win;

void start_stdout();
void stop_stdout();
void show_sc_row_headings(WINDOW * win, int mxrow);
void show_sc_col_headings(WINDOW * win, int mxcol, int mxrow);
extern int offscr_sc_rows, offscr_sc_cols;
int calc_offscr_sc_rows();
int calc_offscr_sc_cols();

void write_j(WINDOW * win, const char * word, const unsigned int row, const unsigned int justif);
void stdin_wa(int wait);
void clr_header(WINDOW * win, int row);
void print_mode(WINDOW * win);
void print_mult_pend(WINDOW * win);
//void print_statusbar(WINDOW * win, char sm); // no usado mas

extern unsigned int curmode;
extern struct srange * ranges;

int calc_cols_show();
void show_cursor(WINDOW * win);
void show_content(WINDOW * win, int mxrow, int mxcol);

//#include "sc.h"
//void show_text_content_of_cell(WINDOW * win, struct ent ** p, int row, int col, int r, int c);
//void show_numeric_content_of_cell(WINDOW * win, struct ent ** p, int col, int r, int c);

//void add_cell_detail(char * d, struct ent * p1); //esto da error al descomentar
void show_celldetails(WINDOW * win);
void update(void);

void yyerror(char *err);               // error routine for yacc (gram.y)
void show_text(char * val);