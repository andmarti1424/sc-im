/*******************************************************************************
 * Copyright (c) 2013-2021, Andrés Martinelli <andmarti@gmail.com>             *
 * All rights reserved.                                                        *
 *                                                                             *
 * This file is a part of sc-im                                                *
 *                                                                             *
 * sc-im is a spreadsheet program that is based on sc. The original authors    *
 * of sc are James Gosling and Mark Weiser, and mods were later added by       *
 * Chuck Martin.                                                               *
 *                                                                             *
 * Redistribution and use in source and binary forms, with or without          *
 * modification, are permitted provided that the following conditions are met: *
 * 1. Redistributions of source code must retain the above copyright           *
 *    notice, this list of conditions and the following disclaimer.            *
 * 2. Redistributions in binary form must reproduce the above copyright        *
 *    notice, this list of conditions and the following disclaimer in the      *
 *    documentation and/or other materials provided with the distribution.     *
 * 3. All advertising materials mentioning features or use of this software    *
 *    must display the following acknowledgement:                              *
 *    This product includes software developed by Andrés Martinelli            *
 *    <andmarti@gmail.com>.                                                    *
 * 4. Neither the name of the Andrés Martinelli nor the                        *
 *   names of other contributors may be used to endorse or promote products    *
 *   derived from this software without specific prior written permission.     *
 *                                                                             *
 * THIS SOFTWARE IS PROVIDED BY ANDRES MARTINELLI ''AS IS'' AND ANY            *
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED   *
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE      *
 * DISCLAIMED. IN NO EVENT SHALL ANDRES MARTINELLI BE LIABLE FOR ANY           *
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES  *
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;*
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND *
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT  *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE       *
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.           *
 *******************************************************************************/

/**
 * \file function.c
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 24/05/2021
 * \brief Source file that implement the different functions that sc-im can handle
 * Based on sc
 *
 */
#include <string.h>
#include <stdlib.h>
#include <ctype.h> // for islower toupper tolower isalpha isalnum isupper
#include <math.h>

#include "sc.h"
#include "macros.h"
#include "cmds/cmds.h"
#include "function.h"
#include "tui.h"
#include "interp.h"
#include "xmalloc.h" // for scxfree
#include "conf.h"
#include "utils/string.h"

extern struct session * session;
extern int cellerror; // get rid of this
extern int rowoffset, coloffset;    /* row & col offsets for range functions */

#ifndef M_PI
    #define M_PI (double)3.14159265358979323846
#endif


/**
 * \brief finfunc()
 * \param[in] fun
 * \param[in] v1
 * \param[in] v2
 * \param[in] v3
 * \return double
 */
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
            sc_error("Unknown function in finfunc");
            cellerror = CELLERROR;
            return ((double)0);
        }
        return (answer);
}


/**
 * \brief dostindex()
 * \param[in] minr
 * \param[in] minc
 * \param[in] maxr
 * \param[in] maxc
 * \param[in] val
 * \return char *
 */
char * dostindex(struct sheet * sh, int minr, int minc, int maxr, int maxc, struct enode * val) {
    int r, c;
    struct ent * p;
    char * pr;

    p = (struct ent *) 0;
    if (minr == maxr) {            /* look along the row */
        r = minr;
        c = minc + (int) eval(sh, NULL, val) - 1;
    } else if (minc == maxc) {        /* look down the column */
        r = minr + (int) eval(sh, NULL, val) - 1;
        c = minc;
    } else {
        r = minr + (int) eval(sh, NULL, val->e.o.left) - 1;
        c = minc + (int) eval(sh, NULL, val->e.o.right) - 1;
    }
    if (c <= maxc && c >=minc && r <= maxr && r >=minr)
        p = *ATBL(sh, sh->tbl, r, c);

    if (p && p->label) {
        pr = scxmalloc((size_t) (strlen(p->label) + 1));
        (void) strcpy(pr, p->label);
        if (p->cellerror)
            cellerror = CELLINVALID;
        return (pr);
    } else
        return ((char *) 0);
}


