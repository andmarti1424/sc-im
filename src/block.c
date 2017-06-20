#include "block.h"
#include <string.h>
#include "utils/string.h"


/*
 * block in block: find out if the int elements in the bus list are
 * inside the ori list. If so, return the ori position of it.
 * TODO: IMPROVE this. Use two while statements in order to create an array
 * from a block, and make them work separately.
 * Leave the logic unmodified
 */
int block_in_block (struct block * o, struct block * b) {
    int lori = get_bufsize(o);
    int lbus = get_bufsize(b);

    if (!lori || !lbus || lbus > lori) return -1;

    // Generate arrays from two block lists
    int ori[lori];
    int bus[lbus];

    struct block * aux = o;
    int i = 0;
    while (aux != NULL) {
        ori[i++] = aux->value;
        aux = aux->pnext;
    }

    aux = b;
    i=0;
    while (aux != NULL) {
        bus[i++] = aux->value;
        aux = aux->pnext;
    }

    int encontrado;
    int k;
    i = 0;
    while (i <= lori-lbus) {
        encontrado = 0;
        for (k = 0; k < lbus; k++) {
            if ( ori[i + k] != bus[k] ) {
                encontrado = -1;
                break;
            }
        }
        if (encontrado != -1) return i;
        i++;
    }
    return -1;
}

/*
 * Replace the content of the block list "olist"
 * replace the nodes of the 'in' list
 * with the nodes of the 'out' list
 * Returns 0 on success, -1 on error.
 */
int replace_block_in_block (struct block * olist, struct block * in, struct block * out) {
    struct block * ori = olist;

    int lori = get_bufsize(ori);
    int lin = get_bufsize(in);
    int lout = get_bufsize(out);

    if ( ! lin || ! lori || ! lout ) return -1;

    //info("%d %d %d", lori, lin, lout); get_key();

    // Remove the 'in' part of "olist"
    int pos = block_in_block (olist, in);

    // Remove the 'pos' position of the "olist" list
    while (lin--) del_buf(ori, lin+pos);

    // Then add the nodes of the 'out' list to "olist"
    while (out != NULL) {
        int e = out->value;
        addto_buf(olist, e);
        out = out->pnext;
    }

    return 0;
}
