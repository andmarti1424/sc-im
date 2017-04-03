#include <stdlib.h>

#include "freeze.h"
#include "macros.h"
#include "tui.h"

struct frange * freeze_ranges = NULL;

// type = 'r' -> freeze a row
// type = 'c' -> freeze a col
// type = 'a' -> freeze an area
void add_frange(struct ent * tl_ent, struct ent * br_ent, char type) {
    struct frange * f = (struct frange *) malloc(sizeof(struct frange));
    f->tl = tl_ent;
    f->br = br_ent;
    f->type = type;
    f->next = freeze_ranges;
    freeze_ranges = f;

    //sc_debug("freeze range: %d %d %d %d - type:%c", freeze_ranges->tl->row, freeze_ranges->tl->col, freeze_ranges->br->row, freeze_ranges->br->col, type);
    return;
}

void remove_frange() {
    extern int center_hidden_rows;
    extern int center_hidden_cols;
    free(freeze_ranges);
    freeze_ranges = NULL;
    center_hidden_rows = 0;
    center_hidden_cols = 0;
    update(TRUE);
    return;
}