/**
 * \brief doascii()
 * \param[in] s
 * \return double
 */
double doascii(char * s) {
    double v = 0.;
    int i ;
    if ( !s ) return ((double) 0);

    for (i = 0; s[i] != '\0' ; v = v*256 + (unsigned char)(s[i++]) ) ;
    scxfree(s);
    return(v);
}


/**
 * \brief doindex()
 * \param[in] minr
 * \param[in] minc
 * \param[in] maxr
 * \param[in] maxc
 * \param[in] val
 * \return double
 */
double doindex(struct sheet * sh, int minr, int minc, int maxr, int maxc, struct enode * val) {
    int r, c;
    struct ent * p;

    if (val->op == ',') {        /* index by both row and column */
        r = minr + (int) eval(sh, NULL, val->e.o.left) - 1;
        c = minc + (int) eval(sh, NULL, val->e.o.right) - 1;
    } else if (minr == maxr) {        /* look along the row */
        r = minr;
        c = minc + (int) eval(sh, NULL, val) - 1;
    } else if (minc == maxc) {        /* look down the column */
        r = minr + (int) eval(sh, NULL, val) - 1;
        c = minc;
    } else {
        sc_error("Improper indexing operation");
        return (double) 0;
    }

    if (c <= maxc && c >=minc && r <= maxr && r >=minr &&
        (p = *ATBL(sh, sh->tbl, r, c)) && p->flags & is_valid) {
        if (p->cellerror)
            cellerror = CELLINVALID;
        return p->v;
    } else
        return (double) 0;
}


/**
 * \brief dolookup()
 * \param[in] val
 * \param[in] minr
 * \param[in] minc
 * \param[in] maxr
 * \param[in] maxc
 * \param[in] offset
 * \param[in] vflag
 * \return double
 */
double dolookup(struct sheet * sh, struct enode * val, int minr, int minc, int maxr, int maxc, int offset, int vflag) {
    double v, ret = (double) 0;
    int r, c;
    struct ent * p = (struct ent *) 0;
    int incr, incc, fndr, fndc;
    char * s;

    incr = vflag; incc = 1 - vflag;
    if (etype(val) == NUM) {
        cellerror = CELLOK;
        v = eval(sh, NULL, val);
        for (r = minr, c = minc; r <= maxr && c <= maxc; r+=incr, c+=incc) {
            if ((p = *ATBL(sh, sh->tbl, r, c)) && p->flags & is_valid) {
                if (p->v <= v) {
                    fndr = incc ? (minr + offset) : r;
                    fndc = incr ? (minc + offset) : c;
                    if (ISVALID(sh, fndr, fndc))
                        if (p == NULL) // three lines added
                            cellerror = CELLINVALID;
                        else // useful when the lookup ends up in a cell with no value
                            p = *ATBL(sh, sh->tbl, fndr, fndc);
                    else {
                        sc_error(" range specified to @[hv]lookup");
                        cellerror = CELLERROR;
                    }
                    if (p && p->flags & is_valid) {
                        if (p->cellerror)
                            cellerror = CELLINVALID;
                        ret = p->v;
                    }
                } else break;
            }
        }
    } else {
        cellerror = CELLOK;
        s = seval(sh, NULL, val);
        for (r = minr, c = minc; r <= maxr && c <= maxc; r+=incr, c+=incc) {
            if ((p = *ATBL(sh, sh->tbl, r, c)) && p->label) {
                if (s && strcmp(p->label,s) == 0) {
                    fndr = incc ? (minr + offset) : r;
                    fndc = incr ? (minc + offset) : c;
                    if (ISVALID(sh, fndr,fndc)) {
                        p = *ATBL(sh, sh->tbl, fndr, fndc);
                        if (p->cellerror)
                            cellerror = CELLINVALID;
                    } else {
                        sc_error(" range specified to @[hv]lookup");
                        cellerror = CELLERROR;
                    }
                    break;
                }
            }
        }
        if (p && p->flags & is_valid)
            ret = p->v;
        if (s != NULL) scxfree(s);
    }
    return ret;
}


