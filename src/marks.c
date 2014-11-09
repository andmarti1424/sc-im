#include <stdlib.h>
#include "marks.h"

static struct mark * marks;

// 'a' - 'z' = 26
// TODO: deben poder marcarse 0-9 (para exportación)
void create_mark_array() {
    marks = (struct mark *) calloc( 26, sizeof(struct mark) );
    return;
}

void free_marks_array() {
    free(marks);
    return;
}

// 'a' = 97
struct mark * get_mark(char c) {
    return (marks + c - 97);
}

void set_range_mark(char c, struct srange * s) {
    (marks + c - 97)->rng = s;
    (marks + c - 97)->row = -1;
    (marks + c - 97)->col = -1;
    return;
}

void set_cell_mark(char c, int row, int col) {
    // Al grabar una marca borro los rangos que utilizaban esa marca !!
    del_ranges_by_mark(c);

    (marks + c - 97)->row = row;
    (marks + c - 97)->col = col;

    return;
}

void fix_marks(int deltar, int deltac, int row_desde, int row_hasta, int col_desde, int col_hasta) {
    int i;
    for (i = 0; i < 26; i++) {
        struct mark * m = marks + i;
        if (m->row >= row_desde && m->row <= row_hasta &&
            m->col >= col_desde && m->col <= col_hasta ) {
                m->row += deltar;
                m->col += deltac;
        }
    }
    return;
}
