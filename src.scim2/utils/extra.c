#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <curses.h>
#include "extra.h"
#include "../sc.h"
#include "../macros.h"
//#include "../range.h"

extern int find_range(char * name, int len, struct ent * lmatch, struct ent * rmatch, struct range ** rng);

#define freen(x) nofreeNULL(x)
void nofreeNULL(void *x) {
    if (x != NULL)
        free(x);
    return;
}

// Returns the ROW/COL cell name
// ex.: D4.
char * v_name(int row, int col) {
    struct ent *v;
    struct range *r;
    static char buf[20];

    v = lookat(row, col);
    if ( ! find_range((char *) 0, 0, v, v, &r) ) {
        return (r->r_name);
    } else {
        (void) sprintf(buf, "%s%d", coltoa(col), row);
        return (buf);
    }
}

// Parse BUF_IN to get a cell name. Skip first blocks with IGNORE_FIRST_BLOCKS
char * parse_cell_name(int ignore_first_blocks, struct block * buf_in) {
    struct block * b = buf_in;
    static char cell_name[3]; //length of max col is 3 (ZZZ)
    cell_name[0] = '\0';

    while (ignore_first_blocks--) b = b->pnext;
    while( b != NULL) {
          (void) sprintf(cell_name + strlen(cell_name), "%c", b->value);
          b = b->pnext;
    }
    return cell_name;
}
