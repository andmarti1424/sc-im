#include "macros.h"

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
int redefine_color(char * color, int r, int g, int b);
