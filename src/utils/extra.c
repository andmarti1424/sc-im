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

