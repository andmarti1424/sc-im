/*    Expression interpreter and assorted support routines
 *    Based on SC
 *        original by James Gosling, September 1982
 *        modified by Mark Weiser and Bruce Israel, University of Maryland
 *
 *        More mods Robert Bond, 12/86
 *        More mods by Alan Silverstein, 3-4/88, see list of changes.
 */

#include <sys/types.h>

#ifdef IEEE_MATH
#include <ieeefp.h>
#endif

#include <math.h>
#include <signal.h>
#include <setjmp.h>
#include <ctype.h>
#include <errno.h>

#define __USE_XOPEN
#define _GNU_SOURCE
#include <time.h>
#include <string.h>

#include <stdlib.h>
#include <curses.h>
#include "sc.h"
#include "macros.h"
#include "color.h"
#include "cmds.h"
#include "format.h"
#include "conf.h"
#include "range.h"
#include "xmalloc.h" // for scxfree
#include "lex.h"     // for atocol
#include <unistd.h>

#ifdef UNDO
#include "undo.h"
#endif

#ifdef REGCOMP
#include <regex.h>
#endif

#ifdef RE_COMP
extern char *re_comp(char *s);
extern char *re_exec(char *s);
#endif

#ifdef REGCMP
char *regcmp();
char *regex();
#endif

void exit_app();

/* Use this structure to save the last 'g' command */
struct go_save gs;

/* g_type can be: */
#define G_NONE 0          /* Starting value - must be 0 */
#define G_NUM  1
#define G_STR  2
#define G_NSTR 3
#define G_XSTR 4
#define G_CELL 5

#define ISVALID(r,c)    ((r)>=0 && (r)<maxrows && (c)>=0 && (c)<maxcols)

jmp_buf fpe_save;
int exprerr;              /* Set by eval() and seval() if expression errors */
double prescale = 1.0;    /* Prescale for constants in let() */
int loading = 0;          /* Set when readfile() is active */
int gmyrow, gmycol;       /* globals used to implement @myrow, @mycol cmds */
int rowoffset = 0, coloffset = 0;    /* row & col offsets for range functions */

extern bool decimal;      /* Set if there was a decimal point in the number */

/* a linked list of free [struct enodes]'s, uses .e.o.left as the pointer */
//struct enode *freeenodes = NULL;

double dolookup(struct enode * val, int minr, int minc, int maxr, int maxc, int offr, int offc);
double fn1_eval(double (* fn)(), double arg);
double fn2_eval(double (* fn)(), double arg1, double arg2);
int RealEvalAll();
int constant(register struct enode * e);
void RealEvalOne(register struct ent * p, int i, int j, int * chgct);
void copydbuf(int deltar, int deltac);
void decompile(register struct enode * e, int priority);
void index_arg(char * s, struct enode * e);
void list_arg(char * s, struct enode * e);
void one_arg(char * s, struct enode * e);
void range_arg(char * s, struct enode * e);
void three_arg(char * s, struct enode * e);
void two_arg(char * s, struct enode * e);
void two_arg_index(char * s, struct enode * e);

double rint(double d);
int cellerror = CELLOK;    /* is there an error in this cell */

#ifndef M_PI
 #define M_PI (double)3.14159265358979323846
#endif

#define dtr(x) ((x)*(M_PI/(double)180.0))
#define rtd(x) ((x)*(180.0/(double)M_PI))

extern int find_range(char * name, int len, struct ent * lmatch, struct ent * rmatch, struct range ** rng);


double finfunc(int fun, double v1, double v2, double v3) {
    double answer,p;

    p = fn2_eval(pow, 1 + v2, v3);

    switch (fun) {
        case PV:
            if (v2)
                answer = v1 * (1 - 1/p) / v2;
            else {
                cellerror = CELLERROR;
                answer = (double)0;
            }
             break;
        case FV:
            if (v2)
                answer = v1 * (p - 1) / v2;
            else {
                cellerror = CELLERROR;
                answer = (double)0;
            }
            break;
        case PMT:
            /* CHECK IF ~= 1 - 1/1 */
            if (p && p != (double)1)
                answer = v1 * v2 / (1 - 1/p);
            else {
                cellerror = CELLERROR;
                answer = (double)0;
            }
            break;
        default:
            scerror("Unknown function in finfunc");
            cellerror = CELLERROR;
            return ((double)0);
        }
        return (answer);
}

char * dostindex(int minr, int minc, int maxr, int maxc, struct enode *val) {
    int r, c;
    register struct ent *p;
    char *pr;

    p = (struct ent *)0;
    if (minr == maxr) {            /* look along the row */
        r = minr;
        c = minc + (int) eval(val) - 1;
    } else if (minc == maxc) {        /* look down the column */
        r = minr + (int) eval(val) - 1;
        c = minc;
    } else {
        r = minr + (int) eval(val->e.o.left) - 1;
        c = minc + (int) eval(val->e.o.right) - 1;
    }
    if (c <= maxc && c >=minc && r <= maxr && r >=minr)
        p = *ATBL(tbl, r, c);

    if (p && p->label) {
        pr = scxmalloc((size_t)(strlen(p->label)+1));
        (void) strcpy(pr, p->label);
        if (p->cellerror)
            cellerror = CELLINVALID;
        return (pr);
    } else
        return ((char *)0);
}

double doascii(char *s) {
    double v=0.;
    int i ;
    if (!s)
    return ((double)0);

    for (i = 0; s[i] != '\0' ; v = v*256 + (unsigned char)(s[i++]) ) ;
    scxfree(s);
    return(v);
}

double doindex(int minr, int minc, int maxr, int maxc, struct enode *val) {
    int r, c;
    register struct ent *p;

    if (val->op == ',') {        /* index by both row and column */
        r = minr + (int) eval(val->e.o.left) - 1;
        c = minc + (int) eval(val->e.o.right) - 1;
    } else if (minr == maxr) {        /* look along the row */
        r = minr;
        c = minc + (int) eval(val) - 1;
    } else if (minc == maxc) {        /* look down the column */
        r = minr + (int) eval(val) - 1;
        c = minc;
    } else {
        scerror("Improper indexing operation");
        return (double) 0;
    }

    if (c <= maxc && c >=minc && r <= maxr && r >=minr &&
        (p = *ATBL(tbl, r, c)) && p->flags&is_valid) {
        if (p->cellerror)
            cellerror = CELLINVALID;
        return p->v;
    } else
        return (double) 0;
}

double dolookup(struct enode * val, int minr, int minc, int maxr, int maxc, int offset, int vflag) {
    double v, ret = (double)0;
    int r, c;
    register struct ent *p = (struct ent *)0;
    int incr, incc, fndr, fndc;
    char *s;

    incr = vflag; incc = 1 - vflag;
    if (etype(val) == NUM) {
        cellerror = CELLOK;
        v = eval(val);
        for (r = minr, c = minc; r <= maxr && c <= maxc; r+=incr, c+=incc) {
            if ((p = *ATBL(tbl, r, c)) && p->flags&is_valid) {
                if (p->v <= v) {
                    fndr = incc ? (minr + offset) : r;
                    fndc = incr ? (minc + offset) : c;
                    if (ISVALID(fndr, fndc))
                        if (p == NULL) // three lines added
                            cellerror = CELLINVALID;
                        else // useful when the lookup ends up in a cell with no value
                            p = *ATBL(tbl, fndr, fndc);
                    else {
                        scerror(" range specified to @[hv]lookup");
                        cellerror = CELLERROR;
                    }
                    if (p && p->flags&is_valid) {
                        if (p->cellerror)
                            cellerror = CELLINVALID;
                        ret = p->v;
                    }
                } else break;
            }
        }
    } else {
        cellerror = CELLOK;
        s = seval(val);
        for (r = minr, c = minc; r <= maxr && c <= maxc; r+=incr, c+=incc) {
            if ((p = *ATBL(tbl, r, c)) && p->label) {
                if (strcmp(p->label,s) == 0) {
                    fndr = incc ? (minr + offset) : r;
                    fndc = incr ? (minc + offset) : c;
                    if (ISVALID(fndr,fndc)) {
                        p = *ATBL(tbl, fndr, fndc);
                        if (p->cellerror)
                            cellerror = CELLINVALID;
                    } else {
                        scerror(" range specified to @[hv]lookup");
                        cellerror = CELLERROR;
                    }
                    break;
                }
            }
        }
        if (p && p->flags&is_valid)
            ret = p->v;
        scxfree(s);
    }
    return ret;
}

double docount(int minr, int minc, int maxr, int maxc, struct enode *e) {
    int v;
    int r, c;
    int cellerr = CELLOK;
    register struct ent *p;

    v = 0;
    for (r = minr; r<=maxr; r++)
    for (c = minc; c<=maxc; c++) {
        if (e) {
            rowoffset = r - minr;
            coloffset = c - minc;
        }
        if (!e || eval(e))
            if ((p = *ATBL(tbl, r, c)) && p->flags & is_valid) {
                if (p->cellerror) cellerr = CELLINVALID;
                v++;
            }
    }
    cellerror = cellerr;
    rowoffset = coloffset = 0;
    return v;
}

double dosum(int minr, int minc, int maxr, int maxc, struct enode *e) {
    double v;
    int r, c;
    int cellerr = CELLOK;
    register struct ent *p;

    v = (double)0;
    for (r = minr; r<=maxr; r++)
        for (c = minc; c<=maxc; c++) {
            if (e) {
                rowoffset = r - minr;
                coloffset = c - minc;
            }
            if (!e || eval(e))
                if ((p = *ATBL(tbl, r, c)) && p->flags & is_valid) {
                    if (p->cellerror)
                        cellerr = CELLINVALID;
                    v += p->v;
                }
        }
    cellerror = cellerr;
    rowoffset = coloffset = 0;
    return v;
}

double doprod(int minr, int minc, int maxr, int maxc, struct enode *e) {
    double v;
    int r, c;
    int cellerr = CELLOK;
    register struct ent *p;

    v = 1;
    for (r = minr; r<=maxr; r++)
        for (c = minc; c<=maxc; c++) {
            if (e) {
                rowoffset = r - minr;
                coloffset = c - minc;
            }
            if (!e || eval(e))
                if ((p = *ATBL(tbl, r, c)) && p->flags & is_valid) {
                    if (p->cellerror) cellerr = CELLINVALID;
                        v *= p->v;
                }
        }
    cellerror = cellerr;
    rowoffset = coloffset = 0;
    return v;
}

double doavg(int minr, int minc, int maxr, int maxc, struct enode *e) {
    double v;
    int r, c;
    int count;
    int cellerr = CELLOK;
    register struct ent *p;

    v = (double)0;
    count = 0;
    for (r = minr; r<=maxr; r++)
        for (c = minc; c<=maxc; c++) {
            if (e) {
                rowoffset = r - minr;
                coloffset = c - minc;
            }
            if (!e || eval(e))
                if ((p = *ATBL(tbl, r, c)) && p->flags&is_valid) {
                    if (p->cellerror) cellerr = CELLINVALID;
                    v += p->v;
                    count++;
                }
        }
    cellerror = cellerr;
    rowoffset = coloffset = 0;

    if (count == 0) return ((double)0);

    return (v / (double)count);
}

