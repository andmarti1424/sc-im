#define N_INIT_PAIRS      17
#define HEADINGS          1
#define WELCOME           2
#define CELL_SELECTION    3
#define CELL_SELECTION_SC 4

#define NUMBER            5
#define STRING            6
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
    int stdout;
    int underline;
    int blink;
};

void start_default_ucolors();
void set_ucolor(WINDOW * w, int uc);

void set_colors_param_dict();
void free_colors_param_dict();

struct dictionary * get_d_colors_param();