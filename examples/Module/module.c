/*
  R.Pollak
  Trigger example in C
  to compile
  gcc -shared -fPIC -o module.so -g -Wall module.c

  on the trigger the ent structure is passed and whether the trigger was on Write or on Read. Write trigger, when it was written to selected cells,
  or Read from selected cells.

  Trigger functions return non-zero return value on error.
*/

#include <stdio.h>

#include "sc.h"
#include "macros.h"
#include "utils/dictionary.h"
#include "utils/string.h"
#include "range.h"
#include "color.h"
#include "undo.h"
#include "conf.h"
#include "cmds.h"
#include "trigger.h"

extern char * query(char * );

int do_c_call(struct ent *p , int rw) {
    FILE *fd = fopen("/tmp/modul.txt","a+");
    fprintf(fd,"%d %d %g %d\n",p->col,p->row,p->v,rw);
    fclose(fd);

    return(0);
}
