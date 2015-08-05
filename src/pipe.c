#include <unistd.h>
#include "sc.h"
#include "conf.h"
#include "main.h"

// TODO pass fd is not neccesary?
void getnum(int r0, int c0, int rn, int cn, FILE * fd) {
    struct ent ** pp;
    struct ent * p;
    int r, c;

    for (r = r0; r <= rn; r++) {
        for (c = c0, pp = ATBL(tbl, r, c); c <= cn; pp++, c++) {
            *line = '\0';
            p = *pp;
            if (p) {
                if (p->cellerror) {
                    sprintf(line, "%s", (*pp)->cellerror == CELLERROR ?  "ERROR" : "INVALID");
                } else if (p->flags & is_valid) {
                    sprintf(line, "%.15g", p->v);
                }
            }
            if (c < cn) {
                strcat(line, "\t");
            } else {
                strcat(line, "\n");
            }

            //if (get_conf_value("output") != NULL && fd != NULL)
            //    fprintf(fd, "%s\n", line);
            //else
            scdebug("%s\n", line);

            //fwrite(fd, line, strlen(line));
            if (brokenpipe) {
                linelim = -1;
                return;
            }

        }
    }
    linelim = -1;
}
 