double dostddev(int minr, int minc, int maxr, int maxc, struct enode *e) {
    double lp, rp, v, nd;
    int r, c;
    int n;
    int cellerr = CELLOK;
    register struct ent *p;

    n = 0;
    lp = 0;
    rp = 0;
    for (r = minr; r<=maxr; r++)
        for (c = minc; c<=maxc; c++) {
            if (e) {
                rowoffset = r - minr;
                coloffset = c - minc;
            }
            if (!e || eval(e))
                if ((p = *ATBL(tbl, r, c)) && p->flags&is_valid) {
                    if (p->cellerror) cellerr = CELLINVALID;
                    v = p->v;
                    lp += v*v;
                    rp += v;
                    n++;
                }
        }
    cellerror = cellerr;
    rowoffset = coloffset = 0;

    if ((n == 0) || (n == 1)) return ((double)0);
    nd = (double) n;
    return ( sqrt((nd*lp-rp*rp) / (nd*(nd-1))) );
}

double domax(int minr, int minc, int maxr, int maxc, struct enode *e) {
    double v = (double)0;
    int r, c;
    int count;
    int cellerr = CELLOK;
    register struct ent *p;

    count = 0;
    for (r = minr; r<=maxr; r++)
        for (c = minc; c<=maxc; c++) {
            if (e) {
                rowoffset = r - minr;
                coloffset = c - minc;
            }
            if (!e || eval(e))
                if ((p = *ATBL(tbl, r, c)) && p->flags&is_valid) {
                    if (p->cellerror) cellerr = CELLINVALID;

                    if (! count) {
                        v = p->v;
                        count++;
                    } else if (p->v > v)
                        v = p->v;
                }
        }
    cellerror = cellerr;
    rowoffset = coloffset = 0;

    if (count == 0) return ((double)0);

    return (v);
}

double domin(int minr, int minc, int maxr, int maxc, struct enode *e) {
    double v = (double)0;
    int r, c;
    int count;
    int cellerr = CELLOK;
    register struct ent *p;

    count = 0;
    for (r = minr; r<=maxr; r++)
        for (c = minc; c<=maxc; c++) {
            if (e) {
                rowoffset = r - minr;
                coloffset = c - minc;
            }
            if (!e || eval(e))
                if ((p = *ATBL(tbl, r, c)) && p->flags&is_valid) {
                    if (p->cellerror) cellerr = CELLINVALID;
                    if (! count) {
                        v = p->v;
                        count++;
                    } else if (p->v < v)
                        v = p->v;
                }
        }
    cellerror = cellerr;
    rowoffset = coloffset = 0;

    if (count == 0) return ((double)0);

    return (v);
}