/**
 * \brief docount()
 *
 * \param[in] minr
 * \param[in] minc
 * \param[in] maxr
 * \param[in] maxc
 * \param[in] e
 *
 * \return double
 */
double docount(struct sheet * sh, int minr, int minc, int maxr, int maxc, struct enode * e) {
    int v;
    int r, c;
    int cellerr = CELLOK;
    struct ent *p;

    v = 0;
    for (r = minr; r <= maxr; r++)
        for (c = minc; c <= maxc; c++) {
            if (e) {
                rowoffset = r - minr;
                coloffset = c - minc;
            }
            if (!e || eval(sh, NULL, e))
                // the following changed for #430. docount should also count cells with strings. not just numbers
                // TODO: create @counta to count both, and leave @count for just numbers
                if ((p = *ATBL(sh, sh->tbl, r, c)) && (p->flags & is_valid || p->label) ) {
                    if (p->cellerror) cellerr = CELLINVALID;
                    v++;
                }
        }
    cellerror = cellerr;
    rowoffset = coloffset = 0;
    return v;
}


/**
 * \brief dosum()
 * \param[in] minr
 * \param[in] minc
 * \param[in] maxr
 * \param[in] maxc
 * \param[in] e
 * \return double
 */
double dosum(struct sheet * sh, int minr, int minc, int maxr, int maxc, struct enode * e) {
    double v;
    int r, c;
    int cellerr = CELLOK;
    struct ent * p;

    v = (double)0;
    for (r = minr; r <= maxr; r++)
        for (c = minc; c <= maxc; c++) {
            if (e) {
                rowoffset = r - minr;
                coloffset = c - minc;
            }
            if ( !e || eval(sh, NULL, e))
                if ((p = *ATBL(sh, sh->tbl, r, c)) && p->flags & is_valid) {
                    if (p->cellerror)
                        cellerr = CELLINVALID;
                    v += p->v;
                }
        }
    cellerror = cellerr;
    rowoffset = coloffset = 0;
    return v;
}

/**
 * \brief doprod()
 * \param[in] minr
 * \param[in] minc
 * \param[in] maxr
 * \param[in] maxc
 * \param[in] e
 * \return double
 */
double doprod(struct sheet * sh, int minr, int minc, int maxr, int maxc, struct enode * e) {
    double v;
    int r, c;
    int cellerr = CELLOK;
    struct ent * p;

    v = 1;
    for (r = minr; r <= maxr; r++)
        for (c = minc; c <= maxc; c++) {
            if (e) {
                rowoffset = r - minr;
                coloffset = c - minc;
            }
            if ( !e || eval(sh, NULL, e))
                if ((p = *ATBL(sh, sh->tbl, r, c)) && p->flags & is_valid) {
                    if (p->cellerror) cellerr = CELLINVALID;
                        v *= p->v;
                }
        }
    cellerror = cellerr;
    rowoffset = coloffset = 0;
    return v;
}


/**
 * \brief doavg()
 * \param[in] minr
 * \param[in] minc
 * \param[in] maxr
 * \param[in] maxc
 * \param[in] e
 * \return double
 */
