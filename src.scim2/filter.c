#include <string.h>
#include <stdio.h>
#include "macros.h"
#include "screen.h"
#include "color.h"
#include "conf.h"
#include "xmalloc.h"
#include "filter.h"
#include "math.h"
#include "utils/string.h"
#include <ctype.h>   //for isalpha toupper
#include <stdlib.h>
#include "sc.h"
#include "cmds.h"

static int howmany = 0;        // how many filters were defined
static int active = 0;         // indicates if those filters are applied or not
static int * results = NULL;   // this keeps the results of the applied filters

// Add a filter to filters structure
void add_filter(char * criteria) {
    int cp = 0;
    char c;

    while (criteria[cp]) {
        int pos = exists_freed_filter(); // we check if there exists a freed filter
        if (pos == -1) {            // if not we alloc a new one
            filters = (struct filter_item *) scxrealloc((char *) filters, (howmany++ + 1) * (sizeof(struct filter_item)));
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

// Apply filters to a range
void enable_filters(struct ent * left, struct ent * right) {
    int minr = left->row < right->row ? left->row : right->row;
    int maxr = left->row > right->row ? left->row : right->row;
    int i, r, c = 0;
    wchar_t cadena [BUFFERSIZE] = L"";
    wchar_t aux [BUFFERSIZE] = L"";
    results = (int *) scxrealloc((char *) results, (maxr - minr + 3) * sizeof(int));
    results[0] = minr; // keep in first position the first row of the range!
    results[1] = maxr; // keep in second position the last row of the range!
    if (filters == NULL) {
        scerror("There are no filters defined");
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

            swprintf(aux, BUFFERSIZE, L"eval %s", cadena);
            send_to_interp(aux);
            if ( (! seval_result && str_in_str(filters[i].eval, "seval") != -1) || ! eval_result) {
                results[r-minr+2] = 1; // this row does not eval to expression. we hide it. (1 = HIDDEN)!
                i = howmany;
            }
        }
    }

    // Hide rows that don't match with filters
    for (r = results[0]; r <= results[1]; r++) {
        row_hidden[r] = results[r-results[0]+2];
    }
    return;
}

// disable any applied filter
void disable_filters() {
    if (results == NULL) {
        scerror("There are no filters active");
        return;
    }
    // Hide rows that don't match with filters
    int r;
    for (r=results[0]; r<=results[1]; r++) {
        row_hidden[r] = 0;
    }
    active = 0;
    return;
}

// Show details of each filter
void show_filters() {
    if (filters == NULL) {
        scerror("There are no filters defined");
        return;
    }

    int i, size = 0;
    char init_msg[100];
    sprintf(init_msg, "Filters status: %s\nFilters:\n", active == 1 ? "ON" : "OFF");

    size += sizeof(init_msg);
    for (i=0; i < howmany; i++)
        size += sizeof(filters[i].eval) + 4 + floor(log10(howmany));

    char valores[ size + howmany ];
    valores[0]='\0';

    strcpy(valores, init_msg);
    for (i=0; i < howmany; i++)
        if (filters[i].eval != NULL) sprintf(valores + strlen(valores), "%d + %s\n", i, filters[i].eval);

    show_text(valores);
    return;
}

// Free memory of entire filters structure
void free_filters() {
    if (filters == NULL) return;
    int i;
    for (i=0; i < howmany; i++)
        if (filters[i].eval != NULL) scxfree((char *) filters[i].eval);
    scxfree((char *) filters);
    filters = NULL;
    return;
}

// Remove a filter, freeing its memory
void del_filter(int id) {
    if (filters == NULL || id < 0 || id > howmany) {
        scerror("Cannot delete the filter");
        return;
    }
    if (filters[id].eval != NULL) {
        scxfree((char *) filters[id].eval);
        filters[id].eval = NULL;
    }
    return;
}

// this functions checks if a filter was deleted, so there would be room in filters structure for a new filter
// and preventing an unnecessary realloc.
int exists_freed_filter() {
    if (filters == NULL) return -1;
    int i;
    for (i=0; i < howmany; i++)
        if (filters[i].eval == NULL) return i;
    return -1;
}