int mdays[12]={ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

double dodts(int e1, int e2, int e3) {
    int yr, mo, day;
    time_t secs;
    struct tm t; 

    if (e2 > 12 || e3 > 31) {
        mo  = e1;
        day = e2;
        yr  = e3;
    } else {
        yr  = e1;
        mo  = e2;
        day = e3;
    }
    mdays[1] = 28 + (yr % 4 == 0) - (yr % 100 == 0) + (yr % 400 == 0);

    t.tm_hour = t.tm_min = t.tm_sec = 0;
    t.tm_mon = --mo;
    t.tm_mday = day;
    t.tm_year = yr -= 1900;
    t.tm_isdst = -1;

    if (mo < 0 || mo > 11 || day < 1 || day > mdays[mo] || (secs = mktime(&t)) == -1) {
        scerror("@dts: invalid argument or date out of range");
        cellerror = CELLERROR;
        return (0.0);
    }

    return ((double)secs);
}

double dotts(int hr, int min, int sec) {
    if (hr < 0 || hr > 23 || min < 0 || min > 59 || sec < 0 || sec > 59) {
        scerror ("@tts: Invalid argument");
        cellerror = CELLERROR;
        return ((double) 0);
    }
    return ((double) (sec + min * 60 + hr * 3600));
}

double dotime(int which, double when) {
    static time_t t_cache;
    static struct tm tm_cache;
    struct tm *tp;
    time_t tloc;

    if (which == NOW) 
        return (double)time(NULL);

    tloc = (time_t)when;

    if (tloc != t_cache) {
        tp = localtime(&tloc);
        tm_cache = *tp;
        tm_cache.tm_mon += 1;
        tm_cache.tm_year += 1900;
        t_cache = tloc;
    }

    switch (which) {
        case HOUR:     return ((double)(tm_cache.tm_hour));
        case MINUTE:   return ((double)(tm_cache.tm_min));
        case SECOND:   return ((double)(tm_cache.tm_sec));
        case MONTH:    return ((double)(tm_cache.tm_mon));
        case DAY:      return ((double)(tm_cache.tm_mday));
        case YEAR:     return ((double)(tm_cache.tm_year));
    }
    /* Safety net */
    cellerror = CELLERROR;
    return ((double)0);
}

double doston(char *s) {
    double v;

    if (!s) return ((double)0);

    v = strtod(s, NULL);
    scxfree(s);
    return(v);
}

int doslen(char *s) {
    if (!s) return 0;

    int i = strlen(s);
    scxfree(s);
    return i;
}

double doeqs(char *s1, char *s2) {
    double v;

    if (!s1 && !s2)
    return ((double)1.0);

    if (!s1 || !s2)
        v = 0.0;
    else if (strcmp(s1, s2) == 0)
        v = 1.0;
    else
        v = 0.0;

    if (s1) scxfree(s1);

    if (s2) scxfree(s2);

    return(v);
}


/*
 * Given a string representing a column name and a value which is a row
 * number, return a pointer to the selected cell's entry, if any, else NULL.
 * Use only the integer part of the column number.  Always free the string.
 */

struct ent * getent(char *colstr, double rowdoub) {
    int collen;        /* length of string */
    int row, col;    /* integer values   */
    struct ent *p = (struct ent *)0;    /* selected entry   */

    if (!colstr) {
        cellerror = CELLERROR;
        return ((struct ent *)0);
    }

    if (((row = (int) floor(rowdoub)) >= 0)
        && (row < maxrows)                /* in range */
        && ((collen = strlen (colstr)) <= 2)    /* not too long */
        && ((col = atocol (colstr, collen)) >= 0)
        && (col < maxcols)) {            /* in range */
            p = *ATBL(tbl, row, col);
            if ((p != NULL) && p->cellerror) cellerror = CELLINVALID;
    }
    scxfree(colstr);
    return (p);
}


/*
 * Given a string representing a column name and a value which is a column
 * number, return the selected cell's numeric value, if any.
 */

double donval(char *colstr, double rowdoub) {
    struct ent *ep;

    return (((ep = getent(colstr, rowdoub)) && ((ep->flags) & is_valid)) ?
        (ep->v) : (double)0);
}

/*
 *    The list routines (e.g. dolmax) are called with an LMAX enode.
 *    The left pointer is a chain of ELIST nodes, the right pointer
 *    is a value.
 */
double dolmax(struct enode *ep) {
    register int count = 0;
    register double maxval = 0; /* Assignment to shut up lint */
    register struct enode *p;
    register double v;

    cellerror = CELLOK;
    for (p = ep; p; p = p->e.o.left) {
        v = eval(p->e.o.right);
        if (!count || v > maxval) maxval = v; count++;
    }
    if (count) return maxval;
    else return (double)0;
}

double dolmin(struct enode *ep) {
    register int count = 0;
    register double minval = 0; /* Assignment to shut up lint */
    register struct enode *p;
    register double v;

    cellerror = CELLOK;
    for (p = ep; p; p = p->e.o.left) {
        v = eval(p->e.o.right);
        if (!count || v < minval) minval = v; count++;
    }
    if (count) return minval;
    else return (double)0;
}

double eval(register struct enode *e) {
    if (e == (struct enode *)0) {
        cellerror = CELLINVALID;
        return (double)0;
    }

    switch (e->op) {
    case '+':    return (eval(e->e.o.left) + eval(e->e.o.right));
    case '-':    {
            double l, r;
            l = eval(e->e.o.left);
            r = eval(e->e.o.right);
            return l - r;
            }
    case '*':    return (eval(e->e.o.left) * eval(e->e.o.right));
    case '/':    {
            double num, denom;
            num = eval(e->e.o.left);
            denom = eval(e->e.o.right);
            if (cellerror) {
                cellerror = CELLINVALID;
                return ((double) 0);
            } else
            if (denom)
                return (num/denom);
            else {
                cellerror = CELLERROR;
                return ((double) 0);
            }
    }
    case '%':    {
            double num, denom;
            num = floor(eval(e->e.o.left));
            denom = floor(eval(e->e.o.right));
            if (denom)
                return (num - floor(num/denom)*denom);
            else {
                cellerror = CELLERROR;
                return ((double) 0);
            }
    }
    case '^':    return (fn2_eval(pow,eval(e->e.o.left),eval(e->e.o.right)));
    case '<':    return (eval(e->e.o.left) < eval(e->e.o.right));
    case '=':    {
            double l, r;
            l = eval(e->e.o.left);
            r = eval(e->e.o.right);
            return (l == r);
            }
    case '>':    return (eval(e->e.o.left) > eval(e->e.o.right));
    case '&':    return (eval(e->e.o.left) && eval(e->e.o.right));
    case '|':    return (eval(e->e.o.left) || eval(e->e.o.right));
    case IF:
    case '?':    return eval(e->e.o.left) ? eval(e->e.o.right->e.o.left)
                        : eval(e->e.o.right->e.o.right);
    case 'm':    return (-eval(e->e.o.left));
    case 'f':    {
            int rtmp = rowoffset;
            int ctmp = coloffset;
            double ret;
            rowoffset = coloffset = 0;
            ret = eval(e->e.o.left);
            rowoffset = rtmp;
            coloffset = ctmp;
            return (ret);
            }
    case 'F':    return (eval(e->e.o.left));
    case '!':    return (eval(e->e.o.left) == 0.0);
    case ';':    return (((int)eval(e->e.o.left) & 7) +
                (((int)eval(e->e.o.right) & 7) << 3));
    case O_CONST:    if (!finite(e->e.k)) {
                e->op = ERR_;
                e->e.k = (double) 0;
                cellerror = CELLERROR;
            }
            return (e->e.k);
    case O_VAR:    {
            struct ent *vp = e->e.v.vp;
            int row, col;
            if (vp && (rowoffset || coloffset)) {
                row = e->e.v.vf & FIX_ROW ? vp->row : vp->row + rowoffset;
                col = e->e.v.vf & FIX_COL ? vp->col : vp->col + coloffset;
                checkbounds(&row, &col);
                vp = *ATBL(tbl, row, col);
            }
            if (!vp || vp->flags & is_deleted) {
                cellerror = CELLERROR;
                return (double) 0;
            }
            if (vp->cellerror) cellerror = CELLINVALID;
            return (vp->v);
            }
    case SUM:
    case PROD:
    case AVG:
    case COUNT:
    case STDDEV:
    case MAX:
    case MIN:
    case INDEX:
    case LOOKUP:
    case HLOOKUP:
    case VLOOKUP: {
        int r, c;
        int maxr, maxc;
        int minr, minc;
        maxr = e->e.o.left->e.r.right.vp->row;
        maxc = e->e.o.left->e.r.right.vp->col;
        minr = e->e.o.left->e.r.left.vp->row;
        minc = e->e.o.left->e.r.left.vp->col;
        if (minr>maxr) r = maxr, maxr = minr, minr = r;
        if (minc>maxc) c = maxc, maxc = minc, minc = c;
        switch (e->op) {
            case LOOKUP:
            return dolookup(e->e.o.right, minr, minc, maxr, maxc, 1, minc==maxc);
            case HLOOKUP:
            return dolookup(e->e.o.right->e.o.left, minr,minc,maxr,maxc,
                (int) eval(e->e.o.right->e.o.right), 0);
            case VLOOKUP:
            return dolookup(e->e.o.right->e.o.left, minr,minc,maxr,maxc,
                (int) eval(e->e.o.right->e.o.right), 1);
            case INDEX:
            return doindex(minr, minc, maxr, maxc, e->e.o.right);
                case SUM:
            return dosum(minr, minc, maxr, maxc, e->e.o.right);
                case PROD:
            return doprod(minr, minc, maxr, maxc, e->e.o.right);
                case AVG:
            return doavg(minr, minc, maxr, maxc, e->e.o.right);
                case COUNT:
            return docount(minr, minc, maxr, maxc, e->e.o.right);
                case STDDEV:
            return dostddev(minr, minc, maxr, maxc, e->e.o.right);
                case MAX:
            return domax(minr, minc, maxr, maxc, e->e.o.right);
                case MIN:
            return domin(minr, minc, maxr, maxc, e->e.o.right);
        }
    }
    case REDUCE | 'R':
    case REDUCE | 'C':
        {    int r, c;
        int maxr, maxc;
        int minr, minc;
        maxr = e->e.r.right.vp->row;
        maxc = e->e.r.right.vp->col;
        minr = e->e.r.left.vp->row;
        minc = e->e.r.left.vp->col;
        if (minr>maxr) r = maxr, maxr = minr, minr = r;
        if (minc>maxc) c = maxc, maxc = minc, minc = c;
            switch (e->op) {
                 case REDUCE | 'R': return (maxr - minr + 1);
                 case REDUCE | 'C': return (maxc - minc + 1);
        }
        }
    case ABS:    return (fn1_eval( fabs, eval(e->e.o.left)));
    case ACOS:   return (fn1_eval( acos, eval(e->e.o.left)));
    case ASIN:   return (fn1_eval( asin, eval(e->e.o.left)));
    case ATAN:   return (fn1_eval( atan, eval(e->e.o.left)));
    case ATAN2:  return (fn2_eval( atan2, eval(e->e.o.left), eval(e->e.o.right)));
    case CEIL:   return (fn1_eval( ceil, eval(e->e.o.left)));
    case COS:    return (fn1_eval( cos, eval(e->e.o.left)));
    case EXP:    return (fn1_eval( exp, eval(e->e.o.left)));
    case FABS:   return (fn1_eval( fabs, eval(e->e.o.left)));
    case FLOOR:  return (fn1_eval( floor, eval(e->e.o.left)));
    case HYPOT:  return (fn2_eval( hypot, eval(e->e.o.left), eval(e->e.o.right)));
    case LOG:    return (fn1_eval( log, eval(e->e.o.left)));
    case LOG10:  return (fn1_eval( log10, eval(e->e.o.left)));
    case POW:    return (fn2_eval( pow, eval(e->e.o.left), eval(e->e.o.right)));
    case SIN:    return (fn1_eval( sin, eval(e->e.o.left)));
    case SQRT:   return (fn1_eval( sqrt, eval(e->e.o.left)));
    case TAN:    return (fn1_eval( tan, eval(e->e.o.left)));
    case DTR:    return (dtr(eval(e->e.o.left)));
    case RTD:    return (rtd(eval(e->e.o.left)));
    case RND:
        if (rndtoeven)
            return rint(eval(e->e.o.left));
        else {
            double temp = eval(e->e.o.left);
            return (temp - floor(temp) < 0.5 ? floor(temp) : ceil(temp));
        }
     case ROUND:
        {
        int precision = (int) eval(e->e.o.right);
        double scale = 1;
        if (0 < precision)
            do scale *= 10; while (0 < --precision);
        else if (precision < 0)
            do scale /= 10; while (++precision < 0);

        if (rndtoeven)
            return (rint(eval(e->e.o.left) * scale) / scale);
        else {
            double temp = eval(e->e.o.left);
            temp *= scale;
            /* xxx */
            /*
            temp = (temp > 0.0 ? floor(temp + 0.5) : ceil(temp - 0.5));
            */
            temp = ((temp - floor(temp)) < 0.5 ?
                floor(temp) : ceil(temp));
            return (temp / scale);
        }
        }
    case FV:
    case PV:
    case PMT:    return (finfunc(e->op, eval(e->e.o.left),
                    eval(e->e.o.right->e.o.left),
                    eval(e->e.o.right->e.o.right)));
    case HOUR:   return (dotime(HOUR, eval(e->e.o.left)));
    case MINUTE: return (dotime(MINUTE, eval(e->e.o.left)));
    case SECOND: return (dotime(SECOND, eval(e->e.o.left)));
    case MONTH:  return (dotime(MONTH, eval(e->e.o.left)));
    case DAY:    return (dotime(DAY, eval(e->e.o.left)));
    case YEAR:   return (dotime(YEAR, eval(e->e.o.left)));
    case NOW:    return (dotime(NOW, (double)0.0));
    case DTS:    return (dodts((int)eval(e->e.o.left),
                    (int)eval(e->e.o.right->e.o.left),
                    (int)eval(e->e.o.right->e.o.right)));
    case TTS:    return (dotts((int)eval(e->e.o.left),
                    (int)eval(e->e.o.right->e.o.left),
                    (int)eval(e->e.o.right->e.o.right)));
    case STON:   return (doston(seval(e->e.o.left)));
    case ASCII:  return (doascii(seval(e->e.o.left)));
    case SLEN:   return (doslen(seval(e->e.o.left)));
    case EQS:    return (doeqs(seval(e->e.o.right), seval(e->e.o.left)));
    case LMAX:   return dolmax(e);
    case LMIN:   return dolmin(e);
    case NVAL:   return (donval(seval(e->e.o.left),eval(e->e.o.right)));
    case MYROW:  return ((double) (gmyrow + rowoffset));
    case MYCOL:  return ((double) (gmycol + coloffset));
    case LASTROW: return ((double) maxrow);
    case LASTCOL: return ((double) maxcol);
    case NUMITER: return ((double) repct);
    case ERR_:    cellerror = CELLERROR;
                 return ((double) 0);
    case PI_:    return ((double) M_PI);
    case BLACK:  return ((double) COLOR_BLACK);
    case RED:    return ((double) COLOR_RED);
    case GREEN:  return ((double) COLOR_GREEN);
    case YELLOW: return ((double) COLOR_YELLOW);
    case BLUE:   return ((double) COLOR_BLUE);
    case MAGENTA: return ((double) COLOR_MAGENTA);
    case CYAN:   return ((double) COLOR_CYAN);
    case WHITE:  return ((double) COLOR_WHITE);
    default:    scerror ("Illegal numeric expression");
                exprerr = 1;
    }
    cellerror = CELLERROR;
    return ((double)0.0);
}

void eval_fpe() { /* Trap for FPE errors in eval */
#if defined(i386)
    scdebug("eval_fpe i386");
    asm("    fnclex");
    asm("    fwait");
#else
 #ifdef IEEE_MATH
    (void)fpsetsticky((fp_except)0);    /* Clear exception */
 #endif /* IEEE_MATH */
#endif
    /* re-establish signal handler for next time */
    (void) signal(SIGFPE, eval_fpe);
    longjmp(fpe_save, 1);
}

double fn1_eval(double (*fn)(), double arg) {
    double res;
    errno = 0;
    res = (*fn)(arg);
    if (errno) cellerror = CELLERROR;

    return res;
}

double fn2_eval(double (*fn)(), double arg1, double arg2) {
    double res;
    errno = 0;
    res = (*fn)(arg1, arg2);
    if (errno) cellerror = CELLERROR;

    return res;
}

/* 
 * Rules for string functions:
 * Take string arguments which they scxfree.
 * All returned strings are assumed to be xalloced.
 */
char * docat(register char *s1, register char *s2) {
    register char *p;
    char *arg1, *arg2;

    if (!s1 && !s2)
        return ((char *)0);
    arg1 = s1 ? s1 : "";
    arg2 = s2 ? s2 : "";
    p = scxmalloc((size_t)(strlen(arg1)+strlen(arg2)+1));
    (void) strcpy(p, arg1);
    (void) strcat(p, arg2);
    if (s1)
        scxfree(s1);
    if (s2)
        scxfree(s2);
    return (p);
}

char * dodate(time_t tloc, char *fmtstr) {
    char buff[FBUFLEN];
    char *p;

    if (! fmtstr)
        fmtstr = "%a %b %d %H:%M:%S %Y";
    strftime(buff, FBUFLEN, fmtstr, localtime(&tloc));
    p = scxmalloc((size_t)(strlen(buff)+1));
    (void) strcpy(p, buff);
    return (p);
}

/*
 * conversion reverse from doascii
 */
char * dochr(double ascii) {
    char *p = scxmalloc((size_t)10);
    char *q = p;
    int digit ;
    int nbdigits = 0;
    int i = 0;
    double stopnbdigits = 1;

    for (stopnbdigits = 1; ascii >= stopnbdigits && nbdigits < 9 ; stopnbdigits *= 256, ++ nbdigits) ;
    for (; nbdigits > 0 ; -- nbdigits) {
        for (stopnbdigits = 1, i = 0; i < nbdigits - 1 ; stopnbdigits *= 256, ++ i) ;
        digit = floor (ascii / stopnbdigits) ;
        ascii -= digit * stopnbdigits ;
        if (ascii >= stopnbdigits && digit < 256) { digit ++ ; ascii += stopnbdigits ; }
        if (ascii < 0 && digit >= 0) { digit -- ; ascii -= stopnbdigits ; }
        *q++ = digit ;
    }
    *q = '\0';
    return p;
}

char * dofmt(char *fmtstr, double v) {
    char buff[FBUFLEN];
    char *p;

    if (!fmtstr)
        return ((char *)0);
    (void) snprintf(buff, FBUFLEN, fmtstr, v);
    p = scxmalloc((size_t)(strlen(buff)+1));
    (void) strcpy(p, buff);
    scxfree(fmtstr);
    return (p);
}


/*
 * Given a command name and a value, run the command with the given value and
 * read and return its first output line (only) as an allocated string, always
 * a copy of se->e.o.s, which is set appropriately first unless external
 * functions are disabled, in which case the previous value is used.  The
 * handling of se->e.o.s and freeing of command is tricky.  Returning an
 * allocated string in all cases, even if null, insures cell expressions are
 * written to files, etc.
 */

char * doext(struct enode *se) {
    char buff[FBUFLEN];        /* command line/return, not permanently alloc */
    char *command;
    double value;

    command = seval(se->e.o.left);
    value = eval(se->e.o.right);
    if ( ! atoi(get_conf_value("external_functions")) ) {
        scerror("Warning: external functions disabled; using %s value",
        (se->e.o.s && *se->e.o.s) ? "previous" : "null");

        if (command) scxfree(command);
    } else {
        if ((! command) || (! *command)) {
            scerror ("Warning: external function given null command name");
            cellerror = CELLERROR;
            if (command) scxfree(command);
        } else {
            FILE *pp;

            (void) sprintf(buff, "%s %g", command, value); /* build cmd line */
            scxfree(command);

            scerror("Running external function...");
            //(void) refresh();

            if ((pp = popen(buff, "r")) == (FILE *) NULL) {    /* run it */
                scerror("Warning: running \"%s\" failed", buff);
                cellerror = CELLERROR;
            } else {
                if (fgets(buff, sizeof(buff)-1, pp) == NULL) {    /* one line */
                    scerror("Warning: external function returned nothing");
                } else {
                    char *cp;
                    //scerror("");                /* erase notice */
                    buff[sizeof(buff)-1] = '\0';

                    if ((cp = strchr(buff, '\n')))    /* contains newline */
                        *cp = '\0';            /* end string there */

                    if (!se->e.o.s || strlen(buff) != strlen(se->e.o.s))
                        se->e.o.s = scxrealloc(se->e.o.s, strlen(buff));
                    (void) strcpy (se->e.o.s, buff);
                /* save alloc'd copy */
                }
                (void) pclose(pp);

            } /* else */
        } /* else */
    } /* else */
    if (se->e.o.s)
        return (strcpy(scxmalloc((size_t) (strlen(se->e.o.s)+1)), se->e.o.s));
    else
        return (strcpy(scxmalloc((size_t)1), ""));
}

/*
 * Given a string representing a column name and a value which is a column
 * number, return the selected cell's string value, if any.  Even if none,
 * still allocate and return a null string so the cell has a label value so
 * the expression is saved in a file, etc.
 */

char * dosval(char *colstr, double rowdoub) {
    struct ent *ep;
    char *llabel;

    //llabel = (ep = getent(colstr, rowdoub)) ? (ep -> label) : "";

    // getent don't return NULL for a cell with no string.
    llabel = ( ep = getent(colstr, rowdoub) ) && ep -> label ? (ep -> label) : "";

    return (strcpy(scxmalloc((size_t) (strlen(llabel) + 1)), llabel));
}


/*
 * Substring:  Note that v1 and v2 are one-based to users, but zero-based
 * when calling this routine.
 */
char * dosubstr(char *s, register int v1, register int v2) {
    register char *s1, *s2;
    char *p;

    if (!s)
        return ((char *)0);

    if (v2 >= strlen(s))        /* past end */
        v2 =  strlen(s) - 1;    /* to end   */

    if (v1 < 0 || v1 > v2) {        /* out of range, return null string */
        scxfree(s);
        p = scxmalloc((size_t)1);
        p[0] = '\0';
        return (p);
    }
    s2 = p = scxmalloc((size_t)(v2-v1+2));
    s1 = &s[v1];
    for (; v1 <= v2; s1++, s2++, v1++)
        *s2 = *s1;
    *s2 = '\0';
    scxfree(s);
    return(p);
}

/*
 * character casing: make upper case, make lower case, set 8th bit
 */
char * docase(int acase, char *s) {
    char *p = s;

    if (s == NULL)
        return(NULL);

    if ( acase == UPPER ) {
        while( *p != '\0' ) {
            if( islower(*p) )
                *p = toupper(*p);
            p++;
        }
    } else if (acase == SET8BIT) {
        while (*p != '\0') {
            if (*p >= 0)
                *p += 128 ;
            p++;
        }
    } else if (acase == LOWER) {
        while (*p != '\0') {
            if (isupper(*p))
                *p = tolower(*p);
            p++;
        }
    }
    return (s);
}

/*
 * make proper capitals of every word in a string
 * if the string has mixed case we say the string is lower
 *    and we will upcase only first letters of words
 * if the string is all upper we will lower rest of words.
 */
char * docapital(char *s) {
    char *p;
    int skip = 1;
    int AllUpper = 1;

    if (s == NULL)
        return (NULL);
    for (p = s; *p != '\0' && AllUpper != 0; p++)
        if (isalpha(*p) && islower(*p))  AllUpper = 0;
    for (p = s; *p != '\0'; p++) {
        if (!isalnum(*p)) skip = 1;
        else if (skip == 1) {
            skip = 0;
            if (islower(*p)) *p = toupper(*p);
        } else    /* if the string was all upper before */
            if (isupper(*p) && AllUpper != 0)
                *p = tolower(*p);
    }
    return (s);
}

char * seval(register struct enode *se) {
    register char *p;

    if (se == (struct enode *)0) return (char *)0;
    switch (se->op) {
    case O_SCONST:
             p = scxmalloc((size_t)(strlen(se->e.s)+1));
             (void) strcpy(p, se->e.s);
             return (p);
    case O_VAR:
            {
            struct ent *vp = se->e.v.vp;
            int row, col;
            if (vp && (rowoffset || coloffset)) {
                row = se->e.v.vf & FIX_ROW ? vp->row : vp->row + rowoffset;
                col = se->e.v.vf & FIX_COL ? vp->col : vp->col + coloffset;
                checkbounds(&row, &col);
                vp = *ATBL(tbl, row, col);
            }
            if (!vp || !vp->label)
                return (NULL);
            p = scxmalloc((size_t)(strlen(vp->label)+1));
            (void) strcpy(p, vp->label);
            return (p);
    }
    case '#':    return (docat(seval(se->e.o.left), seval(se->e.o.right)));
    case 'f':
             {
             int rtmp = rowoffset;
             int ctmp = coloffset;
             char *ret;
             rowoffset = coloffset = 0;
             ret = seval(se->e.o.left);
             rowoffset = rtmp;
             coloffset = ctmp;
             return (ret);
             }
    case 'F':    return (seval(se->e.o.left));
    case IF:
    case '?':    return (eval(se->e.o.left) ? seval(se->e.o.right->e.o.left)
                         : seval(se->e.o.right->e.o.right));
    case DATE:   return (dodate((time_t)(eval(se->e.o.left)),
                 seval(se->e.o.right)));
    case FMT:    return (dofmt(seval(se->e.o.left), eval(se->e.o.right)));
    case UPPER:  return (docase(UPPER, seval(se->e.o.left)));
    case LOWER:  return (docase(LOWER, seval(se->e.o.left)));
    case SET8BIT:  return (docase(SET8BIT, seval(se->e.o.left)));
    case CAPITAL:return (docapital(seval(se->e.o.left)));
    case STINDEX: {
        int r, c;
        int maxr, maxc;
        int minr, minc;
        maxr = se->e.o.left->e.r.right.vp->row;
        maxc = se->e.o.left->e.r.right.vp->col;
        minr = se->e.o.left->e.r.left.vp->row;
        minc = se->e.o.left->e.r.left.vp->col;
        if (minr>maxr) r = maxr, maxr = minr, minr = r;
        if (minc>maxc) c = maxc, maxc = minc, minc = c;
        return dostindex(minr, minc, maxr, maxc, se->e.o.right);
    }
    case EXT:    return (doext(se));
    case SVAL:   return (dosval(seval(se->e.o.left), eval(se->e.o.right)));
    case SUBSTR: return (dosubstr(seval(se->e.o.left),
                (int)eval(se->e.o.right->e.o.left) - 1,
                (int)eval(se->e.o.right->e.o.right) - 1));
    case COLTOA: return (strcpy(scxmalloc((size_t)10),
                   coltoa((int)eval(se->e.o.left))));
    case CHR: return (strcpy(scxmalloc((size_t)10), dochr(eval(se->e.o.left))));
    case FILENAME: {
             int n = eval(se->e.o.left);
             char *s = strrchr(curfile, '/');

             if (n || s++ == NULL) s = curfile;
             p = scxmalloc((size_t)(strlen(s)+1));
             (void) strcpy(p, s);
             return (p);
    }
    default:
             scerror("Illegal string expression");
             exprerr = 1;
             return (NULL);
    }
}

/*
 * The graph formed by cell expressions which use other cells's values is not
 * evaluated "bottom up".  The whole table is merely re-evaluated cell by cell,
 * top to bottom, left to right, in RealEvalAll().  Each cell's expression uses
 * constants in other cells.  However, RealEvalAll() notices when a cell gets a
 * new numeric or string value, and reports if this happens for any cell.
 * EvalAll() repeats calling RealEvalAll() until there are no changes or the
 * evaluation count expires.
 */

int propagation = 10;    /* max number of times to try calculation */
int repct = 1;        /* Make repct a global variable so that the 
                function @numiter can access it */

void setiterations(int i) {
    if (i < 1) {
        scerror("iteration count must be at least 1");
        propagation = 1;
    } else
        propagation = i;
}

void EvalAll() {
    int lastcnt;//, pair, v;

    repct = 1;
    (void) signal(SIGFPE, eval_fpe);

    while ((lastcnt = RealEvalAll()) && (++repct <= propagation));
    if ((propagation > 1) && (lastcnt > 0) && ! atoi(get_conf_value("nocurses"))) {
        scerror("Still changing after %d iterations", repct - 1);
    }
    /*
    if (usecurses && color && has_colors()) {
    for (pair = 0; pair < 8; pair++) {
        cellerror = CELLOK;
        if (cpairs[pair] && cpairs[pair]->expr) {
        v = (int) eval(cpairs[pair]->expr);
        cpairs[pair]->fg = v & 7;
        cpairs[pair]->bg = (v >> 3) & 7;
        init_pair(pair + 1, cpairs[pair]->fg, cpairs[pair]->bg);
        }
        // Can't see to fix the problem if color 1 has an error, so
        // turn off color in that case.
        if (pair == 0 && cellerror) {
        color = 0;
        attron(COLOR_PAIR(0));
        color_set(0, NULL);
        scerror("Error in color 1: color turned off");
        }
    }
    }
    */

    (void) signal(SIGFPE, exit_app);
}

/*
 * Evaluate all cells which have expressions and alter their numeric or string
 * values.  Return the number of cells which changed.
 */
int RealEvalAll() {
    register int i,j;
    int chgct = 0;
    register struct ent *p;

    if (calc_order == BYROWS) {
    for (i = 0; i <= maxrow; i++)
        for (j = 0; j <= maxcol; j++)
        if ((p = *ATBL(tbl, i, j)) && p->expr)
            RealEvalOne(p, i, j, &chgct);
    }
    else if (calc_order == BYCOLS) {
    for (j = 0; j <= maxcol; j++) {
        for (i = 0; i <= maxrow; i++)
        if ((p = *ATBL(tbl,i, j)) && p->expr)
            RealEvalOne(p, i, j, &chgct);
    }
    }
    else {
        scerror("Internal error calc_order");
    }
    return (chgct);
}

void RealEvalOne(register struct ent *p, int i, int j, int *chgct) {
    gmyrow=i; gmycol=j;

    if (p->flags & is_strexpr) {
        char *v;
        if (setjmp(fpe_save)) {
            scerror("Floating point exception %s", v_name(i, j));
            cellerror = CELLERROR;
            v = "";
        } else {
            cellerror = CELLOK;
            v = seval(p->expr);
        }
        p->cellerror = cellerror;
        if (!v && !p->label) /* Everything's fine */
            return;
        if (!p->label || !v || strcmp(v, p->label) != 0 || cellerror) {
            (*chgct)++;
            p->flags |= is_changed;
            changed++;
        }
        if (p->label)
            scxfree(p->label);
        p->label = v;
    } else {
        double v;
        if (setjmp(fpe_save)) {
            scerror("Floating point exception %s", v_name(i, j));
            cellerror = CELLERROR;
            v = (double)0.0;
        } else {
            cellerror = CELLOK;
            v = eval(p->expr);
            if (cellerror == CELLOK && !finite(v))
                cellerror = CELLERROR;
        }
        if ((cellerror != p->cellerror) || (v != p->v)) {
            p->cellerror = cellerror;
            p->v = v;

            // FIXME HERE: aqui controlar dependencia

            if (! cellerror)    /* don't keep eval'ing an error */
                (*chgct)++;
            p->flags |= is_changed | is_valid;
            changed++;
        }
    }
}

struct enode * new(int op, struct enode *a1, struct enode *a2) {
    register struct enode *p;
    //if (freeenodes) {
    //     p = freeenodes;
    //    freeenodes = p->e.o.left;
    //} else
    p = (struct enode *) scxmalloc((size_t) sizeof(struct enode));
    p->op = op;
    p->e.o.left = a1;
    p->e.o.right = a2;
    p->e.o.s = NULL;

    return p;
}

struct enode * new_var(int op, struct ent_ptr a1) {
    register struct enode *p;
    //if (freeenodes) {
    //    p = freeenodes;
    //    freeenodes = p->e.o.left;
    //} else
    p = (struct enode *) scxmalloc((size_t) sizeof(struct enode));
    p->op = op;
    p->e.v = a1; // ref to cell needed for this expr

    return p;
}

struct enode * new_range(int op, struct range_s a1) {
    register struct enode *p;
    //if (freeenodes)
    //{   p = freeenodes;
    //    freeenodes = p->e.o.left;
    //}
    //else
    p = (struct enode *) scxmalloc((size_t) sizeof(struct enode));
    p->op = op;
    p->e.r = a1;
    return p;
}

struct enode * new_const(int op, double a1) {
    register struct enode *p;
    //if (freeenodes) {    /* reuse an already free'd enode */
    //    p = freeenodes;
    //    freeenodes = p->e.o.left;
    //} else
    p = (struct enode *) scxmalloc((size_t) sizeof(struct enode));
    p->op = op;
    p->e.k = a1;
    return p;
}

struct enode * new_str(char *s) {
    register struct enode *p;
    //if (freeenodes) {    /* reuse an already free'd enode */
    //    p = freeenodes;
    //    freeenodes = p->e.o.left;
    //} else
    p = (struct enode *) scxmalloc((size_t) sizeof(struct enode));
    p->op = O_SCONST;
    p->e.s = s;
    return (p);
}

/*
void copy(struct ent *dv1, struct ent *dv2, struct ent *v1, struct ent *v2) {
    struct ent *p;
//    struct ent *n;
    static int minsr = -1, minsc = -1;
    static int maxsr = -1, maxsc = -1;
    int mindr, mindc;
    int maxdr, maxdc;
//    int vr, vc;
    int r, c;
    int deltar, deltac;

    if (dv1) {
    mindr = dv1->row;
    mindc = dv1->col;
    maxdr = dv2->row;
    maxdc = dv2->col;
    if (mindr > maxdr) r = maxdr, maxdr = mindr, mindr = r;
    if (mindc > maxdc) c = maxdc, maxdc = mindc, mindc = c;
    } else {
    //if (showrange) {
    //    showrange = 0;
        mindr = showsr < currow ? showsr : currow;
        mindc = showsc < curcol ? showsc : curcol;
        maxdr = showsr > currow ? showsr : currow;
        maxdc = showsc > curcol ? showsc : curcol;
    } else if (v1) {
        // Set up the default source range for the "c." command.
        minsr = maxsr = v1->row;
        minsc = maxsc = v1->col;
        return;
    } else {
        mindr = maxdr = currow;
        mindc = maxdc = curcol;
    }
    }

    if (v1) {
    minsr = v1->row;
    minsc = v1->col;
    maxsr = v2->row;
    maxsc = v2->col;
    if (minsr > maxsr) r = maxsr, maxsr = minsr, minsr = r;
    if (minsc > maxsc) c = maxsc, maxsc = minsc, minsc = c;
    } else if (dv1 == NULL || v2 != NULL) {
//    if (qbuf && delbuf[qbuf]) {
//        delbuf[++dbidx] = delbuf[qbuf];
//        delbuffmt[dbidx] = delbuffmt[qbuf];
//    } else if (dbidx < 0) return;
//    minsr = maxrow;
//    minsc = maxcol;
//    maxsr = 0;
//    maxsc = 0;
//    for (p = delbuf[dbidx]; p; p = p->next) {
//        if (p->row < minsr) minsr = p->row;
//        if (p->row > maxsr) maxsr = p->row;
//        if (p->col < minsc) minsc = p->col;
//        if (p->col > maxsc) maxsc = p->col;
//    }
    //} else if (showrange && !(showsr == currow && showsc == curcol &&
        mindr == currow && mindc == curcol &&
        maxdr == currow && maxdc == curcol)) {
    minsr = showsr < currow ? showsr : currow;
    minsc = showsc < curcol ? showsc : curcol;
    maxsr = showsr > currow ? showsr : currow;
    maxsc = showsc > curcol ? showsc : curcol;
    } else
    if (minsr == -1) return;

    checkbounds(&maxdr, &maxdc);

    if (maxdr - mindr < maxsr - minsr) maxdr = mindr + (maxsr - minsr);
    if (maxdc - mindc < maxsc - minsc) maxdc = mindc + (maxsc - minsc);
    if (dv1 && (v1 || !v2))
    yank_area(minsr, minsc, maxsr, maxsc);
    erase_area(mindr, mindc, maxdr, maxdc, 0);
    sync_refs();
    flush_saved();

    scerror("Copying...");
    if (!loading)
    refresh();
//   p = delbuf[dbidx];
//   if (minsr == maxsr && minsc == maxsc) {
//    // Source is a single cell
//    for (deltar = mindr - p->row; deltar <= maxdr - p->row; deltar++)
//        for (deltac = mindc - p->col; deltac <= maxdc - p->col; deltac++)
//        copydbuf(deltar, deltac);
//   } else if (minsr == maxsr) {
//    // Source is a single row
//    deltac = mindc - p->col;
//    for (deltar = mindr - p->row; deltar <= maxdr - p->row; deltar++)
//        copydbuf(deltar, deltac);
//   } else if (minsc == maxsc) {
//    // Source is a single column
//    deltar = mindr - p->row;
//    for (deltac = mindc - p->col; deltac <= maxdc - p->col; deltac++)
//        copydbuf(deltar, deltac);
//   } else {
//    // Everything else
//    deltar = mindr - p->row;
//    deltac = mindc - p->col;
//    copydbuf(deltar, deltac);
//   }

    if (dv1 && (v1 || !v2)) {
    sync_refs();
    flush_saved();
    }

    if (dv1 == NULL) {
//      if (qbuf && delbuf[qbuf]) {
//        delbuf[dbidx] = NULL;
//        delbuffmt[dbidx--] = NULL;
//    }
    qbuf = 0;
    }
    scerror("Copy done.");
}

void copydbuf(int deltar, int deltac) {
    int vr, vc;
    struct ent *p = delbuf[dbidx];
    struct ent *n;

    while (p) {
    vr = p->row + deltar;
    vc = p->col + deltac;
    n = lookat(vr, vc);
    if (n->flags&is_locked) continue;
    copyent(n, p, deltar, deltac, 0, 0, maxrow, maxcol, 0);
    p = p->next;
    }
}

// ERASE a Range of cells
// this function should not be used
void eraser(struct ent *v1, struct ent *v2) {
    flush_saved();
    erase_area(v1->row, v1->col, v2->row, v2->col, 0);
    sync_refs();
    modflg++;
}

// YANK a Range of cells
void yankr(struct ent *v1, struct ent *v2) {
    int i, qtmp;
    struct ent *obuf = NULL;

    if (dbidx < 0) dbidx++;
    delbuf[dbidx] = delbuf[DELBUFSIZE - 10];
    delbuf[DELBUFSIZE - 10] = NULL;
    delbuffmt[dbidx] = delbuffmt[DELBUFSIZE - 10];
    delbuffmt[DELBUFSIZE - 10] = NULL;
    for (i = dbidx + 1; i < DELBUFSIZE; i++) {
    if (delbuf[i] == delbuf[dbidx]) {
        delbuf[dbidx] = NULL;
        delbuffmt[dbidx] = NULL;
        break;
    }
    }
    flush_saved();
    if (qbuf) {
    if (dbidx < 0) dbidx++;
    delbuf[dbidx] = delbuf[qbuf];
    delbuffmt[dbidx] = delbuffmt[qbuf];
    flush_saved();
    obuf = delbuf[qbuf];    // orig. contents of the del. buffer
    }
    qtmp = qbuf;
    qbuf = 0;
    yank_area(v1->row, v1->col, v2->row, v2->col);
    qbuf = qtmp;
    for (i = 0; i < DELBUFSIZE; i++)
    if ((obuf && delbuf[i] == obuf) || (qbuf && i == qbuf)) {
        delbuf[i] = delbuf[dbidx];
        delbuffmt[i] = delbuffmt[dbidx];
    }
    qbuf = 0;
    delbuf[DELBUFSIZE - 10] = delbuf[dbidx];
    delbuffmt[DELBUFSIZE - 10] = delbuffmt[dbidx];
}

// MOVE a Range of cells
void mover(struct ent *d, struct ent *v1, struct ent *v2) {
    move_area(d->row, d->col, v1->row, v1->col, v2->row, v2->col);
    sync_refs();
}
*/

// Goto subroutines */
void g_free() {
    switch (gs.g_type) {
    case G_STR:
    case G_NSTR: scxfree(gs.g_s); break;
    default: break;
    }
    gs.g_type = G_NONE;
    gs.errsearch = 0;
}

/* repeat the last goto command */
void go_last() {
    int num = 0;

    switch (gs.g_type) {
    case G_NONE:
        scerror("Nothing to repeat");
        break;
    case G_NUM:
        num_search(gs.g_n, gs.g_row, gs.g_col,
        gs.g_lastrow, gs.g_lastcol, gs.errsearch);
        break;
//  case G_CELL:
//      moveto(gs.g_row, gs.g_col, gs.g_lastrow, gs.g_lastcol, gs.strow, gs.stcol);
//      break;
    case G_XSTR:
    case G_NSTR:
        num++;
    case G_STR:
        gs.g_type = G_NONE;    /* Don't free the string */
        str_search(gs.g_s, gs.g_row, gs.g_col, gs.g_lastrow, gs.g_lastcol, num);
        break;

    default:
        scerror("go_last: internal error");
    }
}

/* Place the cursor on a given cell.  If cornerrow >= 0, place the cell
 * at row cornerrow and column cornercol in the upper left corner of the
 * screen if possible.
 */
void moveto(int row, int col, int lastrow_, int lastcol_, int cornerrow, int cornercol) {
    register int i;

    //if (!loading && row != -1 && (row != currow || col != curcol)) remember(0);

    lastrow = currow;
    lastcol = curcol;
    currow = row;
    curcol = col;
    g_free();
    gs.g_type = G_CELL;
    gs.g_row = currow;
    gs.g_col = curcol;
    gs.g_lastrow = lastrow_;
    gs.g_lastcol = lastcol_;
    //gs.strow = cornerrow;
    //gs.stcol = cornercol;
    if (cornerrow >= 0) {
    //strow = cornerrow;
    //stcol = cornercol;
    gs.stflag = 1;
    } else
    gs.stflag = 0;

    for (rowsinrange = 0, i = row; i <= lastrow_; i++) {
        if (row_hidden[i]) {
            scinfo("Cell's row is hidden");
            continue;
        }
        rowsinrange++;
    }
    for (colsinrange = 0, i = col; i <= lastcol_; i++) {
        if (col_hidden[i]) {
            colsinrange = 0;
            scinfo("Cell's col is hidden");
            continue;
        }
        colsinrange += fwidth[i];
    }
    if (loading) {
        //update(1);
        changed = 0;
    } //else remember(1);
}

/*
 * 'goto' either a given number,'error', or 'invalid' starting at (currow,curcol)
 */
void num_search(double n, int firstrow, int firstcol, int lastrow_, int lastcol_, int errsearch) {
    register struct ent *p;
    register int r,c;
    int    endr, endc;

    //if (!loading) remember(0);
    g_free();
    gs.g_type = G_NUM;
    gs.g_n = n;
    gs.g_row = firstrow;
    gs.g_col = firstcol;
    gs.g_lastrow = lastrow_;
    gs.g_lastcol = lastcol_;
    gs.errsearch = errsearch;

    if (currow >= firstrow && currow <= lastrow_ &&
        curcol >= firstcol && curcol <= lastcol_) {
            endr = currow;
            endc = curcol;
    } else {
        endr = lastrow_;
        endc = lastcol_;
    }
    r = endr;
    c = endc;
    while (1) {
        if (c < lastcol_)
            c++;
        else {
            if (r < lastrow_) {
                while (++r < lastrow_ && row_hidden[r]) /* */;
                c = firstcol;
            } else {
                r = firstrow;
                c = firstcol;
            }
        }
        p = *ATBL(tbl, r, c);
        if (!col_hidden[c] && p && (p->flags & is_valid) &&
        (errsearch || (p->v == n)) && (!errsearch ||
        (p->cellerror == errsearch)))    /* CELLERROR vs CELLINVALID */
            break;
        if (r == endr && c == endc) {
            if (errsearch) {
                scerror("no %s cell found", errsearch == CELLERROR ? "ERROR" : "INVALID");
            } else {
                scerror("Number not found");
            }
            return;
        }
    }

    lastrow = currow;
    lastcol = curcol;
    currow = r;
    curcol = c;
    rowsinrange = 1;
    colsinrange = fwidth[curcol];
    if (loading) {
        //update(1);
        changed = 0;
    } //else remember(1);
}

/* 'goto' a cell containing a matching string */
void str_search(char *s, int firstrow, int firstcol, int lastrow_, int lastcol_, int num) {
    struct ent *p;
    int r, c;
    int endr, endc;
    char * tmp;
#if defined(REGCOMP)
    regex_t preg;
    int errcode;
#endif

    //if (!loading) remember(0);

#if defined(REGCOMP)
    if ((errcode = regcomp(&preg, s, REG_EXTENDED))) {
        scxfree(s);
        tmp = scxmalloc((size_t)160);
        regerror(errcode, &preg, tmp, sizeof(tmp));
        scerror(tmp);
        scxfree(tmp);
        return;
    }
#endif
#if defined(RE_COMP)
    if ((tmp = re_comp(s)) != NULL) {
        scxfree(s);
        scerror(tmp);
        return;
    }
#endif
#if defined(REGCMP)
    if ((tmp = regcmp(s, NULL)) == NULL) {
        scxfree(s);
        cellerror = CELLERROR;
        scerror("Invalid search string");
        return;
    }
#endif
    g_free();
    gs.g_type = G_STR + num;
    gs.g_s = s;
    gs.g_row = firstrow;
    gs.g_col = firstcol;
    gs.g_lastrow = lastrow_;
    gs.g_lastcol = lastcol_;
    if (currow >= firstrow && currow <= lastrow_ &&
        curcol >= firstcol && curcol <= lastcol_) {
        endr = currow;
        endc = curcol;
    } else {
        endr = lastrow_;
        endc = lastcol_;
    }
    r = endr;
    c = endc;
    while (1) {
       if (c < lastcol_) {
            c++;
        } else {
            if (r < lastrow_) {
                while (++r < lastrow_ && row_hidden[r]) /* */;
                c = firstcol;
            } else {
                r = firstrow;
                c = firstcol;
            }
        }
        p = *ATBL(tbl, r, c);
        if (gs.g_type == G_NSTR) {
            *line = '\0';
            if (p) {
                if (p->cellerror)
                    sprintf(line, "%s", p->cellerror == CELLERROR ?  "ERROR" : "INVALID");
                else if (p->flags & is_valid) {
                    if (p->format) {
                        if (*(p->format) == ctl('d')) {
                            time_t i = (time_t) (p->v);
                            strftime(line, sizeof(line), (p->format)+1,
                            localtime(&i));
                        } else
                            format(p->format, precision[c], p->v, line, sizeof(line));
                    } else
                        engformat(realfmt[c], fwidth[c], precision[c], p->v, line, sizeof(line));
                }
            }
        } else if (gs.g_type == G_XSTR) {
            *line = '\0';
            if (p && p->expr) {
                linelim = 0;
                decompile(p->expr, 0);    /* set line to expr */
                line[linelim] = '\0';
                if (*line == '?')
                    *line = '\0';
            }
        }
        if (! col_hidden[c]) {
            if (gs.g_type == G_STR) {
                if (p && p->label
#if defined(REGCOMP)
                && (regexec(&preg, p->label, 0, NULL, 0) == 0))
#else
#if defined(RE_COMP)
                && (re_exec(p->label) != 0))
#else
#if defined(REGCMP)
                && (regex(tmp, p->label) != (char *)0))
#else
                && (strcmp(s, p->label) == 0))
#endif
#endif
#endif
            break;
        } else            /* gs.g_type != G_STR */
        if (*line != '\0'
#if defined(REGCOMP)
            && (regexec(&preg, line, 0, NULL, 0) == 0))
#else
#if defined(RE_COMP)
            && (re_exec(line) != 0))
