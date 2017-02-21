#include <stdlib.h>
#include "freeze.h"
#include "macros.h"

struct frange * freeze_ranges = NULL;

void add_frange(struct ent * tl_ent, struct ent * br_ent) {
    struct frange * f = (struct frange *) malloc(sizeof(struct frange));
    f->tl = tl_ent;
    f->br = br_ent;
    f->next = freeze_ranges;
    freeze_ranges = f;

    sc_debug("freeze range: %d %d %d %d", freeze_ranges->tl->row, freeze_ranges->tl->col, freeze_ranges->br->row, freeze_ranges->br->col);
    return;
}
