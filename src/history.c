#include "macros.h"
#include <string.h>

// current command before tab completion
static char curcmd [BUFFERSIZE];

void copy_to_curcmd(char * inputline) {
    strcpy(curcmd, inputline);
}

char * get_curcmd() {
    return curcmd;
}

// this comp mark is used to mark when tab completion is started
static int comp = 0;

int get_comp() {
    return comp;
}

void set_comp(int i) {
    comp = i;
}

#if defined HISTORY_FILE

#include <curses.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

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

void load_history(struct history * h) {
    char infofile[PATHLEN];
    char linea[FBUFLEN];
    int c;
    char * home;
    FILE * f;

    if ((home = getenv("HOME"))) {
        sprintf(infofile, "%s/", home);
        strcat(infofile, HISTORY_FILE);
        if ((c = open(infofile, O_RDONLY)) > -1) {
            close(c);
            f = fopen(infofile, "r");
            if (f == NULL) return;
            while ( feof(f) == 0 ) {

                fgets(linea, sizeof(linea), f);
                int s = strlen(linea)-1;
                del_range_chars(linea, s, s);

                if (linea[0] == ':') {
                    del_range_chars(linea, 0, 0);
                    add(h, linea);
                }
            }
            fclose(f);
        } 
    }

    return;
}

// Save history to file
// returns 0 on success, -1 otherwise
int save_history(struct history * h) {
    if (h->mode != ':' ) return -1;
    char infofile[PATHLEN];
    char * home;
    FILE * f;
    int i;
    struct hlist * nl = h->list;

    if ((home = getenv("HOME"))) {
        sprintf(infofile, "%s/", home);
        strcat(infofile, HISTORY_FILE);
        f = fopen(infofile, "w");
        if (f == NULL) return -1;

        // Go to the end
        for (i=1; i < h->len; i++) {
            nl = nl->pnext;
        }
        // Traverse list back to front, so the history is saved in chronological order
        for (i=0; i < h->len; i++) {
            fprintf(f, ":");
            fprintf(f, "%s\n", nl->line);
            nl = nl->pant;
        }
        fclose(f);
        return 0;
    }
    return -1;
}

// Remove history element
// 0 first element, -1 second element
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

// Find a history element and move it. Starts from POS
// pos=0 first element, pos=-1 second element
// returns 1 if moved, 0 otherwise.
int move_item_from_history_by_str(struct history * h, char * item, int pos) {
    if (h->len - 1 < -pos || ! pos || ! strlen(item)) return 0; // Move the first element is disallowed
    struct hlist * nl = h->list;
    struct hlist * n_ant = NULL;
    int i;
 
    for (i=0; i<-pos; i++) {
        n_ant = nl;
        nl = nl->pnext;
    }
    for (i=-pos; i < h->len ; i++) {
        if (strcmp(item, nl->line) == 0) break;
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

// Add recent entry at the beginning
void add(struct history * h, char * line) {
    struct hlist * nl = (struct hlist *) malloc(sizeof(struct hlist));

    // Save the line
    int size = strlen(line)+1;
    if (size == 1) size = BUFFERSIZE + 1;

    char * val = (char *) malloc(sizeof(char) * size);

    val[0]='\0';
    strcpy(val, line);
    nl->line = val;

    // Append at the beginning
    nl->pant = NULL;
    nl->pnext = h->list;
    if (h->list != NULL) h->list->pant = nl;
    h->list = nl;
    h->len++;

    return;
}

// Returns a history line from COMMAND_MODE
// POS 0 is the most recent line
char * get_line_from_history(struct history * h, int pos) {
    return get_hlist_from_history(h, pos)->line;
}

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