#else
#if defined(REGCMP)
            && (regex(tmp, line) != (char *)0))
#else
            && (strcmp(s, line) == 0))
#endif
#endif
#endif
            break;
    }
    if (r == endr && c == endc) {
        scerror("String not found");
#if defined(REGCOMP)
        regfree(&preg);
#endif
#if defined(REGCMP)
        free(tmp);
#endif
        linelim = -1;
        return;
    }
    }
    linelim = -1;
    lastrow = currow;
    lastcol = curcol;
    currow = r;
    curcol = c;
    rowsinrange = 1;
    colsinrange = fwidth[curcol];
#if defined(REGCOMP)
    regfree(&preg);
#endif
#if defined(REGCMP)
    free(tmp);
#endif
    if (loading) {
        //update(1);
        changed = 0;
    } //else remember(1);
}

/* fill a range with constants */
void fill(struct ent *v1, struct ent *v2, double start, double inc) {
    int r, c;
    register struct ent *n;
    int maxr, maxc;
    int minr, minc;

    maxr = v2->row;
    maxc = v2->col;
    minr = v1->row;
    minc = v1->col;
    if (minr>maxr) r = maxr, maxr = minr, minr = r;
    if (minc>maxc) c = maxc, maxc = minc, minc = c;
    checkbounds(&maxr, &maxc);
    if (minr < 0) minr = 0;
    if (minc < 0) minc = 0;

    if (calc_order == BYROWS) {
        for (r = minr; r<=maxr; r++)
            for (c = minc; c<=maxc; c++) {
                n = lookat(r, c);
                if (n->flags&is_locked) continue;
                (void) clearent(n);
                n->v = start;
                start += inc;
                n->flags |= (is_changed|is_valid);
                n->flags &= ~(iscleared);
            }
    }
    else if (calc_order == BYCOLS) {
        for (c = minc; c<=maxc; c++)
            for (r = minr; r<=maxr; r++) {
                n = lookat(r, c);
                (void) clearent(n);
                n->v = start;
                start += inc;
                n->flags |= (is_changed|is_valid);
                n->flags &= ~(iscleared);
            }
    }
    else {
        scerror(" Internal error calc_order");
    }
    changed++;
}

