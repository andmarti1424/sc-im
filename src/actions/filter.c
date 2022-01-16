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
 * \file filter.c
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 2017-07-18
 * \brief TODO Write a tbrief file description.
 */

#include <string.h>
#include <stdio.h>
#include <ctype.h>             // for isalpha toupper
#include <stdlib.h>
// #include <math.h>

#include "../macros.h"
#include "../tui.h"
#include "../conf.h"
#include "../xmalloc.h"
#include "filter.h"
#include "../utils/string.h"
#include "../sc.h"
#include "../cmds/cmds.h"

static int howmany = 0;      /**< how many filters were defined */
static int active = 0;       /**< indicates if those filters are applied or not */
static int * results = NULL; /**< this keeps the results of the applied filters */
static struct filter_item * filters = NULL;
extern struct session * session;


/**
 * \brief Add a filter to filters structure
 * \param[in] criteria
 * \return none
 */
void add_filter(char * criteria) {
    int cp = 0;
    char c;

    while (criteria[cp]) {
        int pos = exists_freed_filter(); // we check if there exists a freed filter
        if (pos == -1) {            // if not we alloc a new one
            filters = (struct filter_item *) scxrealloc((char *) filters, (++howmany) * (sizeof(struct filter_item)));
            pos = howmany-1;
        }

        filters[pos].eval = (char *) scxmalloc(sizeof(char) * strlen(criteria) + 1);
        filters[pos].eval[0] = '\0';

        while (criteria[cp] && criteria[cp] != ';' && criteria[cp] != '\n') {
            c = criteria[cp];
            if (c == '"') { cp++; continue; }
            if (criteria[cp++] == '\'') c ='"';
            sprintf(filters[pos].eval + strlen(filters[pos].eval), "%c", c);
        }

        if (criteria[cp] == ';') cp++;
    }
    return;
}


/**
 * \brief Apply filters to a range
 * \param[in] left
 * \param[in] right
 * \return none
 */
void enable_filters(struct ent * left, struct ent * right) {
    struct roman * roman = session->cur_doc;
    struct sheet * sh = roman->cur_sh;
    int minr = left->row < right->row ? left->row : right->row;
    int maxr = left->row > right->row ? left->row : right->row;
    int i, r, c = 0;
    wchar_t cadena [BUFFERSIZE] = L"";
    wchar_t aux [BUFFERSIZE] = L"";
    results = (int *) scxrealloc((char *) results, (maxr - minr + 3) * sizeof(int));
    results[0] = minr; // keep in first position the first row of the range!
    results[1] = maxr; // keep in second position the last row of the range!
    if (filters == NULL) {
        sc_error("There are no filters defined");
        return;
    }
    active = 1;

    for (r = minr; r <= maxr; r++) {
        results[r-minr+2] = 0; // show row by default (0 = NOT HIDDEN)
        for (i = 0; i < howmany; i++, c=0) {
            cadena[0]=L'\0';
            if (filters[i].eval == NULL) continue;
            while (filters[i].eval[c] != '\0') {

                if (filters[i].eval[c] == '#' || filters[i].eval[c] == '$') {
                    if (isalpha(toupper(filters[i].eval[++c])))
                        swprintf(cadena + wcslen(cadena), BUFFERSIZE, L"%c", filters[i].eval[c]);
                    if (isalpha(toupper(filters[i].eval[++c])))
                        swprintf(cadena + wcslen(cadena), BUFFERSIZE, L"%c", filters[i].eval[c]);
                    swprintf(cadena + wcslen(cadena), BUFFERSIZE, L"%d", r);
                    continue;
                } else
                    swprintf(cadena + wcslen(cadena), BUFFERSIZE, L"%c", filters[i].eval[c]);
                c++;
            }

            swprintf(aux, BUFFERSIZE, L"eval %ls", cadena);
            send_to_interp(aux);
            if ( (! seval_result && str_in_str(filters[i].eval, "seval") != -1) || ! eval_result) {
                results[r-minr+2] = 1; // this row does not eval to expression. we hide it. (1 = HIDDEN)!
                i = howmany;
            }
            if (seval_result != NULL) free(seval_result);
        }
    }

    // Hide rows that don't match with filters
    for (r = results[0]; r <= results[1]; r++) {
        sh->row_hidden[r] = results[r-results[0]+2];
    }
    sc_info("Filters enabled");
    return;
}


/**
 * \brief Disable any applied filters
 * \return none
 */
void disable_filters() {
    struct roman * roman = session->cur_doc;
    struct sheet * sh = roman->cur_sh;
    if (results == NULL) {
        sc_error("There are no filters active");
        return;
    }
    // Hide rows that don't match with filters
    int r;
    for (r = results[0]; r <= results[1]; r++) {
        sh->row_hidden[r] = 0;
    }
    active = 0;
    sc_info("Filters disabled");
    return;
}


/**
 * \brief Show details of each filter
 * \return none
 */
void show_filters() {
    if (filters == NULL) {
        sc_error("There are no filters defined");
        return;
    }

    int i, size = 0;
    char init_msg[BUFFERSIZE];
    sprintf(init_msg, "Filters status: %s\nFilters:\n", active == 1 ? "ON" : "OFF");

    size += sizeof(init_msg);
    for (i=0; i < howmany; i++)
        size += sizeof(filters[i].eval) + 4 + floor(log10(howmany));

    char valores[ size + howmany ];
    valores[0]='\0';

    strcpy(valores, init_msg);
    for (i=0; i < howmany; i++)
        if (filters[i].eval != NULL) sprintf(valores + strlen(valores), "%d + %s\n", i, filters[i].eval);

    ui_show_text(valores);
    return;
}


/**
 * \brief Free memory of entire filters structure
 * \return int: -1 not removed - 0 removed
 */
/*
 * FIXME: howmany in the forloop should be the max id, cause:
 * you could for instance create 12 filters and remove the filters, 2 to 11.
 * howmany would be two there, but filter12 would be allocated.
 */
int free_filters() {
    if (filters == NULL) return -1;
    int i;
    disable_filters();
    for (i=0; i < howmany; i++) {
        if (filters[i].eval != NULL) {
            scxfree((char *) filters[i].eval);
            filters[i].eval = NULL;
        }
    }
    howmany = 0;
    scxfree((char *) filters);
    filters = NULL;
    return 0;
}


/**
 * \brief Remove a filter, freeing its memory
 * \param[in] id
 * \return int: -1 not removed - 0 removed
 */
int del_filter(int id) {
    if (filters == NULL || id < 0) {
        sc_error("Cannot delete the filter");
        return -1;
    }
    if (filters[id].eval != NULL) {
        scxfree((char *) filters[id].eval);
        filters[id].eval = NULL;
    }
    howmany--;
    return 0;
}


/**
 * \brief Check if a filter was deleted
 * \details This function checks if a filter was deleted, so there would
 * be room in filters structure for a new filter and preventing
 * an unnecessary realloc.
 * \return how many filters exist; -1 otherwise
 */
int exists_freed_filter() {
    if (filters == NULL) return -1;
    int i;
    for (i=0; i < howmany; i++)
        if (filters[i].eval == NULL) return i;
    return -1;
}
