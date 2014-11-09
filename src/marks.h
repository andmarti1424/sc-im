#include "range.h"

// struct mark
// row and col -1 indicates a the node represents
// a mark of a range of cells
// rather than a mark of just one cell
struct mark {
    int row;
    int col;
    srange * rng; 
};
typedef struct mark mark;

void create_mark_array();
void free_marks_array();
mark * get_mark(char c);
void set_cell_mark(char c, int row, int col);
void set_range_mark(char c, struct srange * s);
void fix_marks(int deltar, int deltac, int row_desde, int row_hasta, int col_desde, int col_hasta);