/* lock a range of cells */
void lock_cells(struct ent *v1, struct ent *v2) {
    int r, c;
    register struct ent *n;
    int maxr, maxc;
    int minr, minc;

    maxr = v2->row;
    maxc = v2->col;
    minr = v1->row;
    minc = v1->col;
    if (minr>maxr) r = maxr, maxr = minr, minr = r;
    if (minc>maxc) c = maxc, maxc = minc, minc = c;
    checkbounds(&maxr, &maxc);
    if (minr < 0) minr = 0;
    if (minc < 0) minc = 0;

    #ifdef UNDO
    create_undo_action();
    #endif
    for (r = minr; r <= maxr; r++)
        for (c = minc; c <= maxc; c++) {
            n = lookat(r, c);
    #ifdef UNDO
            copy_to_undostruct(r, c, r, c, 'd');
    #endif
            n->flags |= is_locked;
    #ifdef UNDO
            copy_to_undostruct(r, c, r, c, 'a');
    #endif
        }
    #ifdef UNDO
    end_undo_action();
    #endif
}

/* unlock a range of cells */
void unlock_cells(struct ent *v1, struct ent *v2) {
    int r, c;
    register struct ent *n;
    int maxr, maxc;
    int minr, minc;

    maxr = v2->row;
    maxc = v2->col;
    minr = v1->row;
    minc = v1->col;
    if (minr>maxr) r = maxr, maxr = minr, minr = r;
    if (minc>maxc) c = maxc, maxc = minc, minc = c;
    checkbounds(&maxr, &maxc);
    if (minr < 0) minr = 0;
    if (minc < 0) minc = 0;

    #ifdef UNDO
    create_undo_action();
    #endif
    for (r = minr; r <= maxr; r++)
        for (c = minc; c <= maxc; c++) {
            n = lookat(r, c);
    #ifdef UNDO
            copy_to_undostruct(r, c, r, c, 'd');
    #endif
            n->flags &= ~is_locked;
    #ifdef UNDO
            copy_to_undostruct(r, c, r, c, 'a');
    #endif
        }
    #ifdef UNDO
    end_undo_action();
    #endif
}

