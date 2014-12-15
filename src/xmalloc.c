#include <curses.h>
#include <stdlib.h>
#include "sc.h"
#include "xmalloc.h"

//void fatal();

#ifdef SYSV3
extern void free();
extern void exit();
#endif

#define    MAGIC    (double)1234567890.12344

char * scxmalloc(unsigned n) {
    register char *ptr;

// if ((ptr = malloc(n + sizeof(double))) == NULL) fatal("scxmalloc: no memory");
// *((double *) ptr) = MAGIC;
// return(ptr + sizeof(double));

    ptr = (char *) malloc(n);
    if (ptr == NULL) fatal("scxmalloc: no memory");
    return (ptr);
}

/* we make sure realloc will do a malloc if needed */
char * scxrealloc(char *ptr, unsigned n) {
    if (ptr == NULL) return(scxmalloc(n));

    //ptr -= sizeof(double);
    //if (*((double *) ptr) != MAGIC) fatal("scxrealloc: storage not scxmalloc'ed");
    //if ((ptr = realloc(ptr, n + sizeof(double))) == NULL) fatal("scxmalloc: no memory");
    //*((double *) ptr) = MAGIC;
    //return(ptr + sizeof(double));

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

void fatal(char *str) {
    (void) fprintf(stderr,"%s\n", str);
    exit(1);
}
