#include "macros.h"

#define N_INIT_PAIRS      19

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
void color_cell(int r, int c, int rf, int cf, char * detail);
void unformat(int r, int c, int rf, int cf);
void set_colors_param_dict();
void free_colors_param_dict();
void chg_color(char * str);
int same_ucolor(struct ucolor * u, struct ucolor * v);
int redefine_color(char * color, int r, int g, int b);
