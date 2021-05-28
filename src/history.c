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
 * \file history.c
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 2017-07-18
 * \brief TODO Write a tbrief file description.
 */

#include <string.h>
#include <wchar.h>
#include "macros.h"

// current command before tab completion
static wchar_t curcmd [BUFFERSIZE];

/**
 * \brief TODO Document copy_to_curcmd()
 *
 * \param[in] inputline
 *
 * \return none
 */

void copy_to_curcmd(wchar_t * inputline) {
    wcscpy(curcmd, inputline);
}

/**
 * \brief TODO Document get_curcmd()
 *
 * \return curcmd
 */

wchar_t * get_curcmd() {
    return curcmd;
}

// this comp mark is used to mark when tab completion is started
static int comp = 0;

/**
 * \brief TODO Document get_comp()
 *
 * \return none
 */

int get_comp() {
    return comp;
}

/**
 * \brief TODO Document set_comp()
 *
 * \param[in] i
 *
 * \return none
 */

void set_comp(int i) {
    comp = i;
}

#if defined HISTORY_FILE
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "history.h"
#include "sc.h"
#include "utils/string.h"

struct history * create_history(char mode) {
    struct history * h = (struct history *) malloc (sizeof (struct history));
    h->len = 0;
    h->pos = 0;
    h->mode = mode;
    h->list = NULL;
    return h;
}

/**
 * \brief TODO Document destroy_history()
 *
 * \param[in] h
 *
 * \return none
 */

void destroy_history(struct history * h) {
    struct hlist * nl;
    struct hlist * n_sig;
    nl = h->list;
    while (nl != NULL) {
        n_sig = nl->pnext;
        free(nl->line);
        free(nl);
        nl = n_sig;
    }
    free(h);
    return;
}

/**
 * \brief TODO Document load_history
 *
 * \details Read the specified history file from the user's home directory
 * and load it into the specified history struct.
 *
 * \param[in] h
 * \param[in] mode
 *
 * returns: none
 */

void load_history(struct history * h, wchar_t mode) {
    char infofile[PATHLEN];
    wchar_t linea[FBUFLEN];
    int c;
    char * home;
    FILE * f;

    if ((home = getenv("XDG_CACHE_HOME"))) {
        /* Try XDG_CACHE_HOME first */
         sprintf(infofile, "%s/%s", home,HISTORY_FILE);
    } else if ((home = getenv("HOME"))) {
        /* Fallback */
        sprintf(infofile, "%s/%s/%s", home,HISTORY_DIR,HISTORY_FILE);
    }

    if ((c = open(infofile, O_RDONLY)) > -1) {
        close(c);
        f = fopen(infofile, "r");
        if (f == NULL) return;
        while ( feof(f) == 0 ) {
            if (!fgetws(linea, sizeof(linea) / sizeof(*linea), f)) break;
            int s = wcslen(linea)-1;
            del_range_wchars(linea, s, s);

            if (linea[0] == mode && mode == L':') {
                del_range_wchars(linea, 0, 0);
                add(h, linea);
            } else if (mode != L':' && (linea[0] == L'=' || linea[0] == L'<' || linea[0] == L'>' || linea[0] == L'\\')) {
                add(h, linea);
            }
        }
        fclose(f);
    }

    return;
}

/**
 * @brief Save history to file
 *
 * \return 0 on success; -1 otherwise
 */

int save_history(struct history * h, char * mode) {
    char infofile [PATHLEN];
    char * home;
    FILE * f;
    int i;
    struct hlist * nl = h->list;
    char history_dir[PATHLEN-(sizeof HISTORY_FILE)];

    if ((home = getenv("XDG_CACHE_HOME"))) {
        snprintf(history_dir, PATHLEN-(sizeof HISTORY_FILE), "%s", home);
        mkdir(history_dir,0777);
        snprintf(infofile, PATHLEN, "%s/%s", history_dir, HISTORY_FILE);
    } else if ((home = getenv("HOME"))) {
        snprintf(history_dir, PATHLEN-(sizeof HISTORY_FILE), "%s/%s", home, HISTORY_DIR);
        mkdir(history_dir,0777);
        snprintf(infofile, PATHLEN, "%s/%s", history_dir, HISTORY_FILE);
    } else {
        /* If both HOME and XDG_CACHE_HOME aren't set, abandon all hope */
        return 0;
    }

    f = fopen(infofile, mode);
    if (f == NULL) return 0;
    // Go to the end
    for (i=1; i < h->len; i++) {
        nl = nl->pnext;
    }
    // Traverse list back to front, so the history is saved in chronological order
    for (i=0; i < h->len; i++) {
        if (! strcmp(mode, "w")) fwprintf(f, L":"); // mode 'w' means we are saving the command mode history
        fwprintf(f, L"%ls\n", nl->line);
        nl = nl->pant;
    }
    fclose(f);
    return 1;
}

