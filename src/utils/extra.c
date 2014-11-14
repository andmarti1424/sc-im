#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "extra.h"
#include <curses.h>
#include "../sc.h"
#include "../macros.h"

#define freen(x)	nofreeNULL(x)
void nofreeNULL(void *x) {
    if (x != NULL)
        free(x);
    return;
}

// devuelve el nombre de una celda, a partir del numero de row y col
// ej D4.
char * v_name(int row, int col) {
    struct ent *v;
    struct range *r;
    static char buf[20];

    v = lookat(row, col);
    if (!find_range((char *)0, 0, v, v, &r)) {
        return (r->r_name);
    } else {
        (void) sprintf(buf, "%s%d", coltoa(col), row);
        return (buf);
    }
}

// funcion que parsea de un buf el nombre de una celda
// ignora los primeros bloques en caso de especificarse
char * parse_cell_name(int ignore_first_blocks, struct block * buf_in) {
    struct block * b = buf_in;
    static char cell_name[3]; //length of max col is 3 (ZZZ)
    cell_name[0] = '\0';

    while (ignore_first_blocks--) b = b->pnext;
    while( b != NULL) {
          (void) sprintf(cell_name, "%s%c", cell_name, b->value);
          b = b->pnext;
    }
    return cell_name;
}
