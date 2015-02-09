#include <stdlib.h>
#include <ctype.h>
#include <curses.h>
#include <string.h>
#include "maps.h"
#include "string.h"
#include "macros.h"
#include "color.h"          // for set_ucolor
#include "block.h"
#include "utils/string.h"

static map * maps;
static int mapdepth = 0;
int len_maps = 0;

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
    
    if (r && m->recursive) replace_maps(b);  // recursive mapping here!
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
           if (! strcmp(sk, "CR"))                                  // CR - ENTER key
               addto_buf(buffer, OKEY_ENTER);
           else if (! strcmp(sk, "TAB"))                            // TAB
               addto_buf(buffer, OKEY_TAB);
           else if (! strcmp(sk, "LEFT"))                           // LEFT
               addto_buf(buffer, OKEY_LEFT);
           else if (! strcmp(sk, "RIGHT"))                          // RIGHT
               addto_buf(buffer, OKEY_RIGHT);
           else if (! strcmp(sk, "DOWN"))                           // DOWN
               addto_buf(buffer, OKEY_DOWN);
           else if (! strcmp(sk, "UP"))                             // UP
               addto_buf(buffer, OKEY_UP);
           else if (! strcmp(sk, "DEL"))                            // DEL
               addto_buf(buffer, OKEY_DEL);
           else if (! strcmp(sk, "BS"))                             // BS
               addto_buf(buffer, OKEY_BS);
           else if (! strcmp(sk, "HOME"))                           // HOME
               addto_buf(buffer, OKEY_HOME);
           else if (! strcmp(sk, "END"))                            // END
               addto_buf(buffer, OKEY_END);
           else if (! strcmp(sk, "PGDOWN"))                         // PGDOWN
               addto_buf(buffer, OKEY_PGDOWN);
           else if (! strcmp(sk, "PGUP"))                           // PGUP
               addto_buf(buffer, OKEY_PGUP);
           else if (! strncmp(sk, "C-", 2) && strlen(sk) == 3       // C-X
                    && ( (sk[2] > 64 && sk[2] < 91) || (sk[2] > 96 && sk[2] < 123)) )
               addto_buf(buffer, ctl(tolower(sk[2])));

           sk[0]='\0';

        } else if (is_specialkey && strlen(sk) < MAXSC-1) {
           add_char(sk, str[i], strlen(sk));

        // Agrego otros caracteres
        } else {
           addto_buf(buffer, (int) str[i]);
        }
    }

    // en caso de que se tenga en el buffer un string del tipo "<algo", sin la terminación ">", se inserta en el buffer
    if (is_specialkey && i == l) {
        j = strlen(sk);
        addto_buf(buffer, '<');
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

// funcion que indica si un string ya está mapeado para un determinado modo
// devuelve -1 en caso de no existir, o la posicion dentro de maps en caso 
// de existir.
int exists_map(char * in, int mode) {
    map * m = maps;
    char str_in[MAXMAPITEM] = "";
    int pos = -1;

    while (m != NULL) {
        pos++;
        get_mapstr_buf(m->in, str_in);
        if ( ! strcmp(in, str_in) && m->mode == mode) {
            return pos; 
        }
        m = m->psig;
    }
    return -1;
}
// Funcion que agrega un mapeo a la lista de mapeos de sesion
// recibe un comando in, un out, y un caracter 'type' que indica el modo
// en donde tiene efecto el mapeo
void add_map(char * in, char * out, int mode, short recursive) {
    map * m;
    
    // si el mapeo ya existe, reemplazo su contenido
    // guardando su posicion!
    int exists = exists_map(in, mode);
    if (exists == -1) {
        m = (map *) malloc (sizeof(map));
    } else {
        m = maps;
        while (exists--) m = m->psig;
        erase_buf(m->in);
        erase_buf(m->out);
        exists = TRUE;
    }

    m->out = (struct block *) get_mapbuf_str(out);
    m->in = (struct block *) get_mapbuf_str(in);
    m->mode = mode;
    m->recursive = recursive;

    if (exists == TRUE) return; // in case a map was updated and not created!

// inserto al comienzo. comentar lo de abajo en caso de desear esta opción
//    m->psig = maps == NULL ? NULL : maps;
//    maps = m;

    // inserto al final
    m->psig= NULL;

    if (maps == NULL) maps = m;
    else
        ((map *) get_last_map())->psig = m;
 


    len_maps++;

    return;
}

// funcion que elimina un mapeo en un determinado modo
// recibe como parámetro un char *, además del modo
void del_map(char * in, int mode) {
    map * ant;
    map * m = maps;

    if (m == NULL) return;

    // Si el nodo a eliminar es el primero
    char strin [MAXSC * get_bufsize(m->in)];
    get_mapstr_buf(m->in, strin);
    if ( ! strcmp(in, strin) && mode == m->mode) {
        maps = m->psig;
        erase_buf(m->out);
        erase_buf(m->in);
        free(m);
        len_maps--;
        return;
    }

    // si el nodo esta al medio o final
    ant = m;
    m = m->psig;
    while (m != NULL) {
        char strin [MAXSC * get_bufsize(m->in)];
        get_mapstr_buf(m->in, strin);
        if ( ! strcmp(in, strin) && mode == m->mode) {
            ant->psig = m->psig;
            erase_buf(m->out);
            erase_buf(m->in);
            free(m);
            m = ant->psig;
            len_maps--;
            return;
        }
        ant = m;
        m = m->psig;
    }
    return;
}

// funcion que convierte un block en una cadena de caracteres
// los caracteres especiales los guarda en el formato <CR>..
void get_mapstr_buf (struct block * b, char * str) {    
    struct block * a = b;
    int i, len = get_bufsize(a);
    
    str[0]='\0';
    for (i=0; i < len; i++) {
        if (isprint(a->value)) {
            sprintf(str + strlen(str), "%c", (char) a->value);
        } else if (a->value == OKEY_ENTER) {
            strcat(str, "<CR>");                                 // CR - ENTER
        } else if ( a->value == (uncl(a->value) & 0x1f)) {       // C-x
            sprintf(str + strlen(str), "<C-%c>", uncl(a->value));
        } else if (a->value == OKEY_TAB) {
            strcat(str, "<TAB>");                                // TAB
        } else if (a->value == OKEY_LEFT) {
            strcat(str, "<LEFT>");                               // LEFT
        } else if (a->value == OKEY_RIGHT) {
            strcat(str, "<RIGHT>");                              // RIGHT
        } else if (a->value == OKEY_DOWN) {
            strcat(str, "<DOWN>");                               // DOWN
        } else if (a->value == OKEY_UP) {
            strcat(str, "<UP>");                                 // UP
        } else if (a->value == OKEY_DEL) {
            strcat(str, "<DEL>");                                // DEL
        } else if (a->value == OKEY_BS) {
            strcat(str, "<BS>");                                 // BS
        } else if (a->value == OKEY_HOME) {
            strcat(str, "<HOME>");                               // HOME
        } else if (a->value == OKEY_END) {
            strcat(str, "<END>");                                // END
        } else if (a->value == OKEY_PGDOWN) {
            strcat(str, "<PGDOWN>");                             // PGDOWN
        } else if (a->value == OKEY_PGUP) {
            strcat(str, "<PGUP>");                               // PGUP
        }
        a = a->pnext;
    }
    return;
}

// función que guarda en un char * el listado y detalle de todos
// los maps existentes
void get_mappings(char * salida) {
   salida[0]='\0';
   if (maps == NULL) return;
   char min[MAXMAPITEM] = "";
   char mout[MAXMAPITEM] = "";
   char nore[5] = "";
   
   char mode = '\0';
   map * m = maps;

   while (m != NULL) {
       nore[0] = '\0';
       switch (m->mode) {
            case NORMAL_MODE:
                mode = 'n';
                break;

            case INSERT_MODE:
                mode = 'i';
                break;
       }
       get_mapstr_buf(m->in, min);
       get_mapstr_buf(m->out, mout);
       if (!m->recursive) strcpy(nore, "nore");
       sprintf(salida + strlen(salida), "+ %c%smap \"%s\" = \"%s\"\n", mode, nore, min, mout);
       m = m->psig;
   }

   return;
}
