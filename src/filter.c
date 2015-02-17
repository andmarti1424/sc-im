#include <string.h>
#include <stdio.h>
#include "macros.h"
#include "screen.h"
#include "color.h"   // for set_ucolor
#include "xmalloc.h" // for scxfree
#include "filter.h"
#include "math.h"
#include "utils/string.h"
#include <ctype.h>   //for isalpha toupper
#include "sc.h"
#include "cmds.h"

static int howmany = 0;
static int active = 0;
static int * results = NULL;

// Add a filter
void add_filter(char * criteria) {
    int cp = 0;
    char c;

    while (criteria[cp]) {
        filters = (struct filter_item *) scxrealloc((char *) filters, (howmany++ + 1) * (sizeof(struct filter_item)));

        filters[howmany-1].eval = (char *) scxmalloc(sizeof(char) * strlen(criteria) + 1);
        filters[howmany-1].eval[0] = '\0';

        while (criteria[cp] && criteria[cp] != ';' && criteria[cp] != '\n') {
            c = criteria[cp];
            if (c == '"') { cp++; continue; }
            if (criteria[cp++] == '\'') c ='"';
            sprintf(filters[howmany-1].eval + strlen(filters[howmany-1].eval), "%c", c);
        }

        if (criteria[cp] == ';') cp++;
    }
}

// Apply filters to a range
void enable_filters(struct ent * left, struct ent * right) {
    int minr = left->row < right->row ? left->row : right->row;
    int maxr = left->row > right->row ? left->row : right->row;
    int i, r, c = 0;
    char cadena[200] = "";
    char aux[200] = "";
    results = (int *) scxrealloc((char *) results, (maxr - minr + 3) * sizeof(int));
    results[0] = minr; // keep in first position the first row of the range!
    results[1] = maxr; // keep in second position the last row of the range!
    active = 1;
    
    for (r = minr; r <= maxr; r++) {
        results[r-minr+2] = 0; // show row by default (0 = NOT HIDDEN)
        for (i = 0; i < howmany; i++, c=0) {
            cadena[0]='\0';
            while (filters[i].eval[c] != '\0') {
                //if (filters[i].eval[c] == '"') { c++; continue; }
                
                if (filters[i].eval[c] == '#' || filters[i].eval[c] == '$') {
                    if (isalpha(toupper(filters[i].eval[++c])))
                        sprintf(cadena + strlen(cadena), "%c", filters[i].eval[c]);
                    if (isalpha(toupper(filters[i].eval[++c])))
                        sprintf(cadena + strlen(cadena), "%c", filters[i].eval[c]);
                    sprintf(cadena + strlen(cadena), "%d", r);
                    continue;
                } else
                    sprintf(cadena + strlen(cadena), "%c", filters[i].eval[c]);
                c++;
            }

            sprintf(aux, "eval %s", cadena);
            send_to_interp(aux);
            if ( (! seval_result && str_in_str(filters[i].eval, "seval") != -1) || ! eval_result) {
                results[r-minr+2] = 1; // this row does not eval to expression. we hide it. (1 = HIDDEN)!
                i = howmany;
            }
        }
    }

    // oculto las filas que no cumplen con los filtros
    for (r = results[0]; r <= results[1]; r++) {
        row_hidden[r] = results[r-results[0]+2];
    }
}

void disable_filters() {
    // oculto las filas que no cumplen con los filtros
    int r;
    for (r=results[0]; r<=results[1]; r++) {
        row_hidden[r] = 0;
    }
    active = 0;
}

// Show details of each filter
void show_filters() {
    if (filters == NULL) {
        error("There are no filters defined");
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
        sprintf(valores + strlen(valores), "%d + %s\n", i, filters[i].eval);

    show_text(valores);
    return;
}

// Free memory of filters structure
void free_filters() {
    if (filters == NULL) return;
    int i;
    for (i=0; i < howmany; i++)
        scxfree((char *) filters[i].eval);
    scxfree((char *) filters);
}