/* set the numeric part of a cell */
void let(struct ent *v, struct enode *e) {

    double val;
    unsigned isconstant = constant(e);

    if (locked_cell(v->row, v->col))
        return;
    if (v->row == currow && v->col == curcol)
        cellassign = 1;
    if (loading && ! isconstant)
        val = (double) 0.0;
    else {
        exprerr = 0;
        (void) signal(SIGFPE, eval_fpe);
        if (setjmp(fpe_save)) {
            scerror("Floating point exception in cell %s", v_name(v->row, v->col));
            val = (double)0.0;
            cellerror = CELLERROR;
        } else {
            cellerror = CELLOK;
            val = eval(e);
        }
        if (v->cellerror != cellerror) {
            v->flags |= is_changed;
            changed++;
            modflg++;
            v->cellerror = cellerror;
        }
        (void) signal(SIGFPE, exit_app);
        if (exprerr) {
            efree(e);
            return;
        }
    }

    if (isconstant) {
        /* prescale input unless it has a decimal */
        if (!loading && !decimal && (prescale < (double) 0.9999999))
            val *= prescale;
        decimal = FALSE;

        v->v = val;

        if (!(v->flags & is_strexpr)) {
            efree(v->expr);
            v->expr = (struct enode *)0;
        }
        efree(e);
    } else {
        efree(v->expr);

        v->expr = e;
        v->flags &= ~is_strexpr;
    }

    changed++;
    modflg++;
    v->flags |= ( is_changed | is_valid );
}

