#include "sc.h"

extern struct frange * freeze_ranges;

void add_frange(struct ent * tl_ent, struct ent * br_ent);

// freeze ranges
struct frange {
    struct ent * tl;
    struct ent * br;
    struct frange * next; /* chained ranges */
    //struct frange * prev; 
};
