#define N_INIT_PAIRS      18

#define BLACK             COLOR_BLACK
#define RED               COLOR_RED
#define GREEN             COLOR_GREEN
#define YELLOW            COLOR_YELLOW
#define BLUE              COLOR_BLUE
#define MAGENTA           COLOR_MAGENTA
#define CYAN              COLOR_CYAN
#define WHITE             COLOR_WHITE

#define HEADINGS          0
#define WELCOME           1
#define CELL_SELECTION    2
#define CELL_SELECTION_SC 3
#define NUMB              4
#define STRG              5
#define DATEF             6
#define EXPRESSION        7
#define INFO_MSG          8
#define ERROR_MSG         9
#define MODE              10
#define CELL_ID           11
#define CELL_FORMAT       12
#define CELL_CONTENT      13
#define INPUT             14
#define NORMAL            15
#define CELL_ERROR        16
#define CELL_NEGATIVE     17
//#define DEFAULT         18


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

struct ucolor ucolors[N_INIT_PAIRS];

struct dictionary * get_d_colors_param();
void start_default_ucolors();
void set_ucolor(WINDOW * w, struct ucolor * uc);
void color_cell(int r, int c, int rf, int cf, char * detail);
void set_colors_param_dict();
void free_colors_param_dict();
void chg_color(char * str);
int same_ucolor(struct ucolor * u, struct ucolor * v);