void slet(struct ent *v, struct enode *se, int flushdir) {
    char *p;

    if (locked_cell(v->row, v->col))
        return;
    if (v->row == currow && v->col == curcol)
        cellassign = 1;
    exprerr = 0;
    (void) signal(SIGFPE, eval_fpe);
    if (setjmp(fpe_save)) {
        scerror ("Floating point exception in cell %s", v_name(v->row, v->col));
        cellerror = CELLERROR;
        p = "";
    } else {
        cellerror = CELLOK;
        p = seval(se);
    }
    if (v->cellerror != cellerror) {
        v->flags |= is_changed;
        changed++;
        modflg++;
        v->cellerror = cellerror;
    }
    (void) signal(SIGFPE, exit_app);
    if (exprerr) {
        efree(se);
        return;
    }
    if (constant(se)) {
        label(v, p, flushdir);
        if (p) scxfree(p);
        efree(se);
        if (v->flags & is_strexpr) {
            efree(v->expr);
            v->expr = (struct enode *) 0;
            v->flags &= ~is_strexpr;
        }
        return;
    }
    efree(v->expr);
    v->expr = se;
    v->flags |= (is_changed|is_strexpr);
    if (flushdir < 0) v->flags |= is_leftflush;

    if (flushdir == 0)
        v->flags |= is_label;
    else
        v->flags &= ~is_label;

    changed++;
    modflg++;
}

void format_cell(struct ent *v1, struct ent *v2, char *s) {
    int r, c;
    register struct ent *n;
    int maxr, maxc;
    int minr, minc;

    maxr = v2->row;
    maxc = v2->col;
    minr = v1->row;
    minc = v1->col;
    if (minr>maxr) r = maxr, maxr = minr, minr = r;
    if (minc>maxc) c = maxc, maxc = minc, minc = c;
    checkbounds(&maxr, &maxc);
    if (minr < 0) minr = 0;
    if (minc < 0) minc = 0;

    modflg++;

    for (r = minr; r <= maxr; r++)
        for (c = minc; c <= maxc; c++) {
            n = lookat(r, c);
            if (locked_cell(n->row, n->col))
                continue;
            if (n->format)
                scxfree(n->format);
            n->format = 0;
            if (s && *s != '\0')
                n->format = strcpy(scxmalloc((unsigned)(strlen(s)+1)), s);
            n->flags |= is_changed;
        }
}

/*
 * Say if an expression is a constant (return 1) or not.
 */
int constant(register struct enode *e) {
    return e == NULL
     || e->op == O_CONST
     || e->op == O_SCONST
     || (e->op == 'm' && constant(e->e.o.left))
     || (e->op != O_VAR
         && !(e->op & REDUCE)
         && constant(e->e.o.left)
         && constant(e->e.o.right)
         && e->op != EXT     /* functions look like constants but aren't */
         && e->op != NVAL
         && e->op != SVAL
         && e->op != NOW
         && e->op != MYROW
         && e->op != MYCOL
         && e->op != LASTROW
         && e->op != LASTCOL
         && e->op != NUMITER
         && e->op != FILENAME
         && optimize );
}

void efree(struct enode *e) {
    if (e) {
        if (e->op != O_VAR && e->op != O_CONST && e->op != O_SCONST
        && !(e->op & REDUCE)) {
            efree(e->e.o.left);
            efree(e->e.o.right);
        }
        if (e->op == O_SCONST && e->e.s)
            scxfree(e->e.s);
        else if (e->op == EXT && e->e.o.s)
            scxfree(e->e.o.s);
        e->e.o.left = NULL;
        // e->e.o.left = freeenodes;
        scxfree((char *) e);
        //freeenodes = e;
    }
}

void label(register struct ent *v, register char *s, int flushdir) {
    if (v) {
        if (flushdir==0 && v->flags&is_valid) {
            register struct ent *tv;
            if (v->col > 0 && ((tv=lookat(v->row, v->col-1))->flags & is_valid) == 0)
            v = tv, flushdir = 1;
            else if (((tv=lookat(v->row, v->col+1))->flags & is_valid) == 0)
            v = tv, flushdir = -1;
            else flushdir = -1;
        }
        if (v->label)
            scxfree((char *)(v->label));
        if (s && s[0]) {
            v->label = scxmalloc((unsigned)(strlen(s)+1));
            (void) strcpy (v->label, s);
        } else
            v->label = (char *) 0;
        if (flushdir<0) v->flags |= is_leftflush;
        else v->flags &= ~is_leftflush;
        if (flushdir==0) v->flags |= is_label;
        else v->flags &= ~is_label;
        modflg++;
    }
}

void decodev(struct ent_ptr v) {
    //if ( v.vp == NULL) return; // FIXME
    struct range * r;
    if ( ! v.vp || v.vp->flags & is_deleted)
        (void) sprintf(line + linelim, "@ERR");
    else if ( ! find_range( (char *)0, 0, v.vp, v.vp, &r) && !r->r_is_range)
        (void) sprintf(line+linelim, "%s", r->r_name);
    else {
        (void) sprintf( line + linelim, "%s%s%s%d", v.vf & FIX_COL ? "$" : "", coltoa(v.vp->col), v.vf & FIX_ROW ? "$" : "", v.vp->row);
    }
    linelim += strlen(line + linelim);

    return;
}

char * coltoa(int col) {
    static char rname[3];
    register char *p = rname;

    if (col > 25) {
        *p++ = col/26 + 'A' - 1;
        col %= 26;
    }
    *p++ = col+'A';
    *p = '\0';
    return (rname);
}

/*
 *    To make list elements come out in the same order
 *    they were entered, we must do a depth-first eval
 *    of the ELIST tree
 */
static void decompile_list(struct enode *p) {
    if (!p) return;
    decompile_list(p->e.o.left);    /* depth first */
    decompile(p->e.o.right, 0);
    line[linelim++] = ',';
}

void decompile(register struct enode *e, int priority) {
    register char *s;
    if (e) {
    int mypriority;
    switch (e->op) {
    default: mypriority = 99; break;
    case ';': mypriority = 1; break;
    case '?': mypriority = 2; break;
    case ':': mypriority = 3; break;
    case '|': mypriority = 4; break;
    case '&': mypriority = 5; break;
    case '<': case '=': case '>': mypriority = 6; break;
    case '+': case '-': case '#': mypriority = 8; break;
    case '*': case '/': case '%': mypriority = 10; break;
    case '^': mypriority = 12; break;
    }
    if (mypriority<priority) line[linelim++] = '(';
    switch (e->op) {
    case 'f':    for (s="@fixed "; (line[linelim++] = *s++); );
            linelim--;
            decompile(e->e.o.left, 30);
            break;
    case 'F':    for (s="(@fixed)"; (line[linelim++] = *s++); );
            linelim--;
            decompile(e->e.o.left, 30);
            break;
    case 'm':    line[linelim++] = '-';
            decompile(e->e.o.left, 30);
            break;
    case '!':    line[linelim++] = '!';
            decompile(e->e.o.left, 30);
            break;
    case O_VAR:    decodev(e->e.v);
            break;
    case O_CONST:    (void) sprintf(line+linelim, "%.15g", e->e.k);
            linelim += strlen(line+linelim);
            break;
    case O_SCONST:    (void) sprintf(line+linelim, "\"%s\"", e->e.s);
            linelim += strlen(line+linelim);
            break;

    case SUM    : index_arg("@sum", e); break;
    case PROD   : index_arg("@prod", e); break;
    case AVG    : index_arg("@avg", e); break;
    case COUNT  : index_arg("@count", e); break;
    case STDDEV : index_arg("@stddev", e); break;
    case MAX    : index_arg("@max", e); break;
    case MIN    : index_arg("@min", e); break;
    case REDUCE | 'R': range_arg("@rows(", e); break;
    case REDUCE | 'C': range_arg("@cols(", e); break;

    case ABS:    one_arg("@abs(", e); break;
    case ACOS:   one_arg("@acos(", e); break;
    case ASIN:   one_arg("@asin(", e); break;
    case ATAN:   one_arg("@atan(", e); break;
    case ATAN2:  two_arg("@atan2(", e); break;
    case CEIL:   one_arg("@ceil(", e); break;
    case COS:    one_arg("@cos(", e); break;
    case EXP:    one_arg("@exp(", e); break;
    case FABS:   one_arg("@fabs(", e); break;
    case FLOOR:  one_arg("@floor(", e); break;
    case HYPOT:  two_arg("@hypot(", e); break;
    case LOG:    one_arg("@ln(", e); break;
    case LOG10:  one_arg("@log(", e); break;
    case POW:    two_arg("@pow(", e); break;
    case SIN:    one_arg("@sin(", e); break;
    case SQRT:   one_arg("@sqrt(", e); break;
    case TAN:    one_arg("@tan(", e); break;
    case DTR:    one_arg("@dtr(", e); break;
    case RTD:    one_arg("@rtd(", e); break;
    case RND:    one_arg("@rnd(", e); break;
    case ROUND:  two_arg("@round(", e); break;
    case HOUR:   one_arg("@hour(", e); break;
    case MINUTE: one_arg("@minute(", e); break;
    case SECOND: one_arg("@second(", e); break;
    case MONTH:  one_arg("@month(", e); break;
    case DAY:    one_arg("@day(", e); break;
    case YEAR:   one_arg("@year(", e); break;
    case NOW:    for (s = "@now"; (line[linelim++] = *s++); );
            linelim--;
            break;
    case DATE: if (e->e.o.right)
                two_arg("@date(", e);
            else
                one_arg("@date(", e);
            break;
    case FMT:   two_arg("@fmt(", e); break;
    case UPPER: one_arg("@upper(", e); break;
    case LOWER: one_arg("@lower(", e); break;
    case CAPITAL: one_arg("@capital(", e); break;
    case DTS:   three_arg("@dts(", e); break;
    case TTS:   three_arg("@tts(", e); break;
    case STON:  one_arg("@ston(", e); break;
    case ASCII: one_arg("@ascii(", e); break;
    case SLEN:  one_arg("@slen(", e); break;
    case EQS:   two_arg("@eqs(", e); break;
    case LMAX:  list_arg("@max(", e); break;
    case LMIN:  list_arg("@min(", e); break;
    case FV:    three_arg("@fv(", e); break;
    case PV:    three_arg("@pv(", e); break;
    case PMT:   three_arg("@pmt(", e); break;
    case NVAL:  two_arg("@nval(", e); break;
    case SVAL:  two_arg("@sval(", e); break;
    case EXT:   two_arg("@ext(", e); break;
    case SUBSTR:  three_arg("@substr(", e); break;
    case STINDEX: index_arg("@stindex", e); break;
    case INDEX: index_arg("@index", e); break;
    case LOOKUP:  index_arg("@lookup", e); break;
    case HLOOKUP: two_arg_index("@hlookup", e); break;
    case VLOOKUP: two_arg_index("@vlookup", e); break;
    case IF:    three_arg("@if(", e); break;
    case MYROW:    for (s = "@myrow"; (line[linelim++] = *s++); );
            linelim--;
            break;
    case MYCOL:    for (s = "@mycol"; (line[linelim++] = *s++); );
            linelim--;
            break;
    case LASTROW:  for (s = "@lastrow"; (line[linelim++] = *s++); );
            linelim--;
            break;
    case LASTCOL:  for (s = "@lastcol"; (line[linelim++] = *s++); );
            linelim--;
            break;
    case COLTOA:   one_arg( "@coltoa(", e); break;
    case CHR: one_arg( "@chr(", e); break;
    case FILENAME: one_arg( "@filename(", e); break;
    case NUMITER:  for (s = "@numiter"; (line[linelim++] = *s++); );
            linelim--;
            break;
    case ERR_:     for (s = "@err"; (line[linelim++] = *s++); );
            linelim--;
            break;
    case PI_:      for (s = "@pi"; (line[linelim++] = *s++); );
            linelim--;
            break;
    case BLACK:    for (s = "@black"; (line[linelim++] = *s++); );
            linelim--;
            break;
    case RED:      for (s = "@red"; (line[linelim++] = *s++); );
            linelim--;
            break;
    case GREEN:    for (s = "@green"; (line[linelim++] = *s++); );
            linelim--;
            break;
    case YELLOW:   for (s = "@yellow"; (line[linelim++] = *s++); );
            linelim--;
            break;
    case BLUE:     for (s = "@blue"; (line[linelim++] = *s++); );
            linelim--;
            break;
    case MAGENTA:  for (s = "@magenta"; (line[linelim++] = *s++); );
            linelim--;
            break;
    case CYAN:     for (s = "@cyan"; (line[linelim++] = *s++); );
            linelim--;
            break;
    case WHITE:    for (s = "@white"; (line[linelim++] = *s++); );
            linelim--;
            break;
/*
    case CLR_FG:       for (s = "@fg"; (line[linelim++] = *s++); );
            linelim--;
            break;
    case CLR_BG:       for (s = "@bg"; (line[linelim++] = *s++); );
            linelim--;
            break;
    case CLR_TYPE:     for (s = "@type"; (line[linelim++] = *s++); );
            linelim--;
            break;
    case CLR_BOLD:     for (s = "@bold"; (line[linelim++] = *s++); );
            linelim--;
            break;
    case CLR_DIM:      for (s = "@dim"; (line[linelim++] = *s++); );
            linelim--;
            break;
    case CLR_REVERSE:  for (s = "@reverse"; (line[linelim++] = *s++); );
            linelim--;
            break;
    case CLR_STANDOUT: for (s = "@standout"; (line[linelim++] = *s++); );
            linelim--;
            break;
    case CLR_BLINK:    for (s = "@blink"; (line[linelim++] = *s++); );
            linelim--;
            break;
    case CLR_UNDERLINE: for (s = "@underline"; (line[linelim++] = *s++); );
            linelim--;
            break;
    case CLR_HEADINGS: for (s = "@headings"; (line[linelim++] = *s++); );
            linelim--;
            break;
    case CLR_WELCOME:  for (s = "@welcome"; (line[linelim++] = *s++); );
            linelim--;
            break;
    case CLR_CELL_SELECTION:     for (s = "@cell_selection"; (line[linelim++] = *s++); );
            linelim--;
            break;
    case CLR_CELL_SELECTION_SC:  for (s = "@cell_selection_sc"; (line[linelim++] = *s++); );
            linelim--;
            break;
    case CLR_NUMB:               for (s = "@numb"; (line[linelim++] = *s++); );
            linelim--;
            break;
    case CLR_STRG:               for (s = "@strg"; (line[linelim++] = *s++); );
            linelim--;
            break;
    case CLR_DATEF:              for (s = "@datef"; (line[linelim++] = *s++); );
            linelim--;
            break;
    case CLR_EXPRESSION:         for (s = "@expression"; (line[linelim++] = *s++); );
            linelim--;
            break;
    case CLR_INFO_MSG:           for (s = "@info_msg"; (line[linelim++] = *s++); );
            linelim--;
            break;
    case CLR_ERROR_MSG:          for (s = "@error_msg"; (line[linelim++] = *s++); );
            linelim--;
            break;
    case CLR_MODE:               for (s = "@mode"; (line[linelim++] = *s++); );
            linelim--;
            break;
    case CLR_CELL_ID:            for (s = "@cell_id"; (line[linelim++] = *s++); );
            linelim--;
            break;
    case CLR_CELL_FORMAT:        for (s = "@cell_format"; (line[linelim++] = *s++); );
            linelim--;
            break;
    case CLR_CELL_CONTENT:       for (s = "@cell_content"; (line[linelim++] = *s++); );
            linelim--;
            break;
    case CLR_INPUT:              for (s = "@input"; (line[linelim++] = *s++); );
            linelim--;
            break;
    case CLR_NORMAL:             for (s = "@normal"; (line[linelim++] = *s++); );
            linelim--;
            break;
    case CLR_CELL_ERROR:         for (s = "@cell_error"; (line[linelim++] = *s++); );
            linelim--;
            break;
    case CLR_CELL_NEGATIVE:      for (s = "@cell_negative"; (line[linelim++] = *s++); );
            linelim--;
            break;
*/
    default:    decompile(e->e.o.left, mypriority);
            line[linelim++] = e->op;
            decompile(e->e.o.right, mypriority+1);
            break;
    }
    if (mypriority<priority) line[linelim++] = ')';
    } else line[linelim++] = '?';
}

