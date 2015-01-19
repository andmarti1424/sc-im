#define N_INIT_PAIRS      19

#define HEADINGS          1
#define WELCOME           2
#define CELL_SELECTION    3
#define CELL_SELECTION_SC 4
#define NUMB              5
#define STRG              6
#define DATEF             7
#define EXPRESSION        8
#define INFO_MSG          9
#define ERROR_MSG         10
#define MODE              11
#define CELL_ID           12
#define CELL_FORMAT       13
#define CELL_CONTENT      14
#define INPUT             15
#define NORMAL            16
#define CELL_ERROR        17
#define CELL_NEGATIVE     18

#define BLACK             COLOR_BLACK
#define RED               COLOR_RED
#define GREEN             COLOR_GREEN
#define YELLOW            COLOR_YELLOW
#define BLUE              COLOR_BLUE
#define MAGENTA           COLOR_MAGENTA
#define CYAN              COLOR_CYAN
#define WHITE             COLOR_WHITE

struct ucolor {
    int fg;
    int bg;
    int bold;
    int dim;
    int reverse;
    int standout;
    int underline;
    int blink;
};

struct dictionary * get_d_colors_param();
void start_default_ucolors();
void set_ucolor(WINDOW * w, int uc);
void set_colors_param_dict();
void free_colors_param_dict();
void chg_color(char * str);
