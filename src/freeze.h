#include "sc.h"

extern struct frange * freeze_ranges;

void add_frange(struct ent * tl_ent, struct ent * br_ent, char type);

// freeze ranges
struct frange {
    struct ent * tl;
    struct ent * br;
    char type;
    struct frange * next; /* chained ranges */
    //struct frange * prev;
};
