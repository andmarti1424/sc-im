/*
Adaptation of Chuck Martin's code - <nrocinu@myrealbox.com>
*/

#include <unistd.h>
#include "sc.h"
#include "conf.h"
#include "main.h"
#include "interp.h"
#include "macros.h"
#include "tui.h"

// FIXME - pass fd is not neccesary?
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
            /*if (c < cn) {
                strcat(line, "\t");
            } else {
                strcat(line, "\n");
            }*/

            //if (get_conf_value("output") != NULL && fd != NULL)
            //    fprintf(fd, "%s\n", line);
            //else

            //fwrite(fd, line, strlen(line));
            sc_info("%s", line);
            if (brokenpipe) {
                linelim = -1;
                return;
            }

        }
    }
    linelim = -1;
}

void getformat(int col, FILE * fd) {
    sprintf(line, "%d %d %d\n", fwidth[col], precision[col], realfmt[col]);
    //write(fd, line, strlen(line));
    sc_info("%s", line);
    linelim = -1;
}

void getfmt(int r0, int c0, int rn, int cn, FILE * fd) {
    struct ent    **pp;
    int        r, c;

    for (r = r0; r <= rn; r++) {
        for (c = c0, pp = ATBL(tbl, r, c); c <= cn; pp++, c++) {
            *line = '\0';
            if (*pp && (*pp)->format) sprintf(line, "%s", (*pp)->format);

            //if (c < cn)
            //    strcat(line, "\t");
            //else
            //    strcat(line, "\n");
            //write(fd, line, strlen(line));

            sc_info("%s", line);
            if (brokenpipe) {
                linelim = -1;
                return;
            }
        }
    }
    linelim = -1;
}

void getstring(int r0, int c0, int rn, int cn, FILE * fd) {
    struct ent    **pp;
    int        r, c;

    for (r = r0; r <= rn; r++) {
        for (c = c0, pp = ATBL(tbl, r, c); c <= cn; pp++, c++) {
            *line = '\0';
            if (*pp && (*pp)->label)
                sprintf(line, "%s", (*pp)->label);
            //if (c < cn)
            //    strcat(line, "\t");
            //else
            //    strcat(line, "\n");
            //write(fd, line, strlen(line));

            sc_info("%s", line);
            if (brokenpipe) {
                linelim = -1;
                return;
            }
        }
    }
    linelim = -1;
}

void getexp(int r0, int c0, int rn, int cn, FILE * fd) {
    struct ent    **pp;
    struct ent    *p;
    int        r, c;

    for (r = r0; r <= rn; r++) {
        for (c = c0, pp = ATBL(tbl, r, c); c <= cn; pp++, c++) {
            *line = '\0';
            p = *pp;
            if (p && p->expr) {
                linelim = 0;
                decompile(p->expr, 0);    /* set line to expr */
                line[linelim] = '\0';
                if (*line == '?')
                    *line = '\0';
            }
            //if (c < cn)
            //    strcat(line, "\t");
            //else
            //    strcat(line, "\n");
            //write(fd, line, strlen(line));

            sc_info("%s", line);
            if (brokenpipe) {
                linelim = -1;
                return;
            }
        }
    }
    linelim = -1;
}
