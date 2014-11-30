#include <stdlib.h>
#include <curses.h>
#include "maps.h"
#include "string.h"
#include "macros.h"
#include "buffer.h"
#include <string.h>

static map * maps;
//unsigned int curmode;
static int mapdepth = 0;

int replace_maps (struct block * b) {
    int r = 0;

    if (++mapdepth == 1000) {
        error("recursive mapping");
        flush_buf(b);
        mapdepth = 0;
        return 0;
    }

    // Recorro mapeos de sesion
    map * m = maps;
    while (m != NULL) {
        // Verifico si algun mapeo existe en el buffer b
        int pos = block_in_block(b, m->in);
        if (pos != -1 && m->mode == curmode) {
            // se reemplaza contenido m->in por m->out en la lista b
            replace_block_in_block (b, m->in, m->out);
            r = 1;
            break;
        }
        m = m->psig;
    }
    if (r) replace_maps(b); 
    return r;
}

#define MAXSC 25
/* ************************************* 
create list of blocks from map strings
MAXSC is max length of special key word
   ************************************* */
struct block * get_mapbuf_str (char * str) {
    struct block * buffer = create_buf();
    unsigned short l = strlen(str);
    unsigned short i;
    unsigned short issk=0;
    char sk[MAXSC];
    sk[0]='\0';

    for (i=0; i<l; i++) {
        if (str[i] == '<') {
           issk = 1;
        } else if (str[i] == '>') {
           issk = 0;
           // Agrego special keys
           if (strcasecmp(sk, "cr") == 0)
               addto_buf(buffer, 10);
           sk[0]='\0';
        } else if (issk) {
           if (strlen(sk) < MAXSC-1) strcat(sk, &str[i]);
        } else {
           addto_buf(buffer, (int) str[i]);
        }
    }
    return buffer;
}

// Funcion que elimina todos los mapeos y libera la memoria asignada
void del_maps () {
    map * m = maps;
    map * e = m;
    while (m != NULL) {
        e = m->psig;
        erase_buf(m->out);
        erase_buf(m->in);
        free(m);
        m = e;
    }
    return;
}

// Funcion que retorna el ultimo mapeo de la lista de mapeos de sesion
map * get_last_map() {
    map * m = maps;
    map * e = m; 

    while (m != NULL) {
        e = m;
        m = m->psig;
    }
    return e;
}

// Funcion que agrega un mapeo a la lista de mapeos de sesion
// recibe un comando in, un out, y un caracter 'type' que indica el modo
// en donde tiene efecto el mapeo
void add_map(char * in, char * out, char mode) {
    //info(">>%c,%s,%s<<", mode, in, out);
    map * m = (map *) malloc (sizeof(map));
    m->out = (struct block *) get_mapbuf_str(out);
    m->in = (struct block *) get_mapbuf_str(in);
    
    switch (mode) {
    case 'n':
        m->mode = NORMAL_MODE;
        break;

    case 'i':
        m->mode = INSERT_MODE;
        break;
    }
    m->psig= NULL;

    if (maps == NULL) maps = m;
    else {
        ((map *) get_last_map())->psig = m;
    }
    return;
}