double doavg(struct sheet * sh, int minr, int minc, int maxr, int maxc, struct enode * e) {
    double v;
    int r, c;
    int count;
    int cellerr = CELLOK;
    struct ent * p;

    v = (double) 0;
    count = 0;
    for (r = minr; r <= maxr; r++)
        for (c = minc; c <= maxc; c++) {
            if (e) {
                rowoffset = r - minr;
                coloffset = c - minc;
            }
            if (!e || eval(sh, NULL, e))
                if ((p = *ATBL(sh, sh->tbl, r, c)) && p->flags & is_valid) {
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


/**
 * \brief dostddev()
 * \param[in] minr
 * \param[in] minc
 * \param[in] maxr
 * \param[in] maxc
 * \param[in] e
 * \return double
 */
double dostddev(struct sheet * sh, int minr, int minc, int maxr, int maxc, struct enode * e) {
    double lp, rp, v, nd;
    int r, c;
    int n;
    int cellerr = CELLOK;
    struct ent * p;

    n = 0;
    lp = 0;
    rp = 0;
    for (r = minr; r <= maxr; r++)
        for (c = minc; c <= maxc; c++) {
            if (e) {
                rowoffset = r - minr;
                coloffset = c - minc;
            }
            if (!e || eval(sh, NULL, e))
                if ((p = *ATBL(sh, sh->tbl, r, c)) && p->flags & is_valid) {
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


/**
 * \brief domax()
 * \param[in] minr
 * \param[in] minc
 * \param[in] maxr
 * \param[in] maxc
 * \param[in] e
 * \return double
 */
double domax(struct sheet * sh, int minr, int minc, int maxr, int maxc, struct enode * e) {
    double v = (double) 0;
    int r, c;
    int count;
    int cellerr = CELLOK;
    struct ent * p;

    count = 0;
    for (r = minr; r <= maxr; r++)
        for (c = minc; c <= maxc; c++) {
            if (e) {
                rowoffset = r - minr;
                coloffset = c - minc;
            }
            if (!e || eval(sh, NULL, e))
                if ((p = *ATBL(sh, sh->tbl, r, c)) && p->flags & is_valid) {
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


/**
 * \brief domin()
 * \param[in] minr
 * \param[in] minc
 * \param[in] maxr
 * \param[in] maxc
 * \param[in] e
 * \return double
 */
double domin(struct sheet * sh, int minr, int minc, int maxr, int maxc, struct enode * e) {
    double v = (double)0;
    int r, c;
    int count;
    int cellerr = CELLOK;
    struct ent * p;

    count = 0;
    for (r = minr; r <= maxr; r++)
        for (c = minc; c <= maxc; c++) {
            if (e) {
                rowoffset = r - minr;
                coloffset = c - minc;
            }
            if (!e || eval(sh, NULL, e))
                if ((p = *ATBL(sh, sh->tbl, r, c)) && p->flags & is_valid) {
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

    if (count == 0) return ((double) 0);

    return (v);
}


int mdays[12]={ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

/**
 * \brief dodts()
 * \param[in] e1
 * \param[in] e2
 * \param[in] e3
 * \return double
 */
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
        sc_error("@dts: invalid argument or date out of range");
        cellerror = CELLERROR;
        return (0.0);
    }

    return ((double) secs);
}


/**
 * \brief dotts()
 * \param[in] hr
 * \param[in] min
 * \param[in] sec
 * \return double
 */
double dotts(int hr, int min, int sec) {
    if (hr < 0 || hr > 23 || min < 0 || min > 59 || sec < 0 || sec > 59) {
        sc_error ("@tts: Invalid argument");
        cellerror = CELLERROR;
        return ((double) 0);
    }
    return ((double) (sec + min * 60 + hr * 3600));
}


/**
 * \brief dorow()
 * \param[in] ep
 * \return double
 */
double dorow(struct enode * ep) {
    return (double) ep->e.v.vp->row;
}


/**
 * \brief docol()
 * \param[in] ep
 * \return double
 */
double docol(struct enode * ep) {
    return (double) ep->e.v.vp->col;
}


/**
 * \brief dotime()
 * \param[in] which
 * \param[in] when
 * \return double
 */
double dotime(int which, double when) {
    static time_t t_cache;
    static struct tm tm_cache;
    struct tm *tp;
    time_t tloc;

    if (which == NOW)
        return (double) time(NULL);

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


/**
 * \brief doston()
 * \param[in] s
 * \return double
 */
double doston(char * s) {
    double v;

    if ( !s ) return ((double)0);

    v = strtod(s, NULL);
    scxfree(s);
    return(v);
}


/**
 * \brief doevaluate(): take a char * with a formula and eval it
 * \param[in] s
 * \return double
 */
double doevaluate(char * s) {
    if ( !s) return ((double)0);
    wchar_t cline [BUFFERSIZE];
    swprintf(cline, BUFFERSIZE, L"eval %s", s);
    send_to_interp(cline);
    double d = eval_result;
    scxfree(s);
    return (double) d;
}


/**
 * \brief doslen()
 * \param[in] s
 * \return int
 */
int doslen(char * s) {
    if (!s) return 0;

    //int i = strlen(s);
    int i = 0;

    wchar_t widestring[BUFFERSIZE] = { L'\0' };
    const char * mbsptr = s;
    size_t result = mbsrtowcs(widestring, &mbsptr, BUFFERSIZE, NULL);
    if ( result != (size_t) -1 ) i = wcslen(widestring);
    scxfree(s);
    return i;
}


/**
 * \brief doeqs()
 * \param[in] s1
 * \param[in] s2
 * \return double
 */
double doeqs(char * s1, char * s2) {
    double v;

    if ( !s1 && !s2 ) return ((double)1.0);

    if ( !s1 || !s2 )
        v = 0.0;
    else if (strcmp(s1, s2) == 0)
        v = 1.0;
    else
        v = 0.0;

    if (s1) scxfree(s1);

    if (s2) scxfree(s2);

    return(v);
}


/**
 * \brief donval()
 * \details Given a string representing a column name and a value which is a
 * column number, return the selected cell's numeric value, if any.
 * \param[in] struct sheet * sh
 * \param[in] colstr
 * \param[in] rowdoub
 * \return double
 */
double donval(struct sheet * sh, char * colstr, double rowdoub) {
    struct ent * ep;
    return (((ep = getent(sh, colstr, rowdoub, 0)) && ((ep->flags) & is_valid)) ? (ep->v) : (double)0);
}


/**
 * \brief dolmax()
 * \details The list routines (e.g. dolmax) are called with an LMAX
 * enode. The left pointer is a chain of ELIST nodes, the right
 * pointer is a value.
 * \param[in] e
 * \param[in] ep
 * \return double
 */
double dolmax(struct sheet * sh, struct ent * e, struct enode * ep) {
    int count = 0;
    double maxval = 0; /* Assignment to shut up lint */
    struct enode * p;
    double v;

    cellerror = CELLOK;
    for (p = ep; p; p = p->e.o.left) {
        v = eval(sh, e, p->e.o.right);
        if ( !count || v > maxval) {
            maxval = v;
            count++;
        }
    }
    if (count) return maxval;
    else return (double)0;
}


/**
 * \brief dolmin()
 * \param[in] e
 * \param[in] ep
 * \return double
 */
double dolmin(struct sheet * sh, struct ent * e, struct enode * ep) {
    int count = 0;
    double minval = 0; /* Assignment to shut up lint */
    struct enode * p;
    double v;

    cellerror = CELLOK;
    for (p = ep; p; p = p->e.o.left) {
        v = eval(sh, e, p->e.o.right);
        if ( !count || v < minval) {
            minval = v;
            count++;
        }
    }
    if (count) return minval;
    else return (double)0;
}


/**
 * \brief docat()
 * \details Tules for string functions:
 * Take string arguments which they scxfree. All returned strings
 * are assumed to be xalloced.
 * \param[in] s1
 * \param[in] s2
 * \return char *
 */
char * docat(char * s1, char * s2) {
    char * p;
    char * arg1, * arg2;

    if ( !s1 && !s2 )
        return ((char *) 0);
    arg1 = s1 ? s1 : "";
    arg2 = s2 ? s2 : "";
    p = scxmalloc( (size_t) (strlen(arg1) + strlen(arg2) + 1));
    (void) strcpy(p, arg1);
    (void) strcat(p, arg2);
    if (s1)
        scxfree(s1);
    if (s2)
        scxfree(s2);
    return (p);
}


/**
 * \brief dodate()
 * \param[in] tloc
 * \param[in] fmstr
 * \return char *
 */
char * dodate(time_t tloc, char * fmtstr) {
    char buff[FBUFLEN];
    char * p;

    if (! fmtstr)
        fmtstr = "%a %b %d %H:%M:%S %Y";
    strftime(buff, FBUFLEN, fmtstr, localtime(&tloc));
    p = scxmalloc( (size_t) (strlen(buff) + 1));
    (void) strcpy(p, buff);
    return (p);
}


/**
 * \brief Conversion reverse from doascii
 * \param[in] ascii
 * \return char *
 */
char * dochr(double ascii) {
    char * p = scxmalloc((size_t) 10);
    char * q = p;
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


/**
 * \brief dofmt()
 * \param[in] fmtstr
 * \param[in] v
 * \return char *
 */
char * dofmt(char * fmtstr, double v) {
    char buff[FBUFLEN];
    char * p;

    if (!fmtstr)
        return ((char *) 0);
    (void) snprintf(buff, FBUFLEN, fmtstr, v);
    p = scxmalloc( (size_t) (strlen(buff) + 1));
    (void) strcpy(p, buff);
    scxfree(fmtstr);
    return (p);
}


/**
 * \brief doext()
 * \details Given a command name and a value, run the command with the given
 * value and read and return its first output line (only) as an allocated
 * string, always a copy of se->e.o.s, whic is set appropriately first
 * unless external functions are disabled, in which case the previous value
 * is used. The handling of se->e.o.s. and freezing of command is tricky.
 * Returning an allocated string in all cases, even if null, insures cell
 * expressions are written to files, etc..
 * \param[in] se
 * \return char *
 */
char * doext(struct sheet * sh, struct enode *se) {
    char buff[FBUFLEN];        /* command line/return, not permanently alloc */
    char * command;
    double value;

    command = seval(sh, NULL, se->e.o.left);
    value = eval(sh, NULL, se->e.o.right);
    if ( ! get_conf_int("external_functions") ) {
        sc_error("Warning: external functions disabled; using %s value",
        (se->e.o.s && *se->e.o.s) ? "previous" : "null");

        if (command) scxfree(command);
    } else {
        if (( !command ) || ( ! *command )) {
            sc_error ("Warning: external function given null command name");
            cellerror = CELLERROR;
            if (command) scxfree(command);
        } else {
            FILE *pp;

            (void) sprintf(buff, "%s %g", command, value); /* build cmd line */
            scxfree(command);

            sc_info("Running external function...");
            //(void) refresh();

            if ((pp = popen(buff, "r")) == (FILE *) NULL) {    /* run it */
                sc_error("Warning: running \"%s\" failed", buff);
                cellerror = CELLERROR;
            } else {
                if (fgets(buff, sizeof(buff)-1, pp) == NULL) {    /* one line */
                    sc_error("Warning: external function returned nothing");
                } else {
                    char *cp;
                    //sc_error("");                /* erase notice */
                    buff[sizeof(buff)-1] = '\0';

                    if ((cp = strchr(buff, '\n')))    /* contains newline */
                        *cp = '\0';            /* end string there */

                    if (!se->e.o.s || strlen(buff) != strlen(se->e.o.s))
                        se->e.o.s = scxrealloc(se->e.o.s, strlen(buff)+1);
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


/**
 * \brief dosval()
 * \details Given a string representing a column name and a value which
 * is a column number, return the selected cell's string value, if any.
 * Even if none, still allocate and return a null string, so the cell
 * has a label value, so the expression is saved in a file, etc..
 * \param[in] struct sheet * sh
 * \param[in] colstr
 * \param[in] rowdoub
 * \return char *
 */
char * dosval(struct sheet * sh, char * colstr, double rowdoub) {
    struct ent * ep;
    char * llabel;

    //llabel = (ep = getent(colstr, rowdoub, 0)) ? (ep -> label) : "";

    // getent don't return NULL for a cell with no string.
    llabel = ( ep = getent(sh, colstr, rowdoub, 0) ) && ep -> label ? (ep -> label) : "";

    return (strcpy(scxmalloc( (size_t) (strlen(llabel) + 1)), llabel));
}


/**
 * \brief doreplace()
 * \param[in] source
 * \param[in] old
 * \param[in] new
 * \return char *
 */
char * doreplace(char * source, char * old, char * newstr) {
    return str_replace(source, old, newstr);
}


/**
 * \brief dosubstring()
 * \param[in] s
 * \param[in] v1
 * \param[in] v2
 * \return char *
 */
char * dosubstr(char * s, int v1, int v2) {
    char * s1, * s2;
    char * p;

    if ( !s ) return ((char *) 0);

    if (v2 >= strlen(s))        /* past end */
        v2 =  strlen(s) - 1;    /* to end   */

    if (v1 < 0 || v1 > v2) {        /* out of range, return null string */
        scxfree(s);
        p = scxmalloc( (size_t) 1);
        p[0] = '\0';
        return (p);
    }
    s2 = p = scxmalloc( (size_t) (v2-v1 + 2));
    s1 = &s[v1];
    for (; v1 <= v2; s1++, s2++, v1++)
        *s2 = *s1;
    *s2 = '\0';
    scxfree (s);
    return (p);
}


/**
 * \brief dosevaluate(): take a char * with a formula and seval it
 * \param[in] s
 * \return char *
 */
char * dosevaluate(char * s) {
    if ( !s ) return ((char *) 0);
    char * p;

    wchar_t cline [BUFFERSIZE];
    swprintf(cline, BUFFERSIZE, L"seval %s", s);
    send_to_interp(cline);

    p = scxmalloc(sizeof(char) * strlen(seval_result)+1);
    strcpy(p, seval_result);
    free(seval_result);

    scxfree(s);
    return p;
}


/**
 * \brief Character casing: make upper case, make lower case, set 8th bit
 * \param[in] acase
 * \param[in] s
 * \return char *
 */
char * docase(int acase, char * s) {
    char * p = s;

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


/**
 * \brief docapital
 * \details Make proper capitals of every word in a string. If the string
 * has mixed case, we say the string is lower and we will upcase only
 * first letters of words. If the string is all upper, we will lower rest
 * of words.
 * \param[in] s
 * \return char *
 */
char * docapital(char * s) {
    char * p;
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


#ifdef RINT
/**
 * \brief Round-to-even
 *
 * \details Round-to-even, also known as "banker's rounding". With
 * round-to-even, a number exactly halfway between two values is
 * rounded to whichever is even; e.g. rnd(0.5)=0, rnd(1.5)=2,
 * rnd(3.5)=4. This is the default rounding mode for IEEE floating
 * point. for good reason: it has better njmeric properties. For example,
 * if X+Y is an integer, then X+Y = rnd(X)+rnd(Y) will round-to-even, but
 * not always with sc's rounding (which is round-to-positive-infinity). I
 * ran into this problem when trying to split interest in an account to
 * two people fairly.
 *
 * \param[in] d
 *
 * \return none
 */
double rint(double d) {
    /* as sent */
    double fl = floor(d), fr = d-fl;
    return
        fr<0.5 || fr==0.5 && fl==floor(fl/2)*2 ? fl : ceil(d);
}
#endif
