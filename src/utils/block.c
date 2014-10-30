#include "block.h"
#include <string.h>
#include "../buffer.h"

// Funcion que determina si dentro de la lista ori
// se encuentran los int que figuran en la lista bus
// En caso de encontrarse, devuelve la posicion
// dentro de la lista ori donde se encuentra.

// TODO - MEJORAR esto. Sacar dos while que arman un array a partir de un bloque
// y hacerlo funciones separadas.
// La logica dejarla
int block_in_block (struct block * o, struct block * b) {
    int lori = get_bufsize(o);
    int lbus = get_bufsize(b);

    if (!lori || !lbus || lbus > lori) return -1;

    // Genero arrays a partir de listas de bloques
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

// Funcion que reemplaza del contenido de la lista de block "olist"
// los nodos correspondientes a la lista in
// con los nodos de la lista out
// Retorna 0 en caso de exito, -1 en caso de error.
int replace_block_in_block (struct block * olist, struct block * in, struct block * out) {
    struct block * ori = olist;

    int lori = get_bufsize(ori);
    int lin = get_bufsize(in);
    int lout = get_bufsize(out);

    if ( !lin || !lori || !lout ) return -1;

    // Primero borro de olist la porcion correspondiente a in
    int pos = block_in_block (olist, in);
    // Borro posicion pos de la lista olist
    while (lin--) del_buf (olist, pos); //FIXME ojo cuando pos es 0

    // Luego agrego los nodos de la lista out a la lista olist
    while (out != NULL) {
        int e = out->value;
        addto_buf(olist, e); 
        out = out->pnext;
    }

    return 0;
}

void block_to_str(struct block * b, char * out) {
    struct block * b_aux = b;
    
    if (b_aux == NULL || b_aux->value == '\0') return;
         
    while (b_aux != NULL) {
        if (b_aux->value > 31 && b_aux->value < 127) {
           add_char(out, (char) b_aux->value, strlen(out));
        } else if (b_aux->value == 27) {
           return;
        }
        b_aux = b_aux->pnext;
    }
    return;
}

// return the contents of a buffer as a single string
/* FIXME esta funcion tiene bugs???
char * get_bufcont(struct block * buf) {
    struct block * aux = buf;

    int slen = get_pbuflen(aux);
    char * strcmd = (char*) malloc((slen + 1) * sizeof(char));
    strcmd[0] = '\0';

    char d[2];
    d[1]='\0';
    while ( aux != NULL && aux->value != '\0' ) {
        d[0] = (char) aux->value; //FIXME when we have special keys like esc
        strcat(strcmd, d);
        aux = aux->pnext;
    }
    return strcmd;
}
*/

