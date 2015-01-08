#include <stdlib.h>
#include <ctype.h>
#include <curses.h>
#include <string.h>
#include "maps.h"
#include "string.h"
#include "macros.h"
#include "color.h"          // for set_ucolor
#include "utils/block.h"
#include "utils/string.h"

#define MAXSC 15            // MAXSC is max length of special key word

static map * maps;
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
            if (replace_block_in_block(b, m->in, m->out) == -1) {
                error("error replacing maps");
                return -1;
            }
            r = 1;
            break;
        }
        m = m->psig;
    }
    
    if (r) replace_maps(b);  // recursive mapping here!
    return r;
}

// create list of blocks based on map strings
struct block * get_mapbuf_str (char * str) {
    struct block * buffer = create_buf();
    unsigned short l = strlen(str);
    unsigned short i, j;
    unsigned short is_specialkey = 0;
    char sk[MAXSC+1];
    sk[0] = '\0';

    for (i=0; i<l; i++) {

        // Agrego special keys
        if (str[i] == '<') {
           is_specialkey = 1;

        } else if (str[i] == '>') {
           is_specialkey = 0;
           if (! strcmp(sk, "CR"))                            // CR - ENTER KEY
               addto_buf(buffer, OKEY_ENTER);

           else if (! strncmp(sk, "C-", 2) && strlen(sk) == 3 // C-x
                    && ( (sk[2] > 64 && sk[2] < 91) || (sk[2] > 96 && sk[2] < 123)) )
               addto_buf(buffer, ctl(sk[2]));

           sk[0]='\0';

        } else if (is_specialkey) {
           if (strlen(sk) < MAXSC-1) {
               str[i] = toupper(str[i]);
               add_char(sk, str[i], strlen(sk));
           }

        // Agrego otros caracteres
        } else {
           addto_buf(buffer, (int) str[i]);
        }
    }

    // en caso de que se tenga en el buffer un string del tipo "<algo", sin la terminación ">", se inserta en el buffer
    if (is_specialkey && i == l) {
        j = strlen(sk);
        for (i=0; i<j; i++) addto_buf(buffer, (int) str[l-j+i]);
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

    //info(">f>%c,in:%s,out:%s<<", mode, in, out); get_key();
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

// inserto al comienzo. comentar lo de abajo en caso de desear esta opción
//    m->psig = maps == NULL ? NULL : maps;
//    maps = m;

    // inserto al final
    m->psig= NULL;

    if (maps == NULL) maps = m;
    else
        ((map *) get_last_map())->psig = m;
 
    return;
}
