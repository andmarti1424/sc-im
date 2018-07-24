/*
  Trigger example2 in C (Directly calling sc-im code from trigger)
  to compile
  gcc -I<path/to/scim/src> -shared -fPIC -o module2.so -g -Wall module2.c

  Copy the resulting module2.so somewhere on the search path, into
  $HOME/.scim/module or /usr/local/share/scim/module.

  Open sc-im and activate the trigger with the command
  :trigger a0:a10 "mode=W type=C file=module2.so function=trig_invert"

  Once the trigger has been activated modifying any cell in the specified
  range (a0:a10), will invoke the trigger function defined below.

  This example trigger function retrieves the new value of the cell and
  writes a corresponding value to the adjecent cell. If you enter a number
  into the cell, the value written to the adjecent cell will be its negative,
  and if you enter a string the value will be the reverse of that string.

*/

#include <stdio.h>

#include "sc.h"
#include "xmalloc.h"
#include "trigger.h"

// These functions are part of sc-im itself, and are only visible to the
// dynamic linker if sc-im has been compiled with "-Wl,--export-dynamic"
extern void label(register struct ent * v, register char * s, int flushdir);
extern struct ent *lookat(int row, int col);

static char* strrev(char* s) {
    int i;
    char *res;
    int slen = strlen(s);

    res = scxmalloc(slen+1);
    if(!res) return NULL;
    res[slen]=0;
    for (i=0;i<slen;i++) {
        res[slen-1-i] = s[i];
    }
    return res;
}

int trig_invert(struct ent *p , int rw) {
    struct ent *p2;
    char *s = NULL;

    if(rw != TRG_WRITE) return(1);

    if((p2 = lookat(p->row, p->col+1))) {
        // if the cell has a label, write its reverse to the target cell
        if (p->flags & is_label){
            if (p->label && !(s=strrev(p->label))) return(2);
            label(p2,s,0);
            if(s) scxfree(s);
        }
        // if the cell has a value, write its negative to the target cell
        p2->flags &= ~is_valid;
        p2->flags |= is_changed | iscleared;
        if (p->flags & is_valid){
            p2->v=-p->v;
            p2->flags |= is_changed | is_valid;
            p2->flags &= ~iscleared;
            p2->cellerror = CELLOK;
        }
    }

    return(0);
}