/**
 * \brief Remove history element
 *
 * \return 0 first element; -1 second element
 */

void del_item_from_history(struct history * h, int pos) {
    if (h->len - 1 < -pos) return;

    struct hlist * nl = h->list;
    struct hlist * n_ant = NULL;
    int i;

    if (pos == 0) {
        h->list = nl->pnext;
        if (nl->pnext != NULL) nl->pnext->pant = NULL;
    } else {
        for (i=0; i<-pos; i++) {
            n_ant = nl;
            nl = nl->pnext;
        }
        n_ant->pnext = nl->pnext;
        if (nl->pnext != NULL) nl->pnext->pant = n_ant;
    }
    free(nl->line);
    free(nl);
    h->len--;

    return;
}

/**
 * \brief TODO <brief function description>
 *
 * \details Find a history element and move it. Starts from POS
 * pos=0 first element, pos=-1 second element. Returns 1 if moved,
 * 0 otherwise
 *
 * \param[in] h
 * \param[in] item
 * \param[in] pos
 *
 * \returns: 1 if moved; 0 otherwise
 */

int move_item_from_history_by_str(struct history * h, wchar_t * item, int pos) {
    if (h->len - 1 < -pos || pos == 0 || ! wcslen(item)) return 0; // Move the first element is not allowed
    struct hlist * nl = h->list;
    if (nl != NULL && !wcscmp(item, nl->line)) return 1;
    struct hlist * n_ant = NULL;
    int i;

    for (i=0; i<-pos; i++) {
        n_ant = nl;
        nl = nl->pnext;
    }
    for (i=-pos; i < h->len ; i++) {
        if (wcscmp(item, nl->line) == 0) break;
        n_ant = nl;
        nl = nl->pnext;
    }
    if (i >= h->len) return 0;
    n_ant->pnext = nl->pnext;
    if (nl->pnext != NULL) nl->pnext->pant = n_ant;

    nl->pant = NULL;
    nl->pnext = h->list;
    h->list->pant = nl;
    h->list = nl;

    return 1;
}

/**
 * @brief Add recent entry at the beginning
 *
 * \param[in] h
 * \param[in] line
 *
 * \return none
 */

void add(struct history * h, wchar_t * line) {
    struct hlist * nl = (struct hlist *) malloc(sizeof(struct hlist));

    // Save the line
    int size = wcslen(line)+1;
    if (size == 1) size = BUFFERSIZE + 1;

    wchar_t * val = (wchar_t *) malloc(sizeof(wchar_t) * size);

    val[0]=L'\0';
    wcscpy(val, line);
    nl->line = val;

    // Append at the beginning
    nl->pant = NULL;
    nl->pnext = h->list;
    if (h->list != NULL) h->list->pant = nl;
    h->list = nl;
    h->len++;

    return;
}

/**
 * \brief Returns
 *
 * \details Returns a history line form COMMAND_MODE
 * POS 0 is the mose recent line
 *
 * \return none
 */

wchar_t * get_line_from_history(struct history * h, int pos) {
    return get_hlist_from_history(h, pos)->line;
}

/**
 * \brief TODO Document get_hlist_from_history()
 *
 * \param[in] h
 * \param[in] pos
 * 
 * \return none
 */

struct hlist * get_hlist_from_history(struct history * h, int pos) {
    if (h->len <= - pos) return NULL;
    int i;
    struct hlist * nl = h->list;

    for (i=0; i<-pos; i++) {
        nl = nl->pnext;
    }
    return nl;
}
#endif
