#include <ncurses.h>
#include <stdlib.h>
#include "sc.h"
#include "xmalloc.h"
#include "macros.h"
#include "color.h"
#include "conf.h"

extern void free();
extern void exit();

#define MAGIC    ((double) 1234567890.12344)

char * scxmalloc(unsigned n) {
    //register char *ptr;
    //if ((ptr = malloc(n + sizeof(double))) == NULL) fatal("scxmalloc: no memory");
    //*((double *) ptr) = MAGIC;
    //return(ptr + sizeof(double));

    register char *ptr;
    ptr = (char *) malloc(n);
    if (ptr == NULL) fatal("scxmalloc: no memory");
    return (ptr);
}

/* we make sure realloc will do a malloc if needed */
char * scxrealloc(char *ptr, unsigned n) {
    //if (ptr == NULL) return(scxmalloc(n));
    //ptr -= sizeof(double);
    //if (*((double *) ptr) != MAGIC) fatal("scxrealloc: storage not scxmalloc'ed");
    //if ((ptr = realloc(ptr, n + sizeof(double))) == NULL) fatal("scxmalloc: no memory");
    //*((double *) ptr) = MAGIC;
    //return(ptr + sizeof(double));

    if (ptr == NULL) return(scxmalloc(n));
    ptr = (char *) realloc(ptr, n);
    if (ptr == NULL) fatal("scxmalloc: no memory");
    return(ptr);
}

void scxfree(char *p) {
    //if (p == NULL) fatal("scxfree: NULL");
    //p -= sizeof(double);
    //if (*((double *) p) != MAGIC) fatal("scxfree: storage not malloc'ed");
    //free(p);

    if (p == NULL) fatal("scxfree: NULL");
    free(p);
}

void fatal(char * str) {
    //fprintf(stderr,"%s\n", str);
    //exit(1);
    sc_error("%s", str);
}
