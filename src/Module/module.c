#include <stdio.h>

#include "sc.h"
#include "macros.h"
#include "utils/dictionary.h"
#include "utils/string.h"
#include "range.h"
#include "color.h"
#include "screen.h"
#include "undo.h"
#include "conf.h"
#include "cmds.h"
#include "trigger.h"


extern char * query(char * );



int do_c_call(struct ent *p , int rw)
{

char * str = query("Call from C");


FILE *fd = fopen("/tmp/modul.txt","a+");
fprintf(fd,"%d %d %g %d\n",p->col,p->row,p->v,rw);
fclose(fd);

 return(0);
}
