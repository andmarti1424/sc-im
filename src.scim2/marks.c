#include <stdlib.h>
#include "marks.h"
#include "macros.h"

#define NUM_MARKS 128
static struct mark * marks;

// 'a' - 'z' = 26
// '0' - '1' = 10
void create_mark_array() {
    marks = (struct mark *) calloc(NUM_MARKS, sizeof(struct mark) );
    return;
}

void free_marks_array() {
    free(marks);
    return;
}

// 'a' = 97
struct mark * get_mark(char c) {
    return (marks + c);
}

void set_range_mark(char c, struct srange * s) {
    // Delete marked ranges when recording a new one with same char
    del_ranges_by_mark(c);

    (marks + c)->rng = s;
    (marks + c)->row = -1;
    (marks + c)->col = -1;
    return;
}

void set_cell_mark(char c, int row, int col) {
    // Delete marked ranges when recording a new one with same char
    del_ranges_by_mark(c);

    (marks + c)->rng = NULL;
    (marks + c)->row = row;
    (marks + c)->col = col;
    return;
}

void fix_marks(int deltar, int deltac, int row_desde, int row_hasta, int col_desde, int col_hasta) {
    int i;
    for (i = 0; i < NUM_MARKS-1; i++) {
        struct mark * m = marks + i;
        if (m->row >= row_desde && m->row <= row_hasta &&
            m->col >= col_desde && m->col <= col_hasta ) {
                m->row += deltar;
                m->col += deltac;
        }
    }
    return;
}
