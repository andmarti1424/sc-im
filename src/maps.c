#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "maps.h"
#include "string.h"
#include "macros.h"
#include "color.h"
#include "conf.h"
#include "sc.h"
#include "block.h"
#include "utils/string.h"

static map * maps;
static int mapdepth = 0;
int len_maps = 0;

int replace_maps (struct block * b) {
    int r = 0;

    if (++mapdepth == 1000) {
        sc_error("recursive mapping");
        flush_buf(b);
        mapdepth = 0;
        return 0;
    }

    // Traverse session mappings
    map * m = maps;
    while (m != NULL) {
        // Check if a mapping already exists in 'b' buffer
        int pos = block_in_block(b, m->in);
        if (pos != -1 && m->mode == curmode) {

            // Replace m->in with m->out in 'b' list
            if (replace_block_in_block(b, m->in, m->out) == -1) {
                sc_error("error replacing maps");
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

        // Add special keys
        if (str[i] == '<') {
           is_specialkey = 1;

        } else if (str[i] == '>') {
           is_specialkey = 0;
           if (! strcasecmp(sk, "CR"))                                  // CR - ENTER key
               addto_buf(buffer, OKEY_ENTER);
           else if (! strcasecmp(sk, "TAB"))                            // TAB
               addto_buf(buffer, OKEY_TAB);
           else if (! strcasecmp(sk, "LEFT"))                           // LEFT
               addto_buf(buffer, OKEY_LEFT);
           else if (! strcasecmp(sk, "RIGHT"))                          // RIGHT
               addto_buf(buffer, OKEY_RIGHT);
           else if (! strcasecmp(sk, "DOWN"))                           // DOWN
               addto_buf(buffer, OKEY_DOWN);
           else if (! strcasecmp(sk, "UP"))                             // UP
               addto_buf(buffer, OKEY_UP);
           else if (! strcasecmp(sk, "DEL"))                            // DEL
               addto_buf(buffer, OKEY_DEL);
           else if (! strcasecmp(sk, "BS"))                             // BS
               addto_buf(buffer, OKEY_BS);
           else if (! strcasecmp(sk, "HOME"))                           // HOME
               addto_buf(buffer, OKEY_HOME);
           else if (! strcasecmp(sk, "END"))                            // END
               addto_buf(buffer, OKEY_END);
           else if (! strcasecmp(sk, "PGDOWN"))                         // PGDOWN
               addto_buf(buffer, OKEY_PGDOWN);
           else if (! strcasecmp(sk, "PGUP"))                           // PGUP
               addto_buf(buffer, OKEY_PGUP);
           else if (! strncmp(sk, "C-", 2) && strlen(sk) == 3       // C-X
                    && ( (sk[2] > 64 && sk[2] < 91) || (sk[2] > 96 && sk[2] < 123)) )
               addto_buf(buffer, ctl(tolower(sk[2])));

           sk[0]='\0';

        } else if (is_specialkey && strlen(sk) < MAXSC-1) {
           add_char(sk, str[i], strlen(sk));

        // Add some other characters
        } else {
           addto_buf(buffer, (int) str[i]);
        }
    }

    // If the buffer lacks the trailing '>', insert it
    if (is_specialkey && i == l) {
        j = strlen(sk);
        addto_buf(buffer, '<');
        for (i=0; i<j; i++) addto_buf(buffer, (int) str[l-j+i]);
    }
    return buffer;
}

// Remove mappings and free corresponding memory
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

// Returns the las mapping of the current session
map * get_last_map() {
    map * m = maps;
    map * e = m;

    while (m != NULL) {
        e = m;
        m = m->psig;
    }
    return e;
}

// Returns the position of a mapping for some mode if it already exists, -1
// otherwise
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
// Append a mapping to the current session
void add_map(char * in, char * out, int mode, short recursive) {
    map * m;

    // If the mapping already exists, replace its content, saving it position
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

// Insert at the beginning
//    m->psig = maps == NULL ? NULL : maps;
//    maps = m;

    // Insert at the end
    m->psig= NULL;

    if (maps == NULL) maps = m;
    else
        ((map *) get_last_map())->psig = m;

    len_maps++;

    return;
}

// Remove a mapping from a specific MODE
void del_map(char * in, int mode) {
    map * ant;
    map * m = maps;

    if (m == NULL) return;

    // If the node is the fist one
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

    // If the node is in the middle or at the end
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

/*
 * Translate a block into a string
 * special characters are in the <CR> form
 */
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
        } else if (a->value == OKEY_BS || a->value == OKEY_BS2) {
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

// Save mapping's details in a char*
void get_mappings(char * salida) {
   salida[0]='\0';
   if (maps == NULL) return;
   char min[MAXMAPITEM] = "";
   char mout[MAXMAPITEM] = "";
   char nore[5] = "";

   char mode = '\0';
   int i = 0;
   map * m = maps;

   while (m != NULL) {
       i++;
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
       sprintf(salida + strlen(salida), "%3d + %c%smap \"%s\" = \"%s\"\n", i, mode, nore, min, mout);
       m = m->psig;
   }
   return;
}
