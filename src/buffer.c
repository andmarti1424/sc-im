#include "buffer.h"
#include <stdlib.h>
#include "macros.h"

// Create buffer as list of blocks
struct block * create_buf() {
    struct block * b = (struct block *) malloc(sizeof(struct block));
    b->value = '\0';
    b->pnext = NULL;
    return b;
}

// Funcion que agrega un item de tipo int a un buffer
void addto_buf(struct block * buf, int d) {
    struct block * aux = buf;

    if (buf->value == '\0') {
        buf->value = d;
    } else {
        struct block * b = (struct block *) malloc(sizeof(struct block));
        b->value = d;
        b->pnext = NULL;

        while (aux->pnext != NULL)
            aux = aux->pnext;
        aux->pnext = b;
    }
    return;
}

// Funcion que copia un buffer en otro
void copybuffer(struct block * origen, struct block * destino) {
    flush_buf(destino);
    int len = get_bufsize(origen);
    int i;
    for (i=0; i < len; i++)
        addto_buf(destino, get_bufval(origen, i));
    return;
}

// Funcion que borra el elemento "pos" de un buffer
//FIXME
void del_buf (struct block * buf, int pos) {
    int i;
    struct block * ant = buf;
    struct block * cur = buf;
    for (i = 0; i < pos; i++) {
        ant = cur; 
        cur = cur->pnext; 
    }
    if (ant == cur) {
        cur->value = '\0';        
        //buf = cur->pnext; //FIXME
        //free(cur);
    } else {
        ant->pnext = cur->pnext;
        free(cur);
    }
    return;
}

void flush_buf (struct block * buf) {
    if (buf == NULL) return;
     
    struct block * aux, * np;
    for (aux = buf->pnext; aux != NULL; aux = np)
    {
        np = aux->pnext;
        free(aux);
    }
    buf->value = '\0';
    buf->pnext = NULL;

    return;
}

// Funcion que borra todos los bloques de un buffer
// incluyendo el nodo inicial
void erase_buf (struct block * buf) {
    flush_buf(buf);
    free(buf);
    return;
}

// get size of buffer (included special chars)
int get_bufsize(struct block * buf) {
    struct block * b_aux = buf;
    if (b_aux == NULL || b_aux->value == '\0') return 0;
    int c = 0;
    while (b_aux != NULL) {
        c++;
        b_aux = b_aux->pnext;
    }
    return c;
}

// get printable buffer lenght (excluded special chars)
// (special chars should never be printed in screen)
int get_pbuflen(struct block * buf) {
    struct block * b_aux = buf;
    if (b_aux == NULL || b_aux->value == '\0') return 0;
    int c = 0;
    while (b_aux != NULL) {
        if ( !is_idchar(b_aux->value) ) c++;
        b_aux = b_aux->pnext;
    }
    return c;
}

// return the int value of the n block
int get_bufval(struct block * buf, int d) {
    int i;
    struct block * b_aux = buf;
    for (i = 0; i < d; i++) {
        b_aux = b_aux->pnext;
    }
    return b_aux->value;
}

// return if an int value is found in a buffer
int find_val(struct block * buf, int value) {
    struct block * b_aux = buf;
    while ( b_aux != NULL && b_aux->value != '\0' ) {
        if (b_aux->value == value) return 1;
        b_aux = b_aux->pnext;
    }
    return 0; 
}

// Funcion que borra el primer elemento de un buffer
struct block * dequeue (struct block * buf) {
    if (buf == NULL) return buf;
    struct block * sig;
    if (buf->value == '\0') return buf;

    if (buf->pnext == NULL) {
       buf->value = '\0';
    } else {
        sig = buf->pnext;
        //free(buf);
        buf = sig;
    }
    return buf;
}