void index_arg(char *s, struct enode *e) {
    if (e->e.o.right && e->e.o.right->op == ',') {
        two_arg_index(s, e);
        return;
    }
    for (; (line[linelim++] = *s++); );
    linelim--;
    range_arg("(", e->e.o.left);
    linelim--;
    if (e->e.o.right) {
        line[linelim++] = ',';
        decompile(e->e.o.right, 0);
    }
    line[linelim++] = ')';
}

void two_arg_index(char *s, struct enode *e) {
    for (; (line[linelim++] = *s++); );
    linelim--;
    range_arg("(", e->e.o.left);
    linelim--;
    line[linelim++] = ',';
    decompile(e->e.o.right->e.o.left, 0);
    line[linelim++] = ',';
    decompile(e->e.o.right->e.o.right, 0);
    line[linelim++] = ')';
}

void list_arg(char *s, struct enode *e) {
    for (; (line[linelim++] = *s++); );
    linelim--;

    decompile(e->e.o.right, 0);
    line[linelim++] = ',';
    decompile_list(e->e.o.left);
    line[linelim - 1] = ')';
}

void one_arg(char *s, struct enode *e) {
    for (; (line[linelim++] = *s++); );
    linelim--;
    decompile(e->e.o.left, 0);
    line[linelim++] = ')';
}

void two_arg(char *s, struct enode *e) {
    for (; (line[linelim++] = *s++); );
    linelim--;
    decompile(e->e.o.left, 0);
    line[linelim++] = ',';
    decompile (e->e.o.right, 0);
    line[linelim++] = ')';
}

void three_arg(char *s, struct enode *e) {
    for (; (line[linelim++] = *s++); );
    linelim--;
    decompile(e->e.o.left, 0);
    line[linelim++] = ',';
    decompile(e->e.o.right->e.o.left, 0);
    line[linelim++] = ',';
    decompile (e->e.o.right->e.o.right, 0);
    line[linelim++] = ')';
}

void range_arg(char *s, struct enode *e) {
    struct range *r;

    for (; (line[linelim++] = *s++); );
    linelim--;
    if ( ! find_range((char *)0, 0, e->e.r.left.vp, e->e.r.right.vp, &r) && r->r_is_range) {
        (void) sprintf(line+linelim, "%s", r->r_name);
        linelim += strlen(line+linelim);
    } else {
        decodev(e->e.r.left);
        line[linelim++] = ':';
        decodev(e->e.r.right);
    }
    line[linelim++] = ')';
}

void editfmt(int row, int col) {
    register struct ent *p;

    p = lookat(row, col);
    if (p->format) {
        (void) sprintf(line, "fmt %s \"%s\"", v_name(row, col), p->format);
        linelim = strlen(line);
    }
}

void editv(int row, int col) {
    register struct ent *p;

    p = lookat(row, col);
    (void) sprintf(line, "let %s = ", v_name(row, col));
    linelim = strlen(line);
    if (p->flags & is_valid) {
        if (p->flags & is_strexpr || p->expr == NULL) {
            (void) sprintf(line+linelim, "%.15g", p->v);
            linelim = strlen(line);
        } else
            editexp(row, col);
    }
}

void editexp(int row, int col) {
    register struct ent *p;

    p = lookat(row, col);
    //if ( !p || !p->expr ) return; 21/06/2014
    decompile(p->expr, 0);
    line[linelim] = '\0';
}

void edits(int row, int col, int saveinfile) {
    register struct ent *p;

    p = lookat(row, col);

    if (saveinfile) {
        if (p->flags & is_label)
               (void) sprintf(line, "label %s = ", v_name(row, col));
        else
            (void) sprintf(line, "%sstring %s = ", ((p->flags & is_leftflush) ? "left" : "right"), v_name(row, col));
    }

    linelim = strlen(line);
    if (p->flags & is_strexpr && p->expr) {
    editexp(row, col);
    } else if (p->label) {
        if (saveinfile) {
            (void) sprintf(line+linelim, "\"%s\"", p->label);
        } else {
            (void) sprintf(line+linelim, "%s", p->label);
        }
        linelim += strlen(line+linelim);
    } else {
        (void) sprintf(line+linelim, "\"");
        linelim += 1;
    }
}

int dateformat(struct ent *v1, struct ent *v2, char * fmt) {
    if ( ! fmt || *fmt == '\0') return -1;

    int r, c;
    struct ent * n;
    int maxr, maxc;
    int minr, minc;
    struct tm tm;

    maxr = v2->row;
    maxc = v2->col;
    minr = v1->row;
    minc = v1->col;
    if (minr>maxr) r = maxr, maxr = minr, minr = r;
    if (minc>maxc) c = maxc, maxc = minc, minc = c;
    checkbounds(&maxr, &maxc);
    if (minr < 0) minr = 0;
    if (minc < 0) minc = 0;

    #ifdef UNDO
    create_undo_action();
    #endif
    for (r = minr; r <= maxr; r++) {
        for (c = minc; c <= maxc; c++) {
            n = lookat(r, c);
    #ifdef UNDO
            copy_to_undostruct(r, c, r, c, 'd');
    #endif
            if ( locked_cell(n->row, n->col) || ! (n)->label ) continue;

            // free all ent content but its label
            n->v = (double) 0;
            if (n->format) scxfree(n->format);
            if (n->expr) efree(n->expr);
            n->expr = NULL;

            memset(&tm, 0, sizeof(struct tm));
            strptime((n)->label, fmt, &tm);
            n->v = (double) mktime(&tm);
            n->flags |= ( is_changed | is_valid );
            label(n, "", -1); // free label

            // agrego formato de fecha
            n->format = 0;
            char * s = scxmalloc((unsigned)(strlen(fmt)+2));
            sprintf(s, "%c", 'd');
            strcat(s, fmt);
            n->format = s;

    #ifdef UNDO
            copy_to_undostruct(r, c, r, c, 'a');
    #endif
        }
    }
    modflg++; // increase just one time
    #ifdef UNDO
    end_undo_action();
    #endif
    return 0;
}

#ifdef RINT
/*    round-to-even, also known as ``banker's rounding''.
    With round-to-even, a number exactly halfway between two values is
    rounded to whichever is even; e.g. rnd(0.5)=0, rnd(1.5)=2,
    rnd(2.5)=2, rnd(3.5)=4.  This is the default rounding mode for
    IEEE floating point, for good reason: it has better numeric
    properties.  For example, if X+Y is an integer,
    then X+Y = rnd(X)+rnd(Y) with round-to-even,
    but not always with sc's rounding (which is
    round-to-positive-infinity).  I ran into this problem when trying to
    split interest in an account to two people fairly.
*/

double rint(double d) {
    /* as sent */
    double fl = floor(d), fr = d-fl;
    return
        fr<0.5 || fr==0.5 && fl==floor(fl/2)*2 ? fl : ceil(d);
}
#endif
