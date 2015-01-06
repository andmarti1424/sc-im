#include "sc.h"

struct srange {
    int tlrow;     // top left row
    int tlcol;     // top left col
    int brrow;     // bottom right row
    int brcol;     // bottom right col
    int orig_row;  // row of selected cell before creating range
    int orig_col;  // col of selected cell before creating range
    char marks[2]; // marks used for creating the range
    int selected;
    struct srange * pnext;
};
typedef struct srange srange;

extern srange * ranges;

srange * create_range(char c, char d, struct ent * tl, struct ent * br);
int find_range(char * name, int len, struct ent * lmatch, struct ent * rmatch, struct range ** rng);

void unselect_ranges();
void free_ranges ();
void del_ranges_by_mark (char c);
srange * get_range_by_pos(int pos);
srange * get_range_by_marks (char c, char d);
int is_range_selected();
srange * get_selected_range();

srange * create_custom_range(int tlrow, int tlcol, int brrow, int brcol);
void free_custom_range(srange * sr);
void clean_range();

