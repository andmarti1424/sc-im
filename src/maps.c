/*******************************************************************************
 * Copyright (c) 2013-2021, Andrés Martinelli <andmarti@gmail.com>             *
 * All rights reserved.                                                        *
 *                                                                             *
 * This file is a part of sc-im                                                *
 *                                                                             *
 * sc-im is a spreadsheet program that is based on sc. The original authors    *
 * of sc are James Gosling and Mark Weiser, and mods were later added by       *
 * Chuck Martin.                                                               *
 *                                                                             *
 * Redistribution and use in source and binary forms, with or without          *
 * modification, are permitted provided that the following conditions are met: *
 * 1. Redistributions of source code must retain the above copyright           *
 *    notice, this list of conditions and the following disclaimer.            *
 * 2. Redistributions in binary form must reproduce the above copyright        *
 *    notice, this list of conditions and the following disclaimer in the      *
 *    documentation and/or other materials provided with the distribution.     *
 * 3. All advertising materials mentioning features or use of this software    *
 *    must display the following acknowledgement:                              *
 *    This product includes software developed by Andrés Martinelli            *
 *    <andmarti@gmail.com>.                                                    *
 * 4. Neither the name of the Andrés Martinelli nor the                        *
 *   names of other contributors may be used to endorse or promote products    *
 *   derived from this software without specific prior written permission.     *
 *                                                                             *
 * THIS SOFTWARE IS PROVIDED BY ANDRES MARTINELLI ''AS IS'' AND ANY            *
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED   *
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE      *
 * DISCLAIMED. IN NO EVENT SHALL ANDRES MARTINELLI BE LIABLE FOR ANY           *
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES  *
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;*
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND *
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT  *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE       *
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.           *
 *******************************************************************************/

/**
 * \file maps.c
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 2017-07-18
 * \brief file that contain the different functions to support mappings
 */

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


/**
 * \brief could_be_mapping
 * a buffer may contain a valid command (for instance, just a letter in insert mode)
 * but that buffer beginning may also be a possible mapping
 *
 * \param[in] struct block * (b)
 * \return 0 if false. 1 otherwise
 */

int could_be_mapping(struct block * b) {
    // Traverse session mappings
    map * m = maps;

    while (m != NULL) {
        // Check if 'b' buffer might be in mapping
        if (block_in_block(m->in, b) == 0 && m->mode == curmode)
            return 1;
        m = m->psig;
    }
    return 0;
}


/**
 * \brief replace_maps() inside a struct block * list.
 * \param[in] b
 * \return r
 * return 0 when no mapping replaced
 * return -1 when recursive mapping was detected
 * return 1 when no recursive mapping was applied
 * return 2 when recursive mapping was applied
 */
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

    if (r && m->recursive) { replace_maps(b); return 2; } // recursive mapping here!
    return r;
}

/**
 * \brief Create list of blocks based on map strings
 * \param[in] str
 * \return buffer
 */

struct block * get_mapbuf_str (char * str) {

    struct block * buffer = create_buf();
    unsigned short i, j;
    unsigned short is_specialkey = 0;
    char sk[MAXSC+1];
    sk[0] = '\0';
    const size_t size = strlen(str) + 1;
    wchar_t wc[BUFFERSIZE] = { L'\0' };
    size_t result = mbstowcs(wc, str, size);
    if (result == (size_t)-1 ) return buffer;
    unsigned short l = wcslen(wc);

    for (i=0; i<l; i++) {
        // handle simple < and > mappings
        if (str[i] == '\\' && i+1 < l && i+2 < l && str[i+1] == '\\' &&
        (str[i+2] == '<' || str[i+2] == '>')) {
           addto_buf(buffer, (wint_t) wc[i+2]);
           i += 2;

        // Add special keys
        } else if (str[i] == '<' && !(i>0 && str[i-1] == '\\')) {
           is_specialkey = 1;

        } else if (str[i] == '>' && !(i>0 && str[i-1] == '\\')) {
           is_specialkey = 0;
           if (! strcasecmp(sk, "CR"))                                  // CR - ENTER key
               addto_buf(buffer, OKEY_ENTER);
           else if (! strcasecmp(sk, "ESC"))                            // ESC
               addto_buf(buffer, OKEY_ESC);
           else if (! strcasecmp(sk, "TAB"))                            // TAB
               addto_buf(buffer, OKEY_TAB);
           else if (! strcasecmp(sk, "SPACE"))                          // SPACE
               addto_buf(buffer, OKEY_SPACE);
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
           else if (! strncmp(sk, "C-", 2) && strlen(sk) == 3           // C-X
                    && ( (sk[2] > 64 && sk[2] < 91) || (sk[2] > 96 && sk[2] < 123)) )
               addto_buf(buffer, ctl(tolower(sk[2])));

           sk[0]='\0';

        } else if (is_specialkey && strlen(sk) < MAXSC-1) {
           add_char(sk, str[i], strlen(sk));

        // Add some other characters
        } else {
                addto_buf(buffer, (wint_t) wc[i]);
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

/**
 * \brief Remove mappings and free corresponding memory
 * \return none
 */

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

/**
 * \brief Removes the last mapping of the current session
 * \return none
 */

map * get_last_map() {
    map * m = maps;
    map * e = m;

    while (m != NULL) {
        e = m;
        m = m->psig;
    }
    return e;
}

/**
 * \brief Returns the position of a mapping for some mode if it already exists, -1 otherwise
 *
 * \param[in] in
 * \param[in] mode
 *
 * \return position of a mapping for some mode if it already exists; -1 otherwise
 */

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

/**
 * \brief Ammend a mapping to the current session
 *
 * \param[in] in
 * \param[in] out
 * \param[in] mode
 * \param[in] recursive
 *
 * \return none
 */

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

/**
 * \brief Remove a mapping from a specific MODE
 *
 * \param[in] in
 * \param[in] mode
 *
 * \return none
 */

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

/**
 * \brief Translate a block into a string
 *
 * \details Translate a block in to a string. Special characters
 * are in the <CR> form.
 *
 * \param[in] b
 * \param[in] str
 *
 * \return none
 */

void get_mapstr_buf (struct block * b, char * str) {
    struct block * a = b;
    int i, len = get_bufsize(a);

    str[0]='\0';
    for (i=0; i < len; i++) {
        if (a->value == OKEY_ENTER) {
            strcat(str, "<CR>");                                 // CR - ENTER
        } else if (a->value == OKEY_TAB) {
            strcat(str, "<TAB>");                                // TAB
        } else if (a->value == OKEY_SPACE) {
            strcat(str, "<SPACE>");                              // SPACE
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
        } else if (a->value == OKEY_ESC) {
            strcat(str, "<ESC>");                                // ESC
        } else if (sc_isprint(a->value)) {
            sprintf(str + strlen(str), "%lc", a->value);         // ISPRINT
        } else if ( a->value == (uncl(a->value) & 0x1f)) {
            sprintf(str + strlen(str), "<C-%c>", uncl(a->value));// C-x
        }
        a = a->pnext;
    }
    return;
}

// Save mapping's details in a char*
/**
 * \brief Save mapping's details in a char*
 * \param[in] salida
 * \return none
 */

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

            case VISUAL_MODE:
                mode = 'v';
                break;

            case COMMAND_MODE:
                mode = 'c';
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
